from testgen.state import GenState

class TestContext:
    def __init__(self, config, coverage):
        self.config = config
        self.coverage = coverage
        self.state = GenState(config)
