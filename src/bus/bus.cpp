// src/bus/bus.cpp

#include "bus/bus.hpp"
#include "errors/errors.hpp"
#include <iostream>

Bus::Bus(Clint& clint) : clint_(clint) {
    dram_ = std::vector<uint8_t>(Bus::DRAM_SIZE, 0);
}

uint8_t Bus::read8(uint32_t addr) const {
    if (addr >= Bus::DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        return dram_[offset];
    }
    return 0;
}

uint16_t Bus::read16(uint32_t addr) const {
    if (addr >= Bus::DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        return dram_[offset] | (dram_[offset+1] << 8);
    }
    return 0;
}

uint32_t Bus::read32(uint32_t addr) const {

    if (addr >= Clint::BASE && addr < Clint::BASE + Clint::SIZE)
        return clint_.read32(addr - Clint::BASE);
    if (addr >= Bus::DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        return dram_[offset] | (dram_[offset+1] << 8) | (dram_[offset+2] << 16) | (dram_[offset+3] << 24);
    }

    throw BusAccessError(addr + " is outside of mapped range");
}

void Bus::write8(uint32_t addr, uint8_t val) {
    if (addr >= Bus::DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        dram_[offset] = val;
    }
}

void Bus::write16(uint32_t addr, uint16_t val) {
    if (addr >= Bus::DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        dram_[offset] = val & 0xFF;
        dram_[offset + 1] = (val >> 8) & 0xFF;
    }
}

void Bus::write32(uint32_t addr, uint32_t val) {

    if (addr == 0x80001000) {
        if (val == 1U) {
            std::cout << "PASS: SUCCESSFUL WRITE TO HOST\n";
            exit(0);
        }
    }

    if (addr >= Clint::BASE && addr < Clint::BASE + Clint::SIZE) {
        clint_.write32(addr - Clint::BASE, val);
    } else if (addr >= Bus::DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        dram_[offset] = val & 0xFF;
        dram_[offset + 1] = (val >> 8) & 0xFF;
        dram_[offset + 2] = (val >> 16) & 0xFF;
        dram_[offset + 3] = (val >> 24) & 0xFF;
    }
}