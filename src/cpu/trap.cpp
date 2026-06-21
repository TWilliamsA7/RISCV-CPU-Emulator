// src/cpu/trap.cpp

#include "cpu/cpu.hpp"
#include "emulator.hpp"
#include <iostream>

PrivilegeLevel CPU::getTrapTargetLevel(uint32_t cause, bool is_interrupt) {
    if (privilege_level_ == PrivilegeLevel::MACHINE) {
        return PrivilegeLevel::MACHINE;
    }
    
    bool delegate = false;
    if (is_interrupt) {
        delegate = (csrs_[CSR::MIDELEG] >> cause) & 1;
    } else {
        delegate = (csrs_[CSR::MEDELEG] >> cause) & 1;
    }
    
    return delegate ? PrivilegeLevel::SUPERVISOR : PrivilegeLevel::MACHINE;
}

void CPU::trap(uint32_t cause, uint32_t tval, bool is_interrupt) {
    trap_occurred_ = true;

    PrivilegeLevel target_level = getTrapTargetLevel(cause, is_interrupt);


    if (sys_.config_.profile.verbose) {
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

    for (int id : {11, 3, 7, 9, 1, 5}) {
        if (!(mip_mie & (1 << id))) continue;

        bool delegate = (csrs_[CSR::MIDELEG] >> id) & 1;

        if (delegate && privilege_level_ <= PrivilegeLevel::SUPERVISOR) {
            bool sie = (mstatus >> 1) & 1;
            if (privilege_level_ < PrivilegeLevel::SUPERVISOR || sie) {
                trap(id, 0, true);
                return;
            }
        } else {
            bool mie = (mstatus >> 3) & 1;
            if (privilege_level_ < PrivilegeLevel::MACHINE || mie) {
                trap(id, 0, true);
                return;
            }
        }
    }
}

bool CPU::handleSBI() {
    uint32_t ext = regs_[17]; // a7 - extension ID
    uint32_t fid = regs_[16]; // a6 - function ID
    // printf("[SBI] ext=0x%08x fid=0x%08x pc=0x%08x\n", ext, fid, pc_);

    // SBI return: a0 = error code, a1 = value
    // SBI_SUCCESS = 0, SBI_ERR_NOT_SUPPORTED = -2

    switch (ext) {

        // ── Legacy (v0.1) extensions ──────────────────────────────────────
        case 0x00: { // sbi_set_timer
            uint64_t stime = ((uint64_t)regs_[11] << 32) | regs_[10];
            sys_.clint.mtimecmp = stime;
            // Clear pending timer interrupt, it will re-fire when mtime >= mtimecmp
            csrs_[CSR::MIP] &= ~(1 << 7); // clear MTIP
            regs_[10] = 0;
            regs_[11] = 0;
            next_pc_ = pc_ + 4;
            set_next_pc_ = true;
            return true;
        }
        case 0x01: { // sbi_console_putchar
            char c = (char)(regs_[10] & 0xFF);
            sys_.bus.uart_.write8(UART::THR, c);
            regs_[10] = 0;
            regs_[11] = 0;
            next_pc_ = pc_ + 4;
            set_next_pc_ = true;
            return true;
        }
        case 0x02: { // sbi_console_getchar
            regs_[10] = -1; // no char available
            regs_[11] = 0;
            next_pc_ = pc_ + 4;
            set_next_pc_ = true;
            return true;
        }
        case 0x04: { // sbi_clear_ipi
            csrs_[CSR::MIP] &= ~(1 << 1); // clear MSIP
            regs_[10] = 0;
            regs_[11] = 0;
            next_pc_ = pc_ + 4;
            set_next_pc_ = true;
            return true;
        }
        case 0x05: { // sbi_send_ipi — single hart, no-op
            regs_[10] = 0;
            regs_[11] = 0;
            next_pc_ = pc_ + 4;
            set_next_pc_ = true;
            return true;
        }
        case 0x08: { // sbi_shutdown
            std::cout << "SBI shutdown requested" << std::endl;
            state_ = CPUState::HALTED;
            return true;
        }

        // ── Base extension (0x10) ─────────────────────────────────────────
        case 0x10: {
            switch (fid) {
                case 0: // sbi_get_spec_version — SBI 2.0
                    regs_[10] = 0;
                    regs_[11] = (2 << 24) | 0;
                    break;
                case 1: // sbi_get_impl_id — custom impl ID
                    regs_[10] = 0;
                    regs_[11] = 4; // SBI impl ID 4 = custom
                    break;
                case 2: // sbi_get_impl_version
                    regs_[10] = 0;
                    regs_[11] = 1;
                    break;
                case 3: { // sbi_probe_extension
                    uint32_t probe_ext = regs_[10];
                    bool supported = (probe_ext == 0x10 ||  // Base
                                      probe_ext == 0x54494D45 || // TIME
                                      probe_ext == 0x48534D ||   // HSM
                                      probe_ext == 0x53525354 || // SRST
                                      probe_ext == 0x00 ||  // legacy set_timer
                                      probe_ext == 0x01 ||  // legacy putchar
                                      probe_ext == 0x02 ||  // legacy getchar
                                      probe_ext == 0x04 ||  // legacy clear_ipi
                                      probe_ext == 0x05 ||  // legacy send_ipi
                                      probe_ext == 0x08);   // legacy shutdown
                    regs_[10] = 0;
                    regs_[11] = supported ? 1 : 0;
                    break;
                }
                case 4: // sbi_get_mvendorid
                    regs_[10] = 0;
                    regs_[11] = 0;
                    break;
                case 5: // sbi_get_marchid
                    regs_[10] = 0;
                    regs_[11] = 0;
                    break;
                case 6: // sbi_get_mimpid
                    regs_[10] = 0;
                    regs_[11] = 0;
                    break;
                default:
                    regs_[10] = (uint32_t)-2; // SBI_ERR_NOT_SUPPORTED
                    regs_[11] = 0;
                    break;
            }
            next_pc_ = pc_ + 4;
            set_next_pc_ = true;
            return true;
        }

        // ── Timer extension (0x54494D45) ──────────────────────────────────
        case 0x54494D45: {
            if (fid == 0) { // sbi_set_timer
                uint64_t stime = ((uint64_t)regs_[11] << 32) | regs_[10];
                sys_.clint.mtimecmp = stime;
                csrs_[CSR::MIP] &= ~(1 << 7); // clear MTIP
                // Set STIP pending if needed — kernel will re-arm via this call
                csrs_[CSR::MIP] &= ~(1 << 5); // clear STIP, let it re-fire naturally
                regs_[10] = 0;
                regs_[11] = 0;
            } else {
                regs_[10] = (uint32_t)-2;
                regs_[11] = 0;
            }
            next_pc_ = pc_ + 4;
            set_next_pc_ = true;
            return true;
        }

        // ── HSM extension (0x48534D) ──────────────────────────────────────
        case 0x48534D: {
            switch (fid) {
                case 0: // sbi_hart_start — single hart, always fail
                    regs_[10] = (uint32_t)-2;
                    regs_[11] = 0;
                    break;
                case 1: // sbi_hart_stop
                    std::cout << "SBI hart stop" << std::endl;
                    state_ = CPUState::HALTED;
                    break;
                case 2: // sbi_hart_get_status — 0 = started
                    regs_[10] = 0;
                    regs_[11] = 0;
                    break;
                case 3: // sbi_hart_suspend — treat as no-op
                    regs_[10] = 0;
                    regs_[11] = 0;
                    break;
                default:
                    regs_[10] = (uint32_t)-2;
                    regs_[11] = 0;
                    break;
            }
            next_pc_ = pc_ + 4;
            set_next_pc_ = true;
            return true;
        }

        // ── System Reset extension (0x53525354) ───────────────────────────
        case 0x53525354: {
            if (fid == 0) { // sbi_system_reset
                std::cout << "SBI system reset type=" << regs_[10]
                          << " reason=" << regs_[11] << std::endl;
                state_ = CPUState::HALTED;
            } else {
                regs_[10] = (uint32_t)-2;
                regs_[11] = 0;
            }
            next_pc_ = pc_ + 4;
            set_next_pc_ = true;
            return true;
        }

        default:
            // Unknown extension — not supported
            regs_[10] = (uint32_t)-2; // SBI_ERR_NOT_SUPPORTED
            regs_[11] = 0;
            next_pc_ = pc_ + 4;
            set_next_pc_ = true;
            return true;
    }
}