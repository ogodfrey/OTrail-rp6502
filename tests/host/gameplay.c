#include <assert.h>
#include <string.h>
#define rand test_rand
#include "game.c"
#undef rand

static unsigned char ram[65536];
static unsigned frames, reads, mode, animal, random_calls;
static unsigned max_hz, last_off;
static bool sounded, bugle;
static bool travel_audio;
static unsigned creak_notes;
static unsigned effect_off_frame, pcm_bytes, pcm_writes;
static int expected_voice = -1;
static const unsigned *event_rolls;
static unsigned event_roll_count;
static int aim_x, aim_y, target_x[3];
struct host_ria RIA = {0};

int test_rand(void)
{
    unsigned n = random_calls++;
    if (mode == 17) {
        assert(n < event_roll_count);
        return event_rolls[n];
    }
    if (mode == 8)
        return n % 4 == 3 ? 4 : 0;
    if (mode == 9 || mode == 10)
        return 4;
    if (mode == 11)
        return 0;
    if (mode == 12)
        return 999;
    if (mode == 13)
        return 2;
    if (mode == 14)
        return n < 2 ? 999 : (n & 1 ? 2 : 0);
    if (mode == 15 || mode == 16)
        return n & 1 ? (int)animal : mode == 15 ? 4 : 1;
    return animal == 5 ? 3 : (int)animal;
}

unsigned char ria_vsync(void)
{
    return ++frames;
}

long ria_attr_get(unsigned char id)
{
    (void)id;
    return 0;
}

int xreg(char device, char channel, unsigned char index, ...)
{
    (void)device;
    (void)channel;
    (void)index;
    return 0;
}

void xram0_set(unsigned dest, unsigned char value, unsigned count)
{
    assert(dest + count <= sizeof ram);
    memset(ram + dest, value, count);
}

void xram0_write(unsigned dest, const void *src, unsigned count)
{
    assert(dest + count <= sizeof ram);
    memcpy(ram + dest, src, count);
    if (travel_audio) {
        unsigned reg = dest - XRAM_OPL;
        assert(count == 1);
        assert(reg == 0x21 || reg == 0x24 || reg == 0x41 || reg == 0x44 ||
               reg == 0x61 || reg == 0x64 || reg == 0x81 || reg == 0x84 ||
               reg == 0xE1 || reg == 0xE4 || reg == 0xC1 ||
               reg == 0xA1 || reg == 0xB1);
        if (reg == 0xB1 && (ram[dest] & 0x20)) {
            static const unsigned expected_hz[] = {330, 370, 415, 392, 349};
            unsigned b = ram[dest];
            unsigned f = ram[XRAM_OPL + 0xA1] | ((b & 3) << 8);
            unsigned hz = (unsigned)((unsigned long)f * 49716 >> (20 - ((b >> 2) & 7)));
            unsigned expected = expected_hz[creak_notes % 5];

            assert(ram[XRAM_OPL + 0x21] == 0x03 && ram[XRAM_OPL + 0x24] == 0x01);
            assert(ram[XRAM_OPL + 0x41] == 0x24 && ram[XRAM_OPL + 0x44] == 0x10);
            assert(ram[XRAM_OPL + 0x61] == 0xA4 && ram[XRAM_OPL + 0x64] == 0xA5);
            assert(ram[XRAM_OPL + 0x81] == 0x56 && ram[XRAM_OPL + 0x84] == 0x56);
            assert(ram[XRAM_OPL + 0xC1] == 0x04);
            assert(hz >= expected - 2 && hz <= expected);
            creak_notes++;
        }
        if (reg == 0xB1 && !(ram[dest] & 0x20))
            effect_off_frame = frames;
    }
    if (dest == XRAM_OPL + 0xB0 && count == 1) {
        unsigned b = ram[dest];
        if (b & 0x20) {
            unsigned f = ram[XRAM_OPL + 0xA0] | ((b & 3) << 8);
            unsigned hz = (unsigned)((unsigned long)f * 49716 >> (20 - ((b >> 2) & 7)));
            if (sounded)
                assert(frames > last_off);
            sounded = true;
            if (ram[XRAM_OPL + 0x40] == 0x24)
                bugle = true;
            if (hz > max_hz)
                max_hz = hz;
        } else {
            last_off = frames;
        }
    }
}

