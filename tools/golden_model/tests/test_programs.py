from golden_model.cpu import CPU
from golden_model.memory import Memory


def test_small_program():
    mem = Memory(128)
    cpu = CPU(mem)

    # Program:
    # addi x1, x0, 5
    # addi x2, x0, 10
    # add x3, x1, x2

    mem.write32(0, 0x00500093)
    mem.write32(4, 0x00A00113)
    mem.write32(8, 0x002081B3)

    cpu.step()
    cpu.step()
    cpu.step()

    assert cpu.regs[3] == 15


def test_loop():
    mem = Memory(128)
    cpu = CPU(mem)

    # x1 = 3
    # loop: addi x1, x1, -1
    #       bne x1, x0, loop

    mem.write32(0, 0x00300093)  # addi x1, x0, 3
    mem.write32(4, 0xFFF08093)  # addi x1, x1, -1
    mem.write32(8, 0xfE009EE3)  # bne x1, x0, -4

    for _ in range(10):
        cpu.step()

    assert cpu.regs[1] == 0
