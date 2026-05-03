// src/cpu/cpu.cpp

#include "cpu/cpu.hpp"
#include "memory/memory.hpp"
#include "errors/errors.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>

CPU::CPU (CPUConfig config, Bus& bus, Clint& clint) : config_(config), bus_(bus), clint_(clint), pc_(0x80000000) {
    regs_.fill(0);
    csrs_[CSR::MVENDORID] = 0xF00DFACE;
    privilege_level_ = PrivilegeLevel::MACHINE;
    csrs_[CSR::MSTATUS] = (3 << 11);

    uint32_t misa = (1U << 30); // RV32
    misa |= (1 << 8); // I (Base)
    misa |= (1 << 18); // S (Supervisor extension)
    misa |= (1 << 20); // U (User extension)

    if (config_.extension_m) misa |= (1U << 12);
    if (config_.extension_c) { 
        misa |= (1U << 2);
        ADDRESS_MISALIGNMENT_MASK = 0x1;
    } else {
        ADDRESS_MISALIGNMENT_MASK = 0x3;
    }
    csrs_[CSR::MISA] = misa;
}

void CPU::run() {
    while (!halted) {
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

    clint_.updateMtime();

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
    di.instr_len = instr_len;
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

    // MSIP -> MIP bit 3
    if (clint_.msip)
        csrs_[CSR::MIP] |= (1 << 3);
    else
        csrs_[CSR::MIP] &= ~(1 << 3);

    // MTIP -> MIP bit 7 (timer)
    if (clint_.mtime >= clint_.mtimecmp)
        csrs_[CSR::MIP] |= (1 << 7);
    else
        csrs_[CSR::MIP] &= ~(1 << 7);

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
    switch (addr) {
        case CSR::SSTATUS:
            return csrs_[CSR::MSTATUS] & 0x000DE122;
        case CSR::SIE:
            return csrs_[CSR::MIE] & csrs_[CSR::MIDELEG];
        case CSR::SIP:
            return csrs_[CSR::MIP] & csrs_[CSR::MIDELEG];
        default:
            return csrs_[addr];
    }
}

void CPU::writeCSR(uint16_t addr, uint32_t val) {
    // Check bits [11:10]. If they are 11 (0xCxx), it's Read-Only
    if ((addr >> 10) == 0x3) {
        trap(2, sr.instruction, false); 
        return;
    }

    switch (addr) {
        case CSR::MSTATUS: {
            uint32_t writeable_mask = 0x000E19AA;
            uint32_t mpp = (val >> 11) & 0x3;
            if (mpp == 2) mpp = 1;
            csrs_[CSR::MSTATUS] = (val & writeable_mask);
            break;
        }
        case CSR::MTVEC:
            // Only allow valid modes (0 or 1)
            if ((val & 0x3) <= 1) csrs_[CSR::MTVEC] = val;
            break;
        case CSR::MEPC:
            csrs_[CSR::MEPC] = val & ~0x1; // Force alignment
            break;

        case CSR::MEDELEG:
            csrs_[CSR::MEDELEG] = val & 0xFFFF;
            break;

        case CSR::MIDELEG:
            csrs_[CSR::MIDELEG] = val & 0xFFFF;
            break;

        case CSR::MISA:
            break;

        case CSR::SSTATUS: 
            uint32_t mask = 0x000DE122;
            csrs_[CSR::MSTATUS] = (csrs_[CSR::MSTATUS] & ~mask) | (val & mask);
            break;

        case CSR::SIE:
            uint32_t mask = csrs_[CSR::MIDELEG];
            csrs_[CSR::MIE] = (csrs_[CSR::MIE] & ~mask) | (val & mask);
            break;

        default:
            csrs_[addr] = val;
            break;
    }
}

void CPU::trap(uint32_t cause, uint32_t tval, bool is_interrupt, PrivilegeLevel target_level) {
    trap_occurred_ = true;
    uint32_t cause_val = is_interrupt ? (cause | (1U << 31)) : cause;
    uint32_t mstatus = csrs_[CSR::MSTATUS];

    if (target_level == PrivilegeLevel::MACHINE) {
        uint32_t mie = (mstatus >> 3) & 1;

        // Save current MIE into MPIE (bit 7)
        mstatus = (mstatus & ~(1 << 7)) | (mie << 7);
        // Clear current MIE (bit 3) to disable interrupts during the handler
        mstatus &= ~(1 << 3);
        // Save current privilege into MPP (bits 11-12)
        mstatus = (mstatus & ~(3 << 11)) | (static_cast<uint32_t>(privilege_level_) << 11);

        csrs_[CSR::MSTATUS] = mstatus;
        csrs_[CSR::MCAUSE] = cause_val;
        // Save the PC where the trap occurred
        csrs_[CSR::MEPC] = pc_;
        // Save specific trap info
        csrs_[CSR::MTVAL] = tval;

        privilege_level_ = PrivilegeLevel::MACHINE;

        uint32_t mtvec = csrs_[CSR::MTVEC];
        uint32_t base = mtvec & ~3;
        uint32_t mode = mtvec & 3;

        if (is_interrupt && mode == 1) {
            pc_ = base + (cause * 4);
        } else {
            pc_ = base;
        }
    } else { // Supervisor trap
        uint32_t sie = (mstatus >> 1) & 1;
        uint32_t spp = (privilege_level_ == PrivilegeLevel::SUPERVISOR) ? 1 : 0;

        mstatus = (mstatus & ~(1 << 5)) | (sie << 5);
        mstatus &= ~(1 << 1);
        mstatus = (mstatus & ~(1 << 8)) | (spp << 8);

        csrs_[CSR::MSTATUS] = mstatus;
        csrs_[CSR::SEPC] = pc_;
        csrs_[CSR::SCAUSE] = cause_val;
        csrs_[CSR::STVAL] = tval;

        privilege_level_ = PrivilegeLevel::SUPERVISOR;

        uint32_t mtvec = csrs_[CSR::STVEC];
        uint32_t base = mtvec & ~3;
        uint32_t mode = mtvec & 3;

        if (is_interrupt && mode == 1) {
            pc_ = base + (cause * 4);
        } else {
            pc_ = base;
        }
    }

    // Prepare MSTATUS Fields
    uint32_t mstatus = csrs_[CSR::MSTATUS];
    uint32_t mie = (mstatus >> 3) & 1;

    // Save current MIE into MPIE (bit 7)
    mstatus = (mstatus & ~(1 << 7)) | (mie << 7);

    // Clear current MIE (bit 3) to disable interrupts during the handler
    mstatus &= ~(1 << 3);

    // Save current privilege into MPP (bits 11-12)
    mstatus = (mstatus & ~(3 << 11)) | (static_cast<uint32_t>(privilege_level_) << 11);

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
    privilege_level_ = PrivilegeLevel::MACHINE;

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
    if (privilege_level_ == PrivilegeLevel::MACHINE && !m_ie) return;

    uint32_t pending = csrs_[CSR::MIP] & csrs_[CSR::MIE];

    for (int id : {11, 3, 7}) { // External, Software, Timer
        if (pending & (1 << id)) {
            bool delegate = (csrs_[CSR::MIDELEG] >> id) & 1;

            if (delegate && privilege_level_ <= PrivilegeLevel::SUPERVISOR) {
                // Signal to Supervisor mode
                csrs_[CSR::MIP] |= (1 << (id - 2));
                if (sModeInterruptsEnabled()) {
                    trap(id - 2, 0, true, PrivilegeLevel::SUPERVISOR); 
                    return;
                }
            } else {
                // Standard M-mode handling
                if (mModeInterruptsEnabled) {
                    trap(id, 0, true, PrivilegeLevel::MACHINE);
                    return;
                }
            }
        }
    }
}

bool CPU::mModeInterruptsEnabled() {
    uint32_t mstatus = csrs_[CSR::MSTATUS];
    bool m_global_ie = (mstatus >> 3) & 1;

    if (privilege_level_ < PrivilegeLevel::MACHINE) {
        return true;
    } else if (privilege_level_ == PrivilegeLevel::MACHINE) {
        return m_global_ie;
    }
    
    return false;
}

bool CPU::sModeInterruptsEnabled() {
    uint32_t mstatus = csrs_[CSR::MSTATUS];
    bool s_global_ie = (mstatus >> 1) & 1; // SIE bit (bit 1)

    if (privilege_level_ < PrivilegeLevel::SUPERVISOR) {
        return true;
    } else if (privilege_level_ == PrivilegeLevel::SUPERVISOR) {
        return s_global_ie;
    } else {
        return false;
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


