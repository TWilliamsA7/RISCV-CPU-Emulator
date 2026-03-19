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

    for _ in range(1024):
        result = cpu.step()
        print_trace(result)


if __name__ == "__main__":
    main()
