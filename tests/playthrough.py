"""Full playthrough of the Oregon Trail game, on the host.

The game is built with gcc from src/ and tests/host/, with the ROM assets
copied to files. The program is driven through its stdin and stdout. Each
time the game stops to wait for a line, a policy reads the prompt and
chooses the answer. The run ends when the game exits. The answers are saved
to tests/playthrough.txt.

With --replay, the saved answers are fed to the game once, and the test fails
if the game does not exit by itself.

Usage: python tests/playthrough.py [--replay]
Requires gcc (MSYS2 UCRT64 at C:\\msys64\\ucrt64\\bin, or on PATH).
"""

import os
import queue
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import threading
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HOST = os.path.join(tempfile.gettempdir(), "oregon-rp6502-host")
EXE = os.path.join(HOST, "oregon.exe")
ANSWERS = os.path.join(ROOT, "tests", "playthrough.txt")
GCC = r"C:\msys64\ucrt64\bin\gcc.exe" if os.path.exists(r"C:\msys64\ucrt64\bin\gcc.exe") else "gcc"
MAX_SCREENS = 4000
MAX_PROMPTS = 9000
QUIET_SECONDS = 0.05
TIMEOUT_SECONDS = 60
MATTS = [("1", "4"), ("2", "600"), ("3", "5"), ("4", "5"), ("5", "1"), ("6", "1"), ("7", "1")]


def build():
    """Copies the sources, changes the ROM asset names to files, and compiles."""
    src = os.path.join(HOST, "src")
    shutil.rmtree(src, ignore_errors=True)
    os.makedirs(src, exist_ok=True)
    for name in os.listdir(os.path.join(ROOT, "src")):
        if name.endswith((".c", ".h")):
            shutil.copy(os.path.join(ROOT, "src", name), src)
    for name in ("msg.c", "talk.c", "scene.c"):
        path = os.path.join(src, name)
        with open(path, encoding="utf-8") as f:
            text = f.read()
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(text.replace("ROM:msg", "rom_msg").replace("ROM:talk", "rom_talk")
                    .replace("ROM:scenes", "rom_scenes"))
    shutil.copy(os.path.join(ROOT, "src", "messages.dat"), os.path.join(HOST, "rom_msg"))
    shutil.copy(os.path.join(ROOT, "src", "talk.txt"), os.path.join(HOST, "rom_talk"))
    shutil.copy(os.path.join(ROOT, "src", "scenes.dat"), os.path.join(HOST, "rom_scenes"))
    sources = [os.path.join(src, n) for n in ("game.c", "main.c", "scene.c", "sound.c", "talk.c", "ui.c", "msg.c")]
    tests = os.path.join(ROOT, "tests", "host")
    result = subprocess.run(
        [GCC, "-std=gnu99", "-O1", "-w", "-include", os.path.join(tests, "binary.h"),
         "-include", os.path.join(tests, "unbuf.h"),
         "-I", tests, "-I", src] + sources + [os.path.join(tests, "shim.c"), "-o", EXE],
        capture_output=True, text=True)
    if result.returncode:
        sys.exit("the host build failed:\n" + result.stderr)


