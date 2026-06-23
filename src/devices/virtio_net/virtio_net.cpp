// src/devices/virtio_net.cpp

#include "devices/virtio_net.hpp"
#include "bus/bus.hpp"
#include <cstring>
#include <stdexcept>
#include <iostream>

// ── virtio-net header prepended to every packet ───────────────────────────────
// Spec section 5.1.6 virtio_net_hdr (without num_buffers — no MRG_RXBUF)
// Replace the VirtioNetHdr struct with the v1 version that includes num_buffers:
struct VirtioNetHdr {
    uint8_t  flags;
    uint8_t  gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
    uint16_t num_buffers;  // add this — required when VIRTIO_F_VERSION_1 is negotiated
};
static constexpr uint32_t NET_HDR_SIZE = sizeof(VirtioNetHdr); // now 12 bytes
VirtioNet::VirtioNet(std::function<void(uint32_t)> set_pending, Bus& bus)
    : set_pending_(set_pending), bus_(bus)
{}

VirtioNet::~VirtioNet() {
    running_ = false;
    if (rx_thread_.joinable())
        rx_thread_.join();
}

// ── MMIO register read ────────────────────────────────────────────────────────
uint32_t VirtioNet::read32(uint32_t offset) {
    const Queue& q = queues_[queue_sel_];
    switch (offset) {
        case VIRTIO_MAGIC:        return 0x74726976; // "virt"
        case VIRTIO_VERSION:      return 0x2;        // non-legacy
        case VIRTIO_DEVICE_ID:    return 0x1;        // network device
        case VIRTIO_VENDOR_ID:    return 0x554D4551; // "QEMU"

        case VIRTIO_DEVICE_FEATS:
            if (device_feat_sel_ == 0) return VIRTIO_NET_F_MAC;
            if (device_feat_sel_ == 1) return VIRTIO_F_VERSION_1;
            return 0;

        case VIRTIO_QUEUE_NUM_MAX: return QUEUE_SIZE;
        case VIRTIO_QUEUE_READY:   return q.ready;
        case VIRTIO_INT_STATUS:    return interrupt_status_;
        case VIRTIO_STATUS:        return status_;
        case VIRTIO_CONFIG_GEN:    return 0;

        // virtio-net config: 6-byte MAC then 2-byte status (spec 5.1.4)
        case VIRTIO_CONFIG + 0:
            return (uint32_t)mac_[0]
                 | ((uint32_t)mac_[1] << 8)
                 | ((uint32_t)mac_[2] << 16)
                 | ((uint32_t)mac_[3] << 24);
        case VIRTIO_CONFIG + 4:
            return (uint32_t)mac_[4]
                 | ((uint32_t)mac_[5] << 8)
                 | (1u << 16); // VIRTIO_NET_S_LINK_UP

        default: return 0;
    }
}

// ── MMIO register write ───────────────────────────────────────────────────────
void VirtioNet::write32(uint32_t offset, uint32_t val) {
    Queue& q = queues_[queue_sel_];
    switch (offset) {
        case VIRTIO_DEVICE_FEAT_SEL: device_feat_sel_ = val; break;
        case VIRTIO_DRIVER_FEATS:    driver_features_  = val; break;
        case VIRTIO_DRIVER_FEAT_SEL: driver_feat_sel_  = val; break;
        case VIRTIO_QUEUE_SEL:
            if (val < 2) queue_sel_ = val;
            break;
        case VIRTIO_QUEUE_NUM:       q.num        = val; break;
        case VIRTIO_QUEUE_READY:     q.ready      = val; break;
        case VIRTIO_QUEUE_DESC_LO:   q.desc_addr  = val; break;
        case VIRTIO_QUEUE_DESC_HI:   break; // RV32 — always 0
        case VIRTIO_QUEUE_AVAIL_LO:  q.avail_addr = val; break;
        case VIRTIO_QUEUE_AVAIL_HI:  break;
        case VIRTIO_QUEUE_USED_LO:   q.used_addr  = val; break;
        case VIRTIO_QUEUE_USED_HI:   break;

        case VIRTIO_QUEUE_NOTIFY:
            if (val == TXQ) process_tx_queue();
            // RXQ kick: driver added more RX buffers, no action needed
            break;

        case VIRTIO_INT_ACK:
            interrupt_status_ &= ~val;
            break;

        case VIRTIO_STATUS:
            status_ = val;
            if (val == 0) { // driver reset
                for (auto& queue : queues_) {
                    queue.num = queue.ready = 0;
                    queue.desc_addr = queue.avail_addr = queue.used_addr = 0;
                    queue.last_avail_idx = 0;
                }
                interrupt_status_ = 0;
            }
            break;

        default: break;
    }
}

uint8_t VirtioNet::read8(uint32_t offset) {
    if (offset >= 0x100 && offset < 0x106) {
        // MAC bytes 0-5
        return mac_[offset - 0x100];
    }
    if (offset == 0x106 || offset == 0x107) {
        // status: VIRTIO_NET_S_LINK_UP = 1
        return (offset == 0x106) ? 1 : 0;
    }
    // Fall back to 32-bit read for other registers
    uint32_t val = read32(offset & ~0x3);
    return (val >> ((offset & 0x3) * 8)) & 0xFF;
}

