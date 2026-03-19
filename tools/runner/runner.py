import subprocess


def run_python_model(model_path="golden_model/run.py", binary_path="temp/test.bin"):
    try:
        result = subprocess.run(
            ['python', model_path, binary_path],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout
    except subprocess.CalledProcessError as e:
        print(f"Script execution failed with return code {e.returncode}")
        print("Stderr:", e.stderr)
        return None
    except FileNotFoundError:
        print(f"Error: The file '{model_path}' does not exist or the 'python' command was not found.")
        return None

def run_cpp_model(model_path="/", binary_path="temp/test.bin"):
    try:
        result = subprocess.run(
            [model_path, binary_path],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout
    except subprocess.CalledProcessError as e:
        print(f"Script execution failed with return code {e.returncode}")
        print("Stderr:", e.stderr)
        return None
    except FileNotFoundError:
        print(f"Error: The file './emulator' does not exist")
        return None