from pathlib import Path
import subprocess
import os
import json
import tempfile
from sys import exit
from typing import Tuple, List

# Configuration
EMULATOR_PATH = "./build/src/riscv_emulator"
TEST_DIR = "validation"
TIMEOUT_SEC = 5


def get_extensions(test_name: str) -> List[str]:
    extensions = []

    if "rv32uc" in test_name:
        extensions.append("c")

    if "rv32um" in test_name:
        extensions.append("m")

    if "rv32ui" in test_name:
        extensions.append("m")

    if "rv32mi" in test_name:
        extensions.append("m")

    if "rv32si" in test_name:
        extensions.extend(["m", "c"])

    if "rv32ua" in test_name:
        extensions.extend(["m", "c", "a"])

    return extensions


def is_success(output: str) -> bool:
    return (
        "Program exit via ECALL with code: 0" in output
        or "Program exit via ECALL with code: 1000" in output
        or "PASS: SUCCESSFUL WRITE TO HOST" in output
    )


def run_test(test_path: Path) -> Tuple[bool, str]:
    """Runs a single test and returns (Success, Output)."""

    try:
        em_path = Path.cwd() / EMULATOR_PATH
        test_name = test_path.name

        profile = {
            "name": test_name,
            "platform": "bare-metal",
            "extensions": get_extensions(test_name),
            "elf-path": str(test_path.resolve()),
            "verbose": False,
        }

        # Automatically cleaned up after use
        with tempfile.TemporaryDirectory() as temp_dir:
            json_path = Path(temp_dir) / "profile.json"

            with open(json_path, "w", encoding="utf-8") as f:
                json.dump(profile, f, indent=2)

            cmd = [str(em_path), str(json_path)]

            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=TIMEOUT_SEC,
            )

        success = is_success(result.stdout)
        return success, result.stdout

    except subprocess.TimeoutExpired:
        return False, "TIMEOUT"

    except Exception as e:
        return False, str(e)


def main():
    path = Path.cwd() / "tools" / TEST_DIR
    tests = [t for t in path.rglob("*") if t.is_file()]
    passed = 0

    if len(tests) == 0:
        print(
            "0 Tests Found! Ensure that this script is executed "
            "from the root of the project!"
        )
        exit(1)

    print(f"--- Running {len(tests)} Validation Tests ---")

    for test in sorted(tests):
        test_name = test.name
        is_ok, output = run_test(test)

        if is_ok:
            print(f"[ PASS ] {test_name}")
            passed += 1
        else:
            print(f"[ FAIL ] {test_name}")

            if output == "TIMEOUT":
                print(
                    "\tReason: Test timed out "
                    "(potential infinite trap loop)"
                )
            else:
                print("\tReason: Exit code mismatch or crash")

    print(f"\nSummary: {passed}/{len(tests)} tests passed.")


if __name__ == "__main__":
    main()