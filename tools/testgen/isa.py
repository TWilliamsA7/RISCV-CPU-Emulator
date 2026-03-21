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

INSTRUCTIONS = list(Instr)

INSTRUCTIONS_SIZE_MAP = {
    "LB": 1,
    "LH": 2,
    "LW": 4,
    "SB": 1,
    "SH": 2,
    "SW": 4
}

def compute_alu(instr, a, b):
    if instr in {Instr.ADD, Instr.ADDI}:
        return a + b

    if instr in {Instr.SUB}:
        return a - b

    if instr in {Instr.AND, Instr.ANDI}:
        return a & b

    if instr in {Instr.OR, Instr.ORI}:
        return a | b

    if instr in {Instr.XOR, Instr.XORI}:
        return a ^ b

    if instr in {Instr.SLL, Instr.SLLI}:
        return a << (b & 0x1F)

    if instr in {Instr.SRL, Instr.SRLI}:
        return (a % (1 << 32)) >> (b & 0x1F)

    if instr in {Instr.SRA, Instr.SRAI}:
        return a >> (b & 0x1F)

    if instr in {Instr.SLT, Instr.SLTI}:
        return int(a < b)

    if instr in {Instr.SLTU, Instr.SLTIU}:
        return int((a & 0xFFFFFFFF) < (b & 0xFFFFFFFF))

    return 0

def compute_load_address(base, imm):
    addr = base + imm
    return addr


def compute_upper(instr, imm):
    if instr == Instr.LUI:
        return imm

    if instr == Instr.AUIPC:
        return imm  # ignoring PC for simplicity

    return 0
