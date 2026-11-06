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

from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "src", "scenes.dat")
PRAIRIE_ART = os.path.join(ROOT, "assets", "prairie.png")
RIVER_ART = os.path.join(ROOT, "assets", "river.png")
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


def sky(d):
    d.rectangle([0, 0, W, 28], fill=SKY_DEEP)
    d.rectangle([0, 32, W, 52], fill=SKY)
    d.rectangle([0, 56, W, HZ], fill=SKY_PALE)
    for y, keep in ((29, 3), (30, 2), (31, 1)):
        dither_row(d, y, keep, SKY_DEEP, SKY)
    for y, keep in ((53, 3), (54, 2), (55, 1)):
        dither_row(d, y, keep, SKY, SKY_PALE)


def dither_row(d, y, keep, upper, lower):
    """Fills row y with upper where x % 4 < keep, and lower elsewhere."""
    for x in range(W):
        d.point((x, y), fill=upper if x % 4 < keep else lower)


def cloud(d, x, y, s):
    parts = [(-18, 0, 13, 8), (0, -6, 16, 11), (17, 0, 12, 8)]
    for dx, dy, rx, ry in parts:
        cx, cy = x + dx * s, y + dy * s
        d.ellipse([cx - rx * s, cy - ry * s + 3, cx + rx * s, cy + ry * s + 3], fill=SKY_PALE)
    for dx, dy, rx, ry in parts:
        cx, cy = x + dx * s, y + dy * s
        d.ellipse([cx - rx * s, cy - ry * s, cx + rx * s, cy + ry * s], fill=WHITE)


def far_hills(d, base, amp, phase):
    pts = []
    for x in range(0, W + 1, 4):
        y = base - amp * (0.5 + 0.5 * math.sin(x / 61 + phase)) - 3 * math.sin(x / 17 + phase * 2)
        pts.append((x, y))
    d.polygon(pts + [(W, H), (0, H)], fill=FAR)


def ridge(d, pts):
    d.polygon(pts + [(pts[-1][0], H), (pts[0][0], H)], fill=FAR)


def ground(d, rng):
    d.rectangle([0, HZ, W, H], fill=GRASS)
    d.rectangle([0, HZ, W, HZ + 2], fill=GRASS_DARK)
    for _ in range(320):
        x = rng.randrange(W)
        y = rng.randrange(HZ + 6, H)
        color = GRASS_DARK if rng.random() < 0.5 else GRASS_LIGHT
        d.line([(x - 2, y), (x, y - 3), (x + 2, y)], fill=color)


def trail(d):
    d.polygon([(314, HZ), (326, HZ), (440, H), (200, H)], fill=DIRT)
    d.line([(320, HZ), (262, H)], fill=WOOD)
    d.line([(322, HZ), (384, H)], fill=WOOD)


def peak(d, ax, ay, half, base):
    d.polygon([(ax, ay), (ax - half, base), (ax, base)], fill=ROCK_LIGHT)
    d.polygon([(ax, ay), (ax, base), (ax + half, base)], fill=ROCK_DARK)
    snow = ay + 0.36 * (base - ay)
    h = half * (snow - ay) / (base - ay)
    d.polygon([(ax, ay), (ax - h, snow), (ax - h * 0.5, snow + 6), (ax, snow + 2),
               (ax + h * 0.45, snow + 7), (ax + h, snow)], fill=WHITE)


def pine(d, x, base, h):
    """A pine of three tiers, each lit on its left side."""
    for top, bottom, half in ((0.0, 0.45, 0.28), (0.28, 0.7, 0.36), (0.5, 0.9, 0.44)):
        y0 = base - h + h * top
        y1 = base - h + h * bottom
        w = h * half
        d.polygon([(x, y0), (x - w, y1), (x + w, y1)], fill=GRASS_DARK)
        d.polygon([(x, y0), (x - w, y1), (x, y1)], fill=GRASS)
    d.rectangle([x - 1, base - h * 0.1, x + 1, base], fill=WOOD)