// ── Descriptor chain helper ───────────────────────────────────────────────────
void VirtioNet::read_desc(uint16_t idx, uint32_t queue_idx,
                           uint64_t& addr, uint32_t& len,
                           uint16_t& flags, uint16_t& next) {
    uint32_t d = queues_[queue_idx].desc_addr + idx * 16;
    addr  = (uint64_t)bus_.read32(d) | ((uint64_t)bus_.read32(d + 4) << 32);
    len   = bus_.read32(d + 8);
    flags = (uint16_t)bus_.read16(d + 12);
    next  = (uint16_t)bus_.read16(d + 14);
}

// ── Write one entry to the used ring ─────────────────────────────────────────
void VirtioNet::push_used(uint32_t queue_idx, uint16_t head, uint32_t len) {
    Queue& q       = queues_[queue_idx];
    uint16_t uidx  = (uint16_t)bus_.read16(q.used_addr + 2);
    uint32_t entry = q.used_addr + 4 + (uidx % QUEUE_SIZE) * 8;
    bus_.write32_unlocked(entry,     head);
    bus_.write32_unlocked(entry + 4, len);
    bus_.write16_unlocked(q.used_addr + 2, uidx + 1);
}


// ── TX queue processing ───────────────────────────────────────────────────────
void VirtioNet::process_tx_queue() {
    Queue& q = queues_[TXQ];
    if (!q.ready || !q.desc_addr || !q.avail_addr || !q.used_addr) return;
    if (tap_fd_ < 0) return;

    uint16_t avail_idx = (uint16_t)bus_.read16(q.avail_addr + 2);

    while (q.last_avail_idx != avail_idx) {
        uint32_t ring_off = q.avail_addr + 4 + (q.last_avail_idx % QUEUE_SIZE) * 2;
        uint16_t head     = (uint16_t)bus_.read16(ring_off);

        // Assemble the full Ethernet frame from the descriptor chain.
        // First descriptor begins with a virtio_net_hdr which we skip.
        std::vector<uint8_t> pkt;
        uint16_t cur = head;
        bool first   = true;

        while (true) {
            uint64_t addr; uint32_t len; uint16_t flags, next;
            read_desc(cur, TXQ, addr, len, flags, next);

            uint32_t skip = first ? NET_HDR_SIZE : 0;
            uint32_t copy = (len > skip) ? (len - skip) : 0;
            first = false;

            for (uint32_t i = 0; i < copy; i++)
                pkt.push_back(bus_.read8((uint32_t)addr + skip + i));

            if (!(flags & VIRTQ_DESC_F_NEXT)) break;
            cur = next;
        }

        if (!pkt.empty())
            platform_send(pkt.data(), (uint32_t)pkt.size());

        push_used(TXQ, head, 0);
        q.last_avail_idx++;
    }

    interrupt_status_ |= 0x1;
    set_pending_(IRQ);
}

// ── RX injection ─────────────────────────────────────────────────────────────
void VirtioNet::rx_inject(const uint8_t* frame, uint32_t len) {
    Queue& q = queues_[RXQ];
    if (!q.ready || !q.desc_addr || !q.avail_addr || !q.used_addr) return;

    uint16_t avail_idx = (uint16_t)bus_.read16(q.avail_addr + 2);
    if (q.last_avail_idx == avail_idx) return; // no RX buffers — drop

    uint32_t ring_off = q.avail_addr + 4 + (q.last_avail_idx % QUEUE_SIZE) * 2;
    uint16_t head     = (uint16_t)bus_.read16(ring_off);

    uint16_t cur   = head;
    uint32_t written = 0;
    bool first = true;

    while (true) {
        uint64_t addr; uint32_t dlen; uint16_t flags, next;
        read_desc(cur, RXQ, addr, dlen, flags, next);

        if (first) {
            // Write virtio_net_hdr then the Ethernet frame into the buffer
            VirtioNetHdr hdr{};
            hdr.flags       = 0;
            hdr.gso_type    = 0;
            hdr.num_buffers = 1; // spec: MUST be 1 when VIRTIO_NET_F_MRG_RXBUF not negotiated
            const uint8_t* hp = reinterpret_cast<const uint8_t*>(&hdr);
            for (uint32_t i = 0; i < NET_HDR_SIZE && i < dlen; i++)
                bus_.write8_unlocked((uint32_t)addr + i, hp[i]);

            uint32_t space = dlen - NET_HDR_SIZE;
            uint32_t copy  = (len < space) ? len : space;
            for (uint32_t i = 0; i < copy; i++)
                bus_.write8_unlocked((uint32_t)addr + NET_HDR_SIZE + i, frame[i]);

            written += NET_HDR_SIZE + copy;
            first = false;
        } else {
            uint32_t offset    = written - NET_HDR_SIZE;
            uint32_t remaining = (len > offset) ? (len - offset) : 0;
            uint32_t copy      = (remaining < dlen) ? remaining : dlen;
            for (uint32_t i = 0; i < copy; i++)
                bus_.write8_unlocked((uint32_t)addr + i, frame[offset + i]);
            written += copy;
        }

        if (!(flags & VIRTQ_DESC_F_NEXT)) break;
        cur = next;
    }

    push_used(RXQ, head, written);
    q.last_avail_idx++;

    interrupt_status_ |= 0x1;
    set_pending_(IRQ);
}
