from collections import defaultdict

class CoverageModel:
    def __init__(self):
        self.instr_count = defaultdict(int)
        self.branch_taken = defaultdict(int)
        self.branch_not_taken = defaultdict(int)
        self.load_addresses = set()
        self.store_addresses = set()

    def record_instruction(self, instr_name):
        self.instr_count[instr_name] += 1

    def record_branch(self, instr_name, taken):
        if taken:
            self.branch_taken[instr_name] += 1
        else:
            self.branch_not_taken[instr_name] += 1

    def record_load(self, addr):
        self.load_addresses.add(addr)

    def record_store(self, addr):
        self.store_addresses.add(addr)

    def get_weight(self, instr_name):
        return 1 / (1 + self.instr_count[instr_name])
