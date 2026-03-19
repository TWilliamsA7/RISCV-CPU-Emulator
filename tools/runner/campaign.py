from coverage.model import CoverageModel
from coverage.collector import collect_from_trace
from failures.save import save_failure
from runner.runner import run_cpp_model, run_python_model
from testgen.generators.program import generate_program
from testgen.isa import INSTRUCTIONS
from testgen.generate import write_binary, remove_binary
from compare.compare import compare

def run_campaign(iterations=1000):
    coverage = CoverageModel()

    for i in range(iterations):
        program = generate_program(instrs=INSTRUCTIONS, coverage=coverage, length=100)

        # Write binary
        write_binary(program)

        py_trace = run_python_model()
        cpp_trace = run_cpp_model()

        match, step = compare(py_trace, cpp_trace)

        if not match:
            save_failure(program, py_trace, cpp_trace)
            print(f"❌ Failure at iteration {i}, step {step}")
            break

        for trace in py_trace:
            collect_from_trace(trace, coverage)

        remove_binary()

    return coverage
