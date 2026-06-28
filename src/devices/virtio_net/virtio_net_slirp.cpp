// src/devices/virtio_net_slirp.cpp
//
// libslirp backend for virtio-net.
// Works on Windows (MSYS2) and Linux/WSL without any host network configuration,
// elevated privileges, or kernel drivers.
//
// Network layout (matches QEMU -netdev user defaults):
//   Host/gateway : 10.0.2.2
//   DNS          : 10.0.2.3
//   Guest DHCP   : 10.0.2.15
//
// In the guest after boot:
//   udhcpc -i eth0
//   (or manually: ip link set eth0 up && ip addr add 10.0.2.15/24 dev eth0
//                 && ip route add default via 10.0.2.2
//                 && echo nameserver 10.0.2.3 > /etc/resolv.conf)
//
// Installation:
//   MSYS2 ucrt64 : pacman -S mingw-w64-ucrt-x86_64-libslirp
//   MSYS2 mingw64: pacman -S mingw-w64-x86_64-libslirp
//   Ubuntu/Debian: apt install libslirp-dev
//   Fedora/RHEL  : dnf install libslirp-devel
//
// Windows runtime: libslirp-0.dll must be on PATH or next to the .exe

#include "devices/virtio_net.hpp"
#include "platform/platform.hpp"

#include <slirp/libslirp.h>
#include <glib.h>

#include <stdexcept>
#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <cstring>
#include <chrono>

#ifdef PLATFORM_WINDOWS
#  include <winsock2.h>
#  include <ws2tcpip.h>
// WSAPoll is our poll() on Windows
#  ifndef POLLIN
#    define POLLIN  0x0001
#    define POLLPRI 0x0002
#    define POLLOUT 0x0004
#    define POLLERR 0x0008
#    define POLLHUP 0x0010
#  endif
   typedef WSAPOLLFD sys_pollfd;
   static inline int sys_poll(sys_pollfd* fds, int n, int timeout) {
       return WSAPoll(fds, (ULONG)n, timeout);
   }
#else
#  include <poll.h>
#  include <arpa/inet.h>
   typedef struct pollfd sys_pollfd;
   static inline int sys_poll(sys_pollfd* fds, int n, int timeout) {
       return poll(fds, (nfds_t)n, timeout);
   }
#endif

#ifdef _WIN32
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0600  // Vista+ required for WSAPoll
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif

// ── libslirp 4.9.0 compat ────────────────────────────────────────────────────
// v4.9 renames register_poll_fd → register_poll_socket and uses
// slirp_os_socket (SOCKET on Win64, int on POSIX).
#if !SLIRP_CHECK_VERSION(4, 9, 0)
#  define slirp_os_socket           int
#  define slirp_pollfds_fill_socket slirp_pollfds_fill
#  define register_poll_socket      register_poll_fd
#  define unregister_poll_socket    unregister_poll_fd
#endif

// ── SlirpTimer ────────────────────────────────────────────────────────────────
// libslirp uses timers for TCP retransmit, ARP expiry, etc.
// We implement them with GLib (already a libslirp dependency).
struct SlirpTimer {
    SlirpTimerCb  cb;
    void*         cb_opaque;
    guint         source_id = 0;
};

// ── SlirpState ────────────────────────────────────────────────────────────────
struct SlirpState {
    Slirp*       slirp   = nullptr;
    VirtioNet*   vnet    = nullptr;

    // Sockets libslirp has registered for polling
    std::mutex              poll_mutex;
    std::vector<slirp_os_socket> poll_socks;

    std::thread       poll_thread;
    std::atomic<bool> running{false};

    GMainContext* gctx = nullptr;
};

// ── SLIRP_POLL ↔ sys poll event conversion ───────────────────────────────────
static short slirp_to_sys(int ev) {
    short r = 0;
    if (ev & SLIRP_POLL_IN)  r |= POLLIN;
    if (ev & SLIRP_POLL_OUT) r |= POLLOUT;
    if (ev & SLIRP_POLL_PRI) r |= POLLPRI;
    if (ev & SLIRP_POLL_ERR) r |= POLLERR;
    if (ev & SLIRP_POLL_HUP) r |= POLLHUP;
    return r;
}

