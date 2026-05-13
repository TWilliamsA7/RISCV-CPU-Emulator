// src/cpu/cpu.cpp

#include "cpu/cpu.hpp"
#include "memory/memory.hpp"
#include "errors/errors.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>

CPU::CPU (CPUConfig config, Bus& bus, Clint& clint) : config_(config), bus_(bus), clint_(clint), pc_(0x80000000), mmu_(*this) {
    regs_.fill(0);
    csrs_[CSR::MVENDORID] = 0xF00DFACE;
    privilege_level_ = PrivilegeLevel::MACHINE;
    csrs_[CSR::MSTATUS] = (3 << 11);

    uint32_t misa = (1U << 30); // RV32
    misa |= (1 << 8); // I (Base)

    if (config_.mode == ExecutionMode::SYSTEM) {
        misa |= (1 << 18); // S (Supervisor extension)
        misa |= (1 << 20); // U (User extension)
    }

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
    state_ = CPUState::ACTIVE;

    while (state_ != CPUState::HALTED) {

        updateCycle();

        if (state_ == CPUState::WAITING_FOR_INTERRUPT) {
            if (csrs_[CSR::MIP] & csrs_[CSR::MIE]) {
                state_ = CPUState::ACTIVE;
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }
        }

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

        uint32_t first_half_address = mmu_.translate(pc_, MMU::AccessType::FETCH);
        uint16_t first_half = bus_.read16(first_half_address);
        
        if ((first_half & 0x3) != 0x3) {
            if (!config_.extension_c) {
                // Illegal if C is disabled
                trap(ExceptionCause::ILLEGAL_INSTRUCTION, first_half, false); 
                return sr;
            }
            sr.instruction = decompress(first_half);
            instr_len = 2;
        } else {
            // 32-bit instruction
            uint32_t second_half_address = mmu_.translate(pc_ + 2, MMU::AccessType::FETCH);
            uint16_t second_half = bus_.read16(second_half_address);
            sr.instruction = (second_half << 16) | first_half;
            instr_len = 4;
        } 
    } catch (const BusAccessError& e) {
        trap(ExceptionCause::INSTRUCTION_ACCESS_FAULT, pc_, false);
        return sr;
    } catch (const InstructionPageError&  e) {
        trap(ExceptionCause::INSTRUCTION_PAGE_FAULT, pc_, false);
        return sr;
    }

    // Decode and execute
    sr.pc_before = pc_;
    DecodedInstr di = decode(sr.instruction);
    di.instr_len = instr_len;
    sr.dInstr = di;
    execute(di);

    if (trap_occurred_) return sr;

    csrs_[CSR::MINSTRET]++;
    if (csrs_[CSR::MINSTRET] == 0)
        csrs_[CSR::MINSTRETH]++;

    if (next_pc_.has_value()) {
        pc_ = next_pc_.value();
    } else {
        pc_ += instr_len;
    }

    sr.pc_after = pc_;
    if (config_.verbose) printTrace();

    return sr;
}

