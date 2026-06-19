// src/cpu/trace.cpp

#include "cpu/cpu.hpp"

#include <string>
#include <iomanip>
#include <iostream>
#include <sstream>

std::string CPU::hex32(uint32_t v) const {
    std::ostringstream oss;
    oss << std::hex << std::setw(8) << std::setfill('0') << v;
    return oss.str();
}

static std::string regStr(uint8_t r) {
    if (r == 0)
        return "zero";
    if (r == 1)
        return "ra";
    if (r == 2)
        return "sp";
    if (r == 3)
        return "gp";
    if (r == 4)
        return "tp";
    if (r >= 5 && r <= 7)
        return "t" + std::to_string((r - 5));
    if (r == 8 || r == 9)
        return "s" + std::to_string((r - 8));
    if (r >= 10 && r <= 17)
        return "a" + std::to_string((r - 10));
    if (r >= 18 && r <= 27)
        return "s" + std::to_string((r - 16));
    if (r >= 28)
        return "t" + std::to_string((r - 25));
    return "unknown";
}

std::string CPU::disasm(const DecodedInstr& di) const {
    std::ostringstream oss;

    switch (di.kind) {

        // R-type
        case InstrKind::ADD:
            oss << "ADD " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::SUB:
            oss << "SUB " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::AND:
            oss << "AND " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::OR:
            oss << "OR " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::XOR:
            oss << "XOR " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::SLL:
            oss << "SLL " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::SLT:
            oss << "SLT " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::SLTU:
            oss << "SLTU " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;
        
        case InstrKind::SRL:
            oss << "SRL " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::SRA:
            oss << "SRA " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        // I-type arithmetic
        case InstrKind::ADDI:
            oss << "ADDI " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << di.imm;
            break;

        case InstrKind::ANDI:
            oss << "ANDI " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << di.imm;
            break;

        case InstrKind::ORI:
            oss << "ORI " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << di.imm;
            break;

        case InstrKind::XORI:
            oss << "XORI " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << di.imm;
            break;

        case InstrKind::SLTI:
            oss << "SLTI " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << di.imm;
            break;

        case InstrKind::SLTIU:
            oss << "SLTIU " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << di.imm;
            break;

        case InstrKind::SLLI:
            oss << "SLLI " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << di.imm;
            break;

        case InstrKind::SRLI:
            oss << "SRLI " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << di.imm;
            break;

        case InstrKind::SRAI:
            oss << "SRAI " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << di.imm;
            break;

        // Loads
        case InstrKind::LW:
            oss << "LW " << regStr(di.rd) << ", "
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::LH:
            oss << "LH " << regStr(di.rd) << ", "
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::LB:
            oss << "LB " << regStr(di.rd) << ", "
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::LBU:
            oss << "LBU " << regStr(di.rd) << ", "
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::LHU:
            oss << "LHU " << regStr(di.rd) << ", "
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        // Stores
        case InstrKind::SW:
            oss << "SW " << regStr(di.rs2) << ", "
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::SH:
            oss << "SH " << regStr(di.rs2) << ", "
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::SB:
            oss << "SB " << regStr(di.rs2) << ", "
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        // Branches
        case InstrKind::BEQ:
            oss << "BEQ " << regStr(di.rs1) << ", "
                << regStr(di.rs2) << ", "
                << di.imm;
            break;

        case InstrKind::BNE:
            oss << "BNE " << regStr(di.rs1) << ", "
                << regStr(di.rs2) << ", "
                << di.imm;
            break;

        case InstrKind::BLT:
            oss << "BLT " << regStr(di.rs1) << ", "
                << regStr(di.rs2) << ", "
                << di.imm;
            break;

        case InstrKind::BGE:
            oss << "BGE " << regStr(di.rs1) << ", "
                << regStr(di.rs2) << ", "
                << di.imm;
            break;

        case InstrKind::BLTU:
            oss << "BLTU " << regStr(di.rs1) << ", "
                << regStr(di.rs2) << ", "
                << di.imm;
            break;

        case InstrKind::BGEU:
            oss << "BGEU " << regStr(di.rs1) << ", "
                << regStr(di.rs2) << ", "
                << di.imm;
            break;

        // Jumps
        case InstrKind::JAL:
            oss << "JAL " << regStr(di.rd) << ", "
                << di.imm;
            break;

        case InstrKind::JALR:
            oss << "JALR " << regStr(di.rd) << ", "
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::LUI:
            oss << "LUI " << regStr(di.rd) << ", "
                << di.imm;
            break;

        case InstrKind::AUIPC:
            oss << "AUIPC " << regStr(di.rd) << ", "
                << di.imm;
            break;

        case InstrKind::ECALL:
            oss << "ECALL";
            break;
            
        case InstrKind::EBREAK:
            oss << "EBREAK ";
            break;
        
        case InstrKind::FENCE:
            oss << "FENCE ";
            break;

        case InstrKind::CSRRW:
            oss << "CSRRW " << regStr(di.rd) << ", "
                << csrName(di.csr) << ", "
                << regStr(di.rs1);
            break;

        case InstrKind::CSRRS:
            oss << "CSRRS " << regStr(di.rd) << ", "
                << csrName(di.csr) << ", "
                << regStr(di.rs1);
            break;
            
        case InstrKind::CSRRC:
            oss << "CSRRC " << regStr(di.rd) << ", "
                << csrName(di.csr) << ", "
                << regStr(di.rs1);
            break;

        case InstrKind::CSRRWI:
            oss << "CSRRWI " << regStr(di.rd) << ", "
                << csrName(di.csr) << ", "
                << regStr(di.rs1);
            break;

        case InstrKind::CSRRSI:
            oss << "CSRRSI " << regStr(di.rd) << ", "
                << csrName(di.csr) << ", "
                << regStr(di.rs1);
            break;

        case InstrKind::CSRRCI:
            oss << "CSRRCI " << regStr(di.rd) << ", "
                << csrName(di.csr) << ", "
                << regStr(di.rs1);
            break;

        case InstrKind::MUL:
            oss << "MUL " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::MULH:
            oss << "MULH " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::MULHSU:
            oss << "MULHSU " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::MULHU:
            oss << "MULHU " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::DIV:
            oss << "DIV " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::DIVU:
            oss << "DIVU " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::REM:
            oss << "REM " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::REMU:
            oss << "REMU " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::MRET:
            oss << "MRET ";
            break;

        case InstrKind::SRET:
            oss << "SRET ";
            break;

        case InstrKind::WFI:
            oss << "WFI ";
            break;

        case InstrKind::SFENCE_VMA:
            oss << "SFENCE.VMA ";
            break;

        case InstrKind::LR_W:
            oss << "LR.W " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::SC_W:
            oss << "SC.W " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::AMOADD_W:
            oss << "AMOADD.W " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::AMOAND_W:
            oss << "AMOAND.W " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::AMOOR_W:
            oss << "AMOOR.W " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::AMOXOR_W:
            oss << "AMOXOR.W " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::AMOSWAP_W:
            oss << "AMOSWAP.W " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        case InstrKind::AMOMAX_W:
            oss << "AMOMAX.W " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

         case InstrKind::AMOMAXU_W:
            oss << "AMOMAXU.W " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

         case InstrKind::AMOMIN_W:
            oss << "AMOMIN.W " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

         case InstrKind::AMOMINU_W:
            oss << "AMOMINU.W " << regStr(di.rd) << ", "
                << regStr(di.rs1) << ", "
                << regStr(di.rs2);
            break;

        default:
            oss << "INVALID";
            break;
    }

    return oss.str();
}

