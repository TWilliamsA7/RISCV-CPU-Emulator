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
        case CSR::MVENDORID:
        case CSR::MARCHID:
        case CSR::MIMPID:
        case CSR::MHARTID:
        case CSR::MISA:
        case CSR::MSTATUS:
        case CSR::MSTATUSH:
        case CSR::MTVEC:
        case CSR::MEDELEG:
        case CSR::MIDELEG:
        case CSR::MIE:
        case CSR::MCOUNTEREN:
        case CSR::MSCRATCH:
        case CSR::MEPC:
        case CSR::MCAUSE:
        case CSR::MTVAL:
        case CSR::MIP:
        case CSR::MCYCLE:
        case CSR::MCYCLEH:
        case CSR::MINSTRET:
        case CSR::MINSTRETH:
        case CSR::CYCLE:
        case CSR::CYCLEH:
        case CSR::PMPCFG0: case CSR::PMPCFG1: case CSR::PMPCFG2: case CSR::PMPCFG3:
        case CSR::PMPADDR0: case CSR::PMPADDR1: case CSR::PMPADDR2: case CSR::PMPADDR3: 
        case CSR::PMPADDR4: case CSR::PMPADDR5: case CSR::PMPADDR6: case CSR::PMPADDR7:
        case CSR::PMPADDR8: case CSR::PMPADDR9: case CSR::PMPADDR10: case CSR::PMPADDR11:
        case CSR::PMPADDR12: case CSR::PMPADDR13: case CSR::PMPADDR14: case CSR::PMPADDR15: 
        case CSR::STVEC:
        case CSR::SCOUNTEREN:
        case CSR::SSCRATCH:
        case CSR::SEPC:
        case CSR::SCAUSE:
        case CSR::STVAL:
                case CSR::TIME:
        case CSR::TIMEH:
        case CSR::SATP:
            return csrs_[addr];
            case 0x747: // mseccfg — Smepmp not supported, return 0
case 0x757: // msecfgh
    return 0;

        default:
            trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
            return std::nullopt;
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
        case CSR::SSTATUS:
        case CSR::SIE:
        case CSR::SIP:
        case CSR::MVENDORID:
        case CSR::MARCHID:
        case CSR::MIMPID:
        case CSR::MHARTID:
        case CSR::MISA:
        case CSR::MSTATUS:
        case CSR::MSTATUSH:
        case CSR::MTVEC:
        case CSR::MEDELEG:
        case CSR::MIDELEG:
        case CSR::MIE:
        case CSR::MCOUNTEREN:
        case CSR::MSCRATCH:
        case CSR::MEPC:
        case CSR::MCAUSE:
        case CSR::MTVAL:
        case CSR::MIP:
        case CSR::MCYCLE:
        case CSR::MCYCLEH:
        case CSR::MINSTRET:
        case CSR::MINSTRETH:
        case CSR::CYCLE:
        case CSR::CYCLEH:
        case CSR::PMPCFG0: case CSR::PMPCFG1: case CSR::PMPCFG2: case CSR::PMPCFG3:
        case CSR::PMPADDR0: case CSR::PMPADDR1: case CSR::PMPADDR2: case CSR::PMPADDR3: 
        case CSR::PMPADDR4: case CSR::PMPADDR5: case CSR::PMPADDR6: case CSR::PMPADDR7:
        case CSR::PMPADDR8: case CSR::PMPADDR9: case CSR::PMPADDR10: case CSR::PMPADDR11:
        case CSR::PMPADDR12: case CSR::PMPADDR13: case CSR::PMPADDR14: case CSR::PMPADDR15: 
        case CSR::STVEC:
        case CSR::SCOUNTEREN:
        case CSR::SSCRATCH:
        case CSR::SEPC:
        case CSR::SCAUSE:
        case CSR::STVAL:
        case CSR::SATP:
        case CSR::TIME:
        case CSR::TIMEH:
        case 0x747:
case 0x757:
    break;
            break;

        default:
            trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
            return false;
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

            case 0x747:
case 0x757:
    csrs_[addr] = 0; // hardwired zero, no Smepmp
    break;

        case CSR::MEDELEG:
            csrs_[CSR::MEDELEG] = val & 0xFFFF;
            break;

        case CSR::MIDELEG:
            csrs_[CSR::MIDELEG] = val & 0xFFFF;
            break;

        case CSR::MISA:
            break;

        case CSR::SSTATUS: {
            uint32_t mask = 0x000DE122;
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

        // Counter enable — allow writes, kernel uses these
        case CSR::MCOUNTEREN:
        case CSR::SCOUNTEREN:
        case CSR::MCOUNTINHIBIT:
            csrs_[addr] = val;
            break;

        // MHARTID is read-only, hardwired to 0
        case CSR::MHARTID:
            break; // ignore writes

        // MSCRATCH — OpenSBI uses this heavily, must work correctly  
        case CSR::MSCRATCH:
            csrs_[addr] = val;
            break;

        default:
            csrs_[addr] = val;
            break;
    }

    return true;
}
