// inc/devices/virtio_blk.hpp

#pragma once
#include <cstdint>
#include <string>
#include <fstream>
#include <functional>

class Bus;

class VirtioBlk {
public:
    static constexpr uint32_t BASE = 0x10001000;
    static constexpr uint32_t SIZE = 0x1000;
    static constexpr uint32_t IRQ  = 1;
    static constexpr uint32_t QUEUE_SIZE = 8;
    static constexpr uint32_t SECTOR_SIZE = 512;

    VirtioBlk(const std::string& disk_path,
              std::function<void(uint32_t)> set_pending,
              Bus& bus);

    uint32_t read32(uint32_t offset);
    void     write32(uint32_t offset, uint32_t val);

private:
    // MMIO registers
    uint32_t status_          = 0;
    uint32_t queue_sel_       = 0;
    uint32_t queue_num_       = 0;
    uint32_t queue_ready_     = 0;
    uint32_t interrupt_status_= 0;
    uint32_t driver_features_ = 0;
    uint32_t driver_feat_sel_ = 0;
    uint32_t device_feat_sel_ = 0;

    // Virtqueue physical addresses (set by driver)
    uint32_t desc_addr_   = 0;
    uint32_t avail_addr_  = 0;
    uint32_t used_addr_   = 0;

    uint16_t last_avail_idx_ = 0;

    std::fstream disk_;
    std::function<void(uint32_t)> set_pending_;
    Bus& bus_;

    void process_queue();
    void handle_request(uint16_t head);

    // Virtio MMIO v2 register offsets
static constexpr uint32_t VIRTIO_MAGIC         = 0x000;
static constexpr uint32_t VIRTIO_VERSION       = 0x004;
static constexpr uint32_t VIRTIO_DEVICE_ID     = 0x008;
static constexpr uint32_t VIRTIO_VENDOR_ID     = 0x00C;
static constexpr uint32_t VIRTIO_DEVICE_FEATS  = 0x010;
static constexpr uint32_t VIRTIO_DEVICE_FEAT_SEL= 0x014;
static constexpr uint32_t VIRTIO_DRIVER_FEATS  = 0x020;
static constexpr uint32_t VIRTIO_DRIVER_FEAT_SEL= 0x024;
static constexpr uint32_t VIRTIO_QUEUE_SEL     = 0x030;
static constexpr uint32_t VIRTIO_QUEUE_NUM_MAX = 0x034;
static constexpr uint32_t VIRTIO_QUEUE_NUM     = 0x038;
static constexpr uint32_t VIRTIO_QUEUE_READY   = 0x044;
static constexpr uint32_t VIRTIO_QUEUE_NOTIFY  = 0x050;
static constexpr uint32_t VIRTIO_INT_STATUS    = 0x060;
static constexpr uint32_t VIRTIO_INT_ACK       = 0x064;
static constexpr uint32_t VIRTIO_STATUS        = 0x070;
static constexpr uint32_t VIRTIO_QUEUE_DESC_LO = 0x080;
static constexpr uint32_t VIRTIO_QUEUE_DESC_HI = 0x084;
static constexpr uint32_t VIRTIO_QUEUE_AVAIL_LO= 0x090;
static constexpr uint32_t VIRTIO_QUEUE_AVAIL_HI= 0x094;
static constexpr uint32_t VIRTIO_QUEUE_USED_LO = 0x0A0;
static constexpr uint32_t VIRTIO_QUEUE_USED_HI = 0x0A4;
static constexpr uint32_t VIRTIO_CONFIG_GEN    = 0x0FC;
static constexpr uint32_t VIRTIO_CONFIG        = 0x100;

// Virtio block request types
static constexpr uint32_t VIRTIO_BLK_T_IN  = 0; // read
static constexpr uint32_t VIRTIO_BLK_T_OUT = 1; // write

// Virtqueue descriptor flags
static constexpr uint16_t VIRTQ_DESC_F_NEXT  = 1;
static constexpr uint16_t VIRTQ_DESC_F_WRITE = 2; // device writes to buffer
};