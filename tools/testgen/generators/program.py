from testgen.generators.instruction import choose_instruction
from testgen.constraints import *
from testgen.encode import encode
from testgen.isa import INSTRUCTIONS, Instr, INSTRUCTIONS_SIZE_MAP

def generate_program(ctx):
    program = []

    for i in range(ctx.config.program_length):
        instr = choose_instruction(INSTRUCTIONS, ctx.coverage)
        entry = generate_instruction(ctx, instr, i)
        program.append(entry)

    return program

def generate_instruction(ctx, instr, idx):
    state = ctx.state

    rd = gen_reg()
    rs1 = gen_reg()
    rs2 = gen_reg()

    imm = None

    match instr.value[0]:
        case 0x03:  # LOAD
            try: 
                access_size = INSTRUCTIONS_SIZE_MAP[instr.name]
                rs1, imm = gen_valid_address(ctx, access_size)
            except RuntimeError:
                instr = Instr.INVALID

        case 0x23:  # STORE
            try: 
                access_size = INSTRUCTIONS_SIZE_MAP[instr.name]
                rs1, imm = gen_valid_address(ctx, access_size)
            except RuntimeError:
                instr = Instr.INVALID

        case 0x63:  # BRANCH
            try:
                rs1, rs2, imm = gen_branch(ctx, ctx.config.memory_start + (idx * 4))
            except RuntimeError:
                instr = Instr.INVALID

        case 0x6F:  # JAL
            try:
                rs1, imm = gen_jalr(ctx)
            except RuntimeError:
                instr = Instr.INVALID

        case 0x13 if instr.name in ["SLLI", "SRLI", "SRAI"]:
            imm = gen_shift_imm()

        case 0x13 | 0x67:
            imm = gen_imm_i()

        case 0x37 | 0x17:
            imm = gen_imm_u()

    encoded = encode(instr, rd=rd, rs1=rs1, rs2=rs2, imm=imm)

    ctx.update_state(instr, rd, rs1, rs2, imm)

    return encoded