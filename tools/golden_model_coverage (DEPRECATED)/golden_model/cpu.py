# cpu.py

from golden_model.decode import decode
from golden_model.instructions.rv32i import INSTRUCTION_SET

class CPU:
    def __init__(self, memory):
        self.pc = 0
        self.regs = [0] * 32
        self.memory = memory
        self.dispatch = INSTRUCTION_SET

    def step(self):
        pc_before = self.pc
        instr = self.memory.read32(self.pc)
        decoded = decode(instr)

        handler = self.dispatch.get(decoded["key"])

        if handler is None:
            raise Exception(f"Illegal instruction {instr:08x}")

        reg_write, mem_write, next_pc = handler(self, decoded)

        self.pc = next_pc
        self.regs[0] = 0

        return {
            "pc_before": pc_before,
            "instr": instr,
            "pc_after": self.pc,
            "reg_write": reg_write,
            "mem_write": mem_write,
            "decoded": decoded
        }

    
