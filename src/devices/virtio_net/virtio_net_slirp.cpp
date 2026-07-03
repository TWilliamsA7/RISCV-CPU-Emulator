// src/devices/virtio_net/virtio_net_slirp.cpp
//
// libslirp backend for virtio-net.
//
// Threading model:
//   ALL slirp_* calls happen exclusively on the poll thread.
//   The CPU thread enqueues TX frames via a mutex + pipe and never
//   calls slirp_* directly.
//
// Poll loop follows the canonical slirp4netns / QEMU pattern exactly:
//   1. Reset pfds to just the fixed fds (wakeup pipe).
//   2. slirp_pollfds_fill_socket — APPENDS slirp sockets to pfds, returns index.
//   3. update_ra_timeout — shrink timeout to nearest timer deadline.
//   4. poll() / WSAPoll().
//   5. If wakeup pipe fired: drain pipe, drain TX queue, call slirp_input().
//      Set had_tx=true so slirp_pollfds_poll is told select_error=true.
//   6. slirp_pollfds_poll(slirp, had_tx ? 1 : (pollrc <= 0), get_revents, pfds).
//      This drives cb_send_packet → rx_inject for any host→guest data.
//   7. check_ra_timeout — fire expired timers.

#include "devices/virtio_net.hpp"
#include "platform/platform.hpp"

#include <slirp/libslirp.h>

#include <stdexcept>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <mutex>
#include <thread>
#include <atomic>
#include <cstring>
#include <chrono>

#ifdef PLATFORM_WINDOWS
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  ifndef POLLIN
#    define POLLIN  0x0001
#    define POLLPRI 0x0002
#    define POLLOUT 0x0004
#    define POLLERR 0x0008
#    define POLLHUP 0x0010
#  endif
   typedef WSAPOLLFD sys_pollfd;
   static inline int sys_poll(sys_pollfd* fds, int n, int timeout_ms) {
       return WSAPoll(fds, (ULONG)n, timeout_ms);
   }
   static int make_wakeup_pair(int fds[2]) {
       // Windows has no pipe() — use a loopback socket pair.
       SOCKET srv = socket(AF_INET, SOCK_STREAM, 0);
       struct sockaddr_in addr{};
       addr.sin_family      = AF_INET;
       addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
       addr.sin_port        = 0;
       bind(srv, (sockaddr*)&addr, sizeof(addr));
       listen(srv, 1);
       int len = sizeof(addr);
       getsockname(srv, (sockaddr*)&addr, &len);
       SOCKET c = socket(AF_INET, SOCK_STREAM, 0);
       connect(c, (sockaddr*)&addr, sizeof(addr));
       SOCKET a = accept(srv, nullptr, nullptr);
       closesocket(srv);
       fds[0] = (int)a;   // read end
       fds[1] = (int)c;   // write end
       return 0;
   }
   static void wakeup_write(int fd) { char b = 1; send((SOCKET)fd, &b, 1, 0); }
   static void wakeup_drain(int fd) { char b; recv((SOCKET)fd, &b, 1, 0); }
#else
#  include <poll.h>
#  include <arpa/inet.h>
#  include <unistd.h>
   typedef struct pollfd sys_pollfd;
   static inline int sys_poll(sys_pollfd* fds, int n, int timeout_ms) {
       return poll(fds, (nfds_t)n, timeout_ms);
   }
   static int make_wakeup_pair(int fds[2]) { return pipe(fds); }
   static void wakeup_write(int fd) { char b = 1; (void)write(fd, &b, 1); }
   static void wakeup_drain(int fd) { char b; (void)read(fd, &b, 1); }
#endif

// ── libslirp 4.9.0 compat ────────────────────────────────────────────────────
#if !SLIRP_CHECK_VERSION(4, 9, 0)
#  define slirp_os_socket           int
#  define slirp_pollfds_fill_socket slirp_pollfds_fill
#  define register_poll_socket      register_poll_fd
#  define unregister_poll_socket    unregister_poll_fd
#endif

// ── Timer ─────────────────────────────────────────────────────────────────────
// Plain deadline trackers — no GLib sources, no g_timeout_add.
struct SlirpTimer {
    SlirpTimerCb cb;
    void*        cb_opaque;
    int64_t      expire_timer_msec = -1; // -1 = not armed
};

// ── SlirpState ────────────────────────────────────────────────────────────────
struct SlirpState {
    Slirp*     slirp = nullptr;
    VirtioNet* vnet  = nullptr;

    // Timers — poll thread only.
    std::vector<SlirpTimer*> timers;

    // TX queue: CPU thread enqueues, poll thread dequeues.
    std::mutex                       tx_mutex;
    std::queue<std::vector<uint8_t>> tx_queue;

    // Wakeup pipe: write end for CPU thread, read end polled by poll thread.
    int wakeup_r = -1;
    int wakeup_w = -1;

