// src/cpu/csr.cpp

#include "cpu/cpu.hpp"
#include "emulator.hpp"

uint32_t CPU::readCSR(uint16_t addr) {

    uint32_t required_privilege = (addr >> 8) & 0x3;
    if (privilege_level_ < required_privilege) {
        trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
        return 0;
    }

    if (addr == CSR::SATP && privilege_level_ == PrivilegeLevel::SUPERVISOR) {
        if ((csrs_[CSR::MSTATUS] >> 20) & 1) {  // TVM bit
            trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
            return 0;
        }
    }

    switch (addr) {
        case CSR::SSTATUS:
            return csrs_[CSR::MSTATUS] & 0x800DE122;
        case CSR::SIE:
            return csrs_[CSR::MIE] & csrs_[CSR::MIDELEG];
        case CSR::SIP:
            return csrs_[CSR::MIP] & csrs_[CSR::MIDELEG];
        case CSR::TIME:
            return (uint32_t)(sys_.clint.mtime);
        case CSR::TIMEH:
            return (uint32_t)(sys_.clint.mtime >> 32);

        // Hardwired zero — defined but unimplemented extensions
        case CSR::MENVCFG:
        case CSR::MENVCFGH:
        case CSR::SENVCFG:
        case CSR::MSTATEEN0:
        case CSR::MSTATEEN1:
        case CSR::MSTATEEN2:
        case CSR::MSTATEEN3:
        case CSR::SSTATEEN0:
        case CSR::STIMECMP:
        case CSR::STIMECMPH:
        case CSR::MSECCFG:
        case CSR::MSECCFGH: {
            if (sys_.config_.profile.platform == Platform::LINUX) {
                trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
                return 0;
            } else {
                return 0;
            }
        }

        default: {

            if (sys_.config_.profile.platform == Platform::LINUX) {
                // Deactivate zihpm (Hardware Performance Monitors)
                if ((addr >= 0xB03 && addr <= 0xB1F) || (addr >= 0x323 && addr <= 0x33F)) {
                    trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
                    return 0;
                }

                // Deactivate sscofpmf (Supervisor Counter Overflow)
                if (addr == 0xDA0) {
                    trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
                    return 0;
                }

                // Deactivate smaia (Machine Advanced Interrupt Architecture)
                if (addr == 0x350 || addr == 0x351 || addr == 0x35C || 
                    addr == 0x150 || addr == 0x151 || addr == 0x15C  || addr == 0xFB0 || addr == 0xDB0) {
                    trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
                    return 0;
                }
            }

            if (addr < 4096)
                return csrs_[addr];
            return 0;
            
        }
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
            csrs_[CSR::MSTATUS] =
                (csrs_[CSR::MSTATUS] & ~mask) |
                (val & mask);
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
        case CSR::SSTATUS: {
            uint32_t mask = 0x800DE122;
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

        case CSR::MISA: {
            break;
        }

        case CSR::SATP: {
            csrs_[CSR::SATP] = val;
            tlb_flush_all(tlb_);
            return true;
        }

        case CSR::PMPADDR0:
        case CSR::PMPADDR1:
        case CSR::PMPADDR2:
        case CSR::PMPADDR3:
        case CSR::PMPADDR4:
        case CSR::PMPADDR5:
        case CSR::PMPADDR6:
        case CSR::PMPADDR7:
        case CSR::PMPADDR8:
        case CSR::PMPADDR9:
        case CSR::PMPADDR10:
        case CSR::PMPADDR11:
        case CSR::PMPADDR12:
        case CSR::PMPADDR13:
        case CSR::PMPADDR14:
        case CSR::PMPADDR15:
            write_pmpaddr(addr, val);
            return true;

        case CSR::PMPCFG0:
        case CSR::PMPCFG1:
        case CSR::PMPCFG2:
        case CSR::PMPCFG3:
            write_pmpcfg(addr, val);
            return true;

        case CSR::MENVCFG:
        case CSR::MENVCFGH:
        case CSR::SENVCFG:
        case CSR::MSTATEEN0:
        case CSR::MSTATEEN1:
        case CSR::MSTATEEN2:
        case CSR::MSTATEEN3:
        case CSR::SSTATEEN0:
        case CSR::STIMECMP: 
        case CSR::STIMECMPH: 
        case CSR::MSECCFG: 
        case CSR::MSECCFGH: {
            if (sys_.config_.profile.platform == Platform::LINUX) {
                trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
            } else {
                return true;
            }
        }
        
        case CSR::MHARTID:
        case CSR::MVENDORID:
        case CSR::MARCHID:
        case CSR::MIMPID: 
            return true;
            

        default: {

            if (sys_.config_.profile.platform == Platform::LINUX) {
                // Deactivate zihpm (Hardware Performance Monitors)
                if ((addr >= 0xB03 && addr <= 0xB1F) || (addr >= 0x323 && addr <= 0x33F)) {
                    trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
                    return false;
                }

                // Deactivate sscofpmf (Supervisor Counter Overflow)
                if (addr == 0xDA0) {
                    trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
                    return false;
                }

                // Deactivate smaia (Machine Advanced Interrupt Architecture)
                if (addr == 0x350 || addr == 0x351 || addr == 0x35C || 
                    addr == 0x150 || addr == 0x151 || addr == 0x15C || addr == 0xFB0 || addr == 0xDB0) {
                    trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
                    return false;
                }
            }

            csrs_[addr] = val;
            return true;
        }
    }

    return true;
}

void CPU::write_pmpcfg(uint16_t addr, uint32_t val) {
    uint32_t current = addr - PMPCFG0;
    uint32_t result = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t cur_byte = (current >> (i * 8)) & 0xFF;
        uint8_t new_byte = (val >> (i * 8)) & 0xFF;
        // If locked, ignore write to this entry
        if (cur_byte & 0x80) {
            result |= (uint32_t)cur_byte << (i * 8);
        } else {
            new_byte &= 0x9F; // clear reserved bits 5-6
            result |= (uint32_t)new_byte << (i * 8);
        }
    }
    csrs_[addr] = result;
}

