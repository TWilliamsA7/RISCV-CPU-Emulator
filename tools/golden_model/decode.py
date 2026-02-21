# decode.py

def sign_extend(value, bits):
    sign_bit = 1 << (bits - 1)
    return (value & (sign_bit - 1)) - (value & sign_bit)


def decode(instr):
    opcode = instr & 0x7F
    rd = (instr >> 7) & 0x1F
    funct3 = (instr >> 12) & 0x7
    rs1 = (instr >> 15) & 0x1F
    rs2 = (instr >> 20) & 0x1F
    funct7 = (instr >> 25) & 0x7F

    decoded = {
        "instr": instr,
        "opcode": opcode,
        "rd": rd,
        "rs1": rs1,
        "rs2": rs2,
        "funct3": funct3,
        "funct7": funct7,
        "imm": None,
        "type": None,
        "key": None
    }

    # R-type
    if opcode == 0x33:
        decoded["type"] = "R"
        decoded["key"] = (opcode, funct3, funct7)

    # I-type arithmetic
    elif opcode == 0x13:
        imm = sign_extend((instr >> 20) & 0xFFF, 12)
        decoded["imm"] = imm
        decoded["type"] = "I"
        decoded["key"] = (opcode, funct3, funct7)

    # Load
    elif opcode == 0x03:
        imm = sign_extend((instr >> 20) & 0xFFF, 12)
        decoded["imm"] = imm
        decoded["type"] = "LOAD"
        decoded["key"] = (opcode, funct3, None)

    # Store
    elif opcode == 0x23:
        imm = ((instr >> 7) & 0x1F) | (((instr >> 25) & 0x7F) << 5)
        decoded["imm"] = sign_extend(imm, 12)
        decoded["type"] = "STORE"
        decoded["key"] = (opcode, funct3, None)

    # Branch
    elif opcode == 0x63:
        imm = (
            ((instr >> 8) & 0xF) << 1 |
            ((instr >> 25) & 0x3F) << 5 |
            ((instr >> 7) & 0x1) << 11 |
            ((instr >> 31) & 0x1) << 12
        )
        decoded["imm"] = sign_extend(imm, 13)
        decoded["type"] = "BRANCH"
        decoded["key"] = (opcode, funct3, None)

    # JAL
    elif opcode == 0x6F:
        imm = (
            ((instr >> 21) & 0x3FF) << 1 |
            ((instr >> 20) & 0x1) << 11 |
            ((instr >> 12) & 0xFF) << 12 |
            ((instr >> 31) & 0x1) << 20
        )
        decoded["imm"] = sign_extend(imm, 21)
        decoded["type"] = "JAL"
        decoded["key"] = (opcode, None, None)

    elif opcode == 0x67:
        imm = sign_extend((instr >> 20) & 0xFFF, 12)
        decoded["imm"] = imm
        decoded["type"] = "JALR"
        decoded["key"] = (opcode, None, None)

    elif opcode == 0x37:
        imm = instr & 0xFFFFF000
        decoded["imm"] = imm
        decoded["type"] = "LUI"
        decoded["key"] = (opcode, None, None)

    elif opcode == 0x17:
        imm = instr & 0xFFFFF000
        decoded["imm"] = imm
        decoded["type"] = "AUIPC"
        decoded["key"] = (opcode, None, None)

    else:
        decoded["type"] = "INVALID"
        decoded["key"] = (None, None, None)

    return decoded
