from golden_model.cpu import CPU
from golden_model.memory import Memory


def test_add():
    mem = Memory(64)
    cpu = CPU(mem)

    # preload registers
    cpu.regs[1] = 10
    cpu.regs[2] = 20

    # add x3, x1, x2
    mem.write32(0, 0x002081B3)

    cpu.step()

    assert cpu.regs[3] == 30


def test_addi():
    mem = Memory(64)
    cpu = CPU(mem)

    # addi x1, x0, 5
    mem.write32(0, 0x00500093)

    cpu.step()

    assert cpu.regs[1] == 5
    assert cpu.pc == 4


def test_lw_sw():
    mem = Memory(64)
    cpu = CPU(mem)

    cpu.regs[1] = 16       # base
    cpu.regs[2] = 0x12345678

    # sw x2, 0(x1)
    mem.write32(0, 0x0020A023)

    # lw x3, 0(x1)
    mem.write32(4, 0x0000A183)

    cpu.step()
    cpu.step()

    assert cpu.regs[3] == 0x12345678


def test_beq_taken():
    mem = Memory(64)
    cpu = CPU(mem)

    cpu.regs[1] = 5
    cpu.regs[2] = 5

    # beq x1, x2, +8
    mem.write32(0, 0x00208463)

    cpu.step()

    assert cpu.pc == 8


def test_beq_not_taken():
    mem = Memory(64)
    cpu = CPU(mem)

    cpu.regs[1] = 5
    cpu.regs[2] = 6

    mem.write32(0, 0x00208463)

    cpu.step()

    assert cpu.pc == 4


def test_bne_taken():
    mem = Memory(64)
    cpu = CPU(mem)

    cpu.regs[1] = 5
    mem.write32(0, 0x00009463)

    cpu.step()

    assert cpu.pc == 8


def test_bne_not_taken():
    mem = Memory(64)
    cpu = CPU(mem)

    cpu.regs[1] = 0
    mem.write32(0, 0x00009463)

    cpu.step()

    assert cpu.pc == 4


def test_jal():
    mem = Memory(64)
    cpu = CPU(mem)

    # jal x1, +8
    mem.write32(0, 0x008000EF)

    cpu.step()

    assert cpu.regs[1] == 4
    assert cpu.pc == 8


def test_jalr():
    mem = Memory(64)
    cpu = CPU(mem)

    cpu.regs[1] = 20

    # jalr x2, 0(x1)
    mem.write32(0, 0x00008167)

    cpu.step()

    assert cpu.regs[2] == 4
    assert cpu.pc == 20


def test_x0_invariant():
    mem = Memory(64)
    cpu = CPU(mem)

    cpu.regs[0] = 123

    # addi x0, x0, 5
    mem.write32(0, 0x00500013)

    cpu.step()

    assert cpu.regs[0] == 0
