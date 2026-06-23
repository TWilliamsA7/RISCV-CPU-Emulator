// src/devices/virtio_net_win.cpp
//
// WinTun backend for native Windows builds.
//
// WinTun is a Layer 3 TUN device — it delivers raw IPv4/IPv6 packets with
// no Ethernet header. Since virtio-net is a Layer 2 Ethernet device we:
//   RX (WinTun -> guest): prepend a synthetic Ethernet header
//   TX (guest -> WinTun): strip the Ethernet header, send raw IP
//
// Prerequisites:
//   1. Download wintun.dll from https://wintun.net
//      Place wintun.dll next to the emulator .exe (or in System32).
//   2. Run the emulator as Administrator — WinTun requires elevation to
//      create adapters.
//   3. After the adapter appears in Windows, assign it an IP address:
//        netsh interface ip set address "riscvemu" static 192.168.100.1 255.255.255.0
//      Or share an existing internet connection to the adapter via ICS.

#include "platform/platform.hpp"

#ifdef PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <unordered_map>

#include "devices/virtio_net.hpp"
#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

// ── WinTun opaque handle types and function pointer typedefs ─────────────────
// From https://git.zx2c4.com/wintun/tree/api/wintun.h
typedef VOID* WINTUN_ADAPTER_HANDLE;
typedef VOID* WINTUN_SESSION_HANDLE;

typedef WINTUN_ADAPTER_HANDLE (WINAPI *WINTUN_CREATE_ADAPTER_FUNC)
    (LPCWSTR Name, LPCWSTR TunnelType, const GUID *RequestedGUID);
typedef VOID (WINAPI *WINTUN_CLOSE_ADAPTER_FUNC)
    (WINTUN_ADAPTER_HANDLE Adapter);
typedef WINTUN_SESSION_HANDLE (WINAPI *WINTUN_START_SESSION_FUNC)
    (WINTUN_ADAPTER_HANDLE Adapter, DWORD Capacity);
typedef VOID (WINAPI *WINTUN_END_SESSION_FUNC)
    (WINTUN_SESSION_HANDLE Session);
typedef HANDLE (WINAPI *WINTUN_GET_READ_WAIT_EVENT_FUNC)
    (WINTUN_SESSION_HANDLE Session);
typedef BYTE* (WINAPI *WINTUN_RECEIVE_PACKET_FUNC)
    (WINTUN_SESSION_HANDLE Session, DWORD *PacketSize);
typedef VOID (WINAPI *WINTUN_RELEASE_RECEIVE_PACKET_FUNC)
    (WINTUN_SESSION_HANDLE Session, const BYTE *Packet);
typedef BYTE* (WINAPI *WINTUN_ALLOCATE_SEND_PACKET_FUNC)
    (WINTUN_SESSION_HANDLE Session, DWORD PacketSize);
typedef VOID (WINAPI *WINTUN_SEND_PACKET_FUNC)
    (WINTUN_SESSION_HANDLE Session, const BYTE *Packet);

// ── Module-level WinTun state (one instance per process) ─────────────────────
static HMODULE                           g_wintun_dll         = nullptr;
static WINTUN_CREATE_ADAPTER_FUNC        g_CreateAdapter      = nullptr;
static WINTUN_CLOSE_ADAPTER_FUNC         g_CloseAdapter       = nullptr;
static WINTUN_START_SESSION_FUNC         g_StartSession       = nullptr;
static WINTUN_END_SESSION_FUNC           g_EndSession         = nullptr;
static WINTUN_GET_READ_WAIT_EVENT_FUNC   g_GetReadWaitEvent   = nullptr;
static WINTUN_RECEIVE_PACKET_FUNC        g_ReceivePacket      = nullptr;
static WINTUN_RELEASE_RECEIVE_PACKET_FUNC g_ReleaseReceivePacket = nullptr;
static WINTUN_ALLOCATE_SEND_PACKET_FUNC  g_AllocateSendPacket = nullptr;
static WINTUN_SEND_PACKET_FUNC           g_SendPacket         = nullptr;

// Per-instance WinTun state hidden behind a void* to keep Windows types
// out of virtio_net.hpp
struct WintunState {
    WINTUN_ADAPTER_HANDLE adapter    = nullptr;
    WINTUN_SESSION_HANDLE session    = nullptr;
    HANDLE                read_event = nullptr;
};

// Map from VirtioNet* to its WintunState — avoids adding Windows types to header
static std::unordered_map<VirtioNet*, WintunState*> g_state_map;

// ── Load wintun.dll and resolve all function pointers ────────────────────────
static bool load_wintun() {
    if (g_wintun_dll) return true;

    g_wintun_dll = LoadLibraryExW(L"wintun.dll", nullptr,
        LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!g_wintun_dll) {
        std::cerr << "[virtio-net] Could not load wintun.dll\n"
                  << "  Download from https://wintun.net and place next to the emulator.\n";
        return false;
    }

    #define GET(var, name) \
        var = (decltype(var))GetProcAddress(g_wintun_dll, name); \
        if (!var) { \
            std::cerr << "[virtio-net] wintun.dll missing symbol: " name "\n"; \
            FreeLibrary(g_wintun_dll); g_wintun_dll = nullptr; return false; \
        }

    GET(g_CreateAdapter,        "WintunCreateAdapter")
    GET(g_CloseAdapter,         "WintunCloseAdapter")
    GET(g_StartSession,         "WintunStartSession")
    GET(g_EndSession,           "WintunEndSession")
    GET(g_GetReadWaitEvent,     "WintunGetReadWaitEvent")
    GET(g_ReceivePacket,        "WintunReceivePacket")
    GET(g_ReleaseReceivePacket, "WintunReleaseReceivePacket")
    GET(g_AllocateSendPacket,   "WintunAllocateSendPacket")
    GET(g_SendPacket,           "WintunSendPacket")
    #undef GET

    return true;
}

