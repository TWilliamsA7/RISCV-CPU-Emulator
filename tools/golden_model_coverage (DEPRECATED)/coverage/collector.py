def collect_from_trace(trace, coverage):
    decoded = trace["decoded"]
    instr = decoded["name"]

    coverage.record_instruction(instr)

    # Branch behavior
    if decoded["type"] == "BRANCH":
        taken = trace["pc_after"] != trace["pc_before"] + 4
        coverage.record_branch(instr, taken)

    # Memory behavior
    if trace["mem_write"]:
        coverage.record_store(trace["mem_write"]["addr"])

    if decoded["type"] == "I" and "load" in instr.lower():
        coverage.record_load(trace["mem_write"]["addr"])
