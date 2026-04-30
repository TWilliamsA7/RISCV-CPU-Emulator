from compare.parser import parse_trace 

def compare(py: str, cpp: str):
    py_parse = parse_trace(py)
    cpp_parse = parse_trace(cpp)

    for i, (a, b) in enumerate(zip(py_parse, cpp_parse)):
        if a != b:
            print(f"\n❌ Mismatch at step {i}")
            print("PY :", a)
            print("CPP:", b)
            return False, i
    return True
