// src/cpu/cpu.cpp

#include "cpu/cpu.hpp"
#include "memory/memory.hpp"
#include "errors/errors.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>

CPU::CPU (CPUConfig config, Bus& bus, Clint& clint, PLIC& plic) 
    : config_(config), bus_(bus), clint_(clint), pc_(0x80000000), mmu_(*this), plic_(plic) {
    regs_.fill(0);
    csrs_.fill(0);
    csrs_[CSR::MVENDORID] = 0xF00DFACE;
    privilege_level_ = PrivilegeLevel::MACHINE;
    csrs_[CSR::MSTATUS] = (3 << 11);

    uint32_t misa = (1U << 30); // RV32
    misa |= (1 << 8); // I (Base)

    if (config_.mode == ExecutionMode::SYSTEM) {
        misa |= (1 << 18); // S (Supervisor extension)
        misa |= (1 << 20); // U (User extension)
    }

    if (config_.extension_m) {
        misa |= (1U << 12);
    }

        
    if (config_.extension_c) { 
        misa |= (1U << 2);
        ADDRESS_MISALIGNMENT_MASK = 0x1;
    } else {
        ADDRESS_MISALIGNMENT_MASK = 0x3;
    }

    if (config_.extension_a) {
        misa |= (1U << 0);
    }

    csrs_[CSR::MISA] = misa;

    bus_.register_cpu(this);
}

void CPU::run() {
    state_ = CPUState::ACTIVE;

    uint64_t insn_counter = 0;

    while (state_ != CPUState::HALTED) {

        insn_counter++;

        if ((insn_counter & 1023) == 0) {
            updateCycle();
        }

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

    bool use_compress = csrs_[CSR::MISA] & 0x4;

    // Fetch
    try {
        if (pc_ & ADDRESS_MISALIGNMENT_MASK) {
            trap(ExceptionCause::MISALIGNED_INSTRUCTION, pc_, false);
            return sr; 
        }
            

        uint32_t first_half_address = mmu_.translate(pc_, MMU::AccessType::FETCH);
        uint16_t first_half = bus_.read16(first_half_address);
        
        if ((first_half & 0x3) != 0x3) {
            if (!use_compress) {
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
    if (clint_.mtime >= clint_.mtimecmp)
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

    // MEIP -> MIP bit 11 (machine external interrupt)
    if (plic_.m_interrupt_pending()) {
        csrs_[CSR::MIP] |= (1 << 11);
    } else {
        csrs_[CSR::MIP] &= ~(1 << 11);
    }

    // SEIP -> MIP bit 9 (supervisor external interrupt)
    if (plic_.s_interrupt_pending()) {
        csrs_[CSR::MIP] |= (1 << 9);
    } else {
        csrs_[CSR::MIP] &= ~(1 << 9);
    }

    csrs_[CSR::TIME] = (uint32_t)(clint_.mtime);
    csrs_[CSR::TIMEH] = (uint32_t)(clint_.mtime >> 32);
 
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

void CPU::invalidateReservation(uint32_t addr) {
    if (reservation_valid_ && reservation_addr_ == addr) {
        reservation_valid_ = false;
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