static int sys_to_slirp(short rev) {
    int r = 0;
    if (rev & POLLIN)  r |= SLIRP_POLL_IN;
    if (rev & POLLOUT) r |= SLIRP_POLL_OUT;
    if (rev & POLLPRI) r |= SLIRP_POLL_PRI;
    if (rev & POLLERR) r |= SLIRP_POLL_ERR;
    if (rev & POLLHUP) r |= SLIRP_POLL_HUP;
    return r;
}

// ── SlirpCb implementations ──────────────────────────────────────────────────

// libslirp calls this to deliver a packet to the guest (RX direction).
// Change cb_send_packet return type from void to ssizet/int64_t:
static int64_t cb_send_packet(const void* pkt, size_t pkt_len, void* opaque) {
    auto* s = static_cast<SlirpState*>(opaque);
    s->vnet->rx_inject(static_cast<const uint8_t*>(pkt), (uint32_t)pkt_len);
    return (int64_t)pkt_len;
}

static void cb_guest_error(const char* msg, void* /*opaque*/) {
    std::cerr << "[slirp] " << msg << "\n";
}

static int64_t cb_clock_get_ns(void* /*opaque*/) {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(
        steady_clock::now().time_since_epoch()).count();
}

// Timer callbacks
static gboolean timer_fire(gpointer data) {
    auto* t = static_cast<SlirpTimer*>(data);
    t->source_id = 0;
    t->cb(t->cb_opaque);
    return G_SOURCE_REMOVE;
}

static void* cb_timer_new(SlirpTimerCb cb, void* cb_opaque, void* /*opaque*/) {
    auto* t      = new SlirpTimer;
    t->cb        = cb;
    t->cb_opaque = cb_opaque;
    return t;
}

static void cb_timer_free(void* timer, void* /*opaque*/) {
    auto* t = static_cast<SlirpTimer*>(timer);
    if (t->source_id) g_source_remove(t->source_id);
    delete t;
}

static void cb_timer_mod(void* timer, int64_t expire_ms, void* /*opaque*/) {
    auto* t = static_cast<SlirpTimer*>(timer);
    if (t->source_id) { g_source_remove(t->source_id); t->source_id = 0; }
    int64_t now_ms = cb_clock_get_ns(nullptr) / 1000000LL;
    guint   delay  = (guint)std::max(int64_t(0), expire_ms - now_ms);
    t->source_id = g_timeout_add(delay, timer_fire, t);
}

// Socket registration
static void cb_register_poll_socket(slirp_os_socket sock, void* opaque) {
    auto* s = static_cast<SlirpState*>(opaque);
    std::lock_guard<std::mutex> lk(s->poll_mutex);
    s->poll_socks.push_back(sock);
}

static void cb_unregister_poll_socket(slirp_os_socket sock, void* opaque) {
    auto* s = static_cast<SlirpState*>(opaque);
    std::lock_guard<std::mutex> lk(s->poll_mutex);
    auto& v = s->poll_socks;
    v.erase(std::remove(v.begin(), v.end(), sock), v.end());
}

// notify: NOP — our poll loop runs continuously
static void cb_notify(void* /*opaque*/) {}

// ── Poll thread ───────────────────────────────────────────────────────────────
// Runs the libslirp I/O loop. All slirp_* calls happen here to respect
// libslirp's single-threaded contract.
static void poll_thread_func(SlirpState* s) {
    s->gctx = g_main_context_new();
    g_main_context_push_thread_default(s->gctx);

    std::vector<sys_pollfd> pfds;

    // Context struct passed into the fill/poll lambdas
    struct Ctx { std::vector<sys_pollfd>* pfds; };

    while (s->running) {
        uint32_t timeout_ms = 10;

        // Snapshot the registered sockets
        pfds.clear();
        {
            std::lock_guard<std::mutex> lk(s->poll_mutex);
            for (auto sock : s->poll_socks) {
                sys_pollfd p{};
                p.fd      = (int)sock;
                p.events  = 0;
                p.revents = 0;
                pfds.push_back(p);
            }
        }

        Ctx ctx{&pfds};

        // Ask libslirp what events it wants on each socket
        slirp_pollfds_fill_socket(s->slirp, &timeout_ms,
            [](slirp_os_socket sock, int slirp_ev, void* op) -> int {
                auto* c = static_cast<Ctx*>(op);
                for (size_t i = 0; i < c->pfds->size(); i++) {
                    if ((slirp_os_socket)(*c->pfds)[i].fd == sock) {
                        (*c->pfds)[i].events |= slirp_to_sys(slirp_ev);
                        return (int)i;
                    }
                }
                return -1;
            }, &ctx);

        // poll() / WSAPoll()
        if (!pfds.empty())
            sys_poll(pfds.data(), (int)pfds.size(), (int)timeout_ms);
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));

        // Deliver results back to libslirp
        slirp_pollfds_poll(s->slirp, 0,
            [](int sock, void* op) -> int {
                auto* c = static_cast<Ctx*>(op);
                for (size_t i = 0; i < c->pfds->size(); i++) {
                    if ((*c->pfds)[i].fd == (int)sock)
                        return sys_to_slirp((*c->pfds)[i].revents);
                }
                return 0;
            }, &ctx);

        // Dispatch pending GLib timer sources (TCP retransmit etc.)
        g_main_context_iteration(s->gctx, FALSE);
    }

    g_main_context_pop_thread_default(s->gctx);
    g_main_context_unref(s->gctx);
    s->gctx = nullptr;
}