void CPU::printTrace() const {
    std::cout 
        << "PC=" << hex32(sr.pc_before)
        << "\tINST=" << hex32(sr.instruction)
        << "\t" << disasm(sr.dInstr)
        << "\tNPC=" << hex32(sr.pc_after)
        << "\n";

    if (sr.reg_write.valid) {
        const RegWrite& rw = sr.reg_write;

        std::cout
            << "\tREG\t"
            << regStr(rw.rd)
            << ": "
            << hex32(rw.old_val)
            << " -> "
            << hex32(rw.new_val)
            << "\n";
    }

    if (sr.mem_write.valid) {
        const MemWrite& mw = sr.mem_write;

        std::cout
            << "\tMEM\t"
            << "["
            << hex32(mw.addr)
            << "]: "
            << hex32(mw.old_val)
            << " -> "
            << hex32(mw.new_val)
            << "\t("
            << static_cast<unsigned int>(mw.size)
            << "B)\n";
    }

    if (sr.csr_write.valid) {
        const CsrWrite& cw = sr.csr_write;

        std::cout
            << "\tCSR\t"
             << "["
            << csrName(cw.addr)
            << "]: "
            << hex32(cw.old_val)
            << " -> "
            << hex32(cw.new_val)
            << "\n";
    }
}

std::string CPU::csrName(uint16_t csr) const {
    switch(csr) {
        case MISA: return "MISA";
        case MTVEC: return "MTVEC";
        case MSTATUS: return "MSTATUS";
        case MSTATUSH: return "MSTATUSH";
        case MIDELEG: return "MIDELEG";
        case MEDELEG: return "MEDELEG";
        case MCAUSE: return "MCAUSE";
        case MEPC: return "MEPC";
        case MTVAL: return "MTVAL";
        case MCYCLE: return "MCYCLE";
        case MCYCLEH: return "MCYCLEH";
        case CYCLE: return "CYCLE";
        case CYCLEH: return "CYCLEH";
        case MINSTRET: return "MINSTRET";
        case MINSTRETH: return "MINSTRETH";
        case MVENDORID: return "MVENDORID";
        case MIP: return "MIP";
        case MIE: return "MIE";
        case SSTATUS: return "SSTATUS";
        case SIE: return "SIE";
        case STVEC: return "STVEC";
        case SSCRATCH: return "SSCRATCH";
        case SEPC: return "SEPC";
        case SCAUSE: return "SCAUSE";
        case STVAL: return "STVAL";
        case SIP: return "SIP";
        case SATP: return "SATP";
        case MENVCFG: return "MENVCFG";
        case MENVCFGH: return "MENVCFGH";
        case SENVCFG: return "SENVCFG";
        case MSTATEEN0: return "MSTATEEN0";
        case MSTATEEN1: return "MSTATEEN1";
        case MSTATEEN2: return "MSTATEEN2";
        case MSTATEEN3: return "MSTATEEN3";
        case SSTATEEN0: return "SSTATEEN0";
        case MCOUNTEREN: return "MCOUNTEREN";
        case SCOUNTEREN: return "SCOUNTEREN";
        case MCOUNTINHIBIT: return "MCOUNTINHIBIT";
        case MSCRATCH: return "MSCRATCH";
        case MHARTID: return "MHARTID";  
        case TIME: return "TIME";
        case TIMEH: return "TIMEH";
        default: return hex32(csr);
    }
}

void CPU::dumpRegisters() const {
    std::cout << "REGISTERS" << "\n";
    for (int i = 0; i < 32; i++) {
        std::cout
            << "\tREG\t"
            << regStr(i)
            << ": "
            << hex32(regs_[i])
            << "\n";
    }
}