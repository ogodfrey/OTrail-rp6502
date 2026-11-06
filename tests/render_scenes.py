"""Checks that the scene code draws the art of assets/make_scenes.py.

Builds src/scene.c on the host with the XRAM stubs in tests/host, decodes each
scene in both palettes, and compares the palette and the pixels with the
generator's output. Writes the decoded pictures to build/scenes/decoded-*.png.

Usage: python tests/render_scenes.py
Requires gcc (MSYS2 UCRT64 at C:\\msys64\\ucrt64\\bin, or on PATH) and Pillow.
"""

import os
import shutil
import subprocess
import sys

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "assets"))
import make_scenes as ms  # noqa: E402

OUT = os.path.join(ROOT, "build", "scenes")
EXE = os.path.join(OUT, "scene_render.exe")
GCC = r"C:\msys64\ucrt64\bin\gcc.exe" if os.path.exists(r"C:\msys64\ucrt64\bin\gcc.exe") else "gcc"
TESTS = os.path.join(ROOT, "tests", "host")
SRC = os.path.join(ROOT, "src")


def build():
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(SRC, "scene.c"), encoding="utf-8") as f:
        text = f.read().replace('"ROM:scenes"', '"scenes.dat"')
    with open(os.path.join(OUT, "scene.c"), "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    shutil.copy(os.path.join(SRC, "scenes.dat"), os.path.join(OUT, "scenes.dat"))
    result = subprocess.run(
        [GCC, "-std=gnu99", "-O1", "-w", "-include", os.path.join(TESTS, "binary.h"),
         "-I", TESTS, "-I", SRC,
         os.path.join(OUT, "scene.c"), os.path.join(TESTS, "scene_render.c"),
         os.path.join(TESTS, "xram_shim.c"), "-o", EXE],
        capture_output=True, text=True)
    if result.returncode:
        sys.exit("the host build failed:\n" + result.stderr)


def decode_palette(raw):
    values = [raw[2 * i] | (raw[2 * i + 1] << 8) for i in range(16)]
    return values


def to_rgb(value):
    return ((value & 31) << 3, ((value >> 6) & 31) << 3, ((value >> 11) & 31) << 3)


def check(name, scene, winter, art_rows, palette):
    path = os.path.join(OUT, "decoded-%s-%s.bin" % (name, "winter" if winter else "summer"))
    result = subprocess.run([EXE, str(scene), "1" if winter else "0", path], cwd=OUT,
                            capture_output=True)
    if result.returncode:
        sys.exit("scene_render failed for %s: %s" % (name, result.stderr))
    with open(path, "rb") as f:
        raw = f.read()
    if decode_palette(raw[:32]) != palette:
        sys.exit("palette of %s (%s) does not match the generator" % (name, "winter" if winter else "summer"))
    pixels = raw[32:]
    if pixels != art_rows:
        first = next(i for i in range(len(pixels)) if pixels[i] != art_rows[i])
        sys.exit("pixels of %s (%s) differ from the art, first at byte %d (row %d)"
                 % (name, "winter" if winter else "summer", first, first // 320))

    indices = bytearray()
    for b in pixels:
        indices.append(b >> 4)
        indices.append(b & 15)
    img = Image.new("P", (ms.W, ms.H))
    img.putdata(list(indices))
    img.putpalette([c for v in palette for c in to_rgb(v)])
    img.save(os.path.join(OUT, "decoded-%s-%s.png" % (name, "winter" if winter else "summer")))


def main():
    build()
    images = ms.build_images()
    palette_summer = [ms.rgb555(c) for c in ms.SUMMER]
    palette_winter = [ms.rgb555(c) for c in ms.WINTER]
    for scene, (name, img) in enumerate(zip(ms.SCENE_NAMES, images)):
        art = ms.pack_rows(img)
        check(name, scene, False, art, palette_summer)
        check(name, scene, True, art, palette_winter)
    print("OK: %d scenes decode to the art in both palettes" % len(images))


if __name__ == "__main__":
    main()
