// src/cpu/cpu.cpp

#include "cpu/cpu.hpp"
#include "memory/memory.hpp"
#include "errors/errors.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>

CPU::CPU (CPUConfig config, Bus& bus, Clint& clint, PLIC& plic) 
    : config_(config), bus_(bus), clint_(clint), pc_(0x80000000), mmu_(*this), plic_(plic), icache_(bus) {
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

    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();
    uint64_t insn_counter = 0;

    while (state_ != CPUState::HALTED) {

        insn_counter++;

        if ((insn_counter & 1023) == 0) {
            updateCycle();
        }

        // if ((insn_counter & 0xFFFFFF) == 0) {  // every ~16M instructions
        //     auto elapsed = std::chrono::duration<double>(clock::now() - t0).count();
        //     fprintf(stderr, "%.2f MIPS\n", insn_counter / elapsed / 1e6);
        // }

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

void CPU::halt() {
    state_ = CPUState::HALTED;
}

const DecodedInstr& CPU::fetch_and_decode(uint32_t pc) {
    uint32_t index = (pc >> 1) & (DECODE_CACHE_SIZE - 1);

    if (decoded_cache_[index].epoch == current_epoch_ && decoded_cache_[index].pc == pc) {
        return decoded_cache_[index].decoded;
    }

    DecodedInstr di = DecodedInstr();

    uint32_t len = fetchInstr(pc);

    if (trap_occurred_) {
        return sr.dInstr;
    }

    di = decode(sr.instruction);
    di.instr_len = len;
    sr.dInstr = di;

    decoded_cache_[index].epoch = current_epoch_;
    decoded_cache_[index].pc = pc;
    decoded_cache_[index].decoded = di;
    return decoded_cache_[index].decoded;
}

 const uint32_t CPU::fetchInstr(uint32_t pc) {

    bool use_compress = csrs_[CSR::MISA] & 0x4;

    try {
        if (pc_ & ADDRESS_MISALIGNMENT_MASK) {
            trap(ExceptionCause::MISALIGNED_INSTRUCTION, pc_, false);
            return 0; 
        }
        uint32_t phys_pc = mmu_.translate(pc_, MMU::AccessType::FETCH);

        // get pointer to page
        uint8_t* page = icache_.fetch_page(phys_pc);

        uint32_t offset = phys_pc & 0xFFF;
        uint8_t* inst_ptr = page + offset;

        // read first 16 bits
        uint16_t first_half;
        memcpy(&first_half, inst_ptr, sizeof(uint16_t));

        if ((first_half & 0x3) != 0x3) {
            if (!use_compress) {
                trap(ExceptionCause::ILLEGAL_INSTRUCTION, first_half, false);
                return 0;
            }

            sr.instruction = decompress(first_half);
            return 2;
        }
        else {
            uint16_t second_half;

            if (offset < 4094) {
                second_half = *(uint16_t*)(inst_ptr + 2);
            }
            else {
                uint32_t phys_pc2 = mmu_.translate(pc_ + 2, MMU::AccessType::FETCH);
                uint8_t* page2 = icache_.fetch_page(phys_pc2);
                memcpy(&second_half, page2, sizeof(uint16_t));
            }

            sr.instruction = (uint32_t(second_half) << 16) | first_half;
            return 4;
        }
    } catch (const BusAccessError& e) {
        trap(ExceptionCause::INSTRUCTION_ACCESS_FAULT, pc_, false);
    } catch (const InstructionPageError&  e) {
        trap(ExceptionCause::INSTRUCTION_PAGE_FAULT, pc_, false);
    }
    return 0;
}


StepResult CPU::step() {
    clearStep();  

    // Check for Asynchronous Interrupts 
    checkInterrupts();
    if (trap_occurred_) return sr;

    
    const DecodedInstr di = fetch_and_decode(pc_);
    if (trap_occurred_) return sr;

    execute(di);
    if (trap_occurred_) return sr;

    csrs_[CSR::MINSTRET]++;
    if (csrs_[CSR::MINSTRET] == 0)
        csrs_[CSR::MINSTRETH]++;

    if (set_next_pc_) {
        pc_ = next_pc_;
    } else {
        pc_ += di.instr_len;
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
    sr.mem_write.valid = false;
    sr.reg_write.valid = false;
    sr.csr_write.valid = false;
    set_next_pc_ = false;
    trap_occurred_ = false;
}

void CPU::clearDecodeCache() {
    current_epoch_++;
        
    if (current_epoch_ == 0) {
        current_epoch_ = 1;
        for (auto& entry : decoded_cache_) {
            entry.pc = 0xFFFFFFFF;
            entry.epoch = 0;
        }
    }
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


