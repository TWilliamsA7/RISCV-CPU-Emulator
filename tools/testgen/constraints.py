import random

def gen_reg():
    return random.randint(0, 31)

def gen_imm_i():
    return random.randint(-2048, 2047)

def gen_imm_u():
    return random.randint(0, 0xFFFFF) << 12

def gen_shift_imm():
    return random.randint(0, 31)

def gen_branch_offset():
    return random.randrange(-4096, 4096, 2)

def gen_jump_offset():
    return random.randrange(-2**20, 2**20, 2)

def gen_aligned_addr(base, offset):
    addr = base + offset
    return addr & ~0x3