void CPU::updateCycle() {
    clint_.updateMtime();

    uint32_t low_before = csrs_[CSR::MCYCLE]; 
    csrs_[CSR::MCYCLE]++;
    
    // If mcycle wrapped around to 0, increment mcycleh
    if (csrs_[CSR::MCYCLE] < low_before) {
        csrs_[CSR::MCYCLEH]++;
    }

    // The 'cycle' (0xC00) and 'cycleh' (0xC80) are read-only views of mcycle
    csrs_[CSR::CYCLE] = csrs_[CSR::MCYCLE];
    csrs_[CSR::CYCLEH] = csrs_[CSR::MCYCLEH];

    // if (clint_.mtime > 0 && clint_.mtime % 100 == 0) { // Log every 100 ticks to avoid spam
    // printf("DEBUG: mtime=%llu, mtimecmp=%llu, MIP_MTIP=%d\n", 
    //        clint_.mtime, clint_.mtimecmp, (csrs_[CSR::MIP] >> 7) & 1);
    // }

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

std::optional<uint32_t> CPU::readCSR(uint16_t addr) {

    uint32_t required_privilege = (addr >> 8) & 0x3;
    if (privilege_level_ < required_privilege) {
        trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
        return std::nullopt;
    }

    if (addr == CSR::SATP && privilege_level_ == PrivilegeLevel::SUPERVISOR) {
        if ((csrs_[CSR::MSTATUS] >> 20) & 1) {  // TVM bit
            trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
            return std::nullopt;
        }
    }

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

bool CPU::writeCSR(uint16_t addr, uint32_t val) {
    // Check bits [11:10]. If they are 11 (0xCxx), it's Read-Only
    if ((addr >> 10) == 0x3) {
        trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false); 
        return false;
    }

    uint32_t required_privilege = (addr >> 8) & 0x3;
    if (privilege_level_ < required_privilege) {
        trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
        return false;
    }

    if (addr == CSR::SATP && privilege_level_ == PrivilegeLevel::SUPERVISOR) {
        if ((csrs_[CSR::MSTATUS] >> 20) & 1) {
            trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
            return false;
        }
    }

    switch (addr) {
        case CSR::MSTATUS: {
            uint32_t mask = 0x007E19EE;

            uint32_t mpp = (val >> 11) & 0x3;
            if (mpp == 2) mpp = 1;  // clamp invalid MPP to S-mode (or U-mode if no S)
            val = (val & ~(3u << 11)) | (mpp << 11);  // write sanitized value back
            csrs_[CSR::MSTATUS] = (val & mask);
            break;
        }
        case CSR::MTVEC: {
            uint32_t mode = val & 0x3;
            if (mode > 1) {
                val = (val & ~0x3); 
            }
            csrs_[CSR::MTVEC] = val; 
            break;
        }
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

        case CSR::SSTATUS: {
            uint32_t mask = 0x000DE122;
            csrs_[CSR::MSTATUS] = (csrs_[CSR::MSTATUS] & ~mask) | (val & mask);
            break;
        }

        case CSR::SIE: {
            uint32_t mask = csrs_[CSR::MIDELEG];
            csrs_[CSR::MIE] = (csrs_[CSR::MIE] & ~mask) | (val & mask);
            break;
        }

        case CSR::SIP: {
            uint32_t mask = csrs_[CSR::MIDELEG] & 0x2;
            csrs_[CSR::MIP] = (csrs_[CSR::MIP] & ~mask) | (val & mask);
            break;
        }

        default:
            csrs_[addr] = val;
            break;
    }

    return true;
}

void CPU::trap(uint32_t cause, uint32_t tval, bool is_interrupt, PrivilegeLevel target_level) {
    trap_occurred_ = true;

    if (config_.verbose) {
        std::cout << "TRAP " << (is_interrupt ? "(INTERRUPT) " : "")
            << "CAUSE: " << cause << " VAL: " << tval << "\n";
    }

    uint32_t cause_val = is_interrupt ? (cause | (1U << 31)) : cause;
    uint32_t mstatus = csrs_[CSR::MSTATUS];

    // printf("TRAP | cause: 0x%08X, tval: 0x%08X, PC: 0x%08X MTVEC: 0x%08X\n", cause_val, tval, pc_, csrs_[MTVEC]);

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

        uint32_t stvec = csrs_[CSR::STVEC];
        uint32_t base = stvec & ~3;
        uint32_t mode = stvec & 3;

        if (is_interrupt && mode == 1) {
            pc_ = base + (cause * 4);
        } else {
            pc_ = base;
        }
    }
}

void CPU::checkInterrupts() {
    uint32_t mip_mie = csrs_[CSR::MIP] & csrs_[CSR::MIE];
    if (mip_mie == 0) return;

    uint32_t mstatus = csrs_[CSR::MSTATUS];

    for (int id : {11, 9, 3, 1, 7, 5}) {
        if (!(mip_mie & (1 << id))) continue;

        bool delegate = (csrs_[CSR::MIDELEG] >> id) & 1;

        if (delegate && privilege_level_ <= PrivilegeLevel::SUPERVISOR) {
            bool sie = (mstatus >> 1) & 1;
            if (privilege_level_ < PrivilegeLevel::SUPERVISOR || sie) {
                trap(id, 0, true, PrivilegeLevel::SUPERVISOR);
                return;
            }
        } else {
            bool mie = (mstatus >> 3) & 1;
            if (privilege_level_ < PrivilegeLevel::MACHINE || mie) {
                trap(id, 0, true, PrivilegeLevel::MACHINE);
                return;
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
    return state_ == CPUState::HALTED;
}


