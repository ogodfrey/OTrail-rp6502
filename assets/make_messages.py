"""Writes the game messages from src/messages.txt to src/messages.dat, the msg asset that
msg_get in src/msg.c reads.

Each line of messages.txt is one message, and its line number is its id (line 0 is id 0).
In a line, \\ is a backslash and \\n is a newline. Each message is one 80-byte record,
NUL-padded. A message can be at most 79 characters, so the game can always end it.

Usage: python assets/make_messages.py
"""

import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src", "messages.txt")
OUT = os.path.join(ROOT, "src", "messages.dat")
RECORD = 80
MAX_TEXT = RECORD - 1


def decode(line):
    out, i = [], 0
    while i < len(line):
        if line[i] == "\\" and i + 1 < len(line):
            out.append("\n" if line[i + 1] == "n" else line[i + 1])
            i += 2
        else:
            out.append(line[i])
            i += 1
    return "".join(out)


def main():
    with open(SRC, encoding="latin-1") as f:
        lines = f.read().split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    data = bytearray()
    for number, line in enumerate(lines):
        text = decode(line).encode("latin-1")
        if len(text) > MAX_TEXT:
            raise SystemExit("messages.txt line %d: %d characters, the limit is %d"
                             % (number + 1, len(text), MAX_TEXT))
        data += text.ljust(RECORD, b"\x00")
    with open(OUT, "wb") as f:
        f.write(bytes(data))
    print("wrote %s (%d messages, %d bytes)" % (OUT, len(lines), len(data)))


if __name__ == "__main__":
    main()
