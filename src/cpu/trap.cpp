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

    static int trap_count = 0;
        if (trap_count < 5 && privilege_level_ != PrivilegeLevel::MACHINE) {
                 printf("[TRAP %d] cause=%d pc=0x%08x stval=0x%08x priv=%d a0=0x%08x a1=0x%08x a6=0x%08x a7=0x%08x\n",
        trap_count, cause, pc_, csrs_[CSR::STVAL], (int)privilege_level_,
        regs_[10], regs_[11], regs_[16], regs_[17]);
    trap_count++;

        }

    static bool first_ipf = true;
    if (cause == 12 && first_ipf) {
        first_ipf = false;
        printf("[TRAP] First IPF: pc=0x%08x sepc=0x%08x stval=0x%08x satp=0x%08x stvec=0x%08x\n",
            pc_,   // the PC that faulted
            csrs_[CSR::SEPC],     // what sepc was before this trap
            csrs_[CSR::STVAL],
            csrs_[CSR::SATP],
            csrs_[CSR::STVEC]);
    }

    if (cause == 1) {
        printf("[PMP] Cause 1 at pc=0x%08x privilege=%d\n", pc_, privilege_level_);
        for (int i = 0; i < 4; i++)
            printf("[PMP] pmpcfg%d=0x%08x\n", i, csrs_[CSR::PMPCFG0 + i]);
        for (int i = 0; i < 8; i++)
            printf("[PMP] pmpaddr%d=0x%08x\n", i, csrs_[CSR::PMPADDR0 + i]);
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