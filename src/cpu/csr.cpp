// src/cpu/csr.cpp

#include "cpu/cpu.hpp"

std::optional<uint32_t> CPU::readCSR(uint16_t addr) {

    uint32_t required_privilege = (addr >> 8) & 0x3;
    if (privilege_level_ < required_privilege) {
        trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
        return std::nullopt;
    }

    if (addr == CSR::SATP && privilege_level_ == PrivilegeLevel::SUPERVISOR) {
        if ((csrs_[CSR::MSTATUS] >> 20) & 1) {  // TVM bit
            trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
            return std::nullopt;
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
            return (uint32_t)(clint_.mtime);
        case CSR::TIMEH:
            return (uint32_t)(clint_.mtime >> 32);

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
        case CSR::MSECCFGH:
            return 0;

        default:
            if (addr < 4096)
                return csrs_[addr];
            return 0;
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
        case CSR::MSECCFGH: 
        case CSR::MHARTID:
        case CSR::MVENDORID:
        case CSR::MARCHID:
        case CSR::MIMPID:
            return true;

        default:
            csrs_[addr] = val;
            return true;
    }

    return true;
}
