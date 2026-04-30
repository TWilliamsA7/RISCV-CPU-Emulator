from testgen.state import GenState

class TestContext:
    def __init__(self, config, coverage):
        self.config = config
        self.coverage = coverage
        self.state = GenState(config)

    def update_state(self, instr, rd, rs1, rs2, imm):
        self.state.update_state(instr, rd, rs1, rs2, imm)