def fort(d):
    d.rectangle([150, 50, 490, 98], fill=DIRT)
    d.rectangle([150, 92, 490, 98], fill=WOOD)
    for x in range(150, 490, 16):
        d.rectangle([x, 42, x + 8, 50], fill=DIRT)
    for tx in (126, 466):
        d.rectangle([tx, 30, tx + 44, 100], fill=DIRT)
        d.rectangle([tx, 92, tx + 44, 100], fill=WOOD)
        for x in range(tx, tx + 44, 12):
            d.rectangle([x, 22, x + 6, 30], fill=DIRT)
        d.rectangle([tx + 18, 50, tx + 26, 60], fill=INK)
    d.rectangle([388, 70, 436, 98], fill=INK)
    d.ellipse([388, 60, 436, 84], fill=INK)
    d.line([(320, 2), (320, 26)], fill=WOOD, width=2)
    d.polygon([(322, 4), (344, 10), (322, 16)], fill=SUN)


def wagon(d, cx, gy):
    r = 15
    for wx in (cx - 28, cx + 30):
        wy = gy - r
        d.ellipse([wx - r, wy - r, wx + r, wy + r], fill=WOOD, outline=INK, width=2)
        for angle in range(0, 360, 60):
            ex = wx + (r - 2) * math.cos(math.radians(angle))
            ey = wy + (r - 2) * math.sin(math.radians(angle))
            d.line([(wx, wy), (ex, ey)], fill=DIRT)
        d.ellipse([wx - 3, wy - 3, wx + 3, wy + 3], fill=INK)
    d.line([(cx - 42, gy - 30), (cx - 76, gy - 56)], fill=WOOD, width=4)
    d.rectangle([cx - 42, gy - 36, cx + 38, gy - 26], fill=WOOD, outline=INK)
    d.rounded_rectangle([cx - 44, gy - 74, cx + 40, gy - 34], radius=16, fill=WHITE, outline=INK, width=2)
    d.rectangle([cx - 40, gy - 46, cx + 36, gy - 36], fill=ROCK_LIGHT)
    for rx in (cx - 22, cx + 2, cx + 26):
        d.line([(rx, gy - 70), (rx, gy - 36)], fill=ROCK_LIGHT)
    d.rectangle([cx + 26, gy - 66, cx + 34, gy - 44], fill=INK)
    d.rectangle([cx - 40, gy - 27, cx + 36, gy - 25], fill=INK)


def ox(d, x, gy, body=DIRT, legs=WOOD, s=1.5):
    """An ox facing left. x is the middle of its body, gy its feet, and s the scale."""
    def at(dx, dy):
        return (x + dx * s, gy + dy * s)

    for lx in (-16, -7, 14, 23):
        d.rectangle([at(lx - 4, -14), at(lx + 3, -3)], fill=legs)
        d.rectangle([at(lx - 4, -3), at(lx + 3, 0)], fill=INK)
    d.polygon([at(*p) for p in [
        (30, -30), (16, -34), (0, -42), (-12, -40),
        (-18, -26), (-6, -16), (12, -14), (28, -17),
    ]], fill=body, outline=INK)
    d.polygon([at(*p) for p in [
        (-14, -40), (-22, -38), (-32, -30), (-36, -22),
        (-32, -15), (-22, -15), (-18, -24), (-10, -28),
    ]], fill=body, outline=INK)
    d.line([at(-20, -38), at(-26, -48), at(-30, -46)], fill=WHITE, width=2)
    d.point(at(-27, -28), fill=INK)
    d.line([at(29, -28), at(34, -18)], fill=INK)


def oxen(d, cx, gy):
    """A yoke of oxen. The far ox is darker, higher, and a little behind the near one."""
    ox(d, cx - 104, gy - 8, body=WOOD, legs=INK)
    ox(d, cx - 66, gy)
    d.line([(cx - 76, gy - 63), (cx - 119, gy - 71)], fill=WOOD, width=4)


def draw_prairie(img, rng):
    """The prairie is a painting already mapped to the game palette, in assets/prairie.png."""
    img.paste(Image.open(PRAIRIE_ART))


def draw_mountain(img, rng):
    d = ImageDraw.Draw(img)
    sky(d)
    cloud(d, 470, 24, 0.9)
    ridge(d, [(0, 60), (70, 34), (120, 48), (200, 26), (260, 50), (340, 30),
              (420, 52), (500, 22), (560, 44), (640, 36)])
    peak(d, 160, 10, 150, HZ + 2)
    peak(d, 470, 22, 130, HZ + 2)
    ground(d, rng)
    for x in (30, 60, 96, 540, 580, 612):
        pine(d, x, HZ + 6, 30 + rng.randrange(8))
    trail(d)
    oxen(d, 330, 118)
    wagon(d, 330, 118)


def draw_river(img, rng):
    """The river crossing is a painting already mapped to the game palette, in assets/river.png."""
    img.paste(Image.open(RIVER_ART))


