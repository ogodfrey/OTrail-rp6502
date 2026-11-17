"""Host regressions plus real keyboard hunting in the built Release emulator."""

import os
import struct
import subprocess
import tempfile
import wave

import playthrough as pt


def check_wagon_sound(build):
    with open(os.path.join(pt.ROOT, "src", "wagon.pcm"), "rb") as source:
        samples = source.read()
    tail = samples[((len(samples) - 1) // 512) * 512:]
    with tempfile.TemporaryDirectory(prefix="oregon-wagon-") as directory:
        commands = [
            "seed 42",
            'wait "Choose: " 600',
            r'type "1\r1\rAlice\r\r\r\r\r1\r1\r3\r2\r600\r3\r5\r4\r5\r0\r\r" 600',
            'wait "9) Talk to people" 600',
        ]
        for i in range(2):
            if i:
                commands.append('wait "M for the menu: " 600')
            if not i:
                commands.append("key 1")
            commands += ["key enter", f'wav "wagon-{i}.wav" 180']
            for offset in range(0, len(tail), 32):
                commands.append(
                    f"peek xram:XRAM_PCM+{offset} "
                    + " ".join(map(str, tail[offset:offset + 32])))
            commands.append('expect-not stderr "Wagon audio"')
        script = os.path.join(directory, "wagon.script")
        with open(script, "w", encoding="utf-8") as output:
            output.write("\n".join(commands) + "\n")
        result = subprocess.run(
            [os.path.join(build, "rp6502-emu.exe"), os.path.join(build, "oregon.rp6502"),
             "--script", script, "--memfill", "0"],
            cwd=directory, capture_output=True, text=True, timeout=90)
        assert result.returncode == 0, result.stdout + result.stderr
        for i in range(2):
            with wave.open(os.path.join(directory, f"wagon-{i}.wav"), "rb") as audio:
                assert audio.getsampwidth() == 2
                for start in (0.2, 1.5):
                    audio.setpos(int(audio.getframerate() * start))
                    data = audio.readframes(int(audio.getframerate() * 0.2))
                    output = struct.unpack("<" + "h" * (len(data) // 2), data)
                    rms = (sum(sample * sample for sample in output) / len(output)) ** 0.5
                    assert rms > 100, f"wagon day {i + 1}: PCM silent at {start}s"
    print("OK: full wagon sample streamed to XRAM and audible on consecutive travel days")


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
        [exe], cwd=pt.HOST, input="bad\n4\n2\n" + "1\n" * 5 + "\n" * 29, capture_output=True,
        text=True, timeout=30)
    assert not result.stderr, result.stderr
    result.check_returncode()
    for landmark, at_landmark in ((0, True), (0, False), (3, True), (3, False), (4, True)):
        menu = result.stdout.split(f"MENU {landmark} {int(at_landmark)}\n", 1)[1]
        menu = menu.split("END MENU", 1)[0]
        assert ("Buy supplies" in menu) == (at_landmark and landmark in (0, 3)), menu
        assert ("Hunt for food" in menu) == (not at_landmark), menu
    assert result.stdout.count("Enter a number from 1 to 3.") == 2
    with open(os.path.join(pt.ROOT, "src", "messages.dat"), "rb") as messages:
        messages.seek(55 * 80)
        prompt = messages.read(80).split(b"\0", 1)[0].decode("ascii")
    assert result.stdout.count(prompt) == 3, result.stdout
    for scenario, clothing, bullets in ((11, 1, 21), (13, 3, 63)):
        loot = result.stdout.split(f"LOOT {scenario}\n", 1)[1].split("END LOOT", 1)[0]
        assert loot.count("Supplies found in the abandoned wagon:") == 1, loot
        assert f"{clothing} sets of clothing" in loot, loot
        assert f"{bullets} bullets" in loot, loot
        for part in ("wheels", "axles", "tongues"):
            assert (f"1 wagon {part}" in loot) == (scenario == 11), loot
            if scenario == 13:
                assert f"wagon {part}" not in loot, loot
    for scenario in (12, 14):
        loot = result.stdout.split(f"LOOT {scenario}\n", 1)[1].split("END LOOT", 1)[0]
        assert loot.strip() == "The abandoned wagon is empty.", loot
    assert result.stdout.count("Press Enter to return to the trail.") == 14
    assert result.stdout.count("Press Enter to continue.") == 15
    for summary in (
        "Animals 6; harvested 30 lb; carried 30 lb.\nAte 15 lb; food now 115 lb.",
        "Animals 2; harvested 1706 lb; carried 100 lb.\nAte 15 lb; food now 1935 lb.",
        "Animals 1; harvested 853 lb; carried 10 lb.\nAte 15 lb; food now 1985 lb.",
        "Animals 1; harvested 1 lb; carried 1 lb.\nAte 1 lb; food now 0 lb.",
    ):
        assert summary in result.stdout, result.stdout
    for name in ("Amos", "Clara"):
        notice = "\x1b[92m" + name + " has recovered and is healthy again.\x1b[39m"
        assert result.stdout.count(notice) == 1, result.stdout
    for tally in ("Meat 0 lb  Carrying 0 lb",
                  "Meat 1 lb  Carrying 1 lb",
                  "Meat 853 lb  Carrying 100 lb",
                  "Meat 853 lb  Carrying 10 lb",
                  "Meat 62 lb  Carrying 62 lb",
                  "Meat 2 lb  Carrying 2 lb",
                  "Meat 1706 lb  Carrying 100 lb"):
        assert tally in result.stdout, result.stdout
    print(result.stdout.splitlines()[-1])
    build = os.path.join(pt.ROOT, "build", "llvm-mos", "release", "oregon")
    with tempfile.TemporaryDirectory(prefix="oregon-hunt-") as directory:
        subprocess.run(
            [os.path.join(build, "rp6502-emu.exe"), os.path.join(build, "oregon.rp6502"),
             "--script", os.path.join(pt.ROOT, "tests", "hunt.script"), "--memfill", "0"],
            cwd=directory, capture_output=True, text=True, timeout=90, check=True)
    print("OK: emulator HID aiming, firing, Escape, menu return and clean exit")
    check_wagon_sound(build)


if __name__ == "__main__":
    main()
