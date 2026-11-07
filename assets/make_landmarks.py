"""Writes the landmarks of the trail from src/landmarks.txt to src/landmarks.dat.

Each landmark is one 24-byte record: the name in 18 bytes, NUL-padded, then the type
(0 other, 1 store, 2 river), the leg that leaves it, the alternate leg (255 for none),
the arrival picture and the crossing picture, as scene numbers from SCENE_NAMES in
make_scenes.py (255 for none). The landmark number is the record number.

Usage: python assets/make_landmarks.py
"""

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "assets"))
import make_scenes as ms  # noqa: E402

SRC = os.path.join(ROOT, "src", "landmarks.txt")
OUT = os.path.join(ROOT, "src", "landmarks.dat")
RECORD = 24
NAME_BYTES = 18
LANDMARKS = 18
NONE = 255
TYPES = {"other": 0, "store": 1, "river": 2}


def scene_number(name):
    if name == "-":
        return NONE
    if name not in ms.SCENE_NAMES:
        raise SystemExit("landmarks.txt: no scene named %r" % name)
    return ms.SCENE_NAMES.index(name)


def number(text, what, lo, hi):
    if text == "-":
        return NONE
    value = int(text)
    if not lo <= value <= hi:
        raise SystemExit("landmarks.txt: %s %d is out of range" % (what, value))
    return value


def parse():
    rows = []
    with open(SRC, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            cols = [c.strip() for c in line.split("|")]
            if len(cols) != 6:
                raise SystemExit("landmarks.txt: expected 6 columns: %r" % line)
            name, kind, leg, alt, arrive, cross = cols
            if len(name.encode("latin-1")) > NAME_BYTES - 1:
                raise SystemExit("landmarks.txt: name too long: %r" % name)
            if kind not in TYPES:
                raise SystemExit("landmarks.txt: unknown type %r" % kind)
            rows.append(
                name.encode("latin-1").ljust(NAME_BYTES, b"\x00")
                + bytes((TYPES[kind],
                         number(leg, "leg", 0, 19),
                         number(alt, "alternate leg", 0, 19),
                         scene_number(arrive),
                         scene_number(cross)))
                + b"\x00" * (RECORD - NAME_BYTES - 5))
    if len(rows) != LANDMARKS:
        raise SystemExit("landmarks.txt: %d landmarks, expected %d" % (len(rows), LANDMARKS))
    return rows


def main():
    data = b"".join(parse())
    with open(OUT, "wb") as f:
        f.write(data)
    print("wrote %s (%d landmarks, %d bytes)" % (OUT, LANDMARKS, len(data)))


if __name__ == "__main__":
    main()
