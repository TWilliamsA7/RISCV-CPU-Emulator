import os

def save_failure(program, py_trace, cpp_trace):
    os.makedirs("failures", exist_ok=True)

    with open("failures/program.bin", "wb") as f:
        f.write(program)

    with open("failures/py_trace.txt", "w") as f:
        f.write(str(py_trace))

    with open("failures/cpp_trace.txt", "w") as f:
        f.write(str(cpp_trace))
