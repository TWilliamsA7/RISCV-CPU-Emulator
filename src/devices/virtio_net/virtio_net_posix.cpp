// src/devices/virtio_net_posix.cpp
//
// TAP backend for Linux and WSL.
// Opens /dev/tap<name>, enables the interface, and starts the RX thread.

#include "platform/platform.hpp"

#ifdef PLATFORM_POSIX

#include "devices/virtio_net.hpp"
#include <stdexcept>
#include <string>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <linux/if.h>
#include <linux/if_tun.h>

void VirtioNet::init(const std::string& tap_name) {
    // Open the TUN/TAP clone device
    tap_fd_ = open("/dev/net/tun", O_RDWR);
    if (tap_fd_ < 0)
        throw std::runtime_error(
            "[virtio-net] Failed to open /dev/net/tun — "
            "make sure the tun module is loaded and you have permission.\n"
            "  sudo modprobe tun\n"
            "  sudo chmod 0666 /dev/net/tun");

    struct ifreq ifr{};
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI; // TAP mode, no packet info header
    std::strncpy(ifr.ifr_name, tap_name.c_str(), IFNAMSIZ - 1);

    if (ioctl(tap_fd_, TUNSETIFF, &ifr) < 0) {
        close(tap_fd_);
        tap_fd_ = -1;
        throw std::runtime_error(
            "[virtio-net] TUNSETIFF failed for interface '" + tap_name + "'\n"
            "  Create the interface first:\n"
            "    sudo ip tuntap add dev " + tap_name + " mode tap\n"
            "    sudo ip addr add 192.168.100.1/24 dev " + tap_name + "\n"
            "    sudo ip link set " + tap_name + " up");
    }

    // Set non-blocking so the RX thread select() can time out cleanly
    int flags = fcntl(tap_fd_, F_GETFL, 0);
    fcntl(tap_fd_, F_SETFL, flags | O_NONBLOCK);

    std::cout << "[virtio-net] TAP interface '" << tap_name << "' opened (fd=" << tap_fd_ << ")\n";

    // Start the RX thread
    running_ = true;
    rx_thread_ = std::thread(&VirtioNet::rx_thread_func, this);
}

void VirtioNet::rx_thread_func() {
    std::vector<uint8_t> buf(2048);
    while (running_) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(tap_fd_, &fds);
        struct timeval tv{};
        tv.tv_usec = 10000; // 10ms timeout so thread can exit cleanly
        int r = select(tap_fd_ + 1, &fds, nullptr, nullptr, &tv);
        if (r <= 0) continue;
        ssize_t n = read(tap_fd_, buf.data(), buf.size());
        if (n > 0)
            rx_inject(buf.data(), (uint32_t)n);
    }
}

void VirtioNet::platform_send(const uint8_t* frame, uint32_t len) {
    if (tap_fd_ >= 0)
        write(tap_fd_, frame, len);
}

#endif