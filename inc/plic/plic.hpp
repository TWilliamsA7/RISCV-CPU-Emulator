#pragma once
#include <cstdint>
#include <array>
#include <atomic>

class PLIC {
    public:
        static constexpr uint32_t BASE = 0x0C000000;
        static constexpr uint32_t SIZE = 0x4000000;
        static constexpr uint32_t NUM_SOURCES = 32;

        static constexpr uint32_t CTX_MACHINE    = 0;
        static constexpr uint32_t CTX_SUPERVISOR = 1;

        PLIC();

        uint32_t read32(uint32_t offset);
        void write32(uint32_t offset, uint32_t val);

        // Called from the poll thread (virtio-net) and CPU thread (virtio-blk).
        // pending_ is std::atomic so these are safe without an external lock.
        void set_pending(uint32_t source);
        void clear_pending(uint32_t source);

        // Called from CPU thread only (updateCycle).
        bool m_interrupt_pending() const;
        bool s_interrupt_pending() const;

    private:
        std::array<uint32_t, NUM_SOURCES> priority_;

        // Atomic: written by poll thread (set_pending from virtio-net rx_inject)
        // and read by CPU thread (best_pending in updateCycle).
        std::atomic<uint32_t> pending_{0};

        std::array<uint32_t, 2> enable_;
        std::array<uint32_t, 2> threshold_;
        std::array<uint32_t, 2> claimed_;

        uint32_t best_pending(uint32_t ctx) const;
};