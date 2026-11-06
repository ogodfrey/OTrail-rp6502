"""Makes the journey map: assets/journey.png, the map painting in the game palette,
and assets/journey_route.bin, the route the wagon marker follows. The generator
of the scenes asset (assets/make_scenes.py) packs the route into src/scenes.dat.

The map strip comes from the journey dashboard mockup, the dashboard image given
to the project. The trail is traced from the mockup's brown pixels, one point per
column, then sampled at evenly spaced points along the route, from Independence
to Willamette.

Requires Pillow. Usage: python assets/make_journey.py MOCKUP.png
The mockup is the dashboard image; it is not stored in the repo.
"""

import os
import struct
import sys

from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "assets"))
import make_scenes as ms  # noqa: E402

OUT_ART = os.path.join(ROOT, "assets", "journey.png")
OUT_ROUTE = os.path.join(ROOT, "assets", "journey_route.bin")
PREVIEW = os.path.join(ROOT, "build", "scenes", "journey-route-preview.png")

# The map band of the mockup, in its own pixels: the strip between the header
# and the status panels, with the terrain only.
BAND_TOP = 82
BAND_BOTTOM = 302
BAND_LEFT = 14
BAND_RIGHT = 2258

ROUTE_POINTS = 24


def is_trail(r, g, b):
    return 80 <= r <= 185 and 45 <= g <= 120 and 15 <= b <= 80 and r - b >= 45 and r > g > b


def trace(img):
    """One trail y per column of the band, or None where the trail is hidden.
    Only trail pixels near the column's median are kept, so stray dark pixels
    of the terrain do not pull the trail off its line."""
    px = img.load()
    ys = []
    for x in range(BAND_LEFT, BAND_RIGHT):
        hits = [y for y in range(BAND_TOP, BAND_BOTTOM) if is_trail(*px[x, y])]
        if not hits:
            ys.append(None)
            continue
        hits.sort()
        median = hits[len(hits) // 2]
        near = [y for y in hits if abs(y - median) <= 8]
        ys.append(sum(near) / len(near))
    return ys


def smooth_fill(ys, radius=12):
    known = [i for i, y in enumerate(ys) if y is not None]
    filled = []
    for i in range(len(ys)):
        window = [ys[j] for j in range(max(0, i - radius), min(len(ys), i + radius + 1))
                  if ys[j] is not None]
        if window:
            filled.append(sum(window) / len(window))
        else:
            nearest = min(known, key=lambda k: abs(k - i))
            filled.append(ys[nearest])
    return filled


def sample(points, count):
    """Evenly spaced points along the polyline, by arc length."""
    lengths = [0.0]
    for (x0, y0), (x1, y1) in zip(points, points[1:]):
        lengths.append(lengths[-1] + ((x1 - x0) ** 2 + (y1 - y0) ** 2) ** 0.5)
    total = lengths[-1]
    out = []
    seg = 0
    for k in range(count):
        target = total * k / (count - 1)
        while seg < len(points) - 2 and lengths[seg + 1] < target:
            seg += 1
        span = lengths[seg + 1] - lengths[seg] or 1.0
        t = (target - lengths[seg]) / span
        (x0, y0), (x1, y1) = points[seg], points[seg + 1]
        out.append((x0 + (x1 - x0) * t, y0 + (y1 - y0) * t))
    return out


LABEL_BOX = (444, 41, 496, 55)
WAGON_BOX = (446, 53, 488, 78)


def clear_painted_wagon(mapped, route):
    """Paints over the mockup's wagon and its "T: 740 mi" label with the grass
    above them, row by row. The trail is drawn again through the wagon's place,
    in the colour of the trail beside it."""
    px = mapped.load()
    for x0, y0, x1, y1 in (LABEL_BOX, WAGON_BOX):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                px[x, y] = px[x, y0 - 4]
    near = min(route, key=lambda p: abs(p[0] - (WAGON_BOX[0] - 6)))
    trail = px[round(near[0]), round(near[1])]
    segment = [p for p in route if WAGON_BOX[0] - 6 <= p[0] <= WAGON_BOX[2] + 6]
    ImageDraw.Draw(mapped).line(segment, fill=trail, width=3)


def main():
    mockup = Image.open(sys.argv[1]).convert("RGB")
    band = mockup.crop((BAND_LEFT, BAND_TOP, BAND_RIGHT, BAND_BOTTOM))
    scale = 640 / band.width
    band_h = round(band.height * scale)
    pad = (128 - band_h) // 2

    canvas = Image.new("RGB", (640, 128), (224, 201, 150))
    canvas.paste(band.resize((640, band_h), Image.LANCZOS), (0, pad))
    pal = Image.new("P", (1, 1))
    pal.putpalette([c for rgb in ms.SUMMER for c in rgb])
    mapped = canvas.quantize(palette=pal, dither=Image.Dither.NONE)
    mapped.putpalette([c for rgb in ms.SUMMER for c in rgb])

    raw = trace(mockup)
    known = [i for i, y in enumerate(raw) if y is not None]
    lo, hi = known[0], known[-1]
    ys = smooth_fill(raw)[lo:hi + 1]
    route = []
    for i, y in enumerate(ys):
        x = min((lo + i) * scale, 639)
        route.append((x, pad + (y - BAND_TOP) * scale))
    # The mockup runs west to east. Independence is on the right, so reverse.
    route.reverse()
    points = sample(route, ROUTE_POINTS)

    clear_painted_wagon(mapped, route)
    mapped.save(OUT_ART, optimize=True)

    data = bytearray()
    for x, y in points:
        data += struct.pack("<HH", round(x), round(y))
    with open(OUT_ROUTE, "wb") as f:
        f.write(bytes(data))

    os.makedirs(os.path.dirname(PREVIEW), exist_ok=True)
    preview = mapped.convert("RGB").resize((1280, 256), Image.NEAREST)
    d = ImageDraw.Draw(preview)
    for x, y in points:
        d.ellipse([x * 2 - 3, y * 2 - 3, x * 2 + 3, y * 2 + 3], fill=(255, 0, 0))
    preview.save(PREVIEW)
    print("wrote %s, %s, preview %s" % (OUT_ART, OUT_ROUTE, PREVIEW))


if __name__ == "__main__":
    main()
