// inc/mmu/mmu.hpp

#pragma once

#include <cstdint>
#include "mmu/tlb.hpp"
#include "types.hpp"
#include "errors/errors.hpp"

class Emulator;

class MMU {
    public:

        explicit MMU(Emulator& sys);

        static constexpr uint32_t PAGE_SIZE = 4096;
        static constexpr uint32_t PAGE_MASK = ~(PAGE_SIZE - 1);
        static constexpr uint8_t PTE_SIZE = 4;
        static constexpr uint8_t LEVELS = 2;
        static constexpr uint32_t VPN_MASK = 0x3FF;
        static constexpr uint32_t PPN_MASK = 0x3FFFFF;
        static constexpr uint16_t ASID_MASK = 0x1FF;

        static constexpr uint32_t PTE_V = (1 << 0);
        static constexpr uint32_t PTE_R = (1 << 1);
        static constexpr uint32_t PTE_W = (1 << 2);
        static constexpr uint32_t PTE_X = (1 << 3);
        static constexpr uint32_t PTE_U = (1 << 4);
        static constexpr uint32_t PTE_G = (1 << 5);
        static constexpr uint32_t PTE_A = (1 << 6);
        static constexpr uint32_t PTE_D = (1 << 7);

        uint32_t translate(uint32_t va, AccessType type);
        
    private:
        Emulator& sys_;

        uint32_t translate_walk(uint32_t va, AccessType type, PrivilegeLevel effective_level, uint32_t full_vpn, uint16_t asid);
        void triggerPageFault(AccessType type);

        // 3. Inline permission checks so they don't break the fast-path speed
        void check_tlb_permissions(const TLBEntry& e, AccessType at, uint32_t priv, uint32_t va);
};