    std::thread       poll_thread;
    std::atomic<bool> running{false};
};

// ── Clock ─────────────────────────────────────────────────────────────────────
static int64_t cb_clock_get_ns(void* /*opaque*/) {
    using namespace std::chrono;
    return (int64_t)duration_cast<nanoseconds>(
        steady_clock::now().time_since_epoch()).count();
}

static int64_t clock_now_ms() {
    return cb_clock_get_ns(nullptr) / 1000000LL;
}

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

// ── Timer helpers (poll thread only) ─────────────────────────────────────────

// Shrink timeout_ms down to the nearest armed timer deadline.
// Mirrors slirp4netns update_ra_timeout exactly.
static void update_ra_timeout(uint32_t& timeout_ms, SlirpState* s) {
    int64_t now = clock_now_ms();
    for (SlirpTimer* t : s->timers) {
        if (t->expire_timer_msec == -1) continue;
        int64_t diff = t->expire_timer_msec - now;
        if (diff < 0) diff = 0;
        if ((uint32_t)diff < timeout_ms)
            timeout_ms = (uint32_t)diff;
    }
}

// Fire timers whose deadline has passed.
// Mirrors slirp4netns check_ra_timeout exactly.
// Iterates a snapshot so a fired cb can safely call timer_free/timer_mod.
static void check_ra_timeout(SlirpState* s) {
    int64_t now = clock_now_ms();
    auto snapshot = s->timers;
    for (SlirpTimer* t : snapshot) {
        if (t->expire_timer_msec != -1) {
            int64_t diff = t->expire_timer_msec - now;
            if (diff <= 0) {
                t->expire_timer_msec = -1;
                t->cb(t->cb_opaque);
            }
        }
    }
}

// ── SlirpCb implementations (all called from poll thread) ────────────────────

// Host → guest packet: called by slirp_pollfds_poll on the poll thread.
static int64_t cb_send_packet(const void* pkt, size_t pkt_len, void* opaque) {
    auto* s = static_cast<SlirpState*>(opaque);
    s->vnet->rx_inject(static_cast<const uint8_t*>(pkt), (uint32_t)pkt_len);
    return (int64_t)pkt_len;
}

static void cb_guest_error(const char* msg, void* /*opaque*/) {
    std::cerr << "[slirp] " << msg << "\n";
}

static void* cb_timer_new(SlirpTimerCb cb, void* cb_opaque, void* opaque) {
    auto* s = static_cast<SlirpState*>(opaque);
    auto* t = new SlirpTimer{cb, cb_opaque, -1};
    s->timers.push_back(t);
    return t;
}

static void cb_timer_free(void* timer, void* opaque) {
    auto* s = static_cast<SlirpState*>(opaque);
    auto* t = static_cast<SlirpTimer*>(timer);
    s->timers.erase(std::remove(s->timers.begin(), s->timers.end(), t),
                    s->timers.end());
    delete t;
}

static void cb_timer_mod(void* timer, int64_t expire_ms, void* /*opaque*/) {
    static_cast<SlirpTimer*>(timer)->expire_timer_msec = expire_ms;
}

// register/unregister: NOP on Linux (QEMU also makes these NOP on Linux).
// On Windows this would call qemu_fd_register; we don't need it because
// we discover sockets via the fill callback's append pattern.
static void cb_register_poll_socket(slirp_os_socket /*sock*/, void* /*opaque*/) {}
static void cb_unregister_poll_socket(slirp_os_socket /*sock*/, void* /*opaque*/) {}

// notify: wake the poll thread so it processes pending work immediately.
static void cb_notify(void* opaque) {
    wakeup_write(static_cast<SlirpState*>(opaque)->wakeup_w);
}

