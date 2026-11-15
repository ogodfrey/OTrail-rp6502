"""Host regressions plus real keyboard hunting in the built Release emulator."""

import os
import subprocess
import tempfile

import playthrough as pt


def main():
    pt.build()
    source = os.path.join(pt.HOST, "src")
    tests = os.path.join(pt.ROOT, "tests", "host")
    exe = os.path.join(pt.HOST, "gameplaytest.exe")
    subprocess.run(
        [pt.GCC, "-std=gnu99", "-O1", "-Wno-format-security",
         "-include", os.path.join(tests, "binary.h"),
         "-I", tests, "-I", source, os.path.join(tests, "gameplay.c")]
        + [os.path.join(source, name + ".c")
           for name in ("scene", "sound", "talk", "ui", "msg")]
        + ["-o", exe], check=True)
    result = subprocess.run(
        [exe], cwd=pt.HOST, input="\n" * 7, capture_output=True,
        text=True, timeout=30)
    assert not result.stderr, result.stderr
    result.check_returncode()
    for name in ("Amos", "Clara"):
        notice = "\x1b[92m" + name + " has recovered and is healthy again.\x1b[39m"
        assert result.stdout.count(notice) == 1, result.stdout
    print(result.stdout.splitlines()[-1])
    build = os.path.join(pt.ROOT, "build", "llvm-mos", "release", "oregon")
    with tempfile.TemporaryDirectory(prefix="oregon-hunt-") as directory:
        subprocess.run(
            [os.path.join(build, "rp6502-emu.exe"), os.path.join(build, "oregon.rp6502"),
             "--script", os.path.join(pt.ROOT, "tests", "hunt.script"), "--memfill", "0"],
            cwd=directory, capture_output=True, text=True, timeout=90, check=True)
    print("OK: emulator HID aiming, firing, Escape, menu return and clean exit")


if __name__ == "__main__":
    main()