// ── VirtioNet::init ───────────────────────────────────────────────────────────
void VirtioNet::init(const std::string& /*tap_name — ignored with slirp*/) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        throw std::runtime_error("[virtio-net] WSAStartup failed");
#endif

    auto* s  = new SlirpState();
    s->vnet  = this;

    // ── Callbacks ────────────────────────────────────────────────────────────
    SlirpCb cb{};
    cb.send_packet            = cb_send_packet;
    cb.guest_error            = cb_guest_error;
    cb.clock_get_ns           = cb_clock_get_ns;
    cb.timer_new              = cb_timer_new;
    cb.timer_free             = cb_timer_free;
    cb.timer_mod              = cb_timer_mod;
    cb.register_poll_socket   = cb_register_poll_socket;
    cb.unregister_poll_socket = cb_unregister_poll_socket;
    cb.notify                 = cb_notify;

    // ── Config — match QEMU -netdev user defaults ─────────────────────────────
    SlirpConfig cfg{};
    memset(&cfg, 0, sizeof(cfg));
    cfg.version    = 1;
    cfg.restricted = 0;
    cfg.in_enabled = true;
    cfg.in6_enabled= false; // keep it simple

    inet_pton(AF_INET, "10.0.2.0",     &cfg.vnetwork);
    inet_pton(AF_INET, "255.255.255.0", &cfg.vnetmask);
    inet_pton(AF_INET, "10.0.2.2",     &cfg.vhost);
    inet_pton(AF_INET, "10.0.2.15",    &cfg.vdhcp_start);
    inet_pton(AF_INET, "10.0.2.3",     &cfg.vnameserver);
    cfg.if_mtu = 1500;
    cfg.if_mru = 1500;

    s->slirp = slirp_new(&cfg, &cb, s);
    if (!s->slirp) {
        delete s;
        throw std::runtime_error("[virtio-net] slirp_new() failed");
    }

    backend_ = s; // store in the void* member — platform_send uses it

    std::cout << "[virtio-net] libslirp ready (10.0.2.0/24, gateway 10.0.2.2)\n"
              << "  Guest: udhcpc -i eth0  (or static 10.0.2.15/24 gw 10.0.2.2)\n"
              << "  DNS  : 10.0.2.3\n";

    s->running = true;
    s->poll_thread = std::thread(poll_thread_func, s);
}

// ── VirtioNet::~VirtioNet ─────────────────────────────────────────────────────
// Defined here (not in virtio_net.cpp) so we can access SlirpState.
// Remove the empty destructor from virtio_net.cpp if you add this.
// (Or keep the one in virtio_net.cpp empty — it's fine since backend_ cleanup
// happens here via a separate shutdown function.)
//
// For clean shutdown, call this from the emulator halt path or just let the
// process exit (slirp cleans up automatically on process exit).

// ── platform_send ─────────────────────────────────────────────────────────────
// Called from process_tx_queue with a complete Ethernet frame from the guest.
void VirtioNet::platform_send(const uint8_t* frame, uint32_t len) {
    if (!backend_) return;
    auto* s = static_cast<SlirpState*>(backend_);
    // slirp_input takes the full Ethernet frame
    slirp_input(s->slirp, frame, (int)len);
}