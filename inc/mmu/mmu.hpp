// inc/mmu/mmu.hpp

#pragma once

#include <cstdint>

class CPU;

class MMU {
    public:

        explicit MMU(CPU& cpu);

        static constexpr uint32_t PAGE_SIZE = 4096;
        static constexpr uint8_t PTE_SIZE = 4;
        static constexpr uint8_t LEVELS = 2;
        static constexpr uint32_t VPN_MASK = 0x3FF;
        static constexpr uint32_t PPN_MASK = 0x3FFFFF;

        static constexpr uint32_t PTE_V = (1 << 0);
        static constexpr uint32_t PTE_R = (1 << 1);
        static constexpr uint32_t PTE_W = (1 << 2);
        static constexpr uint32_t PTE_X = (1 << 3);
        static constexpr uint32_t PTE_U = (1 << 4);
        static constexpr uint32_t PTE_G = (1 << 5);
        static constexpr uint32_t PTE_A = (1 << 6);
        static constexpr uint32_t PTE_D = (1 << 7);



        enum AccessType {
            FETCH,
            LOAD,
            STORE
        };

        uint32_t translate(uint32_t va, AccessType type);
        
    private:
        void triggerPageFault(AccessType type);
        CPU& cpu_;
};