int read_xram(unsigned xaddr, unsigned count, int fildes)
{
    assert(xaddr == XRAM_PCM && count == PCM_CHUNK);
    assert(expected_voice >= 0);
    assert(frames - effect_off_frame >= 6);
    return read(fildes, ram + xaddr, count);
}

int write_xram(unsigned xaddr, unsigned count, int fildes)
{
    unsigned char expected[37];
    int n;

    assert(xaddr >= XRAM_PCM && xaddr + count <= XRAM_PCM + PCM_CHUNK);
    assert(expected_voice >= 0);
    if (pcm_writes++ % 4 == 0)
        return 0; /* A full PCM FIFO must not drop or duplicate samples. */
    if (count > sizeof expected)
        count = sizeof expected;
    n = write(fildes, ram + xaddr, count);
    assert(n > 0);
    assert(read(expected_voice, expected, n) == n);
    assert(memcmp(expected, ram + xaddr, n) == 0);
    pcm_bytes += n;
    return n;
}

static int first_color(unsigned color, unsigned top, unsigned bottom, bool vertical)
{
    unsigned row, x;
    for (row = top; row < bottom; row++)
        for (x = 0; x < SCENE_ROW_BYTES; x++)
            if (ram[XRAM_SCENE + row * SCENE_ROW_BYTES + x] == color)
                return vertical ? (int)row : (int)x * 2;
    return -1;
}

void xram0_read(void *dest, unsigned src, unsigned count)
{
    assert(src + count <= sizeof ram);
    if (src == XRAM_KEYBOARD && count == 32) {
        unsigned char *keys = dest;
        unsigned shot = animal == 5 ? 79 : 53;
        memset(keys, 0, count);
        reads++;
        if (mode == 1 || mode == 9) {
            if (reads <= 14) keys[10] |= 4;
            if (reads <= 39) {
                if (mode == 9) keys[9] |= 128;
                else keys[10] |= 1;
            }
            if ((reads >= shot && reads < shot + 8) || reads == shot + 9)
                keys[5] |= 16;
            if (reads == shot + 10) keys[5] |= 2;
        } else if (mode == 10) {
            if (reads <= 3) keys[10] |= 4;
            if (reads <= 18) keys[9] |= 128;
            if (reads >= 53 && reads <= 63 && (reads & 1)) keys[5] |= 16;
            if (reads == 64) keys[5] |= 2;
        } else if (mode == 2 && reads == 1) {
            keys[5] = 2;
        } else if (mode == 4 && reads <= 300) {
            keys[5] = 16;
        } else if (mode == 5) {
            if (reads == 2) {
                unsigned lane;
                aim_x = first_color(0x44, 0, 128, false);
                aim_y = first_color(0x44, 0, 128, true);
                assert(aim_x >= 0);
                for (lane = 0; lane < 3; lane++) {
                    target_x[lane] = first_color(0x66, lane * 40, lane * 40 + 16, false);
                    assert(target_x[lane] >= 0);
                }
                keys[9] = 128;
                keys[10] = 2;
            } else if (reads == 3) {
                int dx = first_color(0x44, 0, 128, false) - aim_x;
                int dy = first_color(0x44, 0, 128, true) - aim_y;
                unsigned lane;
                int moved = first_color(0x66, 0, 16, false) - target_x[0];
                assert(dx == 16 && dy == 16 && moved == 12);
                assert(dx > moved && dy > moved);
                for (lane = 1; lane < 3; lane++)
                    assert(first_color(0x66, lane * 40, lane * 40 + 16, false) -
                           target_x[lane] == moved);
            } else if (reads == 4) {
                keys[5] = 2;
            }
        } else if (mode == 6) {
            if (reads == 15 || reads == 17)
                keys[5] = 16;
            if (reads == 16) {
                assert(hunt_kills == 0 && hunt_meat == 0);
                keys[9] = 128;
            }
            if (reads == 18) {
                assert(hunt_kills == 1 && hunt_meat == 62);
                keys[5] = 2;
            }
        } else if (mode == 7) {
            if (reads <= 3) keys[10] |= 4;
            if (reads <= 19) keys[10] |= 1;
            if (reads == shot || reads == shot + 2) keys[5] |= 16;
            if (reads == shot + 3) keys[5] |= 2;
        } else if (mode == 8) {
            unsigned lane;
            if (reads == 2)
                for (lane = 0; lane < 3; lane++) {
                    target_x[lane] = first_color(0x66, lane * 40, lane * 40 + 16, false);
                    assert(target_x[lane] >= 0);
                }
            if (reads == 3)
                for (lane = 0; lane < 3; lane++)
                    assert(first_color(0x66, lane * 40, lane * 40 + 16, false) -
                           target_x[lane] == (lane == 1 ? -12 : 12));
            if (reads == 37)
                assert(first_color(0x66, 40, 56, false) == -1);
            if (reads == 38) {
                int entered = first_color(0x66, 40, 56, false);
                assert(entered >= 0 && entered < 16);
            }
            if (reads == 53)
                assert(first_color(0x66, 0, 16, false) == -1);
            if (reads == 54)
                assert(first_color(0x66, 0, 16, false) > 600);
            if (reads == 55)
                keys[5] = 2;
        }
        return;
    }
    memcpy(dest, ram + src, count);
}

