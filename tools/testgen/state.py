from testgen.isa import compute_alu, compute_load_address, compute_upper

class GenState:
    def __init__(self, config):
        self.regs = [0] * 32

        self.mem_start = config.memory_start
        self.mem_end   = config.memory_end

        for i in range(1, 32):
            self.regs[i] = self.mem_start

    def write_reg(self, rd, value):
        if rd != 0:
            self.regs[rd] = value

    def read_reg(self, rs):
        return self.regs[rs]

    def update_state(self, instr, rd, rs1, rs2, imm):
        match instr.value[0]:
            case 0x33:
                self.regs[rd] = compute_alu(instr, self.regs[rs1], self.regs[rs2])
            case 0x13:
                self.regs[rd] = compute_alu(instr, self.regs[rs1], imm)
            case 0x03:
                self.regs[rd] = compute_load_address(self.regs[rs1], imm)
            case 0x37 | 0x17:
                self.regs[rd] = compute_upper(instr, imm)

