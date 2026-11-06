# Oregon Trail for the Picocomputer 6502

The Picocomputer 6502 was created by Rumbledethumps. Learn more at
[Picocomputer 6502](https://picocomputer.github.io/index.html).

This is a C version of The Oregon Trail for the
[RP6502](https://picocomputer.github.io/sdk.html), built with the llvm-mos
compiler and the RP6502 SDK. Lead a party of five from Independence, Missouri,
2040 miles to the Willamette Valley in Oregon.

<!-- rp6502
preset: llvm-mos/Release
publish: oregon.zip
-->
[![Play The Oregon Trail in a web browser](https://ogodfrey.github.io/OTrail-rp6502/oregon/screenshot.png)](https://ogodfrey.github.io/OTrail-rp6502/oregon/)

[Play in a web browser](https://ogodfrey.github.io/OTrail-rp6502/oregon/). No download is needed.

The game follows the 1985 MECC version for the Apple II, first released by the
Minnesota Educational Computing Consortium (MECC). This project is not
affiliated with or endorsed by MECC.

## Sources and licensing

The rules, prices, distances, climate, river data, and event probabilities come
from the reproduction specification in
[OpenTrail](https://github.com/dannyheskett/opentrail) by Dan Heskett
(`docs/OREGON-TRAIL-SPEC.md`). That spec was used as a reference for the game's
facts and rules only. This project does not copy OpenTrail's code, its text,
its data files, or MECC's machine code. The game's text and code are original.

OpenTrail's code is MIT licensed. Its notice says that the original game's
content is published with the permission of its rights holder, which is not
granted to this project. Any reuse of that content needs its own permission.

Some parts are approximations, listed under [Known differences](#known-differences).

The talk-to-people quotes are paraphrases of the landmark dialogue in
OpenTrail's `legacy/extracted/` text databases (`OREGON1.SEQ` and `OREGON2.SEQ`).
The project owner states that they hold the rights to that material. The
paraphrases keep each speaker's point of view and the landmark, but the words
and speaker names are new, and the quotes are stored as the `talk` asset in
`src/talk.txt`. The original text is not in this repository.

## Requirements

* [CMake](https://cmake.org/) 3.28 or newer
* Python 3
* Optional: [Pillow](https://pypi.org/project/pillow/), only to redraw the scenes
  with `assets/make_scenes.py`.
* Ninja or Make
* [llvm-mos](https://github.com/picocomputer?view_as=public), the 6502 C
  compiler. The SDK installs it into `~/.rp6502/llvm-mos`.
* Optional: VS Code with the C/C++ Extension Pack, CMake Tools, and LLDB DAP,
  for building, debugging, and running from the editor.

Install steps for Windows, macOS, and Linux are in the
[SDK prerequisites](https://picocomputer.github.io/sdk.html#installing-the-prerequisites).
On Windows, CMake needs `CMAKE_GENERATOR=Ninja` to build with llvm-mos.

## Building

```bash
cmake --preset llvm-mos/Debug
cmake --build --preset llvm-mos/Debug
```

The ROM is written to `build/llvm-mos/debug/oregon/oregon.rp6502`. The
`llvm-mos/Release` preset builds `build/llvm-mos/release/oregon/oregon.rp6502`.

Both presets compile with `-Oz`. The ROM fills most of the 6502's RAM, and the
other optimization levels do not fit. Do not change that flag. The Debug preset
keeps line information with `-g1`, which the emulator uses for source-level
debugging.

## Running

* **Emulator (VS Code):** press F5 with the "RP6502-EMU" launch configuration.
* **Emulator (command line):** run the emulator that the build downloaded, with
  the ROM as its argument. Input comes from stdin, and the console prints to
  stdout:

  ```bash
  build/llvm-mos/debug/oregon/rp6502-emu --headless build/llvm-mos/debug/oregon/oregon.rp6502
  ```

* **RP6502-PICO:** press F5 with the "RP6502-PICO" launch configuration, after
  setting `device` in `.rp6502`. Or use `tools/rp6502.py run`.
* **Browser:** the "RP6502-WEB" launch configuration opens the web player that
  the build makes.

## Testing

`tests/playtest.py` is a smoke test. It runs the Debug ROM in the headless
emulator with scripted input, then checks that the game reaches the trail and
writes nothing to stderr. The game waits for input that the script doesn't
supply, so the test stops the emulator after a timeout.

```bash
python tests/playtest.py
```

`tests/playthrough.py` plays a whole game on the host. It compiles `src/` with
gcc (MSYS2 UCRT64, or any gcc on the PATH) together with the stubs in
`tests/host/`, then drives the game through stdin and stdout. A policy reads
each prompt and picks an answer. The run ends when the game exits. The answers
are saved to `tests/playthrough.txt`.

```bash
python tests/playthrough.py            # play a new game and save the answers
python tests/playthrough.py --replay   # feed the saved answers back; fails if the game does not exit cleanly
```

`tests/render_scenes.py` checks the scene code. It builds `src/scene.c` on the
host with a 64 KB XRAM stub, decodes each scene in both palettes, and fails if
the pixels or the palette differ from what `assets/make_scenes.py` drew. It
writes the decoded pictures to `build/scenes/`.

```bash
python tests/render_scenes.py
```

The host build writes its score and grave files to the system temp folder, so
the test doesn't touch the repo.

## The game

### Setup

1. Choose **Start a new journey**.
2. Choose a profession. It sets your starting money:

   | Profession | Money |
   | ---------- | ----- |
   | Banker     | $1600 |
   | Carpenter  | $800  |
   | Farmer     | $400  |

   The score multiplier is 1, 2, or 3 in the same order.
3. Name the party leader, then the four companions. Pressing Enter gives a
   companion a default name.
4. Choose a departure month from March to July, in 1848.
5. Outfit the wagon at Matt's General Store in Independence. You must buy at
   least one yoke of oxen, which is two oxen, before you can leave.

### Matt's General Store

| Item                       | Price                 | Limit                 |
| -------------------------- | --------------------- | --------------------- |
| Oxen, yoke of two          | $40                   | 9 yokes               |
| Food, per pound            | $0.20                 | 2000 pounds           |
| Clothing, per set          | $10                   | 99 sets               |
| Ammunition, box of 20      | $2                    | 99 boxes              |
| Wagon wheel, axle, tongue  | $10 each              | 3 of each             |

Quantities are set, not added: choosing an item again replaces its quantity.

### Forts

Forts sell the same items one at a time, at a price that rises along the trail.
The price is the base price times 1.00 at Independence, rising by 0.25 at each
fort, to 2.50 at Fort Walla Walla. Oxen sell singly at $20 each, with a cap of
20 oxen, and food has a cap of 2000 pounds.

### The trail

The route has 18 landmarks and 2040 miles, split into 19 legs. Each leg has a
length and a base speed. Speed is 20 miles a day up to Fort Laramie and 12 miles
a day after it. The pace multiplies it: Steady is 1, Strenuous 1.5, and Grueling
2. Speed falls with fewer than four oxen, with each sick or injured party member,
and with deep snow.

| Landmark          | Type  | Next stop and leg length                             |
| ----------------- | ----- | ---------------------------------------------------- |
| Independence      | Store | Kansas River, 102 miles                              |
| Kansas River      | River | Big Blue River, 83 miles                             |
| Big Blue River    | River | Fort Kearney, 119 miles                              |
| Fort Kearney      | Fort  | Chimney Rock, 250 miles                              |
| Chimney Rock      | Site  | Fort Laramie, 86 miles                               |
| Fort Laramie      | Fort  | Independence Rock, 190 miles                         |
| Independence Rock | Site  | South Pass, 102 miles                                |
| South Pass        | Site  | Green River, 57 miles, or Fort Bridger, 125 miles    |
| Fort Bridger      | Fort  | Soda Springs, 162 miles                              |
| Green River       | River | Soda Springs, 144 miles                              |
| Soda Springs      | Site  | Fort Hall, 57 miles                                  |
| Fort Hall         | Fort  | Snake River, 182 miles                               |
| Snake River       | River | Fort Boise, 114 miles                                |
| Fort Boise        | Fort  | Blue Mountains, 160 miles                            |
| Blue Mountains    | Site  | Fort Walla Walla, 55 miles, or The Dalles, 125 miles |
| Fort Walla Walla  | Fort  | The Dalles, 120 miles                                |
| The Dalles        | Site  | Columbia raft or Barlow Road, 100 miles              |

The default route through Green River and Fort Hall totals 1771 miles to The
Dalles, plus the Barlow Road's 100 miles to the Willamette Valley. South Pass
and the Blue Mountains are route choices, and The Dalles is the last decision.

### Weeks and the menu

The trail runs in weeks. At the start of each week you see the date, weather,
health, supplies, and the next landmark, then choose:

1. **Continue on the trail**: travel up to seven days, or until a landmark or a
   problem stops you. Each day after the first shows the status screen and asks
   `Enter to keep traveling, M for the menu`. Press Enter to go on, or type M and
   Enter to open the menu in the middle of the week.
2. **Check supplies**
3. **Change pace**: Steady, Strenuous, or Grueling. The screen explains what each
   one does to distance and wear before you choose.
4. **Change food rations**: Filling (3 pounds a person a day), Meager (2), or
   Bare-bones (1). The screen explains the health cost of each before you choose.
5. **Stop to rest**: 0 to 9 days.
6. **Attempt to trade**: one day, whatever the outcome.
7. **Hunt for food**: on the trail only, one day.
8. **Buy supplies**: at a fort. Matt's store is only at Independence.
9. **Look at the map**: lists the 18 landmarks, marks the ones you've reached,
   and shows the current position and miles traveled.
10. **Talk to people**: at a landmark only. Each visit hears the next of three
   speakers for that landmark.
11. **Give up the journey**: ends the game with no score.

Continue is refused while a broken wagon part has no spare, or while you have no
oxen.

### Weather and the calendar

Each landmark belongs to one of five climate zones, which set the monthly
temperature and the chance of rain or snow. Weather changes about every other
day, and it can be rainy, very rainy, snowy, or very snowy. Rain and snow build
up ground moisture and snow depth. Moisture makes rivers deeper, wider, and
faster. Snow slows the wagon, and 40 inches of snow stops it. There is no
deadline. The calendar has no leap years.

### Health

Party health is one score from 0 (best) to 139. It shows as Good, Fair, Poor, or
Very poor. Each day it moves toward ten times the day's stress. The stress comes
from temperature, clothing, rations, pace, weather, starvation, and exposure.
Afflicted party members add to it. Health never goes above 139, and a death
brings it down to 105.

Afflictions are a broken arm or leg (30 days), a snakebite (10 days), or an
illness (10 days). When a misfortune picks a party member, the leader is not
chosen while others live. A member dies of an illness that strikes someone who
is already sick, or drowns in a river. The game ends when everyone is dead.

### Random events

Events are checked once a travel day, in order. Each has the chance from the
spec, and a handler applies its effect:

* Snowbound (certain when snow is over 30 inches): lose 1 to 10 days.
* Snakebite (only when the weather is warm or hotter): one member, 10 days.
* Illness or death.
* Indians find food: 30 pounds, when you have none.
* Severe weather: a blizzard adds snow and costs a day; a thunderstorm adds
  rain and costs a day.
* Fog or hail.
* Mishap: a broken wagon part (you may try to repair it), an injured ox, or an
  injured or broken arm or leg.
* Lost trail or wrong trail: 1 to 5 days.
* Rough trail (more health stress) or an impassable trail: 1 to 10 days.
* Wild fruit: 20 pounds of food.
* A fire, a lost member, or an ox that wanders off.
* An abandoned wagon or a thief.
* Bad water, very little water, or inadequate grass.

### Rivers

Each river has a depth, width, and swiftness that depend on the moisture. The
choices are:

* **Ford**: free. Shallow fords are mostly safe, but mud and rough bottoms can
  cost a day or some supplies. Deep fords can cost supplies, oxen, and members.
* **Caulk and float**: costs a day. The wagon can tip, which costs supplies and
  can drown members. The river must be at least 1.5 feet deep.
* **Ferry** (Kansas and Green rivers): $5 and two to six days of waiting. It can
  break loose and lose supplies, oxen, or members.
* **Guide** (Snake River): pay two or three sets of clothing for a guide. A
  guided crossing has less risk.
* **Wait a day.**

### Splits and The Dalles

At South Pass you can head for the Green River or Fort Bridger. At the Blue
Mountains you can head for Fort Walla Walla or The Dalles.

At The Dalles you choose:

* **Float the Columbia River on a raft.** This is an approximation: three
  hazards, each a rock or the shore, with losses. See below.
* **Take the Barlow Toll Road.** The toll is $5 plus $0.50 for each ox. You
  need enough cash, and the last 100 miles take 12 miles a day.

### Graves

When the whole party dies, the leader's name and an epitaph of up to 29
characters go into `oregon.grv`, with the leg and the miles left on that leg.
When a later party travels the same leg and passes a grave, the game asks
whether to look closer. Looking shows the name and epitaph. Graves are read
from the file when each leg starts.

### Hunting

Hunting costs one day and bullets. Each shot can kill an animal, and the
animals depend on where you are:

* Rabbits (1 to 5 pounds), squirrels (1 to 3), and deer (60 to 75) are always
  available.
* Antlered deer (50 to 70) appear from Chimney Rock to the Snake River.
* Bears (100 to 200) appear from South Pass on.
* Buffalo (850 to 1000) appear before South Pass.

Meat is capped so that food does not go above 2000 pounds.

### Trading

A trade costs a day. A stranger offers to swap one item for another at a rate
set by the item values, with a 5% chance that nobody wants to trade. The rules
are from the spec. Trades that would go past a cap are refused.

### Arrival and scoring

Reaching the Willamette Valley scores points. Each survivor scores 500, less
100 for each health band. The wagon scores 50, each ox 4, each spare part 2, and
each set of clothing 2. Bullets score 1 per 50, food 1 per 25 pounds, and money
1 per $5. The total is multiplied by the profession's multiplier. The rating is
Trail guide at 6000 or more, Adventurer at 3000 or more, and Greenhorn below
that.

The top 10 scores are saved to `oregon.sco` in the directory the program runs
from. The list starts empty.

## Known differences

These parts of the Apple II game are missing or simplified here:

* **Hunting** is a text hunt, not the arcade game. Each shot has a 40% chance to
  hit. That chance is my assumption, not a value from the spec.
* **The Columbia raft** is three random hazards with the spec's losses. It is
  not the arcade game with its arrow-key steering.
* **Scene graphics** are hand-drawn pictures in 16 colors. They are not the
  Apple II pictures.
* **Music and sound** are not implemented.
* **Native American encounters** are not implemented.
* **Give up the journey** is an addition. The Apple II ends only by arrival or
  death.
* **The Top Ten** starts empty. The original's list is not included.
* **Travel input** is a line, not a keypress. Each day waits for Enter, so you
  type M and Enter to open the menu.

The spec's values have not been checked against the Apple II by playing it.
Under the spec's river formulas, a Kansas River ford in March can be very deadly,
because the early ground moisture makes the river deep. The ferry or caulking
are safer choices there.

## Screen layout

The canvas is 640x480. The top 128 scanlines show the scene, a 4-bit VGA mode 3
bitmap, and the console runs in VGA mode 0 below it, with 80 columns and 22 rows.

There are four scenes: the prairie, the mountains, a river crossing, and a fort.
Each is a 640x128 picture in a 16-color palette, with the wagon and its oxen in
front. The winter scenes use the same pictures with a second palette, which
turns the sky and the ground white and blue. The pictures are stored in
`src/scenes.dat` as run-length pixels, and the palettes are in the same file.

## Source layout

| Path                      | Purpose                                                                      |
| ------------------------- | ---------------------------------------------------------------------------- |
| `src/main.c`              | Video setup, title screen, and the top 10 file.                              |
| `src/game.c`, `game.h`    | Rules: party, supplies, weather, health, events, rivers, trade, and scoring. |
| `src/scene.c`, `scene.h`  | Decodes scenes into the XRAM bitmap and sets the palette.                    |
| `src/talk.c`, `talk.h`    | Talk to people: reads the talk asset and wraps each quote.                   |
| `src/talk.txt`            | The talk asset: landmark, speaker, and quote on each line.                   |
| `src/msg.c`, `msg.h`      | Looks up a game message in the msg asset by its number.                      |
| `src/messages.dat`        | The msg asset: 80-byte records, one message each. Built from the game source.|
| `src/ui.c`, `ui.h`        | Console helpers: line input, numbered prompts, pauses, and money formatting. |
| `src/xram.h`              | XRAM map: scene config, palette, bitmap, and VGA macros.                     |
| `src/help.txt`            | The help text stored in the ROM as the `help` asset.                         |
| `src/scenes.dat`          | The scenes asset: the four pictures and their summer and winter palettes.    |
| `assets/make_scenes.py`   | Draws the scenes with Pillow and writes `src/scenes.dat`.                    |
| `tests/playtest.py`       | The emulator smoke test.                                                     |
| `tests/playthrough.py`    | The host playthrough: plays a full game and saves the answers.               |
| `tests/playthrough.txt`   | The answers from the saved playthrough, used by `--replay`.                  |
| `tests/render_scenes.py`  | Decodes scenes on the host and compares them with the art.                   |
| `tests/host/`             | Host shim: RIA and XRAM stubs for building `src/` with gcc.                  |
| `CMakeLists.txt`          | The `oregon` ROM target and its sources.                                     |
| `CMakePresets.json`       | The llvm-mos Debug and Release presets.                                      |
| `tools/`                  | The SDK scripts. `rp6502.cmake` fetches the emulator and tools.              |

The constants for prices, mileages, climate, rivers, and event chances are at the
top of `src/game.c`.

## Updating the SDK tools

`tools/` holds the CMake and Python scripts that the SDK runs. To update them,
run the "RP6502: update tools" task from VS Code, or run:

```bash
cmake -P tools/rp6502.cmake
```

Commit `tools/` with the project. The `build/` directory and `.rp6502` are
ignored by git.