// ── Poll thread ───────────────────────────────────────────────────────────────
static void poll_thread_func(SlirpState* s) {
    // pfds is rebuilt every iteration:
    //   [0]     = wakeup pipe read end  (fixed)
    //   [1..]   = sockets appended by slirp_pollfds_fill_socket
    std::vector<sys_pollfd> pfds;

    while (s->running) {
        uint32_t timeout_ms = 10;

        // ── Step 1: Reset pfds to just the wakeup fd ─────────────────────
        pfds.clear();
        {
            sys_pollfd w{};
            w.fd     = s->wakeup_r;
            w.events = POLLIN;
            pfds.push_back(w);
        }

        // ── Step 2: Let libslirp APPEND its sockets ───────────────────────
        // The fill callback appends a new entry to pfds and returns its index.
        // libslirp stores that index and passes it back in get_revents.
        // This exactly mirrors slirp4netns libslirp_add_poll / libslirp_get_revents.
        slirp_pollfds_fill_socket(s->slirp, &timeout_ms,
            [](slirp_os_socket sock, int slirp_ev, void* op) -> int {
                auto* pfds = static_cast<std::vector<sys_pollfd>*>(op);
                sys_pollfd p{};
                p.fd     = (int)sock;
                p.events = slirp_to_sys(slirp_ev);
                int idx  = (int)pfds->size();   // index BEFORE push
                pfds->push_back(p);
                return idx;
            }, &pfds);

        // ── Step 3: Tighten timeout to nearest timer deadline ─────────────
        update_ra_timeout(timeout_ms, s);

        // ── Step 4: poll() ────────────────────────────────────────────────
        int pollrc = sys_poll(pfds.data(), (int)pfds.size(), (int)timeout_ms);

        // ── Step 5: Handle TX frames from the CPU thread ──────────────────
        // Process wakeup + TX queue BEFORE slirp_pollfds_poll.
        // When we feed frames to slirp_input we set had_tx so that
        // slirp_pollfds_poll is called with select_error=true — matching the
        // canonical slirp4netns pattern (pollout = -1 after slirp_input).
        bool had_tx = false;
        if (!pfds.empty() && (pfds[0].revents & POLLIN)) {
            wakeup_drain(s->wakeup_r);

            std::queue<std::vector<uint8_t>> local;
            {
                std::lock_guard<std::mutex> lk(s->tx_mutex);
                std::swap(local, s->tx_queue);
            }
            while (!local.empty()) {
                auto& frame = local.front();
                slirp_input(s->slirp, frame.data(), (int)frame.size());
                local.pop();
                had_tx = true;
            }
        }

        // ── Step 6: Deliver poll results to libslirp ──────────────────────
        // get_revents receives the index that fill_socket returned and looks
        // up revents in pfds — same array, same indices.
        // select_error = true when poll timed out/errored OR when we just
        // fed TX frames (mirrors slirp4netns: pollout=-1 → (pollout<=0)=true).
        int select_error = (had_tx || pollrc <= 0) ? 1 : 0;
        slirp_pollfds_poll(s->slirp, select_error,
            [](int idx, void* op) -> int {
                auto* pfds = static_cast<std::vector<sys_pollfd>*>(op);
                if (idx < 0 || idx >= (int)pfds->size()) return 0;
                return sys_to_slirp((*pfds)[idx].revents);
            }, &pfds);

        // ── Step 7: Fire expired timers ───────────────────────────────────
        check_ra_timeout(s);
    }
}

// ── VirtioNet::init ───────────────────────────────────────────────────────────
void VirtioNet::init(const std::string& /*tap_name*/) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        throw std::runtime_error("[virtio-net] WSAStartup failed");
#endif

    auto* s = new SlirpState();
    s->vnet = this;

    int wakeup_fds[2];
    if (make_wakeup_pair(wakeup_fds) != 0)
        throw std::runtime_error("[virtio-net] failed to create wakeup pipe");
    s->wakeup_r = wakeup_fds[0];
    s->wakeup_w = wakeup_fds[1];

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

    // Follow slirp4netns: start at version 1, bump only for features used.
    SlirpConfig cfg{};
    memset(&cfg, 0, sizeof(cfg));
    cfg.version    = 1;
    cfg.restricted = 0;
    cfg.in_enabled = 1;

    inet_pton(AF_INET, "10.0.2.0",      &cfg.vnetwork);
    inet_pton(AF_INET, "255.255.255.0",  &cfg.vnetmask);
    inet_pton(AF_INET, "10.0.2.2",      &cfg.vhost);
    inet_pton(AF_INET, "10.0.2.15",     &cfg.vdhcp_start);
    inet_pton(AF_INET, "10.0.2.3",      &cfg.vnameserver);
    cfg.if_mtu = 1500;
    cfg.if_mru = 1500;

    // Bump version for the slirp_os_socket callbacks (libslirp >= 4.9.0).
#if SLIRP_CONFIG_VERSION_MAX >= 6
    cfg.version = 6;
#endif

    s->slirp = slirp_new(&cfg, &cb, s);
    if (!s->slirp) {
        delete s;
        throw std::runtime_error("[virtio-net] slirp_new() failed");
    }

    backend_ = s;

    std::cout << "[virtio-net] libslirp ready (10.0.2.0/24, gateway 10.0.2.2)\n"
              << "  Guest: udhcpc -i eth0\n"
              << "  DNS  : 10.0.2.3\n";

    s->running = true;
    s->poll_thread = std::thread(poll_thread_func, s);
}

// ── platform_send ─────────────────────────────────────────────────────────────
// Called from the CPU thread. Enqueue the frame and wake the poll thread.
// Must never call slirp_input() directly — libslirp is not thread-safe.
void VirtioNet::platform_send(const uint8_t* frame, uint32_t len) {
    if (!backend_) return;
    auto* s = static_cast<SlirpState*>(backend_);
    {
        std::lock_guard<std::mutex> lk(s->tx_mutex);
        s->tx_queue.emplace(frame, frame + len);
    }
    wakeup_write(s->wakeup_w);
}