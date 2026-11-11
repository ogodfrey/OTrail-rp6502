"""Check every dialogue record on the host and talking in the release emulator.

Usage: python tests/talktest.py [--preset debug|release]
Requires gcc and the ROM built with the selected preset.
"""

import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import textwrap

ROOT = Path(__file__).resolve().parent.parent
GCC = r"C:\msys64\ucrt64\bin\gcc.exe" if os.path.exists(r"C:\msys64\ucrt64\bin\gcc.exe") else "gcc"
TIMEOUT_SECONDS = 90


def check_host():
    asset = (ROOT / "src" / "talk.txt").read_text(encoding="utf-8")
    records = {}
    for line in asset.splitlines():
        landmark, turn, speaker, quote = line.split("|", 3)
        assert len(line.encode("utf-8")) < 319, "dialogue exceeds the reader buffer"
        records[int(landmark), int(turn)] = (speaker, quote)
    assert len(records) == 51
    expected = ""
    for landmark in range(17):
        for turn in range(4):
            speaker, quote = records[landmark, turn % 3]
            expected += speaker + ' says:\n"' + textwrap.fill(
                quote, width=76, break_long_words=False, break_on_hyphens=False) + '"\n'

    with tempfile.TemporaryDirectory(prefix="oregon-talk-") as directory:
        work = Path(directory)
        source = (ROOT / "src" / "talk.c").read_text(encoding="utf-8")
        (work / "talk.c").write_text(source.replace("ROM:talk", "talk.txt"), encoding="utf-8")
        (work / "main.c").write_text(
            '#include "talk.h"\n'
            'int main(void) {\n'
            '    int landmark, turn, i;\n'
            '    for (landmark = 0; landmark < TALK_LANDMARKS; landmark++) {\n'
            '        turn = 0;\n'
            '        for (i = 0; i < 4; i++)\n'
            '            if (!talk_to_people(landmark, &turn) || turn != i + 1)\n'
            '                return 1;\n'
            '    }\n'
            '    if (talk_to_people(-1, &turn) || talk_to_people(TALK_LANDMARKS, &turn))\n'
            '        return 2;\n'
            '    return 0;\n'
            '}\n', encoding="utf-8")
        exe = work / "talktest.exe"
        subprocess.run(
            [GCC, "-std=gnu99", "-O1", "-Wall", "-Wextra",
             "-include", str(ROOT / "tests" / "host" / "binary.h"),
             "-I", str(ROOT / "src"), str(work / "talk.c"),
             str(work / "main.c"), "-o", str(exe)], check=True)
        for ending in ("\n", "\r\n", ""):
            lines = asset.splitlines()
            separator = ending or "\n"
            (work / "talk.txt").write_bytes(
                (separator.join(lines) + ending).encode("utf-8"))
            result = subprocess.run(
                [str(exe)], cwd=work, capture_output=True, text=True,
                timeout=10, check=True)
            assert not result.stderr, result.stderr
            assert result.stdout == expected, "dialogue text or speaker rotation changed"
    print("OK: all 51 quotes, speaker rotation, LF/CRLF and final line without newline")


def check_emulator(preset):
    build = ROOT / "build" / "llvm-mos" / preset / "oregon"
    # Leave the store, dismiss the landmark announcement, then talk three times.
    setup = ["1", "1", "Alice", "", "", "", "", "1", "1", "3", "0", ""]
    answers = setup + ["9", "", "9", "", "9", "", "10", "1", "", "3"]
    with tempfile.TemporaryDirectory(prefix="oregon-talk-emu-") as directory:
        with tempfile.TemporaryFile() as source:
            source.write(("\n".join(answers) + "\n").encode("utf-8"))
            source.seek(0)
            result = subprocess.run(
                [str(build / "rp6502-emu.exe"), "--headless", str(build / "oregon.rp6502")],
                cwd=directory, stdin=source, capture_output=True, text=True,
                encoding="utf-8", errors="replace", timeout=TIMEOUT_SECONDS, check=True)
    assert not result.stderr, result.stderr
    text = result.stdout
    assert text.count(" says:\n") == 3, "not all conversations completed"
    speakers = [
        "A trader with spare goods",
        "A traveler from Ohio, Silas Greer",
        "A town resident",
    ]
    position = text.index("9) Talk to people")
    for speaker in speakers:
        position = text.index(speaker + " says:\n", position)
        position = text.index("Press Enter to continue.", position)
        position = text.index("9) Talk to people", position)
    assert "HIGH SCORES" in text[position:], "game did not return from the action menu"
    print("OK: three conversations, pauses, menu returns and clean exit in " + preset)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preset", choices=("debug", "release"), default="release")
    args = parser.parse_args()
    check_host()
    check_emulator(args.preset)


if __name__ == "__main__":
    main()
