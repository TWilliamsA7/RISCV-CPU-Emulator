# trace.py
from golden_model.instructions.rv32i import INSTRUCTION_SET

def hex32(value):
    return f"{value & 0xFFFFFFFF:08x}"


def reg_str(r):
    return f"x{r}"


def disasm(decoded):
    instr_name = INSTRUCTION_SET[decoded["key"]].__name__
    
    if decoded["type"] == "R":
        return f"{instr_name} {reg_str(decoded["rd"])}, {reg_str(decoded["rs1"])}, {reg_str(decoded["rs2"])}"
    if decoded["type"] == "I":
        return f"{instr_name} {reg_str(decoded["rd"])}, {reg_str(decoded["rs1"])}, {str(decoded["imm"])}"
    if decoded["type"] == "LOAD":
        return f"{instr_name} {reg_str(decoded["rd"])}, {str(decoded["imm"])} ({reg_str(decoded["rs1"])})"
    if decoded["type"] == "STORE":
        return f"{instr_name} {reg_str(decoded["rs2"])}, {str(decoded["imm"])} ({reg_str(decoded["rs1"])})"
    if decoded["type"] == "BRANCH":
        return f"{instr_name} {reg_str(decoded["rs1"])}, {reg_str(decoded["rs2"])}, {str(decoded["imm"])}"
    if decoded["type"] == "JAL":
        return f"{instr_name} {reg_str(decoded["rd"])}, {str(decoded["imm"])}"
    if decoded["type"] == "JALR":
        return f"{instr_name} {reg_str(decoded["rd"])}, {str(decoded["imm"])} ({reg_str(decoded["rs1"])})"
    if decoded["type"] == "LUI":
        return f"{instr_name} {reg_str(decoded["rd"])}, {str(decoded["imm"])}"
    if decoded["type"] == "AUIPC":
        return f"{instr_name} {reg_str(decoded["rd"])}, {str(decoded["imm"])}"
    return "INVALID"


def print_trace(step_result):
    print(
        f"PC={hex32(step_result['pc_before'])}\t"
        f"INST={hex32(step_result['instr'])}\t"
        f"{disasm(step_result["decoded"])}\t"
        f"NPC={hex32(step_result['pc_after'])}"
    )

    if step_result["reg_write"]:
        rd, old, new = step_result["reg_write"]
        print(
            f"\tREG\t{reg_str(rd)}: "
            f"{hex32(old)} -> {hex32(new)}"
        )

    if step_result["mem_write"]:
        addr, old, new, size = step_result["mem_write"]
        print(
            f"\tMEM\t[{hex32(addr)}]: "
            f"{hex32(old)} -> {hex32(new)} ({size}B)"
        )
