import yaml

class Config:
    def __init__(self, path="config/default.yaml"):
        with open(path, "r") as f:
            data = yaml.safe_load(f)

        self.memory_start = int(data["memory"]["start"])
        self.memory_size  = int(data["memory"]["size"])
        self.memory_end   = self.memory_start + self.memory_size

        self.program_length = data["program"]["length"]
        self.max_steps      = data["program"]["max_steps"]

        self.python_model = data["execution"]["python_model"]
        self.cpp_model    = data["execution"]["cpp_model"]

        self.temp_dir     = data["output"]["temp_dir"]
        self.failure_dir  = data["output"]["failure_dir"]