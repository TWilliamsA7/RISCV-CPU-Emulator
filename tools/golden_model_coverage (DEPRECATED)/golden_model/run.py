# run.py

import sys

from golden_model.cpu import CPU
from golden_model.memory import Memory
from golden_model.trace import print_trace

def main():

    if len(sys.argv) < 2:
        print("Usage: python run.py [Path to program binary]")
        exit(1)

    mem = Memory(4096)
    cpu = CPU(mem)

    mem.loadBinary(sys.argv[1])

    invalid_instructions = []

    for i in range(1024):
        try:
            result = cpu.step()
            print_trace(result)
        except Exception as e:
            instr = mem.read32(cpu.pc)
            invalid_instructions.append(f"{instr:08x}")
            print(f"Exeception: {e} occurred at step {i}")
            print(f"Encountered instruction was {instr:08x}")
            cpu.pc += 4




if __name__ == "__main__":
    main()