static game_t party(void)
{
    game_t g = {0};
    int i;
    g.np = 5;
    g.month = 3;
    g.day = 1;
    g.food = 100;
    g.clothing = 5;
    g.bullets = 3;
    g.leg = 0;
    for (i = 0; i < g.np; i++)
        g.party[i].affl = -1;
    return g;
}

static void event_timing_case(void)
{
    static const unsigned food_rolls[] = {999, 0, 999, 999, 999, 0, 999, 999, 999, 999};
    static const unsigned health_rolls[] = {999, 0, 999, 999, 100, 1, 0, 999, 999, 999, 999, 999};
    static const unsigned fruit_rolls[] = {999, 999, 999, 999, 999, 30, 999, 999};
    game_t g = party();

    mode = 17;
    event_rolls = food_rolls;
    event_roll_count = sizeof food_rolls / sizeof *food_rolls;
    random_calls = 0;
    g.ox2 = 8;
    g.tm = 3;
    g.as = FP(31.0);
    g.ar = FP(1.0);
    g.food = 1;
    run_events(&g); /* Snowbound rest consumes the last food before event 5. */
    assert(g.day == 2 && g.food == 0);
    assert(random_calls == event_roll_count);

    g = party();
    event_rolls = health_rolls;
    event_roll_count = sizeof health_rolls / sizeof *health_rolls;
    random_calls = 0;
    g.ox2 = 8;
    g.tm = 3;
    g.as = FP(31.0);
    g.ar = FP(1.0);
    g.health = FP(139.0);
    run_events(&g); /* Rest lowers health stress, but today's illness roll stays fixed. */
    assert(g.health < FP(139.0) && g.party[1].affl == 3);
    assert(random_calls == event_roll_count);

    g = party();
    g.ox2 = 8;
    g.tm = 3;
    g.ar = FP(1.0);
    event_rolls = fruit_rolls;
    event_roll_count = sizeof fruit_rolls / sizeof *fruit_rolls;
    start_leg(&g); /* April departure, then the calendar enters May on this leg. */
    g.has_grave = false;
    g.month = 4;
    random_calls = 0;
    run_events(&g);
    assert(g.food == 100);
    g.lm = 1;
    start_leg(&g);
    g.has_grave = false;
    random_calls = 0;
    run_events(&g);
    assert(g.food == 120);
    g.month = 9;
    random_calls = 0;
    run_events(&g);
    assert(g.food == 140);
    g.lm = 2;
    start_leg(&g);
    g.has_grave = false;
    random_calls = 0;
    run_events(&g);
    assert(g.food == 140);
}

static void hunt_case(unsigned scenario, unsigned species, int food, int expected)
{
    game_t g = party();
    mode = scenario;
    animal = species;
    reads = random_calls = 0;
    g.food = food;
    if (mode == 10)
        g.bullets = 10;
    do_hunt(&g);
    assert(g.day == 2 && !g.said);
    if (g.food != expected)
        fprintf(stderr, "hunt mode %u species %u: food %d, expected %d; bullets %d\n",
                mode, animal, g.food, expected, g.bullets);
    assert(g.food == expected);
    assert(g.bullets == (mode == 10 ? 4 :
                        mode == 1 || mode == 6 || mode == 7 || mode == 9 ? 1 :
                        mode == 4 ? 2 : 3));
    if (mode == 10)
        assert(hunt_kills == 6 && hunt_meat == 30);
    if (mode == 9)
        assert(hunt_kills == 1 && hunt_meat == 5);
    if (mode == 7)
        assert(hunt_kills == 2 && hunt_meat == (animal == 5 ? 1706 : 2));
    if (mode == 3 || mode == 4)
        assert(reads == 301);
}

