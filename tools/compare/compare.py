from compare.parser import parse_trace 

py = parse_trace("trace_py.txt")
cpp = parse_trace("trace_cpp.txt")

for i, (a, b) in enumerate(zip(py, cpp)):
    if a != b:
        print(f"\n❌ Mismatch at step {i}")
        print("PY :", a)
        print("CPP:", b)
        break
else:
    print("✅ PASS")
