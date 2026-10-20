import subprocess
import sys

result = subprocess.run([sys.argv[1]], capture_output=True, text=True, check=True, timeout=10)
assert result.stderr == "", result.stderr
lines = result.stdout.splitlines()
assert len(lines) == 7, result.stdout
assert "[INFO]" in lines[0] and "info message" in lines[0]
assert "[WARN]" in lines[1] and "warning message" in lines[1]
assert "[ERROR]" in lines[2] and "error message" in lines[2]
assert "hidden" not in result.stdout
assert len([line for line in lines if "[DEBUG]" in line and "worker=" in line]) == 4
print("PASS: stdout routing, severity filtering, suppressed arguments, OFF and thread-safe lines")
