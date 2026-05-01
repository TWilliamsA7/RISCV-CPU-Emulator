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
    uint32_t low_before = csrs_[CSR::MCYCLE]; 
    csrs_[CSR::MCYCLE]++;
    
    // If mcycle wrapped around to 0, increment mcycleh
    if (csrs_[CSR::MCYCLE] < low_before) {
        csrs_[CSR::MCYCLEH]++;
    }

    // The 'cycle' (0xC00) and 'cycleh' (0xC80) are read-only views of mcycle
    csrs_[CSR::CYCLE] = csrs_[CSR::MCYCLE];
    csrs_[CSR::CYCLEH] = csrs_[CSR::MCYCLEH];

    csrs_[CSR::MINSTRET]++; // minstret
    if (csrs_[CSR::MINSTRET] == 0) csrs_[CSR::MINSTRETH]++; // minstret h

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
        case CSR::MSTATUS:
            // Only allow writing to supported bits (like MIE)
            csrs_[CSR::MSTATUS] = val & 0x00001888; 
            break;
        case CSR::MTVEC:
            // Only allow valid modes (0 or 1)
            if ((val & 0x3) <= 1) csrs_[CSR::MTVEC] = val;
            break;
        case CSR::MEPC:
            csrs_[CSR::MEPC] = val & ~0x1; // Force alignment
            break;
        default:
            csrs_[addr] = val;
            break;
    }
}

void CPU::trap(uint32_t cause, uint32_t tval) {
    // Save problematic PC
    csrs_[CSR::MEPC] = pc_;
    
    // Save cause (MSB is 0 for exceptions, 1 for interrupts)
    csrs_[CSR::MCAUSE] = cause;

    // Save additional trap info
    csrs_[CSR::MTVAL] = tval;

    uint32_t mstatus = csrs_[CSR::MSTATUS];
    uint32_t mie = (mstatus >> 3) & 1;

    mstatus &= ~(1 << 7);
    mstatus |= (mie << 7);
    mstatus &= ~(1 << 3);

    mstatus |= (3 << 11);
    csrs_[CSR::MSTATUS] = mstatus;

    // Jump to trap handler
    uint32_t mtvec = csrs_[CSR::MTVEC];
    uint32_t mode = mtvec & 0x3;
    uint32_t base = mtvec & ~0x3;

    if (mode == 0) { 
        // Direct mode
        pc_ = base;
    } else {
        // Vectored mode (Used for interrupts)
        pc_ = base + (cause & 0x7FFFFFFF) * 4;
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


