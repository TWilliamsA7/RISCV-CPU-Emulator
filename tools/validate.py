from pathlib import Path
import subprocess
import os
from sys import exit
from typing import Tuple, List

# Configuration
EMULATOR_PATH = "./build/src/riscv_emulator"
TEST_DIR = "validation"
TIMEOUT_SEC = 5 

def get_flags(test_name: str) -> List[str]:
    flags = []

    if "rv32ui" in test_name:
        flags.extend(["-b", "-m"])
    elif "rv32uc" in test_name:
        flags.extend(["-b", "-m", "-c"])
    elif "rv32um" in test_name:
        flags.extend(["-b", "-m"])
    elif "rv32mi" in test_name:
        flags.extend(["-m"])
    elif "rv32si" in test_name:
        flags.extend(["-m", "-c"])

    return flags


def is_success(output: str) -> bool:
    return ("Program exit via ECALL with code: 0" in output or
        "Program exit via ECALL with code: 1000" in output or
        "PASS: SUCCESSFUL WRITE TO HOST" in output)


def run_test(test_path: str) -> Tuple[bool, str]:
    """Runs a single test and returns (Success, Output)."""
    try:
        em_path = Path.cwd() / (EMULATOR_PATH)
        cmd = [em_path, *get_flags(os.path.basename(test_path)), test_path]
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=TIMEOUT_SEC
        )
        
        success = is_success(result.stdout)
        return success, result.stdout
        
    except subprocess.TimeoutExpired:
        return False, "TIMEOUT"
    except Exception as e:
        return False, str(e)


def main():
    path = Path.cwd() / ("tools") / (TEST_DIR)
    tests = [t for t in path.rglob('*') if t.is_file()]
    passed = 0

    if len(tests) == 0:
        print("0 Tests Found! Ensure that this script is executed from the root of the project!")
        exit(1)
    
    print(f"--- Running {len(tests)} Validation Tests ---")
    
    for test in sorted(tests):
        test_name = os.path.basename(test)
        is_ok, output = run_test(test)
        
        if is_ok:
            print(f"[ PASS ] {test_name}")
            passed += 1
        else:
            print(f"[ FAIL ] {test_name}")
            if "TIMEOUT" in output:
                print("\tReason: Test timed out (potential infinite trap loop)")
            else:
                # Print last few lines of output for context
                print("\tReason: Exit code mismatch or crash")
                
    print(f"\nSummary: {passed}/{len(tests)} tests passed.")

if __name__ == "__main__":
    main()