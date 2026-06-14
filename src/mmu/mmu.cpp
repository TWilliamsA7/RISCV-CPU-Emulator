// src/mmu/mmu.cpp

#include "mmu/mmu.hpp"
#include "cpu/cpu.hpp"
#include "errors/errors.hpp"
#include "mmu/tlb.hpp"
#include <mmu/pmp.hpp>

MMU::MMU(CPU& cpu): cpu_(cpu) {}

static inline uint32_t satp_ppn(uint32_t satp)  { return satp & MMU::PPN_MASK; }
static inline uint16_t satp_asid(uint32_t satp) { return (satp >> 22) & MMU::ASID_MASK; }
static inline bool     satp_sv32(uint32_t satp)  { return (satp >> 31) & 1; }

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

static void triggerPageFault(MMU::AccessType type) {
    switch (type) {
        case MMU::AccessType::FETCH: throw InstructionPageError("");
        case MMU::AccessType::LOAD: throw LoadPageError("");
        case MMU::AccessType::STORE: throw StorePageError("");
    }
}

void MMU::check_tlb_permissions(const TLBEntry& e, MMU::AccessType at,
                                   uint32_t priv, uint32_t va) {
   bool sum = (cpu_.csrs_[CPU::CSR::MSTATUS] >> 18) & 1;
    if (priv == CPU::PrivilegeLevel::SUPERVISOR && e.flags.user && !sum)
        triggerPageFault(at);
    if (priv == CPU::PrivilegeLevel::USER && !e.flags.user)
        triggerPageFault(at);
 
    switch (at) {
        case MMU::AccessType::LOAD:
            if (!e.flags.read) triggerPageFault(at);
            break;
        case MMU::AccessType::STORE:
            if (!e.flags.write) triggerPageFault(at);
            break;
        case MMU::AccessType::FETCH:
            if (!e.flags.execute) triggerPageFault(at);
            break;
    }
}

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

    const uint32_t full_vpn = va >> 12;
    uint32_t vpn[] = { (va >> 12) & VPN_MASK, (va >> 22) & VPN_MASK };
    uint32_t a = (cpu_.csrs_[CPU::CSR::SATP] & PPN_MASK) * 4096;
    uint32_t pte;
    const uint16_t asid = satp_asid(cpu_.csrs_[CPU::CSR::SATP]);
    const uint32_t off  = va & 0xFFF;

    TLBEntry* hit = tlb_lookup(cpu_.tlb_, full_vpn, asid);
    if (hit) {
        check_tlb_permissions(*hit, type, effective_level, va);
        uint32_t pa;
        if (hit->superpage) {
            // 4 MB superpage: PPN[1] from entry, VPN[0] from VA, offset from VA.
            pa = ((hit->ppn & 0xFFFFF000u) << 2)  // PPN[1] → pa[33:22]
               | ((vpn[0] & 0x3FF) << 12)             // VPN[0] → pa[21:12]
               | off;
        } else {
            pa = (hit->ppn << 12) | off;
        }
        check_pmp(cpu_, pa, type);
        return pa;
    }

    for (int i = LEVELS - 1; i >= 0; i--) {
        uint32_t pte_addr = a + (vpn[i] * PTE_SIZE);

        try {
            pte = cpu_.bus_.read32(pte_addr);
        } catch (const BusAccessError&) {
            triggerPageFault(type);
        }

        TLBFlags flags = pte_to_flags(pte);

        if (!(pte & PTE_V) || (!(pte & PTE_R) && (pte &  PTE_W))) {
            triggerPageFault(type);
        }

        if (pte & (PTE_R | PTE_W | PTE_X)) {
            if (effective_level == CPU::PrivilegeLevel::USER && !(pte & PTE_U))
                triggerPageFault(type);
            if (effective_level == CPU::PrivilegeLevel::SUPERVISOR && (pte & PTE_U) && !((cpu_.csrs_[CPU::CSR::MSTATUS] >> 18) & 1))
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

            uint32_t pa;

            if (i == 1) {
                uint32_t ppn1 = (pte >> 20) & 0xFFF;
                tlb_fill(cpu_.tlb_, vpn[i], asid, ppn1, flags, true);
                pa = (ppn1 << 22) | (va & PPN_MASK);
            } else {
                uint32_t ppn = (pte >> 10);
                tlb_fill(cpu_.tlb_, vpn[i], asid, ppn, flags);
                pa = (ppn << 12) | (va & 0xFFF);
            }

            check_pmp(cpu_, pa, type);
            return pa;
        }

        a = (pte >> 10) * 4096;
    }
    triggerPageFault(type);
    return 0;
}

