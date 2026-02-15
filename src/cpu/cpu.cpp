// src/cpu/cpu.cpp

#include "cpu/cpu.hpp"
#include "memory/memory.hpp"

#include <algorithm>
#include <cassert>

CPU::CPU(Memory& mem) : pc_(0), memory_(mem) {
    std::fill(std::begin(r), std::end(r), 0);
}


StepResult CPU::step() {
    StepResult sr;
    sr.pc_before = pc_;
    sr.instruction = memory_.read32(pc_);
    DecodedInstr di = decode(sr.instruction);
    sr.dInstr = di;
    execute(di);
    sr.pc_after = pc_;
    return sr;
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

bool CPU::isHalted() const {
    return halted;
}


