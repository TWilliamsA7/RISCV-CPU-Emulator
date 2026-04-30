import os
from argparse import ArgumentParser
from testgen.encode import Instr, encode
from testgen.generators.program import generate_program

# TODO: Use Context within this file!

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