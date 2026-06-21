// src/cpu/cpu.cpp

#include "cpu/cpu.hpp"
#include "emulator.hpp"
#include "errors/errors.hpp"


#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

CPU::CPU(Emulator& sys) : sys_(sys), icache_(sys.bus) {
    regs_.fill(0);
    csrs_.fill(0);
    csrs_[CSR::MVENDORID] = 0xF00DFACE;
    privilege_level_ = PrivilegeLevel::MACHINE;
    csrs_[CSR::MSTATUS] = (3 << 11);

    uint32_t misa = (1U << 30); // RV32
    misa |= (1 << 8); // I (Base)

    if (sys_.config_.cpu_config.mode == ExecutionMode::SYSTEM) {
        misa |= (1 << 18); // S (Supervisor extension)
        misa |= (1 << 20); // U (User extension)
    }

    if (sys_.config_.cpu_config.extensions.m) {
        misa |= (1U << 12);
    }

        
    if (sys_.config_.cpu_config.extensions.c) { 
        misa |= (1U << 2);
        ADDRESS_MISALIGNMENT_MASK = 0x1;
    } else {
        ADDRESS_MISALIGNMENT_MASK = 0x3;
    }

    if (sys_.config_.cpu_config.extensions.a) {
        misa |= (1U << 0);
    }

    csrs_[CSR::MISA] = misa;

    pc_ = sys.config_.cpu_config.starting_pc;
    regs_[11] = sys.config_.profile.dtb_address;

    sys_.bus.register_cpu(this);
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

        // if ((insn_counter & 0xFFFFFFF) == 0) {  // every ~16M instructions
        //     auto elapsed = std::chrono::duration<double>(clock::now() - t0).count();
        //     fprintf(stderr, "%.2f BIPS\n", insn_counter / elapsed / 1e9);
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
        sr.instruction = decoded_cache_[index].raw;
        sr.dInstr = decoded_cache_[index].decoded;
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
    decoded_cache_[index].raw = sr.instruction;
    return decoded_cache_[index].decoded;
}

const uint32_t CPU::fetchInstr(uint32_t pc) {

    bool use_compress = csrs_[CSR::MISA] & 0x4;

    try {
        if (pc_ & ADDRESS_MISALIGNMENT_MASK) {
            trap(ExceptionCause::MISALIGNED_INSTRUCTION, pc_, false);
            return 0; 
        }
        uint32_t phys_pc = sys_.mmu.translate(pc_, AccessType::FETCH);

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
                uint32_t phys_pc2 = sys_.mmu.translate(pc_ + 2, AccessType::FETCH);
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

    sr.pc_before = pc_;

    static auto start = std::chrono::steady_clock::now();
    const std::chrono::milliseconds delay(5000);

    static bool count_instr = false;
    static int instr_count = 0;

    // if (!count_instr) {

    //     auto currentTime = std::chrono::steady_clock::now();

    //     auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - start);

    //     if (elapsedTime >= delay) {
    //         sys_.config_.profile.verbose = true;
    //         count_instr = true;
    //     }

    // }

    // if (count_instr) {
    //     instr_count++;
    //     if (instr_count > 100) {
    //         halt();
    //     }
    // }

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
    if (sys_.config_.profile.verbose) printTrace();

    return sr;
}

void CPU::updateCycle() {
    sys_.clint.updateMtime();

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
    if (sys_.clint.msip)
        csrs_[CSR::MIP] |= (1 << 3);
    else
        csrs_[CSR::MIP] &= ~(1 << 3);

    // MTIP -> MIP bit 7 (timer)
    // MTIP -> MIP bit 7 (timer)
    if (sys_.clint.mtime >= sys_.clint.mtimecmp) {
        csrs_[CSR::MIP] |= (1 << 7);
        // Forward to STIP if timer interrupt is delegated to S-mode
        if ((csrs_[CSR::MIDELEG] >> 5) & 1) {
            csrs_[CSR::MIP] |= (1 << 5);  // set STIP
            csrs_[CSR::MIP] &= ~(1 << 7); // clear MTIP — M-mode won't handle it
        }
    } else {
        csrs_[CSR::MIP] &= ~(1 << 7);
    }

    // MEIP -> MIP bit 11 (machine external interrupt)
    if (sys_.plic.m_interrupt_pending()) {
        csrs_[CSR::MIP] |= (1 << 11);
    } else {
        csrs_[CSR::MIP] &= ~(1 << 11);
    }

    // SEIP -> MIP bit 9 (supervisor external interrupt)
    if (sys_.plic.s_interrupt_pending()) {
        csrs_[CSR::MIP] |= (1 << 9);
    } else {
        csrs_[CSR::MIP] &= ~(1 << 9);
    }

    csrs_[CSR::TIME] = (uint32_t)(sys_.clint.mtime);
    csrs_[CSR::TIMEH] = (uint32_t)(sys_.clint.mtime >> 32);
 
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


