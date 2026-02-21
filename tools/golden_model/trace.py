# trace.py

def hex32(value):
    return f"{value & 0xFFFFFFFF:08x}"


def reg_str(r):
    return f"x{r}"


def print_trace(step_result, disasm_str):
    print(
        f"PC={hex32(step_result['pc_before'])}\t"
        f"INST={hex32(step_result['instr'])}\t"
        f"{disasm_str}\t"
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
