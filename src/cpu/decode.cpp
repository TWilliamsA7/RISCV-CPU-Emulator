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
    d.raw = instr;
    
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

// C-extension Instruction Decompression

// Maps 3-bit C-register index to standard x-register index
uint32_t encodeJType(int32_t imm, uint32_t rd) {
    uint32_t u = static_cast<uint32_t>(imm);
    
    uint32_t i20    = (u >> 20) & 0x1;
    uint32_t i10_1  = (u >> 1)  & 0x3FF;
    uint32_t i11    = (u >> 11) & 0x1;
    uint32_t i19_12 = (u >> 12) & 0xFF;

    uint32_t res = (i20 << 31) | (i10_1 << 21) | (i11 << 20) | (i19_12 << 12) | (rd << 7) | 0x6F;
    return res;
}

uint32_t encodeBType(int32_t imm, uint32_t rs1, uint32_t rs2, uint32_t f3) {
    uint32_t u = static_cast<uint32_t>(imm);
    uint32_t imm_12 = (u & 0x1000) << 19;      // bit 12 -> 31
    uint32_t imm_10_5 = (u & 0x7E0) << 20;     // bits 10:5 -> 30:25
    uint32_t imm_4_1 = (u & 0x1E) << 7;        // bits 4:1 -> 11:8
    uint32_t imm_11 = (u & 0x800) >> 4;        // bit 11 -> 7
    return imm_12 | imm_10_5 | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | imm_4_1 | imm_11 | 0x63;
}

// Helper to extract the 6-bit signed immediate used in Q1 and Q2
int32_t get_imm6(uint16_t i) {
    // bit 12 is the sign bit (MSB), bits 6:2 are the rest
    int32_t imm = ((i >> 12) & 0x1) << 5 | ((i >> 2) & 0x1F);
    // Sign extend from bit 5 to bit 31
    if (imm & 0x20) imm |= 0xFFFFFFC0;
    return imm;
}


