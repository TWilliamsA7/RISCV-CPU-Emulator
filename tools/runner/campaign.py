import sys

from coverage.model import CoverageModel, print_coverage
from coverage.collector import collect_from_trace
from failures.save import save_failure
from runner.runner import run_cpp_model, run_python_model
from testgen.generators.program import generate_program
from testgen.isa import INSTRUCTIONS
from testgen.generate import write_binary, remove_binary
from compare.compare import compare
from compare.parser import parse_trace

from config.loader import Config
from config.context import TestContext

def main():
    config = Config()
    coverage = CoverageModel()

    ctx = TestContext(config, coverage)

    run_campaign(ctx)

    print_coverage(coverage)



def run_campaign(ctx):

    for i in range(ctx.config.iterations):

        program = generate_program(ctx)

        write_binary(program, ctx.config.temp_dir + "/test.bin")

        py_trace = run_python_model(ctx)
        cpp_trace = run_cpp_model(ctx)

        match = compare(py_trace, cpp_trace)

        if not match:
            save_failure(ctx, program, py_trace, cpp_trace)
            print(f"❌ Failure at iteration {i}")
            break

        actions = parse_trace(py_trace)

        for step in actions:
            collect_from_trace(actions, ctx.coverage)

        remove_binary(ctx.config.temp_dir + "/test.bin")


if __name__ == "__main__":
    main()
