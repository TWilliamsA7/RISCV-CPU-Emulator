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


def sign_extend(value, bits):
    sign_bit = 1 << (bits - 1)
    return (value & (sign_bit - 1)) - (value & sign_bit)


def gen_valid_address(ctx, access_size=4, max_attempts=20, unsigned=False):
    state = ctx.state
    cfg   = ctx.config

    for _ in range(max_attempts):

        base_reg = 0 # Always pick zero to ensure we stay in range
        base_val = state.read_reg(base_reg)

        min_addr = cfg.memory_start
        max_addr = cfg.memory_end - access_size

        min_offset = min_addr - base_val
        max_offset = max_addr - base_val

        min_offset = max(min_offset, 0) # Restrict to only positives
        max_offset = min(max_offset, 2047)

        if unsigned and min_offset < 0:
            min_offset = 0

        if min_offset > max_offset:
            continue

        offset = random.randint(min_offset, max_offset)

        imm_encoded = offset & 0xFFF
        imm = sign_extend(imm_encoded, 12)

        addr = base_val + imm

        if access_size > 1:
            addr &= ~(access_size - 1)
            imm = addr - base_val

        # Final validation

        if not (min_addr <= addr <= max_addr):
            continue

        if not (-2048 <= imm <= 2047):
            continue

        return base_reg, 8

    raise RuntimeError("Failed to generate valid address")


def gen_branch(ctx, pc):
    cfg = ctx.config

    rs1 = random.randint(0, 31)
    rs2 = random.randint(0, 31)

    # Constrain target PC
    program_bytes = cfg.program_length * 4

    min_target = 0
    max_target = program_bytes - 4

    min_offset = min_target - pc
    max_offset = max_target - pc

    min_offset = max(min_offset, -4096)
    max_offset = min(max_offset, 4094)

    if min_offset > max_offset:
        raise RuntimeError("Failed to generate valid offset")
    else:
        offset = random.randrange(min_offset, max_offset, 4)

    return rs1, rs2, offset


def gen_jal(ctx, pc):
    cfg = ctx.config

    rd = random.randint(0, 31)

    prog_start = cfg.memory_start
    prog_end = prog_start + (cfg.program_length * 4)

    max_jal_offset = 1048574
    min_jal_offset = -1048576

    min_target = max(prog_start, pc + min_jal_offset)
    max_target = min(prog_end - 4, pc + max_jal_offset)

    if min_target > max_target:
        raise RuntimeError(f"PC {pc} is out of range for any valid program jump")

    aligned_min = (min_target + 3) & ~3
    aligned_max = max_target & ~3
    
    if aligned_min > aligned_max:
         raise RuntimeError("No 4-byte aligned target available in range")

    target = random.randrange(aligned_min, aligned_max + 4, 4)

    offset = target - pc

    return rd, offset

def gen_jalr(ctx):
    state = ctx.state
    cfg = ctx.config

    rs1 = random.randint(0, 31)
    base = state.read_reg(rs1)

    # Constrain target to program space
    program_bytes = cfg.program_length * 4

    min_target = 0
    max_target = program_bytes - 4

    # Compute valid imm range
    min_imm = min_target - base
    max_imm = max_target - base

    min_imm = max(min_imm, -2048)
    max_imm = min(max_imm, 2047)

    if min_imm > max_imm:
        raise RuntimeError("Failed to generate valid imm")

    imm = random.randint(min_imm, max_imm)

    target = (base + imm) & ~3

    aligned_imm = target - base

    return rs1, aligned_imm
