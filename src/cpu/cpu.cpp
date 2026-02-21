// src/cpu/cpu.cpp

#include "cpu/cpu.hpp"
#include "memory/memory.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>

CPU::CPU(Memory& mem) : pc_(0), memory_(mem) {
    std::fill(std::begin(r), std::end(r), 0);
}

CPU::CPU(Memory& mem, bool trace_enabled) : pc_(0), memory_(mem), trace_enabled_(trace_enabled) {
    std::fill(std::begin(r), std::end(r), 0);
}

void CPU::run() {
    while (!halted) {
        if (breakpoints_.contains(pc_)) {
            std::cout << "Breakpoint hit at PC=" << hex32(pc_) << "\n";
            break;
        }

        step();
    }
}

void CPU::run(uint32_t count) {
    for (uint32_t i = 0; i < count && !halted; i++) {
        step();
    }
}


StepResult CPU::step() {
    clearStep();
    sr.pc_before = pc_;
    sr.instruction = memory_.read32(pc_);
    DecodedInstr di = decode(sr.instruction);
    sr.dInstr = di;
    execute(di);
    sr.pc_after = pc_;
    if (trace_enabled_) printTrace();
    return sr;
}

void CPU::clearStep() {
    sr.dInstr = DecodedInstr{},
    sr.pc_before = 0;
    sr.pc_after = 0;
    sr.instruction = 0x0;
    sr.mem_write.reset();
    sr.reg_write.reset();
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


