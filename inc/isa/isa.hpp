// inc/cpu/isa.hpp

#pragma once
#include <cstdint>

enum class InstrKind {
    ADD, SUB, AND, OR, XOR, SLL, SLT, SLTU, SRL, SRA, // R-type Instructions
    ADDI, ANDI, ORI, XORI, SLTI, SLTIU, SLLI, SRLI, SRAI, // I-type Instructions
    LW, LH, LHU, LB, LBU, // Loads
    SW, SH, SB, // Stores
    BEQ, BNE, BLT, BGE, BLTU, BGEU, // Branches
    JAL, JALR, // Jumps
    LUI, AUIPC, // Upper immediates
    ECALL, EBREAK, // System calls
    FENCE, // Memory fence
    CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI, // CSR Instructions
    MUL, MULH, MULHSU, MULHU, // Multiply instructions
    DIV, DIVU, REM, REMU, // Divide instructions
    MRET, SRET, WFI,
    SFENCE_VMA,
    LR_W, SC_W, AMOSWAP_W, AMOADD_W, AMOAND_W, AMOOR_W, AMOXOR_W,
    AMOMAX_W, AMOMAXU_W, AMOMIN_W, AMOMINU_W,
    INVALID, 
    COUNT
};

struct DecodedInstr {
    InstrKind kind = InstrKind::INVALID;

    uint8_t rd  = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;

    int32_t imm = 0;
    uint16_t csr = 0;
    uint32_t raw = 0;
    uint32_t instr_len = 0;
};

struct Extensions {
    bool m = false;
    bool c = false;
    bool a = false;
};