def start():
    """Starts the game in the host folder, with its files removed first."""
    for name in ("oregon.sco", "oregon.grv"):
        path = os.path.join(HOST, name)
        if os.path.exists(path):
            os.chmod(path, stat.S_IWRITE)
            os.remove(path)
    proc = subprocess.Popen([EXE], cwd=HOST, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    chunks = queue.Queue()

    def pump():
        while True:
            byte = proc.stdout.read(1)
            if not byte:
                chunks.put(None)
                return
            chunks.put(byte)

    threading.Thread(target=pump, daemon=True).start()
    return proc, chunks


def read_until_waiting(proc, chunks):
    """Reads the output until the game is quiet (waiting for input) or has exited.
    Returns (text, exited)."""
    text = bytearray()
    deadline = time.time() + TIMEOUT_SECONDS
    while time.time() < deadline:
        try:
            item = chunks.get(timeout=QUIET_SECONDS)
        except queue.Empty:
            if proc.poll() is not None:
                return text.decode("latin-1"), True
            return text.decode("latin-1"), False
        if item is None:
            return text.decode("latin-1"), True
        text += item
    sys.exit("the game did not wait for input within %d seconds" % TIMEOUT_SECONDS)


def last_menu_numbers(text):
    """Maps each menu label in the last numbered block to its number."""
    found = {}
    for number, label in re.findall(r"(?m)^(\d+)\) (.+)$", text[-1500:]):
        found[label.strip()] = number
    return found


def find_number(menu, word):
    for label, number in menu.items():
        if word in label:
            return number
    return None


def money(text):
    matches = re.findall(r"Money \$([\d,]+)\.(\d\d)", text[-1500:])
    if not matches:
        return 0.0
    whole, cents = matches[-1]
    return int(whole.replace(",", "")) + int(cents) / 100


def food(text):
    matches = re.findall(r"Food (\d+) lb", text[-1500:])
    return int(matches[-1]) if matches else 0


def bullets_left(text):
    matches = re.findall(r"Bullets (\d+)", text[-1500:])
    return int(matches[-1]) if matches else 0


def clothing(text):
    matches = re.findall(r"Clothing (\d+)", text[-1500:])
    return int(matches[-1]) if matches else 0


def policy(text):
    """The answer to the prompt at the end of text. None if it is unknown."""
    tail = text[-400:]
    recent = text[-1200:]
    weeks = text.count("Miles: ")

    if tail.endswith("Press Enter to continue."):
        return ""
    if tail.endswith("M for the menu: "):
        return ""
    if tail.endswith("Name of the party leader: "):
        return "Alice"
    if re.search(r"Name of companion \d \(Enter for \w+\): $", tail):
        return ""
    if tail.endswith("Choose your profession: "):
        return "1"
    if "Departure month" in text[-600:] and tail.endswith("Choose: "):
        return "1"

    if "MATT'S GENERAL STORE" in text and tail.endswith(("Buy what? ", "How many? ")):
        last = text[text.rindex("MATT'S GENERAL STORE"):]
        cart = {int(n): int(h) for n, h in re.findall(r"(?m)^(\d)\) .*?have (\d+)", last)}
        pending = [i for i in range(len(MATTS)) if cart.get(i + 1, 0) == 0]
        if not pending:
            return "0"
        if tail.endswith("How many? "):
            return MATTS[pending[0]][1]
        return MATTS[pending[0]][0]
    if "FORT STORE" in text[-3000:] and tail.endswith(("Buy what? ", "How many? ")):
        return "0"

    if tail.endswith("Pace: "):
        return "1"
    if tail.endswith("Rations: "):
        return "1"
    if tail.endswith("How many days would you like to rest? (0-9) "):
        return "0"
    if tail.endswith("How many bullets to use? (0 to cancel) "):
        bullets = re.findall(r"Bullets (\d+)", text[-1500:])
        available = int(bullets[-1]) if bullets else 0
        return str(min(4, available))
    if tail.endswith("Epitaph (up to 29 characters): "):
        return "The trail ended"
    if tail.endswith("1) Yes, give up the journey   2) No: "):
        return "1" if weeks >= MAX_SCREENS else "2"
    if tail.endswith("1) Yes   2) No: "):
        return "1"
    if tail.endswith("1) Accept   2) Decline: "):
        return "1" if clothing(text) >= 2 else "2"
    if tail.endswith("1) Accept the trade   2) Decline: "):
        return "1"
    if tail.endswith("1) Try to repair it   2) Do not try: "):
        return "1"
    if "The river is" in recent and tail.endswith("Choose: "):
        menu = last_menu_numbers(text)
        depths = re.findall(r"(\d+)\.(\d) feet deep", text[-900:])
        depth = float("%s.%s" % depths[-1]) if depths else 0.0
        if find_number(menu, "Take the ferry") and money(text) >= 5:
            return find_number(menu, "Take the ferry")
        if find_number(menu, "Hire the guide") and clothing(text) >= 2:
            return find_number(menu, "Hire the guide")
        if depth < 2.5:
            return find_number(menu, "Ford")
        return find_number(menu, "Caulk") or find_number(menu, "Wait")
    if "The trail divides here." in recent and tail.endswith("Choose: "):
        return "2"
    if "The trail divides." in recent and tail.endswith("Choose: "):
        return "1"

    at_title = text.rfind("1) Start a new journey") > text.rfind("Continue on the trail")
    if tail.endswith("Choose: ") and at_title:
        return "1" if text.count("1) Start a new journey") == 1 else "3"

    if tail.endswith("Choose: ") and "Continue on the trail" in recent and not at_title:
        menu = last_menu_numbers(text)
        if weeks >= MAX_SCREENS:
            return find_number(menu, "Give up") or "1"
        if "You have no oxen" in recent or "must trade for a spare" in recent:
            return find_number(menu, "Attempt to trade") or "1"
        if food(text) < 100 and find_number(menu, "Hunt") and bullets_left(text) > 0:
            return find_number(menu, "Hunt")
        return find_number(menu, "Continue") or "1"

    return None


def play(answers=None):
    """Plays the game. With answers, feeds them in order. Without, chooses each
    answer with the policy. Returns (answers, transcript, exit code)."""
    proc, chunks = start()
    transcript = ""
    given = []
    while True:
        new_text, exited = read_until_waiting(proc, chunks)
        transcript += new_text
        if exited:
            proc.wait()
            return given, transcript, proc.returncode
        if answers is None:
            if len(given) % 100 == 0:
                print("prompts: %d, weeks: %d" % (len(given), transcript.count("Miles: ")), flush=True)
            if len(given) >= MAX_PROMPTS:
                proc.kill()
                sys.exit("the game asked more than %d prompts; last text: %r"
                         % (MAX_PROMPTS, transcript[-300:]))
            if len(given) >= 6 and len(set(given[-6:])) == 1 and given[-1] != "":
                proc.kill()
                sys.exit("the same answer %r was given six times; last text: %r"
                         % (given[-1], transcript[-700:]))
            answer = policy(transcript)
            if answer is None:
                proc.kill()
                sys.exit("no answer for the prompt at the end of: %r" % transcript[-200:])
        else:
            if len(given) >= len(answers):
                proc.kill()
                return given, transcript, None
            answer = answers[len(given)]
        given.append(answer)
        proc.stdin.write((answer + "\n").encode("latin-1"))
        proc.stdin.flush()


def main():
    build()
    if "--replay" in sys.argv:
        with open(ANSWERS, encoding="utf-8") as f:
            answers = [line.rstrip("\n") for line in f]
        _, text, code = play(answers)
        print("Replay: %d answers, exit code %s" % (len(answers), code))
        if code != 0:
            sys.exit("the saved playthrough did not finish cleanly")
        return

    given, text, code = play()
    if code != 0:
        sys.exit("the playthrough did not finish (exit code %s)" % code)
    with open(ANSWERS, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(given) + "\n")
    miles = re.findall(r"Miles: (\d+) of 2040", text)
    print("Playthrough finished: %d answers saved to tests/playthrough.txt" % len(given))
    print("Last miles shown: %s" % (miles[-1] if miles else "none"))
    if "Congratulations" in text:
        print("Ending: arrived; context:")
        print(text[text.index("Congratulations") - 200:text.index("Congratulations") + 200])
    print("Reprompts (Enter a number): %d" % text.count("Enter a number"))
    print("--- last 900 characters ---")
    print(text[-900:])
    for m in re.finditer(r"Enter a number", text):
        print("--- reprompt context ---")
        print(text[max(0, m.start() - 250):m.start() + 40])
        break


if __name__ == "__main__":
    main()
