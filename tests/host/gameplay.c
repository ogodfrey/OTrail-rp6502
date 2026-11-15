#include <assert.h>
#include <string.h>
#define rand test_rand
#include "game.c"
#undef rand

static unsigned char ram[65536];
static unsigned frames, reads, mode, random_calls, animal;
static unsigned max_hz, last_off;
static bool sounded, bugle;
struct host_ria RIA = {0};

int test_rand(void)
{
    unsigned n = random_calls++;
    return n == 0 || n == 2 ? (int)animal : 0;
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

void xram0_read(void *dest, unsigned src, unsigned count)
{
    assert(src + count <= sizeof ram);
    if (src == XRAM_KEYBOARD && count == 32) {
        unsigned char *keys = dest;
        unsigned shot = animal == 5 ? 79 : 53;
        memset(keys, 0, count);
        reads++;
        if (mode == 1) {
            if (reads <= 14) keys[10] |= 4;
            if (reads <= 39) keys[10] |= 1;
            if ((reads >= shot && reads < shot + 8) || reads == shot + 9)
                keys[5] |= 16;
            if (reads == shot + 10) keys[5] |= 2;
        } else if (mode == 2 && reads == 1) {
            keys[5] = 2;
        } else if (mode == 4 && reads <= 300) {
            keys[5] = 16;
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

static void hunt_case(unsigned scenario, unsigned species, int food, int expected)
{
    game_t g = party();
    mode = scenario;
    animal = species;
    reads = random_calls = 0;
    g.food = food;
    do_hunt(&g);
    assert(g.day == 2);
    if (g.food != expected)
        fprintf(stderr, "hunt mode %u species %u: food %d, expected %d; bullets %d\n",
                mode, animal, g.food, expected, g.bullets);
    assert(g.food == expected);
    assert(g.bullets == (mode == 1 ? 1 : mode == 4 ? 2 : 3));
    if (mode == 3 || mode == 4)
        assert(reads == 301);
}

int main(void)
{
    game_t g = party();
    unsigned i;

    scene_hunt_clear();
    scene_hunt_sprite(156, 112, 5, false);
    assert(ram[XRAM_SCENE + SCENE_ROW_BYTES * SCENE_HEIGHT_PX] == 0);
    scene_hunt_sprite(156, 112, 5, true);
    for (i = XRAM_SCENE; i < XRAM_SCENE + SCENE_ROW_BYTES * SCENE_HEIGHT_PX; i++)
        assert(ram[i] == 0);
    scene_hunt_sprite(255, 255, 0, false);
    assert(ram[XRAM_KEYBOARD] == 0);

    hunt_case(1, 0, 100, 86);    /* one rabbit, one miss; holding Space doesn't repeat */
    hunt_case(1, 5, 1850, 1935); /* buffalo: carry only 100 pounds */
    hunt_case(1, 5, 1990, 1985); /* still honor the wagon's food cap */
    hunt_case(2, 0, 100, 85);    /* Escape: no ammunition spent */
    hunt_case(3, 0, 100, 85);    /* timeout without firing */
    hunt_case(4, 0, 100, 85);    /* held Space: only one missed shot */
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
    puts("OK: hunting, recovery, sprite bounds, high notes and bugle restoration");
    return 0;
}
