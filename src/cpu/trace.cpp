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
    return "x" + std::to_string(r);
}

static std::string disasm(const DecodedInstr& di) {
    std::ostringstream oss;

    switch (di.kind) {

        // R-type
        case InstrKind::ADD:
            oss << "ADD " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << regStr(di.rs2);
            break;

        case InstrKind::SUB:
            oss << "SUB " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << regStr(di.rs2);
            break;

        case InstrKind::AND:
            oss << "AND " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << regStr(di.rs2);
            break;

        case InstrKind::OR:
            oss << "OR " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << regStr(di.rs2);
            break;

        case InstrKind::XOR:
            oss << "XOR " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << regStr(di.rs2);
            break;

        case InstrKind::SLL:
            oss << "SLL " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << regStr(di.rs2);
            break;

        case InstrKind::SLT:
            oss << "SLT " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << regStr(di.rs2);
            break;

        case InstrKind::SLTU:
            oss << "SLTU " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << regStr(di.rs2);
            break;
        
        case InstrKind::SRL:
            oss << "SRL " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << regStr(di.rs2);
            break;

        case InstrKind::SRA:
            oss << "SRA " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << regStr(di.rs2);
            break;

        // I-type arithmetic
        case InstrKind::ADDI:
            oss << "ADDI " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << di.imm;
            break;

        case InstrKind::ANDI:
            oss << "ANDI " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << di.imm;
            break;

        case InstrKind::ORI:
            oss << "ORI " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << di.imm;
            break;

        case InstrKind::XORI:
            oss << "XORI " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << di.imm;
            break;

        case InstrKind::SLTI:
            oss << "SLTI " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << di.imm;
            break;

        case InstrKind::SLTIU:
            oss << "SLTIU " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << di.imm;
            break;

        case InstrKind::SLLI:
            oss << "SLLI " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << di.imm;
            break;

        case InstrKind::SRLI:
            oss << "SRLI " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << di.imm;
            break;

        case InstrKind::SRAI:
            oss << "SRAI " << regStr(di.rd) << ","
                << regStr(di.rs1) << ","
                << di.imm;
            break;

        // Loads
        case InstrKind::LW:
            oss << "LW " << regStr(di.rd) << ","
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::LH:
            oss << "LH " << regStr(di.rd) << ","
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::LB:
            oss << "LB " << regStr(di.rd) << ","
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::LBU:
            oss << "LBU " << regStr(di.rd) << ","
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::LHU:
            oss << "LHU " << regStr(di.rd) << ","
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        // Stores
        case InstrKind::SW:
            oss << "SW " << regStr(di.rs2) << ","
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::SH:
            oss << "SH " << regStr(di.rs2) << ","
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::SB:
            oss << "SB " << regStr(di.rs2) << ","
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        // Branches
        case InstrKind::BEQ:
            oss << "BEQ " << regStr(di.rs1) << ","
                << regStr(di.rs2) << ","
                << di.imm;
            break;

        case InstrKind::BNE:
            oss << "BNE " << regStr(di.rs1) << ","
                << regStr(di.rs2) << ","
                << di.imm;
            break;

        case InstrKind::BLT:
            oss << "BLT " << regStr(di.rs1) << ","
                << regStr(di.rs2) << ","
                << di.imm;
            break;

        case InstrKind::BGE:
            oss << "BGE " << regStr(di.rs1) << ","
                << regStr(di.rs2) << ","
                << di.imm;
            break;

        case InstrKind::BLTU:
            oss << "BLTU " << regStr(di.rs1) << ","
                << regStr(di.rs2) << ","
                << di.imm;
            break;

        case InstrKind::BGEU:
            oss << "BGEU " << regStr(di.rs1) << ","
                << regStr(di.rs2) << ","
                << di.imm;
            break;

        // Jumps
        case InstrKind::JAL:
            oss << "JAL " << regStr(di.rd) << ","
                << di.imm;
            break;

        case InstrKind::JALR:
            oss << "JALR " << regStr(di.rd) << ","
                << di.imm << "(" << regStr(di.rs1) << ")";
            break;

        case InstrKind::LUI:
            oss << "LUI " << regStr(di.rd) << ","
                << di.imm;
            break;

        case InstrKind::AUIPC:
            oss << "AUIPC " << regStr(di.rd) << ","
                << di.imm;
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

    if (sr.reg_write.has_value()) {
        const RegWrite& rw = sr.reg_write.value();

        std::cout
            << "\tREG\t"
            << regStr(rw.rd)
            << ": "
            << hex32(rw.old_val)
            << " -> "
            << hex32(rw.new_val)
            << "\n";
    }

    if (sr.mem_write.has_value()) {
        const MemWrite& mw = sr.mem_write.value();

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
}

void CPU::dumpRegisters() const {
    std::cout << "REGISTERS" << "\n";
    for (int i = 0; i < 32; i++) {
        std::cout
            << "\tREG\t"
            << regStr(i)
            << ": "
            << hex32(r[i])
            << "\n";
    }
}

void CPU::addBreakpoint(uint32_t addr) {
    breakpoints_.insert(addr);
}

void CPU::removeBreakpoint(uint32_t addr) {
    breakpoints_.erase(addr);
}