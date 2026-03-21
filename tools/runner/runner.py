import subprocess


def run_python_model(ctx):
    try:
        result = subprocess.run(
            ['python', '-m', ctx.config.python_model, f"{ctx.config.temp_dir}/test.bin"],
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
        print(f"Error: The file '{ctx.config.python_model}' does not exist or the 'python' command was not found.")
        return None


def run_cpp_model(ctx):
    try:
        result = subprocess.run(
            [ctx.config.cpp_model, f"{ctx.config.temp_dir}/test.bin"],
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
        print(f"Error: The file '{ctx.config.cpp_model}' does not exist")
        return None