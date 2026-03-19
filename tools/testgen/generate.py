import os
from argparse import ArgumentParser
from testgen.encode import Instr, encode


def main():

    parser = ArgumentParser(description="Generate RISC-V RV32I Program Binary")
    parser.add_argument("filename", help="Specify path for created binary")

    args = parser.parse_args()

    try:
        program = generate_program()

        with open(args.filename, "wb") as f:
            for instr in program:
                f.write(instr.to_bytes(4, "little"))
    except:
        print(f"Could not generate program binary at {args.filename}")
    else:
        print(f"Generated binary at {args.filename}")


def write_binary(program: list, filename="temp/test.bin"):
    try:
        program = generate_program()

        with open(filename, "wb") as f:
            for instr in program:
                f.write(instr.to_bytes(4, "little"))
    except:
        print(f"Could not generate program binary at {filename}")


def remove_binary(filename="temp/test.bin"):
    try:
        os.remove(filename)
    except:
        print(f"Could not remove file '{filename}'")


def generate_program():
    program = []
    
    program.append(encode(Instr.ADDI, rd=1, rs1=0, imm=5))
    program.append(encode(Instr.ADDI, rd=2, rs1=1, imm=10))
    program.append(encode(Instr.ADD, rd=3, rs1=1, rs2=2))

    return program

if __name__ == "__main__":
    main()