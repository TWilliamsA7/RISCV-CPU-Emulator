// src/devices/virtio_blk.cpp

#include "devices/virtio_blk.hpp"
#include "bus/bus.hpp"
#include <cstring>
#include <stdexcept>

VirtioBlk::VirtioBlk(std::function<void(uint32_t)> set_pending,
                     Bus& bus)
    : set_pending_(set_pending), bus_(bus)
{}

void VirtioBlk::init(const std::string& disk_path) {
    disk_.open(disk_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!disk_.is_open())
        throw std::runtime_error("Failed to open disk image: " + disk_path);
}

uint32_t VirtioBlk::read32(uint32_t offset) {
    switch (offset) {
        case VIRTIO_MAGIC:         return 0x74726976; // "virt"
        case VIRTIO_VERSION:       return 0x2;        // non-legacy
        case VIRTIO_DEVICE_ID:     return 0x2;        // block device
        case VIRTIO_VENDOR_ID:     return 0x554D4551; // "QEMU"
        case VIRTIO_DEVICE_FEATS:
            // Feature bits — report none
            // xv6 only needs basic block read/write
            return 0;
        case VIRTIO_QUEUE_NUM_MAX: return QUEUE_SIZE;
        case VIRTIO_QUEUE_READY:   return queue_ready_;
        case VIRTIO_INT_STATUS:    return interrupt_status_;
        case VIRTIO_STATUS:        return status_;
        case VIRTIO_CONFIG_GEN:    return 0;
        case VIRTIO_CONFIG:
        case VIRTIO_CONFIG + 4: {
            // Block device config: capacity in sectors (uint64)
            disk_.seekg(0, std::ios::end);
            uint64_t size = disk_.tellg();
            uint64_t sectors = size / SECTOR_SIZE;
            if (offset == VIRTIO_CONFIG)
                return (uint32_t)(sectors);
            else
                return (uint32_t)(sectors >> 32);
        }
        default: return 0;
    }
}

void VirtioBlk::write32(uint32_t offset, uint32_t val) {
    switch (offset) {
        case VIRTIO_DEVICE_FEAT_SEL: device_feat_sel_ = val; break;
        case VIRTIO_DRIVER_FEATS:    driver_features_  = val; break;
        case VIRTIO_DRIVER_FEAT_SEL: driver_feat_sel_  = val; break;
        case VIRTIO_QUEUE_SEL:       queue_sel_        = val; break;
        case VIRTIO_QUEUE_NUM:       queue_num_        = val; break;
        case VIRTIO_QUEUE_READY:     queue_ready_      = val; break;

        case VIRTIO_QUEUE_DESC_LO:   desc_addr_        = val; break;
        case VIRTIO_QUEUE_DESC_HI:   break; // RV32, always 0
        case VIRTIO_QUEUE_AVAIL_LO:  avail_addr_       = val; break;
        case VIRTIO_QUEUE_AVAIL_HI:  break;
        case VIRTIO_QUEUE_USED_LO:   used_addr_        = val; break;
        case VIRTIO_QUEUE_USED_HI:   break;

        case VIRTIO_QUEUE_NOTIFY:
            // Driver just submitted requests — process them
            process_queue();
            break;

        case VIRTIO_INT_ACK:
            interrupt_status_ &= ~val;
            break;

        case VIRTIO_STATUS:
            status_ = val;
            // If driver resets device (writes 0), reset state
            if (val == 0) {
                queue_ready_      = 0;
                interrupt_status_ = 0;
                last_avail_idx_   = 0;
                desc_addr_        = 0;
                avail_addr_       = 0;
                used_addr_        = 0;
            }
            break;

        default: break;
    }
}

