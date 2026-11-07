"""Writes the event pictures from src/events.txt to src/events.dat.

The asset is one byte per event, in event order (the EVENT_ numbers in src/sound.h).
Each byte is the picture's scene number from SCENE_NAMES in make_scenes.py, or 255
for no picture.

Usage: python assets/make_events.py
"""

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "assets"))
import make_scenes as ms  # noqa: E402

SRC = os.path.join(ROOT, "src", "events.txt")
OUT = os.path.join(ROOT, "src", "events.dat")
EVENTS = 12
NONE = 255


def parse():
    pictures = []
    with open(SRC, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            cols = [c.strip() for c in line.split("|")]
            if len(cols) != 2:
                raise SystemExit("events.txt: expected 2 columns: %r" % line)
            picture = cols[1]
            if picture == "-":
                pictures.append(NONE)
            elif picture in ms.SCENE_NAMES:
                pictures.append(ms.SCENE_NAMES.index(picture))
            else:
                raise SystemExit("events.txt: no scene named %r" % picture)
    if len(pictures) != EVENTS:
        raise SystemExit("events.txt: %d events, expected %d" % (len(pictures), EVENTS))
    return bytes(pictures)


def main():
    data = parse()
    with open(OUT, "wb") as f:
        f.write(data)
    print("wrote %s (%d events, %d bytes)" % (OUT, EVENTS, len(data)))


if __name__ == "__main__":
    main()
