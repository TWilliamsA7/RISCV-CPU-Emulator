// src/mmu/mmu.cpp

#include "mmu/mmu.hpp"
#include "emulator.hpp"
#include "errors/errors.hpp"
#include "mmu/tlb.hpp"
#include "mmu/pmp.hpp"

MMU::MMU(Emulator& sys): sys_(sys) {}

static inline uint32_t satp_ppn(uint32_t satp)  { return satp & MMU::PPN_MASK; }
static inline uint16_t satp_asid(uint32_t satp) { return (satp >> 22) & MMU::ASID_MASK; }
static inline bool     satp_sv32(uint32_t satp)  { return (satp >> 31) & 1; }

void MMU::triggerPageFault(AccessType type) {
    switch (type) {
        case AccessType::FETCH: throw InstructionPageError("");
        case AccessType::LOAD: throw LoadPageError("");
        case AccessType::STORE: throw StorePageError("");
    }
}

void MMU::check_tlb_permissions(const TLBEntry& e, AccessType at, uint32_t priv, uint32_t va) {
    bool sum = (sys_.cpu.csrs_[CPU::CSR::MSTATUS] >> 18) & 1;
    if (priv == PrivilegeLevel::SUPERVISOR && e.flags.user && !sum)
        triggerPageFault(at); // Assuming triggerPageFault is available here
    if (priv == PrivilegeLevel::USER && !e.flags.user)
        triggerPageFault(at);

    switch (at) {
        case AccessType::LOAD:   if (!e.flags.read) triggerPageFault(at); break;
        case AccessType::STORE:  if (!e.flags.write) triggerPageFault(at); break;
        case AccessType::FETCH:  if (!e.flags.execute) triggerPageFault(at); break;
    }
}

static TLBFlags pte_to_flags(uint32_t pte) {
    return TLBFlags{
        .read     = (bool)((pte >> 1) & 1),
        .write    = (bool)((pte >> 2) & 1),
        .execute  = (bool)((pte >> 3) & 1),
        .user     = (bool)((pte >> 4) & 1),
        .global   = (bool)((pte >> 5) & 1),
        .accessed = (bool)((pte >> 6) & 1),
        .dirty    = (bool)((pte >> 7) & 1),
    };
}

uint32_t MMU::translate(uint32_t va, AccessType type) {
    // MMU disabled (Bare Mode)
    if (!(sys_.cpu.csrs_[CPU::CSR::SATP] >> 31)) {
        return va;
    }

    // Resolve effective privilege level
    PrivilegeLevel effective_level = sys_.cpu.privilege_level_;
    if (((sys_.cpu.csrs_[CPU::CSR::MSTATUS] >> 17) & 1) && type != AccessType::FETCH) {
        effective_level = static_cast<PrivilegeLevel>((sys_.cpu.csrs_[CPU::CSR::MSTATUS] >> 11) & 0x3);
    }

    const uint32_t full_vpn = va >> 12;
    const uint16_t asid = (sys_.cpu.csrs_[CPU::CSR::SATP] >> 22) & ASID_MASK;

    // INLINED LOOKUP (Zero function call overhead here!)
    TLBEntry* hit = tlb_lookup(sys_.cpu.tlb_, full_vpn, asid);
    
    if (hit) {
        check_tlb_permissions(*hit, type, effective_level, va);
        uint32_t pa;
        if (hit->superpage) {
            uint32_t vpn0 = (va >> 12) & 0x3FF;
            pa = ((hit->ppn & 0xFFFFF000u) << 2) | (vpn0 << 12) | (va & 0xFFF);
        } else {
            pa = (hit->ppn << 12) | (va & 0xFFF);
        }
        check_pmp(sys_.cpu, pa, type); 
        return pa;
    }

    // SLOW PATH: Call out to the heavy page table walk in the .cpp file
    return translate_walk(va, type, effective_level, full_vpn, asid);
}

uint32_t MMU::translate_walk(uint32_t va, AccessType type, PrivilegeLevel effective_level, uint32_t full_vpn, uint16_t asid) {
    uint32_t vpn[] = { (va >> 12) & VPN_MASK, (va >> 22) & VPN_MASK };
    uint32_t a = (sys_.cpu.csrs_[CPU::CSR::SATP] & PPN_MASK) * 4096;
    uint32_t pte;

    // The actual hardware page table walk
    for (int i = LEVELS - 1; i >= 0; i--) {
        uint32_t pte_addr = a + (vpn[i] * PTE_SIZE);

        try {
            pte = sys_.bus.read32(pte_addr);
        } catch (const BusAccessError&) {
            triggerPageFault(type);
        }

        TLBFlags flags = pte_to_flags(pte);

        if (!(pte & PTE_V) || (!(pte & PTE_R) && (pte & PTE_W))) {
            triggerPageFault(type);
        }

        if (pte & (PTE_R | PTE_W | PTE_X)) {
                        if (effective_level == PrivilegeLevel::USER && !(pte & PTE_U))
                triggerPageFault(type);
            if (effective_level == PrivilegeLevel::SUPERVISOR && (pte & PTE_U) && !((sys_.cpu.csrs_[CPU::CSR::MSTATUS] >> 18) & 1))
                triggerPageFault(type);

            // Access Type vs RXW
            bool readable = (pte & PTE_R) || (((sys_.cpu.csrs_[CPU::CSR::MSTATUS] >> 19) & 1) && (pte & PTE_X));
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
                sys_.bus.write32(pte_addr, pte);
            }

            if (i == 1 && (pte & 0x000FFC00))
                triggerPageFault(type);

            uint32_t pa;
            if (i == 1) { // Superpage
                uint32_t ppn1 = (pte >> 20) & 0xFFF;
                tlb_fill(sys_.cpu.tlb_, full_vpn, asid, ppn1, flags, true);
                pa = (ppn1 << 22) | (va & PPN_MASK);
            } else { // 4KB page
                uint32_t ppn = (pte >> 10);
                tlb_fill(sys_.cpu.tlb_, full_vpn, asid, ppn, flags, false);
                pa = (ppn << 12) | (va & 0xFFF);
            }

            check_pmp(sys_.cpu, pa, type);
            return pa;
        }

        a = (pte >> 10) * 4096;
    }
    
    triggerPageFault(type);
    return 0;
}

