#pragma once
#include <cstdint>
#include <array>

class PLIC {
    public:
        static constexpr uint32_t BASE = 0x0C000000;
        static constexpr uint32_t SIZE = 0x4000000;
        static constexpr uint32_t NUM_SOURCES = 32;

        // Two contexts: 0 = M-mode hart 0, 1 = S-mode hart 0
        static constexpr uint32_t CTX_MACHINE    = 0;
        static constexpr uint32_t CTX_SUPERVISOR = 1;

        PLIC();

        uint32_t read32(uint32_t offset) const;
        void write32(uint32_t offset, uint32_t val);

        // Called by Bus::write8 on UART interrupt, virtio, etc.
        void set_pending(uint32_t source);
        void clear_pending(uint32_t source);

        // Called by CPU::updateCycle() — returns whether MEIP/SEIP should be asserted
        bool m_interrupt_pending() const;
        bool s_interrupt_pending() const;

    private:
        // Priority for each source (1–7, 0 = disabled)
        std::array<uint32_t, NUM_SOURCES> priority_;

        // Pending bits — set by hardware, cleared by claim/complete
        uint32_t pending_;

        // Enable bits per context
        std::array<uint32_t, 2> enable_;

        // Priority threshold per context — only sources above this fire
        std::array<uint32_t, 2> threshold_;

        // In-service tracking — which source each context has claimed
        std::array<uint32_t, 2> claimed_;

        uint32_t best_pending(uint32_t ctx) const;
};