static void loot_case(unsigned scenario, int spares, int clothing, int bullets, int after)
{
    game_t g = party();
    unsigned i;
    mode = scenario;
    random_calls = 0;
    for (i = 0; i < PART_COUNT; i++)
        g.spares[i] = spares;
    printf("LOOT %u\n", scenario);
    abandoned_wagon(&g);
    puts("END LOOT");
    assert(g.said && g.clothing == 5 + clothing && g.bullets == 3 + bullets);
    assert(g.food == 100 && g.ox2 == 0);
    for (i = 0; i < PART_COUNT; i++)
        assert(g.spares[i] == after);
}

static void menu_case(int landmark, bool at_landmark)
{
    game_t g = party();
    landmark_t here;

    landmark_read(landmark, &here, NULL);
    g.lm = landmark;
    g.leg = here.leg;
    g.dist = 50 * FX;
    g.ox2 = 8;
    printf("MENU %d %d\n", landmark, at_landmark);
    assert(action_menu(&g, at_landmark));
    puts("END MENU");
}

static void measles_case(void)
{
    game_t g = party();
    unsigned roll;

    mode = 15;
    animal = 3;
    random_calls = 0;
    strcpy(g.party[4].name, "Ruth");
    illness(&g);
    assert(g.party[4].affl == AFFLICTION_MEASLES);
    g.party[4].affl_days = 1;
    day_tick(&g, false);
    assert(g.party[4].affl == -1);
    random_calls = 0;
    illness(&g);
    assert(g.party[4].affl != AFFLICTION_MEASLES);

    g.party[4].affl = -1;
    member_die(&g, 1, "drowning");
    assert(strcmp(g.party[1].name, "Ruth") == 0);
    mode = 16;
    for (roll = 0; roll < 5; roll++) {
        g.party[1].affl = -1;
        animal = roll;
        random_calls = 0;
        illness(&g);
        assert(g.party[1].affl == (int)(roll + 3 + (roll >= 3)));
    }

    g = party();
    mode = 15;
    animal = 3;
    random_calls = 0;
    illness(&g);
    assert(g.party[4].affl == AFFLICTION_MEASLES);
    mode = 16;
    random_calls = 0;
    illness(&g);
    assert(g.party[1].affl == AFFLICTION_MEASLES);
    random_calls = 0;
    illness(&g);
    assert(g.np == 4);
}

