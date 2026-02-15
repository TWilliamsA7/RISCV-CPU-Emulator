// src/cpu/cpu.cpp

#include "cpu/cpu.hpp"
#include "memory/memory.hpp"

#include <algorithm>
#include <cassert>

CPU::CPU(Memory& mem) : pc_(0), memory_(mem) {
    std::fill(std::begin(r), std::end(r), 0);
}


StepResult CPU::step() {

}

void CPU::writeReg(uint8_t rd, uint32_t value) {
    if (rd != 0)
        r[rd] = value;
}

uint32_t CPU::pc() const { return pc_; }

uint32_t CPU::reg(size_t idx) const {
    assert(idx < 32);
    return r[idx];
}


