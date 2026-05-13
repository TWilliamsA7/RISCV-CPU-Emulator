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

        static constexpr uint32_t PTE_V = 0;
        static constexpr uint32_t PTE_R = 1;
        static constexpr uint32_t PTE_W = 2;
        static constexpr uint32_t PTE_X = 4;
        static constexpr uint32_t PTE_U = 8;
        static constexpr uint32_t PTE_G = 16;
        static constexpr uint32_t PTE_A = 32;
        static constexpr uint32_t PTE_D = 64;



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