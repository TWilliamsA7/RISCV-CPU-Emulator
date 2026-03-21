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


def gen_valid_address(ctx, access_size=4, max_attempts=20):
    state = ctx.state
    cfg   = ctx.config

    for _ in range(max_attempts):

        base_reg = random.randint(0, 31)
        base_val = state.read_reg(base_reg)

        # Compute valid address window
        min_addr = cfg.memory_start
        max_addr = cfg.memory_end - access_size

        # Convert to offset constraints
        min_offset = min_addr - base_val
        max_offset = max_addr - base_val

        # Clamp to 12-bit signed immediate
        min_offset = max(min_offset, -2048)
        max_offset = min(max_offset, 2047)

        if min_offset > max_offset:
            continue

        offset = random.randint(min_offset, max_offset)

        # Enforce alignment
        aligned_addr = (base_val + offset) & ~(access_size - 1)

        # Recompute offset after alignment
        aligned_offset = aligned_addr - base_val

        # Final validation
        if not (min_addr <= aligned_addr <= max_addr):
            continue  

        if not (-2048 <= aligned_offset <= 2047):
            continue

        return base_reg, aligned_offset

    # Return Invalid
    raise RuntimeError("Failed to generate valid memory address after retries")


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

    # Branch offsets are multiples of 2
    min_offset = max(min_offset, -4096)
    max_offset = min(max_offset, 4094)

    if min_offset > max_offset:
        raise RuntimeError("Failed to generate valid offset")
    else:
        offset = random.randrange(min_offset, max_offset, 2)

    return rs1, rs2, offset


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

    target = (base + imm) & ~1 

    aligned_imm = target - base

    return rs1, aligned_imm
