// src/cpu/cpu.cpp

#include "cpu/cpu.hpp"
#include "memory/memory.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>

CPU::CPU (Bus& bus) : bus_(bus), pc_(0x80000000) {
    regs_.fill(0);
    csrs_.fill(0);
    csrs_[0xF11] = 0xF00DFACE;
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
    sr.instruction = bus_.read32(pc_);
    DecodedInstr di = decode(sr.instruction);
    sr.dInstr = di;
    execute(di);
    
    // Cycle counting
    // mcycle
    uint32_t low_before = csrs_[0xB00]; 
    csrs_[0xB00]++;
    
    // If mcycle wrapped around to 0, increment mcycleh
    if (csrs_[0xB00] < low_before) {
        csrs_[0xB80]++;
    }

    // The 'cycle' (0xC00) and 'cycleh' (0xC80) are read-only views of mcycle
    csrs_[0xC00] = csrs_[0xB00];
    csrs_[0xC80] = csrs_[0xB80];

    csrs_[0xB02]++; // minstret
    if (csrs_[0xB02] == 0) csrs_[0xB82]++; // minstret h

    sr.pc_after = pc_;
    if (true) printTrace();
    return sr;
}

void CPU::clearStep() {
    sr.dInstr = DecodedInstr{},
    sr.pc_before = 0;
    sr.pc_after = 0;
    sr.instruction = 0x0;
    sr.mem_write.reset();
    sr.reg_write.reset();
    sr.csr_write.reset();
}

void CPU::writeReg(uint8_t rd, uint32_t value) {
    if (rd != 0)
        regs_[rd] = value;
}

void CPU::writeCSR(uint16_t addr, uint32_t val) {
    // Check bits [11:10]. If they are 11 (0xCxx), it's Read-Only
    if ((addr >> 10) == 0x3) {
        return; 
    }

    switch (addr) {
        case 0x300: // mstatus
            // Only allow writing to supported bits (like MIE)
            csrs_[0x300] = val & 0x00001888; 
            break;
        case 0x305: // mtvec
            // Only allow valid modes (0 or 1)
            if ((val & 0x3) <= 1) csrs_[0x305] = val;
            break;
        case 0x341: // mepc
            csrs_[0x341] = val & ~0x1; // Force alignment
            break;
        default:
            csrs_[addr] = val;
            break;
    }
}

void CPU::setPC(uint32_t pc) { pc_ = pc; }

uint32_t CPU::pc() const { return pc_; }

uint32_t CPU::reg(size_t idx) const {
    assert(idx < 32);
    return regs_[idx];
}

bool CPU::isHalted() const {
    return halted;
}