void VirtioBlk::process_queue() {
    
    if (!queue_ready_ || !desc_addr_ || !avail_addr_ || !used_addr_)
    return;
    
    // Read available ring index
    // avail ring layout: uint16 flags, uint16 idx, uint16 ring[QUEUE_SIZE]
    uint16_t avail_idx = (uint16_t)bus_.read16(avail_addr_ + 2);



    while (last_avail_idx_ != avail_idx) {
        // Get next descriptor head index from available ring
        uint32_t ring_offset = avail_addr_ + 4 + (last_avail_idx_ % QUEUE_SIZE) * 2;
        uint16_t head = (uint16_t)bus_.read16(ring_offset);
        handle_request(head);

        // Write to used ring
        // used ring layout: uint16 flags, uint16 idx,
        //                   struct{uint32 id, uint32 len}[QUEUE_SIZE]
        uint16_t used_idx = (uint16_t)bus_.read16(used_addr_ + 2);
        uint32_t used_ring_entry = used_addr_ + 4 + (used_idx % QUEUE_SIZE) * 8;
        bus_.write32_unlocked(used_ring_entry,     head); // id
        bus_.write32_unlocked(used_ring_entry + 4, 0);    // len (bytes written)

        // Increment used idx
        bus_.write16_unlocked(used_addr_ + 2, used_idx + 1);

        last_avail_idx_++;
    }

    // Assert interrupt — used buffer notification
    interrupt_status_ |= 0x1;
    set_pending_(IRQ);
}

void VirtioBlk::handle_request(uint16_t head) {
    // Descriptor chain for a block request is 3 descriptors:
    // [0] virtio_blk_req header (type, reserved, sector)  READ by device
    // [1] data buffer (sector data)                        READ or WRITE
    // [2] status byte                                      WRITE by device

    // Descriptor layout: uint64 addr, uint32 len, uint16 flags, uint16 next
    auto read_desc = [&](uint16_t idx, uint64_t& addr, uint32_t& len,
                         uint16_t& flags, uint16_t& next) {
        uint32_t d = desc_addr_ + idx * 16;
        uint32_t addr_lo = bus_.read32(d);
        uint32_t addr_hi = bus_.read32(d + 4); // always 0 on RV32
        addr  = ((uint64_t)addr_hi << 32) | addr_lo;
        len   = bus_.read32(d + 8);
        flags = (uint16_t)bus_.read16(d + 12);
        next  = (uint16_t)bus_.read16(d + 14);
    };

    uint64_t addr; uint32_t len; uint16_t flags, next;

    // Descriptor 0: request header
    read_desc(head, addr, len, flags, next);

    // virtio_blk_req: uint32 type, uint32 reserved, uint64 sector
    uint32_t type     = bus_.read32((uint32_t)addr);
    uint64_t sector   = (uint64_t)bus_.read32((uint32_t)addr + 8)
                      | ((uint64_t)bus_.read32((uint32_t)addr + 12) << 32);

    // Descriptor 1: data buffer
    uint16_t data_desc_idx = next;
    read_desc(data_desc_idx, addr, len, flags, next);

    uint32_t data_addr = (uint32_t)addr;

    // Descriptor 2: status byte
    uint16_t status_desc_idx = next;
    uint64_t status_addr_64; uint32_t status_len;
    uint16_t status_flags, status_next;
    read_desc(status_desc_idx, status_addr_64, status_len,
              status_flags, status_next);
    uint32_t status_addr = (uint32_t)status_addr_64;

    // Perform disk operation
    uint8_t status = 0; // VIRTIO_BLK_S_OK

    if (type == VIRTIO_BLK_T_IN) {
        // Read from disk into guest memory
        disk_.seekg(sector * SECTOR_SIZE);
        if (!disk_) { bus_.write8_unlocked(status_addr, 1); return; }

        uint32_t offset = data_addr - Bus::DRAM_BASE;
        disk_.read(reinterpret_cast<char*>(bus_.dram_.data() + offset), len);
        if (!disk_) { bus_.write8_unlocked(status_addr, 1); return; }

    } else if (type == VIRTIO_BLK_T_OUT) {
        // Write from guest memory to disk
        std::vector<uint8_t> buf(len);
        for (uint32_t i = 0; i < len; i++)
            buf[i] = bus_.read8(data_addr + i);

        disk_.seekp(sector * SECTOR_SIZE);
        if (!disk_) { bus_.write8_unlocked(status_addr, 1); return; }

        disk_.write(reinterpret_cast<char*>(buf.data()), len);
        disk_.flush();
        if (!disk_) { bus_.write8_unlocked(status_addr, 1); return; }
    }

    bus_.write8_unlocked(status_addr, status);
}