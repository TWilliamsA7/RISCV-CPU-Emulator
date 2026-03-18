import re

def parse_trace(file_path):
    entries = []
    current = {}

    with open(file_path) as f:
        for line in f:
            line = line.strip()

            if line.startswith("PC="):
                if current:
                    entries.append(current)
                current = {"reg": [], "mem": []}

                m = re.match(r"PC=(0x[0-9a-f]+)\s+INST=(0x[0-9a-f]+).*NPC=(0x[0-9a-f]+)", line)
                current["pc"] = m.group(1)
                current["instr"] = m.group(2)
                current["npc"] = m.group(3)

            elif line.startswith("REG"):
                current["reg"].append(line)

            elif line.startswith("MEM"):
                current["mem"].append(line)

    if current:
        entries.append(current)

    return entries
