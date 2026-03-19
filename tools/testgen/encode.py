# encode.py

from enum import Enum

class Instr(Enum):
    ADD = (0x33, 0x0, 0x00)
    SUB = (0x33, 0x0, 0x20)
    SLL = (0x33, 0x1, None)
    SLT = (0x33, 0x2, None)
    SLTU = (0x33, 0x3, None)
    XOR = (0x33, 0x4, None)
    SRL = (0x33, 0x5, 0x0)
    SRA = (0x33, 0x5, 0x20)
    OR = (0x33, 0x6, None)
    AND = (0x33, 0x7, None)
    ADDI = (0x13, 0x0, None)
    SLLI = (0x13, 0x1, None)
    SLTI = (0x13, 0x2, None)
    SLTIU = (0x13, 0x3, None)
    XORI = (0x13, 0x4, None)
    SRLI = (0x13, 0x5, 0x0)
    SRAI = (0x13, 0x5, 0x20)
    ORI = (0x13, 0x6, None)
    ANDI = (0x13, 0x7, None)
    LB = (0x03, 0x0, None)
    LH = (0x03, 0x1, None)
    LW = (0x03, 0x2, None)
    LBU = (0x03, 0x4, None)
    LHU = (0x03, 0x5, None)
    SB = (0x23, 0x0, None)
    SH = (0x23, 0x1, None)
    SW = (0x23, 0x2, None)
    BEQ = (0x63, 0x0, None)
    BNE = (0x63, 0x1, None)
    BLT = (0x63, 0x4, None)
    BGE = (0x63, 0x5, None)
    BLTU = (0x63, 0x6, None)
    BGEU = (0x63, 0x7, None)
    JAL = (0x6F, None, None)
    JALR = (0x67, None, None)
    LUI = (0x37, None, None)
    AUIPC = (0x17, None, None)
    INVALID = (None, None, None)



def encode(instr: Instr, rd=None, rs1=None, rs2=None, imm=None):
    opcode = instr.value[0]
    funct3 = instr.value[1]
    funct7 = instr.value[2]

    match opcode:
        case 0x33: # R-type
            return ((funct7 & 0x7f) << 25) | ((rs2 & 0x1f) << 20) | ((rs1 & 0x1f) << 15) | ((funct3 & 0x7) << 12) | ((rd & 0x1f) << 7)  | (opcode & 0x7f)
        case 0x13 | 0x03 | 0x67 if instr.name not in ["SLLI", "SRLI", "SRAI"]: # I-type, Load, and JALR
            return ((imm & 0xfff) << 20) | ((rs1 & 0x1f) << 15) | ((funct3 & 0x7) << 12) | ((rd & 0x1f) << 7)  | (opcode & 0x7f)
        case 0x13: # I-type Shifts
            return ((funct7 & 0x7f) << 25) | ((imm & 0x1f) << 20) | ((rs1 & 0x1f) << 15) | ((funct3 & 0x7) << 12) | ((rd & 0x1f) << 7)  | (opcode  & 0x7f)
        case 0x23: # S-type
            return (((imm >> 5) & 0x7f) << 25) | ((rs2 & 0x1f) << 20) | ((rs1 & 0x1f) << 15) | ((funct3 & 0x7) << 12) | ((imm & 0x1f) << 7) | (opcode & 0x7f)
        case 0x63: # B-type
            return (((imm >> 12) & 0x1) << 31) | (((imm >> 5) & 0x3f) << 25) | ((rs2 & 0x1f) << 20) | ((rs1 & 0x1f) << 15) | ((funct3 & 0x7) << 12) | (((imm >> 1) & 0xf) << 8)  | (((imm >> 11) & 0x1) << 7)  | (opcode & 0x7f)
        case 0x37 | 0x17: # U-type
            return ((imm & 0xfffff000)) | ((rd & 0x1f) << 7) | (opcode & 0x7f)
        case 0x6F: # J-type
            return (((imm >> 20) & 0x1) << 31) | (((imm >> 1)  & 0x3ff) << 21) | (((imm >> 11) & 0x1) << 20) | (((imm >> 12) & 0xff) << 12) | ((rd & 0x1f) << 7) | (opcode & 0x7f)
        case _: # Invalid
            return 0x0

