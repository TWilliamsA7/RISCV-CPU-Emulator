# instructions/rv32i.py

def ADD(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    rs2 = d["rs2"]

    old = cpu.regs[rd]
    result = (cpu.regs[rs1] + cpu.regs[rs2]) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def SUB(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    rs2 = d["rs2"]

    old = cpu.regs[rd]
    result = (cpu.regs[rs1] - cpu.regs[rs2]) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def AND(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    rs2 = d["rs2"]

    old = cpu.regs[rd]
    result = (cpu.regs[rs1] & cpu.regs[rs2]) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4

def OR(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    rs2 = d["rs2"]

    old = cpu.regs[rd]
    result = (cpu.regs[rs1] | cpu.regs[rs2]) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def XOR(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    rs2 = d["rs2"]

    old = cpu.regs[rd]
    result = (cpu.regs[rs1] ^ cpu.regs[rs2]) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def SLL(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    rs2 = d["rs2"]

    old = cpu.regs[rd]
    result = (cpu.regs[rs1] << cpu.regs[rs2]) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def SRL(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    rs2 = d["rs2"]

    old = cpu.regs[rd]
    result = ((cpu.regs[rs1] % 0x100000000) >> cpu.regs[rs2]) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def SRA(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    rs2 = d["rs2"]

    old = cpu.regs[rd]
    result = (cpu.regs[rs1] >> cpu.regs[rs2]) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def SLT(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    rs2 = d["rs2"]

    old = cpu.regs[rd]
    result = 1 if (cpu.regs[rs1] < cpu.regs[rs2]) else 0

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def SLTU(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    rs2 = d["rs2"]

    old = cpu.regs[rd]
    result = 1 if (abs(cpu.regs[rs1]) < abs(cpu.regs[rs2])) else 0

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def ADDI(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    old = cpu.regs[rd]
    result = (cpu.regs[rs1] + imm) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def ANDI(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    old = cpu.regs[rd]
    result = (cpu.regs[rs1] & imm) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def ORI(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    old = cpu.regs[rd]
    result = (cpu.regs[rs1] | imm) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def XORI(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    old = cpu.regs[rd]
    result = (cpu.regs[rs1] ^ imm) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def SLTI(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    old = cpu.regs[rd]
    result = 1 if (cpu.regs[rs1] < imm) else 0

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def SLTIU(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    old = cpu.regs[rd]
    result = 1 if (abs(cpu.regs[rs1]) < imm) else 0

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def SLLI(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    old = cpu.regs[rd]
    result = (cpu.regs[rs1] << imm) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def SRLI(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    old = cpu.regs[rd]
    result = ((cpu.regs[rs1] % 0x100000000) >> imm) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def SRAI(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    old = cpu.regs[rd]
    result = (cpu.regs[rs1] >> imm) & 0xFFFFFFFF

    if rd != 0:
        cpu.regs[rd] = result
        reg_write = (rd, old, result)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def LW(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    addr = (cpu.regs[rs1] + imm) & 0xFFFFFFFF
    value = cpu.memory.read32(addr)

    old = cpu.regs[rd]

    if rd != 0:
        cpu.regs[rd] = value
        reg_write = (rd, old, value)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def LH(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    addr = (cpu.regs[rs1] + imm) & 0xFFFFFFFF
    value = cpu.memory.read16(addr)

    old = cpu.regs[rd]

    if rd != 0:
        cpu.regs[rd] = value
        reg_write = (rd, old, value)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def LHU(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    addr = (cpu.regs[rs1] + imm) & 0xFFFFFFFF
    value = abs(cpu.memory.read16(addr))

    old = cpu.regs[rd]

    if rd != 0:
        cpu.regs[rd] = value
        reg_write = (rd, old, value)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def LB(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    addr = (cpu.regs[rs1] + imm) & 0xFFFFFFFF
    value = abs(cpu.memory.read8(addr))

    old = cpu.regs[rd]

    if rd != 0:
        cpu.regs[rd] = value
        reg_write = (rd, old, value)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def LBU(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    addr = (cpu.regs[rs1] + imm) & 0xFFFFFFFF
    value = abs(cpu.memory.read8(addr))

    old = cpu.regs[rd]

    if rd != 0:
        cpu.regs[rd] = value
        reg_write = (rd, old, value)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def SW(cpu, d):
    rs1 = d["rs1"]
    rs2 = d["rs2"]
    imm = d["imm"]

    addr = (cpu.regs[rs1] + imm) & 0xFFFFFFFF

    old = cpu.memory.read32(addr)
    new = cpu.regs[rs2]

    cpu.memory.write32(addr, new)

    mem_write = (addr, old, new, 4)

    return None, mem_write, cpu.pc + 4


def SH(cpu, d):
    rs1 = d["rs1"]
    rs2 = d["rs2"]
    imm = d["imm"]

    addr = (cpu.regs[rs1] + imm) & 0xFFFFFFFF

    old = cpu.memory.read16(addr)
    new = cpu.regs[rs2]

    cpu.memory.write16(addr, new)

    mem_write = (addr, old, new, 4)

    return None, mem_write, cpu.pc + 4


def SB(cpu, d):
    rs1 = d["rs1"]
    rs2 = d["rs2"]
    imm = d["imm"]

    addr = (cpu.regs[rs1] + imm) & 0xFFFFFFFF

    old = cpu.memory.read8(addr)
    new = cpu.regs[rs2]

    cpu.memory.write8(addr, new)

    mem_write = (addr, old, new, 4)

    return None, mem_write, cpu.pc + 4


def BEQ(cpu, d):
    rs1 = d["rs1"]
    rs2 = d["rs2"]
    imm = d["imm"]

    val1 = cpu.regs[rs1]
    val2 = cpu.regs[rs2]

    if val1 == val2:
        next_pc = (cpu.pc + imm) & 0xFFFFFFFF
    else:
        next_pc = cpu.pc + 4

    return None, None, next_pc


def BNE(cpu, d):
    rs1 = d["rs1"]
    rs2 = d["rs2"]
    imm = d["imm"]

    val1 = cpu.regs[rs1]
    val2 = cpu.regs[rs2]

    if val1 != val2:
        next_pc = (cpu.pc + imm) & 0xFFFFFFFF
    else:
        next_pc = cpu.pc + 4

    return None, None, next_pc


def BLT(cpu, d):
    rs1 = d["rs1"]
    rs2 = d["rs2"]
    imm = d["imm"]

    val1 = cpu.regs[rs1]
    val2 = cpu.regs[rs2]

    if val1 < val2:
        next_pc = (cpu.pc + imm) & 0xFFFFFFFF
    else:
        next_pc = cpu.pc + 4

    return None, None, next_pc


def BGE(cpu, d):
    rs1 = d["rs1"]
    rs2 = d["rs2"]
    imm = d["imm"]

    val1 = cpu.regs[rs1]
    val2 = cpu.regs[rs2]

    if val1 >= val2:
        next_pc = (cpu.pc + imm) & 0xFFFFFFFF
    else:
        next_pc = cpu.pc + 4

    return None, None, next_pc


def BLTU(cpu, d):
    rs1 = d["rs1"]
    rs2 = d["rs2"]
    imm = d["imm"]

    val1 = abs(cpu.regs[rs1])
    val2 = abs(cpu.regs[rs2])

    if val1 < val2:
        next_pc = (cpu.pc + imm) & 0xFFFFFFFF
    else:
        next_pc = cpu.pc + 4

    return None, None, next_pc


def BGEU(cpu, d):
    rs1 = d["rs1"]
    rs2 = d["rs2"]
    imm = d["imm"]

    val1 = abs(cpu.regs[rs1])
    val2 = abs(cpu.regs[rs2])

    if val1 >= val2:
        next_pc = (cpu.pc + imm) & 0xFFFFFFFF
    else:
        next_pc = cpu.pc + 4

    return None, None, next_pc


def JAL(cpu, d):
    rd = d["rd"]
    imm = d["imm"]

    return_addr = cpu.pc + 4
    target = (cpu.pc + imm) & 0xFFFFFFFF

    old = cpu.regs[rd]

    if rd != 0:
        cpu.regs[rd] = return_addr
        reg_write = (rd, old, return_addr)
    else:
        reg_write = None

    return reg_write, None, target


def JALR(cpu, d):
    rd = d["rd"]
    rs1 = d["rs1"]
    imm = d["imm"]

    return_addr = (cpu.pc + 4) & 0xFFFFFFFF

    target = (cpu.regs[rs1] + imm) & 0xFFFFFFFF
    target &= ~1

    old = cpu.regs[rd]

    if rd != 0:
        cpu.regs[rd] = return_addr
        reg_write = (rd, old, return_addr)
    else:
        reg_write = None

    return reg_write, None, target


def LUI(cpu, d):
    rd = d["rd"]
    imm = d["imm"]

    value = imm & 0xFFFFFFFF

    old = cpu.regs[rd]

    if rd != 0:
        cpu.regs[rd] = value
        reg_write = (rd, old, value)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def AUIPC(cpu, d):
    rd = d["rd"]
    imm = d["imm"]

    value = (cpu.pc + imm) & 0xFFFFFFFF

    old = cpu.regs[rd]

    if rd != 0:
        cpu.regs[rd] = value
        reg_write = (rd, old, value)
    else:
        reg_write = None

    return reg_write, None, cpu.pc + 4


def INVALID(cpu, d):
    return None, None, cpu.pc + 4


INSTRUCTION_SET = {
    (0x33, 0x0, 0x00): ADD,
    (0x33, 0x0, 0x20): SUB,
    (0x33, 0x1, 0x0): SLL,   
    (0x33, 0x2, 0x0): SLT, 
    (0x33, 0x3, 0x0): SLTU, 
    (0x33, 0x4, 0x0): XOR, 
    (0x33, 0x5, 0x0): SRL, 
    (0x33, 0x5, 0x20): SRA, 
    (0x33, 0x6, 0x0): OR,
    (0x33, 0x7, 0x0): AND,
    (0x13, 0x0, 0x00): ADDI,
    (0x13, 0x1, 0x0): SLLI,   
    (0x13, 0x2, 0x0): SLTI, 
    (0x13, 0x3, 0x0): SLTIU, 
    (0x13, 0x4, 0x0): XORI, 
    (0x13, 0x5, 0x0): SRLI, 
    (0x13, 0x5, 0x20): SRAI, 
    (0x13, 0x6, 0x0): ORI,
    (0x13, 0x7, 0x0): ANDI,
    (0x03, 0x0, None): LB,
    (0x03, 0x1, None): LH,
    (0x03, 0x2, None): LW,
    (0x03, 0x4, None): LBU,
    (0x03, 0x5, None): LHU,
    (0x23, 0x0, None): SB,
    (0x23, 0x1, None): SH,
    (0x23, 0x2, None): SW,
    (0x63, 0x0, None): BEQ,
    (0x63, 0x1, None): BNE,
    (0x63, 0x4, None): BLT,
    (0x63, 0x5, None): BGE,
    (0x63, 0x6, None): BLTU,
    (0x63, 0x7, None): BGEU,
    (0x6F, None, None): JAL,
    (0x67, None, None): JALR,
    (0x37, None, None): LUI,
    (0x17, None, None): AUIPC,
    (None, None, None): INVALID,
}
