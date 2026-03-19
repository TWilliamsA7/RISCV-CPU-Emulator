import os
from argparse import ArgumentParser
from testgen.encode import Instr, encode
from testgen.generators.program import generate_program


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
        os.makedirs("temp", exist_ok=True)

        with open(filename, "wb") as f:
            for instr in program:
                f.write(instr.to_bytes(4, "little"))
    except Exception as e:
        print(f"An exception occurred: {type(e).__name__} - {e}")
        print(f"Could not generate program binary at {filename}")


def remove_binary(filename="temp/test.bin"):
    try:
        os.remove(filename)
    except:
        print(f"Could not remove file '{filename}'")

if __name__ == "__main__":
    main()