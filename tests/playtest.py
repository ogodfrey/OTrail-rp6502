"""Smoke test for the Oregon Trail ROM.

Runs the Debug ROM in the headless emulator with scripted keyboard input: the
setup, one store purchase, then turns of travel. The game waits for input the
script does not supply, so the test stops the emulator after a timeout. It
passes when the game reaches the trail, travels at least one week, and writes
nothing to stderr.

Usage: python tests/playtest.py
"""

import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, "build", "llvm-mos", "debug", "oregon")
EMU = os.path.join(BUILD, "rp6502-emu.exe")
ROM = os.path.join(BUILD, "oregon.rp6502")
TIMEOUT_SECONDS = 60
TURNS = 30

# Title: new journey. Profession 1, leader name, four default companions,
# departure March, then Matt's store: 3 yokes of oxen, then leave.
SETUP = ["1", "1", "Alice", "", "", "", "", "1", "1", "3", "0"]


def main():
    script = SETUP + ["1", ""] * TURNS
    with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False, encoding="utf-8") as f:
        f.write("\n".join(script) + "\n")
        path = f.name
    try:
        with open(path, "rb") as stdin:
            try:
                result = subprocess.run(
                    [EMU, "--headless", ROM], stdin=stdin, capture_output=True,
                    text=True, encoding="utf-8", errors="replace", timeout=TIMEOUT_SECONDS)
                text = result.stdout
                errors = result.stderr.strip()
            except subprocess.TimeoutExpired as expired:
                text = expired.stdout or ""
                errors = (expired.stderr or "").strip()
    finally:
        os.remove(path)

    weeks = text.count("Miles: ")
    print("Weeks shown on the trail: %d" % weeks)
    if errors:
        print("stderr: " + errors)
        sys.exit("the emulator reported an error")
    if weeks < 1:
        sys.exit("the game did not reach the trail")
    print("OK")


if __name__ == "__main__":
    main()
