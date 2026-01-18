// inc/memory/memory.hpp

#pragma once
#include <cstdint>

class Memory {
    public:
        uint8_t read8(uint32_t addr);
        uint16_t read16(uint32_t addr);
        uint32_t read32(uint32_t addr);

        void write8(uint32_t addr, uint8_t val);
        void write16(uint32_t addr, uint16_t val);
        void write32(uint32_t addr, uint32_t val);
};