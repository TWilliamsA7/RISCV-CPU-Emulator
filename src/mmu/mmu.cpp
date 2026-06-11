// src/mmu/mmu.cpp

#include "mmu/mmu.hpp"
#include "cpu/cpu.hpp"
#include "errors/errors.hpp"

MMU::MMU(CPU& cpu): cpu_(cpu) {}

uint32_t MMU::translate(uint32_t va, AccessType type) {

    // MMU disabled (Bare Mode)
    if (!(cpu_.csrs_[CPU::CSR::SATP] >> 31)) {
        return va;
    }

    // Resolve effective privilege level
    CPU::PrivilegeLevel effective_level = cpu_.privilege_level_;
    if (((cpu_.csrs_[CPU::CSR::MSTATUS] >> 17) & 1) && type != AccessType::FETCH) {
        effective_level = static_cast<CPU::PrivilegeLevel>((cpu_.csrs_[CPU::CSR::MSTATUS] >> 11) & 0x3);
    }

    uint32_t vpn[] = { (va >> 12) & VPN_MASK, (va >> 22) & VPN_MASK };
    uint32_t a = (cpu_.csrs_[CPU::CSR::SATP] & PPN_MASK) * 4096;
    uint32_t pte;

    for (int i = LEVELS - 1; i >= 0; i--) {
        uint32_t pte_addr = a + (vpn[i] * PTE_SIZE);

        try {
            pte = cpu_.bus_.read32(pte_addr);
        } catch (const BusAccessError&) {
            triggerPageFault(type);
        }

        if (!(pte & PTE_V) || (!(pte & PTE_R) && (pte &  PTE_W))) {
            triggerPageFault(type);
        }

        if (pte & (PTE_R | PTE_W | PTE_X)) {
            if (effective_level == CPU::PrivilegeLevel::USER && !(pte & PTE_U))
                triggerPageFault(type);
            if (effective_level == CPU::PrivilegeLevel::SUPERVISOR && (pte & PTE_U) && !(cpu_.csrs_[CPU::CSR::MSTATUS] >> 18) & 1)
                triggerPageFault(type);

            // Access Type vs RXW
            bool readable = (pte & PTE_R) || (((cpu_.csrs_[CPU::CSR::MSTATUS] >> 19) & 1) && (pte & PTE_X));
            if (type == AccessType::FETCH && !(pte & PTE_X))
                triggerPageFault(type);
            if (type == AccessType::LOAD && !readable)
                triggerPageFault(type);
            if (type == AccessType::STORE && !(pte & PTE_W))
                triggerPageFault(type);

            bool pte_changed = false;

            if (!(pte & PTE_A)) {
                pte |= PTE_A;
                pte_changed = true;
            }
            if (type == AccessType::STORE && !(pte & PTE_D)) {
                pte |= PTE_D;
                pte_changed = true;
            }

            if (pte_changed) {
                cpu_.bus_.write32(pte_addr, pte);
            }

            if (i == 1 && (pte & 0x000FFC00))
                triggerPageFault(type);

            if (i == 1) {
                uint32_t ppn1 = (pte >> 20) & 0xFFF;
                return (ppn1 << 22) | (va & PPN_MASK);
            } else {
                uint32_t ppn = (pte >> 10);
                return (ppn << 12) | (va & 0xFFF);
            }
        }

        a = (pte >> 10) * 4096;
    }
    triggerPageFault(type);
    return 0;
}

void MMU::triggerPageFault(AccessType type) {
    switch (type) {
        case FETCH: throw InstructionPageError("");
        case LOAD: throw LoadPageError("");
        case STORE: throw StorePageError("");
    }
}