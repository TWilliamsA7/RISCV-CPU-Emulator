// inc/devices/virtio_net.hpp

#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <atomic>
#include <mutex>
#include <string>

class Bus;

class VirtioNet {
public:
    static constexpr uint32_t BASE       = 0x10002000;
    static constexpr uint32_t SIZE       = 0x1000;
    static constexpr uint32_t IRQ        = 2;
    static constexpr uint32_t QUEUE_SIZE = 64;

    static constexpr uint32_t RXQ = 0;
    static constexpr uint32_t TXQ = 1;

    VirtioNet(std::function<void(uint32_t)> set_pending, Bus& bus);
    ~VirtioNet();

    void init(const std::string& tap_name);

    uint32_t read32(uint32_t offset);
    uint8_t  read8 (uint32_t offset);
    void     write32(uint32_t offset, uint32_t val);

    // Called from the poll thread via cb_send_packet.
    // Acquires queue_mutex_ internally.
    void rx_inject(const uint8_t* frame, uint32_t len);

private:
    struct Queue {
        uint32_t num            = 0;
        uint32_t ready          = 0;
        uint32_t desc_addr      = 0;
        uint32_t avail_addr     = 0;
        uint32_t used_addr      = 0;
        uint16_t last_avail_idx = 0;
    };

    // Protects queues_[] and all Queue member accesses.
    // Acquired by the CPU thread in read32/write32/process_tx_queue
    // and by the poll thread in rx_inject.
    std::mutex queue_mutex_;

    Queue    queues_[2];
    uint32_t queue_sel_        = 0;
    uint32_t status_           = 0;

    // Accessed from both threads — must be atomic.
    std::atomic<uint32_t> interrupt_status_{0};

    uint32_t device_feat_sel_  = 0;
    uint32_t driver_feat_sel_  = 0;
    uint32_t driver_features_  = 0;

    uint8_t mac_[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

    std::function<void(uint32_t)> set_pending_;
    Bus& bus_;

    // Opaque pointer to SlirpState — avoids leaking libslirp types into header.
    void* backend_ = nullptr;

    // Called with queue_mutex_ held.
    void process_tx_queue();
    void read_desc(uint16_t idx, uint32_t queue_idx,
                   uint64_t& addr, uint32_t& len,
                   uint16_t& flags, uint16_t& next);
    void push_used(uint32_t queue_idx, uint16_t head, uint32_t len);

    // Enqueues frame for the poll thread. Does NOT call slirp_* directly.
    void platform_send(const uint8_t* frame, uint32_t len);

    static constexpr uint32_t VIRTIO_MAGIC          = 0x000;
    static constexpr uint32_t VIRTIO_VERSION        = 0x004;
    static constexpr uint32_t VIRTIO_DEVICE_ID      = 0x008;
    static constexpr uint32_t VIRTIO_VENDOR_ID      = 0x00C;
    static constexpr uint32_t VIRTIO_DEVICE_FEATS   = 0x010;
    static constexpr uint32_t VIRTIO_DEVICE_FEAT_SEL= 0x014;
    static constexpr uint32_t VIRTIO_DRIVER_FEATS   = 0x020;
    static constexpr uint32_t VIRTIO_DRIVER_FEAT_SEL= 0x024;
    static constexpr uint32_t VIRTIO_QUEUE_SEL      = 0x030;
    static constexpr uint32_t VIRTIO_QUEUE_NUM_MAX  = 0x034;
    static constexpr uint32_t VIRTIO_QUEUE_NUM      = 0x038;
    static constexpr uint32_t VIRTIO_QUEUE_READY    = 0x044;
    static constexpr uint32_t VIRTIO_QUEUE_NOTIFY   = 0x050;
    static constexpr uint32_t VIRTIO_INT_STATUS     = 0x060;
    static constexpr uint32_t VIRTIO_INT_ACK        = 0x064;
    static constexpr uint32_t VIRTIO_STATUS         = 0x070;
    static constexpr uint32_t VIRTIO_QUEUE_DESC_LO  = 0x080;
    static constexpr uint32_t VIRTIO_QUEUE_DESC_HI  = 0x084;
    static constexpr uint32_t VIRTIO_QUEUE_AVAIL_LO = 0x090;
    static constexpr uint32_t VIRTIO_QUEUE_AVAIL_HI = 0x094;
    static constexpr uint32_t VIRTIO_QUEUE_USED_LO  = 0x0A0;
    static constexpr uint32_t VIRTIO_QUEUE_USED_HI  = 0x0A4;
    static constexpr uint32_t VIRTIO_CONFIG_GEN     = 0x0FC;
    static constexpr uint32_t VIRTIO_CONFIG         = 0x100;

    static constexpr uint16_t VIRTQ_DESC_F_NEXT  = 1;
    static constexpr uint16_t VIRTQ_DESC_F_WRITE = 2;

    static constexpr uint32_t VIRTIO_NET_F_MAC   = (1u << 5);
    static constexpr uint32_t VIRTIO_F_VERSION_1 = (1u << 0);
};