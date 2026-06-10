// src/cpu/trap.cpp

#include "cpu/cpu.hpp"
#include <iostream>

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