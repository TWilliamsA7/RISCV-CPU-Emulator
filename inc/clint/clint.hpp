// inc/clint/clint.hpp

#pragma once

#include <cstdint>
#include <chrono> 

class Clint {
    public:

        Clint();

        static constexpr uint32_t BASE        = 0x02000000;
        static constexpr uint32_t MSIP_OFFSET = 0x00000000;
        static constexpr uint32_t MTIMECMP_LO = 0x00004000;
        static constexpr uint32_t MTIMECMP_HI = 0x00004004;
        static constexpr uint32_t MTIME_LO    = 0x0000BFF8;
        static constexpr uint32_t MTIME_HI    = 0x0000BFFC;
        static constexpr uint32_t SIZE        = 0x0000C000;

        bool msip = false;           // software interrupt pending
        uint64_t mtime    = 0;
        uint64_t mtimecmp = UINT64_MAX; // disabled by default

        uint32_t read32(uint32_t offset);
        void write32(uint32_t offset, uint32_t val);
        void updateMtime();

    private:
        std::chrono::high_resolution_clock::time_point start_time_;
        uint64_t frequency_ = 10000000;


};