// ── Narrow -> wide string helper ──────────────────────────────────────────────
static std::wstring widen(const std::string& s) {
    std::wstring w(s.size(), L'\0');
    for (size_t i = 0; i < s.size(); i++)
        w[i] = static_cast<wchar_t>(static_cast<unsigned char>(s[i]));
    return w;
}

// ── Ethernet constants ────────────────────────────────────────────────────────
static constexpr uint32_t ETH_HDR_LEN   = 14;
static constexpr uint16_t ETHERTYPE_IP4 = 0x0800;
static constexpr uint16_t ETHERTYPE_IP6 = 0x86DD;
// Fixed MAC for the "host" side of the link (appears as the gateway MAC)
static constexpr uint8_t HOST_MAC[6] = {0x52, 0x54, 0x00, 0xAB, 0xCD, 0xEF};

// ── Wrap raw IP packet in an Ethernet frame for the guest ─────────────────────
static std::vector<uint8_t> make_eth_frame(const uint8_t* ip, DWORD ip_len,
                                            const uint8_t guest_mac[6]) {
    uint16_t etype = (ip_len > 0 && (ip[0] >> 4) == 6) ? ETHERTYPE_IP6 : ETHERTYPE_IP4;

    std::vector<uint8_t> frame;
    frame.reserve(ETH_HDR_LEN + ip_len);

    // dst MAC = guest, src MAC = host/gateway
    for (int i = 0; i < 6; i++) frame.push_back(guest_mac[i]);
    for (int i = 0; i < 6; i++) frame.push_back(HOST_MAC[i]);
    frame.push_back(static_cast<uint8_t>(etype >> 8));
    frame.push_back(static_cast<uint8_t>(etype & 0xFF));
    for (DWORD i = 0; i < ip_len; i++) frame.push_back(ip[i]);
    return frame;
}

// ── VirtioNet::init ───────────────────────────────────────────────────────────
void VirtioNet::init(const std::string& tap_name) {
    if (!load_wintun()) {
        std::cout << "[virtio-net] WinTun not available — networking disabled.\n";
        tap_fd_ = -1;
        return;
    }

    auto* wt = new WintunState();

    wt->adapter = g_CreateAdapter(widen(tap_name).c_str(), L"RISCVEmu", nullptr);
    if (!wt->adapter) {
        delete wt;
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED)
            throw std::runtime_error(
                "[virtio-net] Access denied — run the emulator as Administrator.");
        throw std::runtime_error(
            "[virtio-net] WintunCreateAdapter failed for '" + tap_name +
            "' error=" + std::to_string(err));
    }

    // Ring capacity: 4MB, must be power of two in [0x20000, 0x4000000]
    wt->session = g_StartSession(wt->adapter, 0x400000);
    if (!wt->session) {
        g_CloseAdapter(wt->adapter);
        delete wt;
        throw std::runtime_error("[virtio-net] WintunStartSession failed error=" +
                                  std::to_string(GetLastError()));
    }

    wt->read_event = g_GetReadWaitEvent(wt->session);

    // Register state and mark as initialised
    g_state_map[this] = wt;
    tap_fd_ = 1; // sentinel: > 0 means WinTun is active

    std::cout << "[virtio-net] WinTun adapter '" << tap_name << "' ready.\n"
              << "  Assign host IP: netsh interface ip set address \""
              << tap_name << "\" static 192.168.100.1 255.255.255.0\n"
              << "  Guest commands:\n"
              << "    ip link set eth0 up\n"
              << "    ip addr add 192.168.100.2/24 dev eth0\n"
              << "    ip route add default via 192.168.100.1\n";

    // Start RX thread
    running_ = true;
    rx_thread_ = std::thread([this, wt]() {
        std::vector<uint8_t> frame;
        while (running_) {
            DWORD pkt_size = 0;
            BYTE* pkt = g_ReceivePacket(wt->session, &pkt_size);
            if (pkt) {
                frame = make_eth_frame(pkt, pkt_size, mac_);
                g_ReleaseReceivePacket(wt->session, pkt);
                rx_inject(frame.data(), static_cast<uint32_t>(frame.size()));
            } else {
                DWORD err = GetLastError();
                if (err == ERROR_HANDLE_EOF) break; // adapter shutting down
                if (err == ERROR_NO_MORE_ITEMS)
                    WaitForSingleObject(wt->read_event, 10 /*ms*/);
                // Other errors: keep going
            }
        }
    });
}

// ── platform_send: strip Ethernet header and inject into WinTun ───────────────
// Called from virtio_net.cpp process_tx_queue instead of write().
// The frame from the guest starts with a 14-byte Ethernet header followed by
// the raw IP packet. WinTun only wants the IP payload.
void VirtioNet::platform_send(const uint8_t* frame, uint32_t len) {
    auto it = g_state_map.find(this);
    if (it == g_state_map.end() || !it->second->session) return;
    WintunState* wt = it->second;

    if (len <= ETH_HDR_LEN) return; // too short to contain IP

    // Skip the 14-byte Ethernet header
    const uint8_t* ip_pkt = frame + ETH_HDR_LEN;
    DWORD ip_len = static_cast<DWORD>(len - ETH_HDR_LEN);

    // Allocate send buffer and copy
    BYTE* buf = g_AllocateSendPacket(wt->session, ip_len);
    if (buf) {
        memcpy(buf, ip_pkt, ip_len);
        g_SendPacket(wt->session, buf);
    }
    // If buf is null the ring is full — silently drop (matches WinTun docs)
}

#endif // PLATFORM_WINDOWS