def draw_fort(img, rng):
    d = ImageDraw.Draw(img)
    sky(d)
    cloud(d, 420, 26, 0.9)
    far_hills(d, HZ, 12, 2.0)
    ground(d, rng)
    trail(d)
    fort(d)
    oxen(d, 330, 124)
    wagon(d, 330, 124)


def store_wheel(d, cx, cy, r):
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=WOOD, outline=INK, width=2)
    for angle in range(0, 360, 45):
        ex = cx + (r - 3) * math.cos(math.radians(angle))
        ey = cy + (r - 3) * math.sin(math.radians(angle))
        d.line([(cx, cy), (ex, ey)], fill=DIRT)
    d.ellipse([cx - 3, cy - 3, cx + 3, cy + 3], fill=INK)


def icon_food(d, cx, cy):
    for x, y in ((cx + 10, cy + 6), (cx - 8, cy + 2)):
        d.ellipse([x - 15, y - 12, x + 15, y + 18], fill=DIRT, outline=INK)
        d.rectangle([x - 6, y - 16, x + 6, y - 10], fill=WOOD, outline=INK)


def icon_clothing(d, cx, cy):
    shirt = [(cx - 20, cy - 14), (cx - 9, cy - 20), (cx + 9, cy - 20), (cx + 20, cy - 14),
             (cx + 28, cy - 2), (cx + 20, cy + 2), (cx + 16, cy - 4), (cx + 16, cy + 18),
             (cx - 16, cy + 18), (cx - 16, cy - 4), (cx - 20, cy + 2), (cx - 28, cy - 2)]
    d.polygon(shirt, fill=WHITE, outline=INK)
    d.line([(cx - 9, cy - 20), (cx, cy - 8), (cx + 9, cy - 20)], fill=INK)


def icon_ammo(d, cx, cy):
    d.rectangle([cx - 22, cy - 2, cx + 22, cy + 20], fill=WOOD, outline=INK)
    for y in (cy + 4, cy + 10, cy + 16):
        d.line([(cx - 22, y), (cx + 22, y)], fill=DIRT)
    for x in (cx - 12, cx - 4, cx + 4):
        d.rectangle([x, cy - 16, x + 5, cy - 2], fill=SUN, outline=INK)


def icon_wheel(d, cx, cy):
    store_wheel(d, cx, cy + 2, 24)


def icon_axle(d, cx, cy):
    d.line([(cx - 30, cy + 6), (cx + 30, cy + 6)], fill=WOOD, width=5)
    store_wheel(d, cx - 30, cy + 6, 12)
    store_wheel(d, cx + 30, cy + 6, 12)


def icon_tongue(d, cx, cy):
    d.line([(cx - 30, cy + 20), (cx + 22, cy - 16)], fill=WOOD, width=6)
    d.ellipse([cx + 14, cy - 22, cx + 30, cy - 6], outline=INK, width=2)


def draw_store(img, rng):
    """The general store: a plank wall and sign, with the seven items on a shelf.
    The oxen are cut from the painted prairie, so they match it."""
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, W, H], fill=DIRT)
    for y in range(12, 84, 12):
        d.line([(0, y), (W, y)], fill=WOOD)
    d.rectangle([170, 0, 470, 18], fill=WOOD, outline=INK)
    d.rectangle([178, 4, 462, 14], fill=DIRT, outline=WOOD)
    d.rectangle([0, 84, W, 89], fill=WOOD, outline=INK)
    d.rectangle([0, 90, W, H], fill=WOOD)
    d.line([(0, 90), (W, 90)], fill=INK)
    oxen_art = Image.open(PRAIRIE_ART).crop((160, 36, 240, 96))
    img.paste(oxen_art, (6, 22))
    icons = [icon_food, icon_clothing, icon_ammo, icon_wheel, icon_axle, icon_tongue]
    for i, icon in enumerate(icons):
        icon(d, 46 + 91 * (i + 1), 52)


SCENE_NAMES = ["prairie", "mountain", "river", "fort", "store"]
SCENE_DRAWERS = [draw_prairie, draw_mountain, draw_river, draw_fort, draw_store]


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
    header_len = 4 * 5 + 2 * 32
    offsets = []
    pos = header_len
    for s in streams:
        offsets.append(pos)
        pos += len(s)
    data = struct.pack("<5L", *offsets) + palette_bytes(SUMMER) + palette_bytes(WINTER)
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
