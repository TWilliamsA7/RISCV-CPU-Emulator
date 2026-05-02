// inc/clint/clint.hpp

#pragma once
#include <cstdint>

class Clint {
    public:
        static constexpr uint32_t BASE        = 0x2000000;
        static constexpr uint32_t MSIP_OFFSET = 0x0000000;
        static constexpr uint32_t MTIMECMP_LO = 0x0004000;
        static constexpr uint32_t MTIMECMP_HI = 0x0004004;
        static constexpr uint32_t MTIME_LO    = 0x000BFF8;
        static constexpr uint32_t MTIME_HI    = 0x000BFFC;
        static constexpr uint32_t SIZE        = 0x000C000;

        bool msip = false;           // software interrupt pending
        uint64_t mtime    = 0;
        uint64_t mtimecmp = UINT64_MAX; // disabled by default

        uint32_t read32(uint32_t offset);
        void write32(uint32_t offset, uint32_t val);
        void tick();


};