int main(void)
{
    game_t g = party();
    unsigned i, row, x;
    static unsigned char forest[SCENE_ROW_BYTES * SCENE_HEIGHT_PX];
    char format[80];
    const char *text = msg_get(33);
    const char *label;

    strcpy(format, text);
    label = msg_label(MSG_MONTH);
    assert(strcmp(text, format) == 0);
    msg_get(34);
    assert(strcmp(label, "January") == 0);
    assert(ui_ask_int(msg_get(55), 1, 3) == 2);

    menu_case(0, true);
    menu_case(0, false);
    menu_case(3, true);
    menu_case(3, false);
    menu_case(4, true);

    loot_case(11, 2, 1, 21, 3);
    loot_case(13, 2, 3, 63, 2); /* rolled spares do not fit; never report them */
    loot_case(12, 0, 0, 0, 0);
    loot_case(14, 1, 0, 0, 1);  /* only rejected spares: still an empty wagon */
    measles_case();
    event_timing_case();

    scene_hunt_clear();
    scene_hunt_sprite(156, 112, 5, false);
    assert(ram[XRAM_SCENE + SCENE_ROW_BYTES * SCENE_HEIGHT_PX] == 0);
    scene_hunt_sprite(156, 112, 5, true);
    for (i = XRAM_SCENE; i < XRAM_SCENE + SCENE_ROW_BYTES * SCENE_HEIGHT_PX; i++)
        assert(ram[i] == 0);
    scene_hunt_sprite(255, 255, 0, false);
    assert(ram[XRAM_KEYBOARD] == 0);

    for (i = 0; i < 6; i++) {
        scene_hunt_clear();
        scene_hunt_sprite(10, 0, i, false);
        scene_hunt_sprite(30, 0, i | HUNT_FACE_LEFT, false);
        for (row = 0; row < 16; row++)
            for (x = 0; x < 8; x++)
                assert(ram[XRAM_SCENE + row * SCENE_ROW_BYTES + 20 + x] ==
                       ram[XRAM_SCENE + row * SCENE_ROW_BYTES + 60 + 7 - x]);
    }
    scene_hunt_clear();
    scene_hunt_trees();
    memcpy(forest, ram + XRAM_SCENE, sizeof forest);
    for (row = 0; row < SCENE_HEIGHT_PX; row++)
        for (x = 0; x < SCENE_WIDTH_PX / 4; x++)
            assert(scene_hunt_cover(x, row) ==
                   (ram[XRAM_SCENE + row * SCENE_ROW_BYTES + x * 2] != 0));
    scene_hunt_sprite(78, 56, 6, false);
    assert(ram[XRAM_SCENE + 64 * SCENE_ROW_BYTES + 160] == 0xAA);
    scene_hunt_sprite(78, 56, 6, true);
    scene_hunt_trees();
    assert(memcmp(forest, ram + XRAM_SCENE, sizeof forest) == 0);

    hunt_case(1, 0, 100, 86);    /* one rabbit, one miss; holding Space doesn't repeat */
    hunt_case(1, 5, 1850, 1935); /* buffalo: carry only 100 pounds */
    hunt_case(1, 5, 1990, 1985); /* still honor the wagon's food cap */
    hunt_case(2, 0, 100, 85);    /* Escape: no ammunition spent */
    hunt_case(3, 0, 100, 85);    /* timeout without firing */
    hunt_case(4, 0, 100, 85);    /* held Space: only one missed shot */
    hunt_case(5, 0, 100, 85);    /* faster aiming and three visible moving targets */
    hunt_case(6, 2, 100, 147);   /* covered shot misses; clear shot hits the middle target */
    hunt_case(7, 0, 100, 87);    /* successive hits accumulate meat */
    hunt_case(7, 5, 1850, 1935); /* earned meat keeps rising beyond the carry limit */
    hunt_case(8, 0, 100, 85);    /* simultaneous opposite travel, edge exits and respawns */
    hunt_case(9, 0, 100, 90);    /* left-moving mirrored animal can be hit and scored */
    hunt_case(10, 0, 100, 115);  /* six rabbits: 30 pounds harvested, 15 eaten */
    hunt_case(1, 0, 0, 0);      /* report actual consumption when food runs out */
    g.bullets = 0;
    do_hunt(&g);
    assert(g.day == 1 && g.food == 100);

    g.party[0].affl = 0;
    g.party[0].affl_days = 1;
    strcpy(g.party[0].name, "Amos");
    g.party[1].affl = AFFLICTION_DYSENTERY;
    g.party[1].affl_days = 2;
    strcpy(g.party[1].name, "Clara");
    day_tick(&g, false);
    assert(g.party[0].affl == -1 && g.party[1].affl >= 0);
    day_tick(&g, false);
    assert(g.party[1].affl == -1 && g.said);
    day_tick(&g, false);

    sound_init();
    sound_event(EVENT_INDIANS_FOOD);
    assert(max_hz >= 780 && max_hz <= 786); /* 784 Hz must not overflow FNUM */
    sound_landmark(3);
    assert(bugle && ram[XRAM_OPL + 0x40] == 0x1A);
    sound_effect(SFX_ARRIVE);
    assert(ram[XRAM_OPL + 0x40] == 0x1A);
    sound_init();
    travel_audio = true;
    x = random_calls;
    for (i = 0; i < 8; i++) {
        unsigned char end;

        pcm_bytes = pcm_writes = 0;
        if (i % 4 == 3) {
            expected_voice = open(i == 3 ? "rom_hyah" : "rom_getalong", O_RDONLY);
            assert(expected_voice >= 0);
        }
        sound_effect(SFX_CREAK);
        assert(creak_notes == (i + 1) * 5);
        if (i % 4 == 3) {
            assert(pcm_bytes > 0 && pcm_writes > 0);
            if (i == 7)
                assert(pcm_bytes > 4096);
            assert(read(expected_voice, &end, 1) == 0);
            close(expected_voice);
            expected_voice = -1;
        } else {
            assert(pcm_bytes == 0 && pcm_writes == 0);
        }
    }
    travel_audio = false;
    assert(random_calls == x);
    puts("OK: store availability, measles immunity, hunting, loot, recovery and trail audio");
    return 0;
}
