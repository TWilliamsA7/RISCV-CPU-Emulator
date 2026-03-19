import random

def choose_instruction(instrs, coverage):
    weights = [
        coverage.get_weight(instr.name)
        for instr in instrs
    ]
    return random.choices(instrs, weights=weights)[0]
