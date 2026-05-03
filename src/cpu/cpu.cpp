// src/cpu/cpu.cpp

#include "cpu/cpu.hpp"
#include "memory/memory.hpp"
#include "errors/errors.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>

CPU::CPU (CPUConfig config, Bus& bus, Clint& clint) : config_(config), bus_(bus), clint_(clint), pc_(0x80000000) {
    regs_.fill(0);
    csrs_.fill(0);
    csrs_[CSR::MVENDORID] = 0xF00DFACE;
    priviledge_level_ = PrivilegeLevel::MACHINE;
    csrs_[CSR::MSTATUS] = (3 << 11);

    uint32_t misa = (1U << 30); // RV32
    misa |= (1 << 8); // I (Base)
    if (config_.extension_m) misa |= (1U << 12);
    if (config.extension_c){ 
        misa |= (1U << 2);
        ADDRESS_MISALIGNMENT_MASK = 0x1;
    } else {
        ADDRESS_MISALIGNMENT_MASK = 0x3;
    }
    csrs_[CSR::MISA] = misa;
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

    // Check for Asynchronous Interrupts 
    checkInterrupts();
    if (trap_occurred_) return sr;

    uint32_t instr_len;

    // Fetch
    try {

        if (pc_ & ADDRESS_MISALIGNMENT_MASK) trap(0, pc_, false);

        uint16_t first_half = bus_.read16(pc_);
        
        if ((first_half & 0x3) != 0x3) {
            if (!config_.extension_c) {
                // Illegal if C is disabled
                trap(2, first_half, false); 
                return sr;
            }
            sr.instruction = decompress(first_half);
            instr_len = 2;
        } else {
            // 32-bit instruction
            uint16_t second_half = bus_.read16(pc_ + 2);
            sr.instruction = (second_half << 16) | first_half;
            instr_len = 4;
        } 
    } catch (const BusAccessError& e) {
        trap(1, pc_, false);
        return sr;
    }

    // Decode and execute
    sr.pc_before = pc_;
    DecodedInstr di = decode(sr.instruction);
    sr.dInstr = di;
    execute(di);

    if (trap_occurred_) return sr;

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
    csrs_[CSR::MINSTRET]++;
    if (csrs_[CSR::MINSTRET] == 0)
        csrs_[CSR::MINSTRETH]++;

    // // Tick CLINT and reflect into MIP
    // clint_.tick();

    // // MSIP -> MIP bit 3
    // if (clint_.msip)
    //     csrs_[CSR::MIP] |= (1 << 3);
    // else
    //     csrs_[CSR::MIP] &= ~(1 << 3);

    // // MTIP -> MIP bit 7 (timer)
    // if (clint_.mtime >= clint_.mtimecmp)
    //     csrs_[CSR::MIP] |= (1 << 7);
    // else
    //     csrs_[CSR::MIP] &= ~(1 << 7);

    if (next_pc_.has_value()) {
        pc_ = next_pc_.value();
    } else {
        pc_ += instr_len;
    }

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
    next_pc_.reset();
    trap_occurred_ = false;
}

void CPU::writeReg(uint8_t rd, uint32_t value) {
    if (rd != 0)
        regs_[rd] = value;
}

uint32_t CPU::readCSR(uint16_t addr) {
    uint32_t val = csrs_[addr];
    return val;
}

void CPU::writeCSR(uint16_t addr, uint32_t val) {
    // Check bits [11:10]. If they are 11 (0xCxx), it's Read-Only
    if ((addr >> 10) == 0x3) {
        trap(2, sr.instruction, false); 
        return;
    }

    switch (addr) {
        case CSR::MSTATUS: {
            // bits 11-12 (MPP) are hardwired to 3 for M-mode only implementations.
            uint32_t writable_mask = 0x00001888; // Adjust based on supported features
            uint32_t hardwired_bits = (3 << 11); // MPP must be 3
            csrs_[CSR::MSTATUS] = (val & writable_mask) | hardwired_bits;
            break;
        }
        case CSR::MTVEC:
            // Only allow valid modes (0 or 1)
            if ((val & 0x3) <= 1) csrs_[CSR::MTVEC] = val;
            break;
        case CSR::MEPC:
            csrs_[CSR::MEPC] = val & ~0x1; // Force alignment
            break;

        case CSR::MISA:
            break;
        
        default:
            csrs_[addr] = val;
            break;
    }
}

void CPU::trap(uint32_t cause, uint32_t tval, bool is_interrupt) {
    trap_occurred_ = true;

    // Prepare MSTATUS Fields
    uint32_t mstatus = csrs_[CSR::MSTATUS];
    uint32_t mie = (mstatus >> 3) & 1;

    // Save current MIE into MPIE (bit 7)
    mstatus = (mstatus & ~(1 << 7)) | (mie << 7);

    // Clear current MIE (bit 3) to disable interrupts during the handler
    mstatus &= ~(1 << 3);

    // Save current privilege into MPP (bits 11-12)
    mstatus = (mstatus & ~(3 << 11)) | (static_cast<uint32_t>(priviledge_level_) << 11);

    csrs_[CSR::MSTATUS] = mstatus;

    // Set Trap Cause Registers
    uint32_t cause_val = cause;

    // If interrupt, set bit 31
    if (is_interrupt) {
        cause_val |= (1U << 31);
    }

    csrs_[CSR::MCAUSE] = cause_val;

    // Save the PC where the trap occurred
    csrs_[CSR::MEPC] = pc_;

    // Save specific trap info
    csrs_[CSR::MTVAL] = tval;

    // Elevate priviledge mode
    priviledge_level_ = PrivilegeLevel::MACHINE;

    // Calculate jump target
    uint32_t mtvec = csrs_[CSR::MTVEC];
    uint32_t base = mtvec & ~3;
    uint32_t mode = mtvec & 3;

    if (is_interrupt && mode == 1) {
        pc_ = base + (cause * 4);
    } else {
        pc_ = base;
    }
}

void CPU::checkInterrupts() {
    uint32_t mstatus = csrs_[CSR::MSTATUS];
    bool m_ie = (mstatus >> 3) & 1;

    // M-mode interrupts are enabled if mstatus.MIE is 1
    if (priviledge_level_ == PrivilegeLevel::MACHINE && !m_ie) return;

    uint32_t pending = csrs_[CSR::MIP] & csrs_[CSR::MIE];

    if (pending != 0) {
        if (pending & (1 << 11)) trap(11, 0, true);
        else if (pending & (1 << 3)) trap(3, 0, true);
        else if (pending & (1 << 7)) trap(7, 0, true);
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


