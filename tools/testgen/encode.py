from testgen.isa import Instr

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

