from testgen.generators.instruction import choose_instruction
from testgen.constraints import *
from testgen.encode import encode

def generate_program(instrs, coverage, length=100):
    program = []

    for _ in range(length):
        instr = choose_instruction(instrs, coverage)

        rd = gen_reg()
        rs1 = gen_reg()
        rs2 = gen_reg()

        imm = None

        match instr[0]:
            case 0x23 | 0x13 | 0x03 | 0x67 if instr.name not in ["SLLI", "SRLI", "SRAI"]:
                imm = gen_imm_i()
            case 0x13:
                imm = gen_shift_imm()
            case 0x63:
                imm = gen_branch_offset()
            case 0x37 | 0x17:
                imm = gen_imm_u()
            case 0x6F:
                imm = gen_jump_offset()

        program.append(encode(instr, rd=rd, rs1=rs1, rs2=rs2, imm=imm))

    return program