void CPU::write_pmpaddr(uint16_t addr, uint32_t val) {
    uint16_t entry = addr - CSR::PMPADDR0;
    uint16_t cfg_reg = (entry / 4) + 0x3A0;
    int cfg_byte = entry % 4;
    uint8_t cfg = (csrs_[cfg_reg] >> (cfg_byte * 8)) & 0xFF;
    if (cfg & 0x80) return; // locked, ignore
    
    // Also check if next entry is TOR and locked (locks this addr too)
    if (entry < 15) {
        int ncfg_reg = ((entry + 1) / 4) + 0x3A0;
        int ncfg_byte = (entry + 1) % 4;
        uint8_t ncfg = (csrs_[ncfg_reg] >> (ncfg_byte * 8)) & 0xFF;
        if ((ncfg & 0x80) && ((ncfg >> 3) & 0x3) == 1) return; // next is locked TOR
    }
    csrs_[addr] = val;
}

// Returns true if access is permitted
bool CPU::pmp_check(uint32_t paddr, AccessType access_type, PrivilegeLevel priv) {
    bool is_mmode = (priv == PrivilegeLevel::MACHINE);

    for (int i = 0; i < 16; i++) {
        uint16_t cfg_reg = (i / 4) + CSR::PMPCFG0;
        int cfg_byte = i % 4;
        uint8_t cfg = (csrs_[cfg_reg] >> (cfg_byte * 8)) & 0xFF;

        uint8_t A = (cfg >> 3) & 0x3;
        if (A == 0) continue;

        bool match = false;
        uint16_t current_pmpaddr_idx = CSR::PMPADDR0 + i;

        if (A == 1) { // TOR
            uint32_t lo = (i == 0) ? 0 : (csrs_[CSR::PMPADDR0 + i - 1] << 2);
            uint32_t hi = csrs_[current_pmpaddr_idx] << 2;
            match = (paddr >= lo && paddr < hi);
        } else if (A == 2) { // NA4
            uint32_t base = csrs_[current_pmpaddr_idx] << 2;
            match = (paddr >= base && paddr < base + 4);
        } else { // NAPOT
            uint32_t addr = csrs_[current_pmpaddr_idx];
            if (addr == 0xffffffff) {
                match = true; // entire address space
            } else {
                uint32_t size_minus1 = addr ^ (addr | (addr + 1));
                uint32_t napot_mask = ~(size_minus1 << 1 | 1);
                uint32_t base = (addr & napot_mask) << 2;
                uint32_t range = (size_minus1 + 1) << 3;
                match = (paddr >= base && paddr < base + range);
            }
        }

        if (match) {
            bool locked = (cfg & 0x80);
            if (is_mmode && !locked) return true;

            bool r = (cfg >> 0) & 1;
            bool w = (cfg >> 1) & 1;
            bool x = (cfg >> 2) & 1;

            bool permitted = (access_type == AccessType::FETCH) ? x :
                             (access_type == AccessType::LOAD)  ? r : w;

            if (paddr == 0x80000410)
                printf("[PMP MATCH] entry=%d paddr=0x%08x cfg=0x%02x A=%d r=%d w=%d x=%d access=%d permitted=%d\n",
                    i, paddr, cfg, A, (int)r, (int)w, (int)x, (int)access_type, (int)permitted);

            return permitted;
        }
    }


    if (!is_mmode) {
        printf("[PMP MATCH] No matching entry paddr=0x%08x\n", paddr);
    }
    
    // No matching entry: M-mode allowed, S/U denied
    return is_mmode;
}