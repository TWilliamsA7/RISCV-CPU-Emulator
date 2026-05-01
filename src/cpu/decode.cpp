// src/cpu/decode.cpp

#include "cpu/cpu.hpp"

static inline uint32_t get_bits(uint32_t x, int hi, int lo) {
    return (x >> lo) & ((1u << (hi - lo + 1)) - 1);
}


static inline int32_t sign_extend(uint32_t value, int bits) {
    uint32_t mask = 1u << (bits - 1);
    return (value ^ mask) - mask;
}


DecodedInstr CPU::decode(uint32_t instr) {
    DecodedInstr d;
    
    uint32_t opcode = get_bits(instr, 6, 0);
    uint32_t funct3 = get_bits(instr, 14, 12);
    uint32_t funct7 = get_bits(instr, 31, 25);

    d.kind = InstrKind::INVALID;
    d.rd  = get_bits(instr, 11, 7);
    d.rs1 = get_bits(instr, 19, 15);
    d.rs2 = get_bits(instr, 24, 20);

    switch (opcode) {
        case 0x33: { // Register ALU
            switch (funct3) {
                case 0x0:
                    if (funct7 == 0x00)
                        d.kind = InstrKind::ADD;
                    else if (funct7 == 0x20)
                        d.kind = InstrKind::SUB;
                    else if (funct7 == 0x01)
                        d.kind = InstrKind::MUL;
                    break;

                case 0x1: 
                    if (funct7 == 0x01)
                        d.kind = InstrKind::MULH;
                    else
                        d.kind = InstrKind::SLL; 
                    break;

                case 0x2: 
                    if (funct7 == 0x01)
                        d.kind = InstrKind::MULHSU;
                    else
                        d.kind = InstrKind::SLT; 
                    break;

                case 0x3:
                    if (funct7 == 0x01)
                        d.kind = InstrKind::MULHU;
                    else
                        d.kind = InstrKind::SLTU; 
                    break;

                case 0x4:
                    if (funct7 == 0x01)
                        d.kind = InstrKind::DIV;
                    else
                        d.kind = InstrKind::XOR; 
                    break;

                case 0x5:
                    if (funct7 == 0x00)
                        d.kind = InstrKind::SRL;
                    else if (funct7 == 0x20)
                        d.kind = InstrKind::SRA;
                    else if (funct7 == 0x1)
                        d.kind = InstrKind::DIVU;
                    break;

                case 0x7:
                    if (funct7 == 0x01)
                        d.kind = InstrKind::REMU;
                    else
                        d.kind = InstrKind::AND; 
                    break;

                case 0x6:
                    if (funct7 == 0x01)
                        d.kind = InstrKind::REM;
                    else
                        d.kind = InstrKind::OR; 
                    break;
            }
            d.imm = 0;
            break;
        }

        case 0x13: { // Immediate ALU
            d.imm = sign_extend(get_bits(instr, 31, 20), 12);
            switch (funct3) {
                case 0x0: d.kind = InstrKind::ADDI; break;

                case 0x1: d.kind = InstrKind::SLLI; break;

                case 0x2: d.kind = InstrKind::SLTI; break;

                case 0x3: d.kind = InstrKind::SLTIU;  break;

                case 0x4: d.kind = InstrKind::XORI; break;

                case 0x5:
                    if (funct7 == 0x00)
                        d.kind = InstrKind::SRLI;
                    else if (funct7 == 0x20)
                        d.kind = InstrKind::SRAI;
                    break;

                case 0x6: d.kind = InstrKind::ORI;  break;

                case 0x7: d.kind = InstrKind::ANDI; break;
            }
            d.rs2 = 0;
            break;
        }

        case 0x03: { // Loads
            d.imm = sign_extend(get_bits(instr, 31, 20), 12);
            switch (funct3) {
                case 0x0: d.kind = InstrKind::LB; break;
                case 0x1: d.kind = InstrKind::LH; break;
                case 0x2: d.kind = InstrKind::LW; break;
                case 0x4: d.kind = InstrKind::LBU; break;
                case 0x5: d.kind = InstrKind::LHU; break;
            }
            break;
        }

        case 0x23: { // Stores
            uint32_t imm_u = (get_bits(instr, 31, 25) << 5) | get_bits(instr, 11, 7);
            d.imm = sign_extend(imm_u, 12);

            switch (funct3) {
                case 0x0: d.kind = InstrKind::SB; break;
                case 0x1: d.kind = InstrKind::SH; break;
                case 0x2: d.kind = InstrKind::SW; break;
            }
            d.rd = 0;
            break;
        }

        case 0x63: { // Branches
            uint32_t imm_u =
                (get_bits(instr, 31, 31) << 12) |
                (get_bits(instr, 7, 7)   << 11) |
                (get_bits(instr, 30, 25) << 5)  |
                (get_bits(instr, 11, 8)  << 1);

            d.imm = sign_extend(imm_u, 13);

            switch (funct3) {
                case 0x0: d.kind = InstrKind::BEQ; break;
                case 0x1: d.kind = InstrKind::BNE; break;
                case 0x4: d.kind = InstrKind::BLT; break;
                case 0x5: d.kind = InstrKind::BGE; break;
                case 0x6: d.kind = InstrKind::BLTU; break;
                case 0x7: d.kind = InstrKind::BGEU; break;
            }
            d.rd = 0;
            break;
        }

        case 0x6F: { // JAL
            uint32_t imm_u =
                (get_bits(instr, 31, 31) << 20) |
                (get_bits(instr, 19, 12) << 12) |
                (get_bits(instr, 20, 20) << 11) |
                (get_bits(instr, 30, 21) << 1);

            d.imm = sign_extend(imm_u, 21);
            d.kind = InstrKind::JAL;
            d.rs1 = d.rs2 = 0;
            break;
     
        }

        case 0x67: { // JALR
            d.imm = sign_extend(get_bits(instr, 31, 20), 12);
            d.kind = InstrKind::JALR;
            break;
        }

        case 0x37: { // LUI
            d.imm = get_bits(instr, 31, 12) << 12;
            d.kind = InstrKind::LUI;
            d.rs1 = d.rs2 = 0;
            break;
        }

        case 0x17: { // AUIPC
            d.imm = get_bits(instr, 31, 12) << 12;
            d.kind = InstrKind::AUIPC;
            d.rs1 = d.rs2 = 0;
            break;
        }   

        case 0x0F: { // FENCE
            if (funct3 == 0x0) {
                d.kind = InstrKind::FENCE;
                d.rd = get_bits(instr, 11, 7);
                d.rs1 = get_bits(instr, 19, 15);
                d.imm = get_bits(instr, 31, 20);
            }
            break;
        }

        case 0x73: { // CSR and SYSTEM

            d.rd  = get_bits(instr, 11, 7);
            d.csr = get_bits(instr, 31, 20);

            uint32_t funct12 = get_bits(instr, 31, 20);

            switch (funct3) {
                case 0x0: {
                    if (funct12 == 0x0)
                        d.kind = InstrKind::ECALL;
                    else if (funct12 == 0x1)
                        d.kind = InstrKind::EBREAK;
                    else if (funct12 = 0x302)
                        d.kind = InstrKind::MRET;
                    break;
                }   

                case 0x1:
                    d.kind = InstrKind::CSRRW;
                    break;
                case 0x2:
                    d.kind = InstrKind::CSRRS;
                    break;
                case 0x3:
                    d.kind = InstrKind::CSRRC;
                    break;
                case 0x5:
                    d.kind = InstrKind::CSRRWI;
                    d.imm = d.rs1;
                    break;
                case 0x6:
                    d.kind = InstrKind::CSRRSI;
                    d.imm = d.rs1;
                    break;
                case 0x7:
                    d.kind = InstrKind::CSRRCI;
                    d.imm = d.rs1;
                    break;
            }
            break;
        }
 
        default:
            d.kind = InstrKind::INVALID;
            break;
    }

    return d;
}