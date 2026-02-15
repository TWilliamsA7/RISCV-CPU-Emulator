// src/cpu/decode.cpp

#include "cpu/decode.hpp"

static inline uint32_t get_bits(uint32_t x, int hi, int lo) {
    return (x >> lo) & ((1u << (hi - lo + 1)) - 1);
}

static inline int32_t sign_extend(uint32_t value, int bits) {
    uint32_t mask = 1u << (bits - 1);
    return (value ^ mask) - mask;
}


DecodedInstr decode(uint32_t instr) {
    DecodedInstr d;
    
    uint32_t opcode = get_bits(instr, 6, 0);
    uint32_t funct3 = get_bits(instr, 14, 12);
    uint32_t funct7 = get_bits(instr, 31, 25);

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
                    break;

                case 0x1:
                    d.kind = InstrKind::SLL;
                    break;

                case 0x2:
                    d.kind = InstrKind::SLT;
                    break;

                case 0x3:
                    d.kind = InstrKind::SLTU;
                    break;

                case 0x4:
                    d.kind = InstrKind::XOR;
                    break;

                case 0x5:
                    if (funct7 == 0x00)
                        d.kind = InstrKind::SRL;
                    else if (funct7 == 0x20)
                        d.kind = InstrKind::SRA;
                    break;

                case 0x7:
                    d.kind = InstrKind::AND;
                    break;

                case 0x6:
                    d.kind = InstrKind::OR;
                    break;

            }
            break;
        }

        case 0x13: { // Immediate ALU
            d.imm = sign_extend(get_bits(instr, 31, 20), 12);
            switch (funct3) {
                case 0x0: d.kind = InstrKind::ADDI; break;
                case 0x7: d.kind = InstrKind::ANDI; break;
                case 0x6: d.kind = InstrKind::ORI;  break;
                case 0x4: d.kind = InstrKind::XORI; break;
            }
            break;
        }
        case 0x03: { // Loads
            d.imm = sign_extend(get_bits(instr, 31, 20), 12);
            switch (funct3) {
                case 0x2:
                    d.kind = InstrKind::LW;
                    break;
            }
            break;
        }

        case 0x23: { // Stores
            uint32_t imm_u = (get_bits(instr, 31, 25) << 5) | get_bits(instr, 11, 7);
            d.imm = sign_extend(imm_u, 12);

            switch (funct3) {
                case 0x2:
                    d.kind = InstrKind::SW;
                    break;
            }
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
            }

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
            break;
     
        }

        case 0x67: { // JALR
            d.imm = sign_extend(get_bits(instr, 31, 20), 12);
            break;
        }

        case 0x37: { // LUI
            d.imm = get_bits(instr, 31, 12) << 12;
            d.kind = InstrKind::LUI;
            break;
        }

        case 0x17: { // AUIPC
            d.imm = get_bits(instr, 31, 12) << 12;
            break;
        }   
 
        default:
            d.kind = InstrKind::INVALID;
            break;
    }

    return d;
}