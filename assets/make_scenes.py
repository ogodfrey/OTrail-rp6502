"""Draws the four scenes of the game and writes them to src/scenes.dat.

Each scene is a 640x128 picture in 4 bits per pixel, drawn with the summer
palette. The winter scenes use the same pictures with the winter palette.
The file holds the start of each scene, both palettes, and the pixels of each
scene, packed with PackBits. Each byte holds two pixels, and the left pixel is
in the high nibble. Previews go to build/scenes.

Requires Pillow. Usage: python assets/make_scenes.py
"""

import math
import os
import random
import struct
import sys

from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "src", "scenes.dat")
ROUTE_BIN = os.path.join(ROOT, "assets", "journey_route.bin")
ROUTE_BYTES = 24 * 4
PRAIRIE_ART = os.path.join(ROOT, "assets", "prairie.png")
RIVER_ART = os.path.join(ROOT, "assets", "river.png")
FORT_ART = os.path.join(ROOT, "assets", "fort.png")
MOUNTAIN_ART = os.path.join(ROOT, "assets", "mountain.png")
STORE_ART = os.path.join(ROOT, "assets", "store.png")
JOURNEY_ART = os.path.join(ROOT, "assets", "journey.png")
CHIMNEY_ART = os.path.join(ROOT, "assets", "chimney.png")
INDEPENDENCE_ART = os.path.join(ROOT, "assets", "independence.png")
PREVIEW_DIR = os.path.join(ROOT, "build", "scenes")

W = 640
H = 128
HZ = 74

# Palette indexes. The summer and winter palettes use the same indexes.
(INK, SKY_DEEP, SKY, SKY_PALE, WHITE, SUN, FAR, GRASS_DARK, GRASS, GRASS_LIGHT,
 WOOD, DIRT, ROCK_DARK, ROCK_LIGHT, WATER, WATER_LIGHT) = range(16)

SUMMER = [
    (27, 27, 27), (61, 127, 208), (124, 192, 240), (196, 230, 250),
    (255, 255, 255), (255, 216, 74), (110, 154, 138), (46, 107, 44),
    (85, 160, 56), (154, 203, 90), (122, 78, 42), (201, 154, 94),
    (90, 95, 110), (163, 169, 184), (42, 111, 184), (143, 208, 245),
]
WINTER = [
    (27, 27, 27), (111, 147, 184), (169, 204, 230), (220, 238, 248),
    (255, 255, 255), (255, 246, 200), (185, 201, 210), (184, 199, 208),
    (221, 232, 238), (255, 255, 255), (110, 90, 68), (213, 208, 198),
    (90, 95, 110), (163, 169, 184), (60, 110, 154), (159, 198, 228),
]


def new_image():
    img = Image.new("P", (W, H), INK)
    img.putpalette([c for rgb in SUMMER for c in rgb])
    return img


def draw_prairie(img, rng):
    """The prairie is a painting already mapped to the game palette, in assets/prairie.png."""
    img.paste(Image.open(PRAIRIE_ART))


def draw_mountain(img, rng):
    """The mountains are a painting already mapped to the game palette, in assets/mountain.png."""
    img.paste(Image.open(MOUNTAIN_ART))


def draw_river(img, rng):
    """The river crossing is a painting already mapped to the game palette, in assets/river.png."""
    img.paste(Image.open(RIVER_ART))


def draw_fort(img, rng):
    """The fort is a painting already mapped to the game palette, in assets/fort.png."""
    img.paste(Image.open(FORT_ART))


def draw_store(img, rng):
    """The general store is a painting already mapped to the game palette, in assets/store.png."""
    img.paste(Image.open(STORE_ART))


def draw_journey(img, rng):
    """The journey map is a painting already mapped to the game palette, in assets/journey.png.
    The wagon marker is drawn over it at run time, by scene_journey_marker."""
    img.paste(Image.open(JOURNEY_ART))


def draw_chimney(img, rng):
    """Chimney Rock is a painting already mapped to the game palette, in assets/chimney.png."""
    img.paste(Image.open(CHIMNEY_ART))


def draw_independence(img, rng):
    """Independence Rock is a painting already mapped to the game palette, in assets/independence.png."""
    img.paste(Image.open(INDEPENDENCE_ART))


SCENE_NAMES = ["prairie", "mountain", "river", "fort", "store", "journey", "chimney",
               "independence"]
SCENE_DRAWERS = [draw_prairie, draw_mountain, draw_river, draw_fort, draw_store, draw_journey,
                 draw_chimney, draw_independence]


def build_images():
    """The four scenes, in the order of scene_t in src/scene.h."""
    rng = random.Random(6502)
    images = []
    for draw in SCENE_DRAWERS:
        img = new_image()
        draw(img, rng)
        images.append(img)
    return images


def pack_rows(img):
    """Two pixels to a byte, left pixel in the high nibble, row by row."""
    px = img.tobytes()
    out = bytearray()
    for y in range(H):
        row = px[y * W:(y + 1) * W]
        for x in range(0, W, 2):
            out.append((row[x] << 4) | row[x + 1])
    return bytes(out)


def pack_bits(data):
    """PackBits: a control byte below 128 is followed by that many plus one literal
    bytes, and one from 129 to 255 is followed by a byte to repeat 257 minus that many times."""
    out = bytearray()
    i = 0
    n = len(data)
    while i < n:
        run = 1
        while i + run < n and data[i + run] == data[i] and run < 128:
            run += 1
        if run >= 2:
            out += bytes((257 - run, data[i]))
            i += run
            continue
        start = i
        i += 1
        while i < n and i - start < 128 and not (i + 1 < n and data[i] == data[i + 1]):
            i += 1
        out.append(i - start - 1)
        out += data[start:i]
    return bytes(out)


def rgb555(rgb):
    """Red in bits 0-4, alpha in bit 5 (set means opaque), green in 6-10, blue in 11-15."""
    r, g, b = rgb
    return ((b >> 3) << 11) | ((g >> 3) << 6) | (1 << 5) | (r >> 3)


def palette_bytes(palette):
    return struct.pack("<16H", *[rgb555(c) for c in palette])


def main():
    images = build_images()
    streams = [pack_bits(pack_rows(img)) for img in images]
    with open(ROUTE_BIN, "rb") as f:
        route = f.read()
    if len(route) != ROUTE_BYTES:
        sys.exit("assets/journey_route.bin is missing or the wrong size; run assets/make_journey.py")
    header_len = 4 * len(images) + 2 * 32 + ROUTE_BYTES
    offsets = []
    pos = header_len
    for s in streams:
        offsets.append(pos)
        pos += len(s)
    data = struct.pack("<%dL" % len(images), *offsets) + palette_bytes(SUMMER) + palette_bytes(WINTER)
    data += route
    data += b"".join(streams)
    with open(OUT, "wb") as f:
        f.write(data)

    os.makedirs(PREVIEW_DIR, exist_ok=True)
    flat_winter = [c for rgb in WINTER for c in rgb]
    for name, img in zip(SCENE_NAMES, images):
        img.save(os.path.join(PREVIEW_DIR, name + "-summer.png"))
        winter = img.copy()
        winter.putpalette(flat_winter)
        winter.save(os.path.join(PREVIEW_DIR, name + "-winter.png"))
    print("wrote %s (%d bytes); previews in %s" % (OUT, len(data), PREVIEW_DIR))


if __name__ == "__main__":
    main()
