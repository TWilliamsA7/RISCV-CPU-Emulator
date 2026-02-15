// inc/cpu/decode.hpp

#pragma once
#include <cstdint>

enum class InstrKind {
    ADD, SUB, AND, OR, XOR, SLL, SLT, SLTU, SRL, SRA, // R-type Instructions
    ADDI, ANDI, ORI, XORI, SLTI, SLTIU, SLLI, SRLI, SRAI, // I-type Instructions
    LW, LB, LH, LBU, LHU, // Loads
    SW, SH, SB, // Stores
    BEQ, BNE, BLT, BGE, BLTU, BGEU, // Branches
    JAL, JALR, // Jumps
    LUI, AUIPC, // Upper immediates
    INVALID
};

struct DecodedInstr {
    InstrKind kind = InstrKind::INVALID;

    uint8_t rd  = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;

    int32_t imm = 0;
};

DecodedInstr decode(uint32_t instr);