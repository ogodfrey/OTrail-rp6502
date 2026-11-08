"""Writes the landmark tunes to src/tunes.dat, the tunes asset that sound_landmark in
src/sound.c reads on arrival.

The file starts with one three-byte entry for each of the 18 landmarks: the offset of
its notes in the file, as 16 bits little-endian, and the number of notes. Each note is
three bytes: the frequency in hertz, as 16 bits little-endian, then the frames to hold
it. Sixty frames is one second, and a frequency of 0 is a rest. Independence and
Willamette Valley have no tune here: the victory ditty plays at Willamette Valley.

Usage: python assets/make_tunes.py
"""

import os
import struct

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "src", "tunes.dat")
LANDMARKS = 18
EVENTS = 20
EFFECTS = 8
ENTRIES = LANDMARKS + EVENTS + EFFECTS
ENTRY = struct.Struct("<HB")
NOTE = struct.Struct("<HB")

# Each tune is in the banjo voice, with its own key, tempo, and rhythm.
TUNES = {
    1: [(392, 8), (494, 8), (587, 12), (494, 8), (392, 8), (330, 8), (392, 16), (494, 8), (587, 8), (659, 12), (587, 8), (494, 8), (392, 20)],
    2: [(294, 10), (370, 10), (440, 10), (554, 10), (440, 10), (370, 10), (330, 10), (294, 20), (220, 10), (294, 10), (370, 20)],
    3: [(523, 6), (523, 6), (392, 6), (523, 6), (659, 12), (587, 6), (523, 6), (587, 6), (659, 6), (523, 6), (392, 6), (392, 6), (523, 18)],
    4: [(440, 12), (494, 8), (523, 8), (587, 12), (659, 12), (587, 8), (523, 8), (494, 8), (440, 24)],
    5: [(587, 6), (587, 6), (740, 6), (587, 6), (494, 12), (440, 6), (494, 6), (587, 6), (659, 6), (587, 6), (740, 12), (587, 6), (494, 6), (440, 6), (587, 18)],
    6: [(392, 20), (440, 20), (523, 20), (587, 30), (523, 20), (440, 20), (392, 40)],
    7: [(349, 8), (392, 8), (440, 8), (523, 8), (587, 8), (698, 8), (587, 8), (523, 16), (440, 8), (523, 24)],
    8: [(392, 6), (494, 6), (587, 6), (392, 6), (494, 12), (587, 6), (494, 6), (392, 6), (330, 6), (392, 6), (494, 12), (587, 6), (659, 6), (587, 6), (494, 6), (392, 18)],
    9: [(440, 12), (523, 12), (587, 12), (523, 12), (440, 12), (392, 12), (440, 12), (523, 12), (659, 12), (587, 12), (523, 12), (440, 24)],
    10: [(523, 4), (0, 4), (698, 4), (0, 4), (659, 4), (0, 4), (523, 4), (0, 4), (587, 4), (0, 4), (659, 4), (0, 4), (523, 20)],
    11: [(440, 6), (554, 6), (659, 6), (440, 6), (554, 12), (659, 6), (554, 6), (440, 6), (370, 6), (440, 6), (554, 12), (659, 6), (740, 6), (659, 6), (554, 6), (440, 18)],
    12: [(659, 4), (587, 4), (523, 4), (494, 4), (440, 4), (392, 4), (330, 4), (294, 4), (330, 4), (392, 4), (440, 4), (494, 4), (523, 4), (587, 4), (659, 4), (659, 20)],
    13: [(349, 6), (440, 6), (523, 6), (349, 6), (440, 12), (523, 6), (440, 6), (349, 6), (330, 6), (349, 6), (440, 12), (523, 6), (587, 6), (523, 6), (440, 6), (349, 18)],
    14: [(440, 24), (392, 16), (349, 16), (330, 24), (294, 16), (330, 16), (440, 40)],
    15: [(330, 6), (392, 6), (494, 6), (330, 6), (392, 12), (494, 6), (392, 6), (330, 6), (294, 6), (330, 6), (392, 12), (494, 6), (523, 6), (494, 6), (392, 6), (330, 18)],
    16: [(523, 8), (392, 8), (311, 8), (262, 8), (311, 8), (392, 8), (523, 12), (587, 8), (523, 8), (392, 24)],
    # Snow Bound is the first event tune: an event entry follows the landmarks.
    18: [(220, 20), (0, 8), (262, 12), (330, 12), (294, 16), (0, 8), (262, 12), (220, 36)],
    # Snake Bite: quick, tense stabs that drop to a low thud.
    19: [(330, 4), (0, 4), (330, 4), (0, 4), (311, 6), (0, 4), (262, 6), (233, 6), (220, 30)],
    # Illness: a slow, wavering tune that sinks into the minor.
    20: [(294, 20), (277, 20), (262, 24), (247, 12), (0, 8), (220, 36)],
    # Gravesite: a slow, hushed tune that rests on a low note.
    21: [(392, 24), (0, 6), (349, 24), (330, 18), (294, 12), (0, 8), (262, 36)],
    # Indians help find food: a bright, friendly tune that rises and settles.
    22: [(392, 8), (494, 8), (523, 8), (587, 12), (0, 4), (523, 8), (587, 8), (659, 8), (784, 24)],
    # Blizzard: a low, howling tune that slides down and fades into the wind.
    23: [(196, 10), (0, 4), (220, 10), (196, 10), (175, 14), (0, 6), (165, 30)],
    # Thunderstorm: a rumbling low start, then a bright flash of notes that rolls away.
    24: [(110, 12), (0, 4), (147, 8), (523, 6), (659, 6), (784, 6), (659, 6), (523, 6), (440, 6), (392, 6), (330, 18)],
    # Heavy fog: a hushed, wandering tune with long gaps, as if feeling the way forward.
    25: [(330, 16), (0, 8), (294, 12), (262, 12), (0, 10), (294, 12), (262, 16), (220, 36)],
    # Hail storm: sharp, quick ticks like stones on a wagon cover, then a rolling finish.
    26: [(784, 3), (0, 3), (698, 3), (0, 3), (784, 3), (0, 6), (659, 3), (0, 3), (523, 3), (0, 3), (587, 3), (0, 6), (392, 24)],
    # Oxen died or injured: a plodding, lurching tune in a minor key, like a team stumbling on.
    27: [(392, 10), (0, 4), (349, 10), (330, 10), (294, 14), (0, 6), (262, 10), (294, 10), (220, 30)],
    # Party member afflicted: a halting, worried tune that sinks down and holds on a low note.
    28: [(330, 12), (0, 6), (294, 12), (262, 12), (0, 6), (247, 12), (262, 24)],
    # Lost or wrong trail: a wandering tune that searches up and down and ends unsure.
    29: [(392, 8), (0, 4), (440, 8), (392, 8), (349, 12), (0, 6), (330, 8), (294, 8), (262, 12), (0, 6), (294, 8), (330, 20)],
    # Wild fruit: a bright, lilting tune for gathering berries by the trail.
    30: [(523, 8), (659, 8), (784, 12), (0, 4), (698, 8), (659, 8), (587, 8), (659, 8), (523, 24)],
    # Rough trail: a jolting tune, bumping along unevenly.
    31: [(330, 6), (0, 3), (294, 6), (330, 6), (262, 10), (0, 4), (247, 6), (262, 6), (220, 24)],
    # Wagon fire: an urgent, rising alarm in a minor key.
    32: [(440, 4), (494, 4), (523, 4), (440, 4), (392, 6), (0, 3), (349, 6), (330, 6), (294, 6), (262, 24)],
    # Party member lost: a searching, wistful tune that trails off into silence.
    33: [(523, 10), (0, 4), (494, 8), (440, 8), (392, 12), (0, 6), (349, 8), (330, 8), (294, 20)],
    # Abandoned wagon: a hopeful tune that rises as if something is found.
    34: [(392, 8), (494, 8), (523, 8), (587, 10), (0, 4), (523, 8), (494, 8), (440, 8), (392, 24)],
    # Thief at night: quiet, sneaking staccato notes that slip away down the scale.
    35: [(330, 4), (0, 4), (330, 4), (0, 4), (311, 4), (0, 8), (294, 6), (0, 4), (262, 4), (247, 20)],
    # Bad or little water: a dry, sparse tune that thins out and sinks low.
    36: [(262, 12), (0, 8), (247, 12), (220, 12), (0, 8), (196, 24)],
    # Inadequate grass: a thin, wavering tune, held long, like grass that will not feed the oxen.
    37: [(330, 16), (0, 8), (294, 16), (262, 12), (0, 8), (247, 28)],
    # Sound effects: river (38), fort (39), wheel creak (40), death (41), arrival ditty (42),
    # weekly riff (43), broken wagon (44). The creak's pitch wanders over a few notes; its
    # OPL2 patch is set in sound.c. The death riff is sad, in a minor key.
    38: [(392, 4), (349, 4), (330, 4), (294, 6)],
    39: [(523, 6), (659, 6), (698, 6), (698, 6), (659, 12)],
    40: [(150, 5), (120, 5), (165, 5), (110, 5), (140, 5)],
    41: [(440, 16), (392, 16), (349, 16), (330, 16), (294, 40)],
    42: [(392, 8), (494, 8), (587, 8), (494, 8), (587, 8), (659, 8), (587, 8), (494, 8),
         (440, 8), (494, 8), (587, 8), (659, 8), (698, 8), (659, 8), (587, 8), (494, 8), (392, 32)],
    43: [(392, 6), (494, 6), (587, 6), (494, 6), (392, 6), (440, 6), (523, 6), (587, 12)],
    44: [(330, 8), (0, 6), (330, 8), (0, 6), (294, 8), (0, 6), (262, 24)],
    # Dysentery ending: a slow minor-key tune, a bit longer than the title, that sinks to a low end.
    45: [(220, 36), (262, 24), (294, 24), (330, 36), (0, 12), (294, 24), (262, 24), (247, 36), (220, 36), (0, 12), (196, 48)],
}


def main():
    notes_out = bytearray()
    entries = bytearray()
    offset = ENTRY.size * ENTRIES
    for index in range(ENTRIES):
        notes = TUNES.get(index, [])
        if len(notes) > 20:
            raise SystemExit("tune %d has more than 20 notes" % index)
        entries += ENTRY.pack(offset, len(notes))
        for hz, frames in notes:
            if frames > 255 or hz > 65535:
                raise SystemExit("note out of range in tune %d" % index)
            notes_out += NOTE.pack(hz, frames)
        offset += NOTE.size * len(notes)
    with open(OUT, "wb") as f:
        f.write(bytes(entries + notes_out))
    print("wrote %s (%d bytes)" % (OUT, len(entries) + len(notes_out)))


if __name__ == "__main__":
    main()