uint32_t CPU::decompress(uint16_t i) {
    uint32_t op = i & 0x3;
    uint32_t funct3 = (i >> 13) & 0x7;
    
    // Common bit-field extractions
    uint32_t rd_rs1_high = (i >> 7) & 0x1F; // bits 11:7
    uint32_t rs2_low = (i >> 2) & 0x1F;     // bits 6:2
    uint32_t rd_p = 8 + ((i >> 2) & 0x7);    // bits 4:2 (C-style register)
    uint32_t rs1_p = 8 + ((i >> 7) & 0x7);   // bits 9:7 (C-style register)
    uint32_t rs2_p = 8 + ((i >> 2) & 0x7);   // bits 4:2 (C-style register)

    switch (op) {
        case 0: // Quadrant 0: Memory/SP-Relative
            switch (funct3) {
                case 0: { // C.ADDI4SPN -> addi rd', x2, nzuimm
                    uint32_t imm = ((i >> 7) & 0x30) | ((i >> 1) & 0x3C0) | ((i >> 4) & 0x4) | ((i >> 2) & 0x8);
                    if (imm == 0) return 0; // Illegal
                    return (imm << 20) | (2 << 15) | (0 << 12) | (rd_p << 7) | 0x13;
                }
                case 2: { // C.LW -> lw rd', offset(rs1')
                    uint32_t imm = ((i >> 7) & 0x38) | ((i >> 4) & 0x4) | ((i << 1) & 0x40);
                    return (imm << 20) | (rs1_p << 15) | (2 << 12) | (rd_p << 7) | 0x03;
                }
                case 6: { // C.SW -> sw rs2', offset(rs1')
                    uint32_t imm = ((i >> 7) & 0x38) | ((i >> 4) & 0x4) | ((i << 1) & 0x40);
                    return ((imm >> 5) << 25) | (rs2_p << 20) | (rs1_p << 15) | (2 << 12) | ((imm & 0x1F) << 7) | 0x23;
                }
                default: return 0; // Reserved/Illegal
            }

        case 1: // Quadrant 1: Arithmetic/Jumps
            switch (funct3) {
                case 0: { // C.ADDI -> addi rd, rd, nzimm
                    uint32_t rd_rs1 = (i >> 7) & 0x1F;
    
                    // Extract the 6-bit immediate: bit 12 is MSB, bits 6:2 are LSBs
                    int32_t imm6 = ((i >> 12) & 0x1) << 5 | ((i >> 2) & 0x1F);
                    
                    // SIGN EXTEND the 6-bit value to 32-bit
                    // If bit 5 (originally bit 12) is set, fill the upper 26 bits with 1s
                    if (imm6 & 0x20) {
                        imm6 |= 0xFFFFFFC0;
                    }

                    // Special case: C.NOP (addi x0, x0, 0) is encoded as 0x0001
                    if (rd_rs1 == 0 && imm6 == 0) return 0x00000013; 

                    // Construct the 32-bit ADDI: [imm11:0][rs1][000][rd][0010011]
                    return (uint32_t(imm6) << 20) | (rd_rs1 << 15) | (0 << 12) | (rd_rs1 << 7) | 0x13;
                }
                case 1: { // C.JAL (RV32 only) -> jal x1, offset
                    int32_t imm = (int32_t(i << 16) >> 31) << 11 | ((i & 0x400) >> 2) | ((i >> 7) & 0x18) | 
                                  ((i >> 1) & 0x40) | ((i >> 7) & 0x4) | ((i >> 2) & 0xE) | ((i << 3) & 0x200) | ((i >> 1) & 0x300);
                    return (uint32_t(imm) << 20) | (1 << 7) | 0x6F; // This needs JAL immediate encoding logic
                    // Note: For J/JAL, bit-swizzling into J-type is complex. Use standard J-type encoding.
                }
                case 2: { // C.LI -> addi rd, x0, imm
                    int32_t imm = get_imm6(i);
                    if ((i >> 7) & 0x1F == 0) return 0;
                    return (uint32_t(imm) << 20) | (0 << 15) | (0 << 12) | (rd_rs1_high << 7) | 0x13;
                }
                case 3: { // C.LUI or C.ADDI16SP
                    if (rd_rs1_high == 2) { // C.ADDI16SP -> addi x2, x2, imm
                        // The immediate is bits: 12|4|3|5|2|6
                        // It is signed and scaled by 16.
                        int32_t imm = ((i >> 12) & 0x1) ? 0xFFFFFE00 : 0; // Sign extend from bit 9
                        imm |= ((i >> 3) & 0x3) << 7;  // bits 8:7
                        imm |= ((i >> 5) & 0x1) << 6;  // bit 6
                        imm |= ((i >> 2) & 0x1) << 5;  // bit 5
                        imm |= ((i >> 6) & 0x1) << 4;  // bit 4
                        
                        // Final signed value (already scaled by logic above)
                        // Let's verify 617d: i=0110000101111101
                        // bit 12=0, bit 6=1, bit 5=1, bit 4=1, bit 3=1, bit 2=1
                        // This should yield 496.
                        
                        // In the 32-bit ADDI, this goes into the I-type imm field
                        return (uint32_t(imm) << 20) | (2 << 15) | (0 << 12) | (2 << 7) | 0x13;
                    }
                    // Standard C.LUI logic for rd != 2
                    int32_t imm = get_imm6(i);
                    if (((i >> 7) & 0x1F) == 0 || imm == 0) return 0; // Reserved
                    // LUI immediate is the upper 20 bits. 
                    // Our 6-bit signed immediate becomes bits 17:12.
                    return (uint32_t(imm) << 12) | (((i >> 7) & 0x1F) << 7) | 0x37;
                }

                case 4: { // Logic/Shifts
                    uint32_t sub = (i >> 10) & 0x3;
                    uint32_t shamt = ((i >> 12) & 0x1) << 5 | ((i >> 2) & 0x1F);
                    if (sub == 0) return (shamt << 20) | (rs1_p << 15) | (5 << 12) | (rs1_p << 7) | 0x13; // SRLI
                    if (sub == 1) return (0x400 << 20) | (shamt << 20) | (rs1_p << 15) | (5 << 12) | (rs1_p << 7) | 0x13; // SRAI
                    if (sub == 2) { // ANDI
                       int32_t imm = get_imm6(i);
                        uint32_t rd_p = 8 + ((i >> 7) & 0x7);
                        return (uint32_t(imm) << 20) | (rd_p << 15) | (7 << 12) | (rd_p << 7) | 0x13;     
                    }
                    if (sub == 3) {
                        uint32_t f2 = (i >> 5) & 0x3;
                        if (f2 == 0) return 0x40000033 | (rs1_p << 15) | (rs1_p << 7) | (rs2_p << 20); // SUB
                        if (f2 == 1) return 0x00004033 | (rs1_p << 15) | (rs1_p << 7) | (rs2_p << 20); // XOR
                        if (f2 == 2) return 0x00006033 | (rs1_p << 15) | (rs1_p << 7) | (rs2_p << 20); // OR
                        if (f2 == 3) return 0x00007033 | (rs1_p << 15) | (rs1_p << 7) | (rs2_p << 20); // AND
                    }
                    return 0;
                }
                case 5: { // C.J -> jal x0, offset
                    // Offset mapping: 11|4|9:8|10|6|7|3:1|5
                    int32_t offset = 
                        ((i >> 12) & 1) << 11 | // inst[12] -> imm[11] (sign)
                        ((i >> 8)  & 1) << 10 | // inst[8]  -> imm[10]
                        ((i >> 9)  & 3) << 8  | // inst[10:9]-> imm[9:8]
                        ((i >> 6)  & 1) << 7  | // inst[6]  -> imm[7]
                        ((i >> 7)  & 1) << 6  | // inst[7]  -> imm[6]
                        ((i >> 2)  & 1) << 5  | // inst[2]  -> imm[5]
                        ((i >> 11) & 1) << 4  | // inst[11] -> imm[4]
                        ((i >> 3)  & 7) << 1;   // inst[5:3] -> imm[3:1]

                    // Sign extend from bit 11 to 32
                    if (offset & 0x800) offset |= 0xFFFFF000;
                    // Construct J-type JAL x0
                    return encodeJType(offset, 0); 
                }
                case 6: case 7: { // C.BEQZ, C.BNEZ -> beq/bne rs1', x0, offset
                   uint32_t rs1_p = 8 + ((i >> 7) & 0x7);
    
                    // Offset bits: [8|4:3|2:1|7:6|5]
                    int32_t offset = ((i >> 12) & 1) << 8 |   // bit 8 (sign)
                                    ((i >> 10) & 3) << 3 |   // bits 4:3
                                    ((i >> 5)  & 3) << 6 |   // bits 7:6
                                    ((i >> 2)  & 1) << 5 |   // bit 5
                                    ((i >> 3)  & 3) << 1;    // bits 2:1
                    
                    // Sign extend from bit 8 to bit 31
                    if (offset & 0x100) offset |= 0xFFFFFE00;

                    // Use your B-Type encoder
                    uint32_t funct3_32 = (funct3 == 6) ? 0 : 1; // BEQ (0) or BNE (1)
                    return encodeBType(offset, rs1_p, 0, funct3_32);
                }
                default: return 0;
            }

        case 2: // Quadrant 2: High-Speed/SP
            switch (funct3) {
                case 0: { // C.SLLI -> slli rd, rd, shamt
                    uint32_t shamt = ((i >> 12) & 0x1) << 5 | ((i >> 2) & 0x1F);
                    return (shamt << 20) | (rd_rs1_high << 15) | (1 << 12) | (rd_rs1_high << 7) | 0x13;
                }
                case 2: { // C.LWSP -> lw rd, offset(x2)
                    uint32_t imm = ((i >> 2) & 0x1C) | ((i >> 7) & 0x20) | ((i << 4) & 0xC0);
                    return (imm << 20) | (2 << 15) | (2 << 12) | (rd_rs1_high << 7) | 0x03;
                }
                case 4: { // C.JR, C.MV, C.EBREAK, C.JALR, C.ADD
                    bool bit12 = (i >> 12) & 0x1;
                    if (!bit12 && rs2_low == 0) return (rd_rs1_high << 15) | (0 << 12) | (0 << 7) | 0x67; // C.JR -> jalr x0, rs1, 0
                    if (!bit12 && rs2_low != 0) return (0 << 15) | (rs2_low << 20) | (rd_rs1_high << 7) | 0x33; // C.MV -> add rd, x0, rs2
                    if (bit12 && rd_rs1_high == 0 && rs2_low == 0) return 0x00100073; // C.EBREAK
                    if (bit12 && rd_rs1_high != 0 && rs2_low == 0) return (rd_rs1_high << 15) | (0 << 12) | (1 << 7) | 0x67; // C.JALR -> jalr x1, rs1, 0
                    if (bit12 && rd_rs1_high != 0 && rs2_low != 0) return (rd_rs1_high << 15) | (rs2_low << 20) | (rd_rs1_high << 7) | 0x33; // C.ADD -> add rd, rd, rs2
                    return 0;
                }
                case 6: { // C.SWSP -> sw rs2, offset(x2)
                    uint32_t imm = ((i >> 7) & 0x3C) | ((i >> 1) & 0xC0);
                    return ((imm >> 5) << 25) | (rs2_low << 20) | (2 << 15) | (2 << 12) | ((imm & 0x1F) << 7) | 0x23;
                }
                default: return 0;
            }
        default: return 0;
    }
}