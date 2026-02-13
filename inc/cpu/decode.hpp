// inc/cpu/decode.hpp

#pragma once
#include <cstdint>

enum class InstrKind {
    ADD, SUB, AND, OR, XOR,
    ADDI, ANDI, ORI,
    LW, SW,
    BEQ, BNE,
    JAL, JALR,
    LUI, AUIPC,
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