# run.py

from memory import Memory
from cpu import CPU
from trace import print_trace


def main():
    mem = Memory(1024)
    cpu = CPU(mem)

    for _ in range(10):
        result = cpu.step()
        print_trace(result, "TODO_DISASM")


if __name__ == "__main__":
    main()
