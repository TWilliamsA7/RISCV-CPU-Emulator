// src/devices/virtio_net/virtio_net_slirp.cpp
//
// libslirp backend for virtio-net with proper threading model.
// - All slirp_* calls happen ONLY on the poll thread
// - CPU thread enqueues TX frames via a queue
// - Poll thread processes TX and RX via libslirp

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
# include <winsock2.h>
# include <ws2tcpip.h>
# ifndef POLLIN
#  define POLLIN  0x0001
#  define POLLPRI 0x0002
#  define POLLOUT 0x0004
#  define POLLERR 0x0008
#  define POLLHUP 0x0010
# endif
typedef WSAPOLLFD sys_pollfd;
static inline int sys_poll(sys_pollfd* fds, int n, int timeout) {
    return WSAPoll(fds, (ULONG)n, timeout);
}
static int make_wakeup_pair(int fds[2]) {
    SOCKET srv = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    bind(srv, (sockaddr*)&addr, sizeof(addr));
    listen(srv, 1);
    int len = sizeof(addr);
    getsockname(srv, (sockaddr*)&addr, &len);
    SOCKET c = socket(AF_INET, SOCK_STREAM, 0);
    connect(c, (sockaddr*)&addr, sizeof(addr));
    SOCKET a = accept(srv, nullptr, nullptr);
    closesocket(srv);
    fds[0] = (int)a;
    fds[1] = (int)c;
    return 0;
}
static void wakeup_write(int fd) { char b = 1; send((SOCKET)fd, &b, 1, 0); }
static void wakeup_drain(int fd) { char b; recv((SOCKET)fd, &b, 1, 0); }
#else
# include <poll.h>
# include <arpa/inet.h>
# include <unistd.h>
typedef struct pollfd sys_pollfd;
static inline int sys_poll(sys_pollfd* fds, int n, int timeout) {
    return poll(fds, (nfds_t)n, timeout);
}
static int make_wakeup_pair(int fds[2]) { return pipe(fds); }
static void wakeup_write(int fd) { char b = 1; (void)write(fd, &b, 1); }
static void wakeup_drain(int fd) { char b; (void)read(fd, &b, 1); }
#endif

#if !SLIRP_CHECK_VERSION(4, 9, 0)
# define slirp_os_socket int
# define slirp_pollfds_fill_socket slirp_pollfds_fill
# define register_poll_socket register_poll_fd
# define unregister_poll_socket unregister_poll_fd
#endif

// ── SlirpTimer ────────────────────────────────────────────────────────────────
struct SlirpTimer {
    SlirpTimerCb cb;
    void*        cb_opaque;
    int64_t      expire_timer_msec = -1;
};

// ── SlirpState ────────────────────────────────────────────────────────────────
struct SlirpState {
    Slirp*       slirp = nullptr;
    VirtioNet*   vnet  = nullptr;
    SlirpCb      cb{};

    std::vector<SlirpTimer*> timers;
    std::vector<sys_pollfd> pfds;

    std::mutex                       tx_mutex;
    std::queue<std::vector<uint8_t>> tx_queue;

    int wakeup_r = -1;
    int wakeup_w = -1;

    std::thread       poll_thread;
    std::atomic<bool> running{false};
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

// ── Clock ─────────────────────────────────────────────────────────────────────
static int64_t cb_clock_get_ns(void* /*opaque*/) {
    using namespace std::chrono;
    return (int64_t)duration_cast<nanoseconds>(
        steady_clock::now().time_since_epoch()).count();
}

static int64_t clock_now_ms() {
    return cb_clock_get_ns(nullptr) / 1000000LL;
}

// ── Timer helpers ─────────────────────────────────────────────────────────────
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

// ── SlirpCb implementations ───────────────────────────────────────────────────
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

static void cb_register_poll_socket(slirp_os_socket /*sock*/, void* /*opaque*/) {}
static void cb_unregister_poll_socket(slirp_os_socket /*sock*/, void* /*opaque*/) {}

static void cb_notify(void* opaque) {
    wakeup_write(static_cast<SlirpState*>(opaque)->wakeup_w);
}

// ── Poll thread ───────────────────────────────────────────────────────────────
static void poll_thread_func(SlirpState* s) {
    while (s->running) {
        uint32_t timeout_ms = 10;

        s->pfds.clear();
        {
            sys_pollfd w{};
            w.fd     = s->wakeup_r;
            w.events = POLLIN;
            s->pfds.push_back(w);
        }

        slirp_pollfds_fill_socket(s->slirp, &timeout_ms,
            [](slirp_os_socket sock, int slirp_ev, void* op) -> int {
                auto* state = static_cast<SlirpState*>(op);
                sys_pollfd p{};
                p.fd     = (int)sock;
                p.events = slirp_to_sys(slirp_ev);
                int idx  = (int)state->pfds.size();
                state->pfds.push_back(p);
                return idx;
            }, s);

        update_ra_timeout(timeout_ms, s);

        int pollrc = sys_poll(s->pfds.data(), (int)s->pfds.size(), (int)timeout_ms);

        bool had_tx = false;
        if (!s->pfds.empty() && (s->pfds[0].revents & POLLIN)) {
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

        int select_error = (had_tx || pollrc <= 0) ? 1 : 0;
        slirp_pollfds_poll(s->slirp, select_error,
            [](int idx, void* op) -> int {
                auto* state = static_cast<SlirpState*>(op);
                if (idx < 0 || idx >= (int)state->pfds.size()) return 0;
                return sys_to_slirp(state->pfds[idx].revents);
            }, s);

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

    // Initialize callback table in SlirpState, not on stack
    s->cb.send_packet            = cb_send_packet;
    s->cb.guest_error            = cb_guest_error;
    s->cb.clock_get_ns           = cb_clock_get_ns;
    s->cb.timer_new              = cb_timer_new;
    s->cb.timer_free             = cb_timer_free;
    s->cb.timer_mod              = cb_timer_mod;
    s->cb.register_poll_socket   = cb_register_poll_socket;
    s->cb.unregister_poll_socket = cb_unregister_poll_socket;
    s->cb.notify                 = cb_notify;

    SlirpConfig cfg{};
    memset(&cfg, 0, sizeof(cfg));
    cfg.version = 1;
    cfg.restricted = 0;
    cfg.in_enabled = 1;

    inet_pton(AF_INET, "10.0.2.0",      &cfg.vnetwork);
    inet_pton(AF_INET, "255.255.255.0",  &cfg.vnetmask);
    inet_pton(AF_INET, "10.0.2.2",      &cfg.vhost);
    inet_pton(AF_INET, "10.0.2.15",     &cfg.vdhcp_start);
    inet_pton(AF_INET, "10.0.2.3",      &cfg.vnameserver);
    cfg.if_mtu = 1500;
    cfg.if_mru = 1500;

#if SLIRP_CONFIG_VERSION_MAX >= 2
    cfg.version = 2;
#endif
#if SLIRP_CONFIG_VERSION_MAX >= 4
    cfg.version = 4;
#endif
#if SLIRP_CONFIG_VERSION_MAX >= 6
    cfg.version = 6;
#endif

    s->slirp = slirp_new(&cfg, &s->cb, s);
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

void VirtioNet::platform_send(const uint8_t* frame, uint32_t len) {
    if (!backend_) return;
    auto* s = static_cast<SlirpState*>(backend_);
    {
        std::lock_guard<std::mutex> lk(s->tx_mutex);
        s->tx_queue.emplace(frame, frame + len);
    }
    wakeup_write(s->wakeup_w);
}