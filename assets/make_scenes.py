"""Draws the four scenes of the game and writes them to src/scenes.dat.

Each scene is a 640x128 picture in 4 bits per pixel, drawn with the summer
palette. The winter scenes use the same pictures with the winter palette.
The file holds the start of each scene, both palettes, and the pixels of each
scene, packed with an LZ77 packer (see lz_pack). Each byte holds two pixels, and the left pixel is
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
SODA_SPRINGS_ART = os.path.join(ROOT, "assets", "soda_springs.png")
BLUE_MOUNTAINS_ART = os.path.join(ROOT, "assets", "blue_mountains.png")
BROKEN_WAGON_ART = os.path.join(ROOT, "assets", "broken_wagon.png")
WILLAMETTE_VALLEY_ART = os.path.join(ROOT, "assets", "willamette_valley.png")
INDEPENDENCE_TOWN_ART = os.path.join(ROOT, "assets", "independence_town.png")
KANSAS_RIVER_ART = os.path.join(ROOT, "assets", "kansas_river.png")
BIG_BLUE_RIVER_ART = os.path.join(ROOT, "assets", "big_blue_river.png")
FORT_KEARNEY_ART = os.path.join(ROOT, "assets", "fort_kearney.png")
SOUTH_PASS_ART = os.path.join(ROOT, "assets", "south_pass.png")
GREEN_RIVER_ART = os.path.join(ROOT, "assets", "green_river.png")
SNOW_BOUND_ART = os.path.join(ROOT, "assets", "snow_bound.png")
SNAKE_BITE_ART = os.path.join(ROOT, "assets", "snake_bite.png")
ILLNESS_ART = os.path.join(ROOT, "assets", "illness.png")
GRAVESITE_ART = os.path.join(ROOT, "assets", "gravesite.png")
INDIANS_FOOD_ART = os.path.join(ROOT, "assets", "indians_food.png")
BLIZZARD_ART = os.path.join(ROOT, "assets", "blizzard.png")
THUNDERSTORM_ART = os.path.join(ROOT, "assets", "thunderstorm.png")
HEAVY_FOG_ART = os.path.join(ROOT, "assets", "heavy_fog.png")
HAIL_STORM_ART = os.path.join(ROOT, "assets", "hail_storm.png")
OXEN_ART = os.path.join(ROOT, "assets", "oxen_died_or_injured.png")
PARTY_AFFLICTED_ART = os.path.join(ROOT, "assets", "party_member_afflicted.png")
LOST_TRAIL_ART = os.path.join(ROOT, "assets", "lost_trail.png")
WILD_FRUIT_ART = os.path.join(ROOT, "assets", "wild_fruit.png")
ROUGH_TRAIL_ART = os.path.join(ROOT, "assets", "rough_trail.png")
WAGON_FIRE_ART = os.path.join(ROOT, "assets", "wagon_fire.png")
PARTY_MEMBER_LOST_ART = os.path.join(ROOT, "assets", "party_member_lost.png")
ABANDONED_WAGON_ART = os.path.join(ROOT, "assets", "abandoned_wagon.png")
THIEF_NIGHT_ART = os.path.join(ROOT, "assets", "thief_night.png")
LITTLE_WATER_ART = os.path.join(ROOT, "assets", "little_water.png")
INADEQUATE_GRASS_ART = os.path.join(ROOT, "assets", "inadequate_grass.png")
DYSENTERY_END_ART = os.path.join(ROOT, "assets", "dysentery_end.png")
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


def draw_soda_springs(img, rng):
    """Soda Springs is a painting already mapped to the game palette, in assets/soda_springs.png."""
    img.paste(Image.open(SODA_SPRINGS_ART))


def draw_blue_mountains(img, rng):
    """The Blue Mountains are a painting already mapped to the game palette, in assets/blue_mountains.png."""
    img.paste(Image.open(BLUE_MOUNTAINS_ART))


def draw_broken_wagon(img, rng):
    """The broken wagon is a painting already mapped to the game palette, in assets/broken_wagon.png."""
    img.paste(Image.open(BROKEN_WAGON_ART))


def draw_willamette_valley(img, rng):
    """The Willamette Valley is a painting already mapped to the game palette, in assets/willamette_valley.png."""
    img.paste(Image.open(WILLAMETTE_VALLEY_ART))


def draw_independence_town(img, rng):
    """Independence is a painting already mapped to the game palette, in assets/independence_town.png."""
    img.paste(Image.open(INDEPENDENCE_TOWN_ART))


def draw_kansas_river(img, rng):
    """The Kansas River crossing is a painting already mapped to the game palette, in assets/kansas_river.png."""
    img.paste(Image.open(KANSAS_RIVER_ART))


def draw_big_blue_river(img, rng):
    """The Big Blue River crossing is a painting already mapped to the game palette, in assets/big_blue_river.png."""
    img.paste(Image.open(BIG_BLUE_RIVER_ART))


def draw_fort_kearney(img, rng):
    """Fort Kearney is a painting already mapped to the game palette, in assets/fort_kearney.png."""
    img.paste(Image.open(FORT_KEARNEY_ART))


def draw_south_pass(img, rng):
    """South Pass is a painting already mapped to the game palette, in assets/south_pass.png."""
    img.paste(Image.open(SOUTH_PASS_ART))


def draw_green_river(img, rng):
    """The Green River crossing is a painting already mapped to the game palette, in assets/green_river.png."""
    img.paste(Image.open(GREEN_RIVER_ART))


def draw_snow_bound(img, rng):
    """Snow Bound is a painting already mapped to the game palette, in assets/snow_bound.png."""
    img.paste(Image.open(SNOW_BOUND_ART))


def draw_snake_bite(img, rng):
    """Snake Bite is a painting already mapped to the game palette, in assets/snake_bite.png."""
    img.paste(Image.open(SNAKE_BITE_ART))


def draw_illness(img, rng):
    """Illness is a painting already mapped to the game palette, in assets/illness.png."""
    img.paste(Image.open(ILLNESS_ART))


def draw_gravesite(img, rng):
    """The gravesite is a painting already mapped to the game palette, in assets/gravesite.png."""
    img.paste(Image.open(GRAVESITE_ART))


def draw_indians_food(img, rng):
    """Indians help find food is a painting already mapped to the game palette, in assets/indians_food.png."""
    img.paste(Image.open(INDIANS_FOOD_ART))


def draw_blizzard(img, rng):
    """The blizzard is a painting already mapped to the game palette, in assets/blizzard.png."""
    img.paste(Image.open(BLIZZARD_ART))


def draw_thunderstorm(img, rng):
    """The thunderstorm is a painting already mapped to the game palette, in assets/thunderstorm.png."""
    img.paste(Image.open(THUNDERSTORM_ART))


def draw_heavy_fog(img, rng):
    """Heavy fog is a painting already mapped to the game palette, in assets/heavy_fog.png."""
    img.paste(Image.open(HEAVY_FOG_ART))


def draw_hail_storm(img, rng):
    """Hail storm is a painting already mapped to the game palette, in assets/hail_storm.png."""
    img.paste(Image.open(HAIL_STORM_ART))


def draw_oxen(img, rng):
    """The oxen picture is a painting already mapped to the game palette, in assets/oxen_died_or_injured.png."""
    img.paste(Image.open(OXEN_ART))


def draw_party_afflicted(img, rng):
    """A party member afflicted is a painting already mapped to the game palette, in assets/party_member_afflicted.png."""
    img.paste(Image.open(PARTY_AFFLICTED_ART))


def draw_lost_trail(img, rng):
    """Lost or wrong trail is a painting already mapped to the game palette, in assets/lost_trail.png."""
    img.paste(Image.open(LOST_TRAIL_ART))


def draw_wild_fruit(img, rng):
    """Wild fruit is a painting already mapped to the game palette, in assets/wild_fruit.png."""
    img.paste(Image.open(WILD_FRUIT_ART))


def draw_rough_trail(img, rng):
    """The rough trail is a painting already mapped to the game palette, in assets/rough_trail.png."""
    img.paste(Image.open(ROUGH_TRAIL_ART))


def draw_wagon_fire(img, rng):
    """The wagon fire is a painting already mapped to the game palette, in assets/wagon_fire.png."""
    img.paste(Image.open(WAGON_FIRE_ART))


def draw_party_member_lost(img, rng):
    """A lost party member is a painting already mapped to the game palette, in assets/party_member_lost.png."""
    img.paste(Image.open(PARTY_MEMBER_LOST_ART))


def draw_abandoned_wagon(img, rng):
    """The abandoned wagon is a painting already mapped to the game palette, in assets/abandoned_wagon.png."""
    img.paste(Image.open(ABANDONED_WAGON_ART))


def draw_thief_night(img, rng):
    """The thief at night is a painting already mapped to the game palette, in assets/thief_night.png."""
    img.paste(Image.open(THIEF_NIGHT_ART))


def draw_little_water(img, rng):
    """Bad and little water are a painting already mapped to the game palette, in assets/little_water.png."""
    img.paste(Image.open(LITTLE_WATER_ART))


def draw_inadequate_grass(img, rng):
    """Inadequate grass is a painting already mapped to the game palette, in assets/inadequate_grass.png."""
    img.paste(Image.open(INADEQUATE_GRASS_ART))


def draw_dysentery_end(img, rng):
    """The dysentery ending is a painting already mapped to the game palette, in assets/dysentery_end.png."""
    img.paste(Image.open(DYSENTERY_END_ART))


SCENE_NAMES = ["prairie", "mountain", "river", "fort", "store", "journey", "chimney",
               "independence", "soda_springs", "blue_mountains", "broken_wagon",
               "willamette_valley", "independence_town", "kansas_river", "big_blue_river",
               "fort_kearney", "south_pass", "green_river", "snow_bound", "snake_bite", "illness",
               "gravesite", "indians_food", "blizzard", "thunderstorm", "heavy_fog", "hail_storm",
               "oxen_died_or_injured", "party_member_afflicted", "lost_trail", "wild_fruit",
               "rough_trail", "wagon_fire", "party_member_lost",
               "abandoned_wagon", "thief_night", "little_water", "inadequate_grass", "dysentery_end"]
SCENE_DRAWERS = [draw_prairie, draw_mountain, draw_river, draw_fort, draw_store, draw_journey,
                 draw_chimney, draw_independence, draw_soda_springs, draw_blue_mountains,
                 draw_broken_wagon, draw_willamette_valley, draw_independence_town,
                 draw_kansas_river, draw_big_blue_river, draw_fort_kearney, draw_south_pass,
                 draw_green_river, draw_snow_bound, draw_snake_bite, draw_illness,
                 draw_gravesite, draw_indians_food, draw_blizzard, draw_thunderstorm, draw_heavy_fog,
                 draw_hail_storm, draw_oxen, draw_party_afflicted, draw_lost_trail, draw_wild_fruit,
                 draw_rough_trail, draw_wagon_fire, draw_party_member_lost,
                 draw_abandoned_wagon, draw_thief_night, draw_little_water, draw_inadequate_grass,
                 draw_dysentery_end]


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


def lz_pack(data):
    """LZ77 over the whole scene. A token below 128 is followed by that many plus one
    literal bytes. A token from 128 to 255 is a match of (token - 128) + 4 bytes, and two
    bytes follow with its distance back into the bytes already unpacked, least significant first."""
    out = bytearray()
    lits = bytearray()
    last = {}
    n = len(data)

    def flush():
        for j in range(0, len(lits), 128):
            chunk = lits[j:j + 128]
            out.append(len(chunk) - 1)
            out.extend(chunk)
        lits.clear()

    i = 0
    while i < n:
        best_len = best_off = 0
        if i + 4 <= n:
            key = bytes(data[i:i + 4])
            cand = last.get(key)
            if cand is not None:
                length = 4
                while length < 131 and i + length < n and data[cand + length] == data[i + length]:
                    length += 1
                best_len, best_off = length, i - cand
            last[key] = i
        if best_len >= 4:
            flush()
            out += bytes((128 + best_len - 4, best_off & 0xFF, best_off >> 8))
            for k in range(i + 1, min(i + best_len, n - 3)):
                last[bytes(data[k:k + 4])] = k
            i += best_len
        else:
            lits.append(data[i])
            i += 1
    flush()
    return bytes(out)


def rgb555(rgb):
    """Red in bits 0-4, alpha in bit 5 (set means opaque), green in 6-10, blue in 11-15."""
    r, g, b = rgb
    return ((b >> 3) << 11) | ((g >> 3) << 6) | (1 << 5) | (r >> 3)


def palette_bytes(palette):
    return struct.pack("<16H", *[rgb555(c) for c in palette])


def main():
    images = build_images()
    streams = [lz_pack(pack_rows(img)) for img in images]
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
