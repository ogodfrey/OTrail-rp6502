#include <rp6502.h>
#include <stdint.h>
#include "sound.h"
#include "xram.h"

/* The OPL2 runs at block 4, the octave of middle C. Its frequency number is
   hz * 2^16 / 49716 for the 3.58 MHz clock, so the highest note is about 776 Hz. */
#define OPL_BLOCK 4
#define OPL_FNUM(hz) ((unsigned)((hz) * 65536L / 49716L))
#define KEY_ON 0x20
#define xreg_ria_opl(...) xreg(0x0, 0x1, 0x01, __VA_ARGS__)

#define TITLE_CHANNEL 0
#define EFFECT_CHANNEL 1

typedef struct
{
    unsigned hz;
    unsigned frames;
} note_t;

/* Sixty frames is one second, the rate of VSYNC. */
static const note_t title_tune[] = {
    {330, 18}, {392, 18}, {440, 18}, {392, 18}, {330, 18}, {294, 18}, {262, 36},
    {330, 18}, {392, 18}, {523, 36}, {440, 18}, {392, 36},
};
static const note_t river_fx[] = {{392, 4}, {349, 4}, {330, 4}, {294, 6}};
static const note_t fort_fx[] = {{523, 6}, {659, 6}, {698, 6}, {698, 6}, {659, 12}};
static const note_t death_fx[] = {{262, 14}, {247, 14}, {233, 24}};
static const note_t arrive_fx[] = {{392, 10}, {523, 10}, {659, 10}, {523, 10}, {659, 10}, {698, 40}};
/* A short wobble of stepped pitches, which sounds like a wheel turning. */
static const note_t creak_fx[] = {{220, 2}, {196, 2}, {233, 2}, {185, 2}};

static void opl_poke(unsigned reg, unsigned char val)
{
    xram0_write(XRAM_OPL + reg, &val, 1);
}

/* A banjo on channel c: a sharp pluck that dies away quickly. The values are
   the OPL2 banjo patch, with a modulator and a carrier in slots c and c+3. */
static void banjo_patch(unsigned c)
{
    unsigned mod = c, car = c + 3;

    opl_poke(0x20 + mod, 0x21);
    opl_poke(0x40 + mod, 0x1A);
    opl_poke(0x60 + mod, 0xF9);
    opl_poke(0x80 + mod, 0xF0);
    opl_poke(0x20 + car, 0x21);
    opl_poke(0x40 + car, 0x00);
    opl_poke(0x60 + car, 0xD6);
    opl_poke(0x80 + car, 0xF4);
    opl_poke(0xC0 + c, 0x0A);
    opl_poke(0xE0 + mod, 0x00);
    opl_poke(0xE0 + car, 0x00);
}

/* One simple patch on channel c for the effects: a modulator and a carrier, slots c and c+3. */
static void effect_patch(unsigned c)
{
    unsigned mod = c, car = c + 3;

    opl_poke(0x20 + mod, 0x01);
    opl_poke(0x40 + mod, 0x10);
    opl_poke(0x60 + mod, 0xF0);
    opl_poke(0x80 + mod, 0x77);
    opl_poke(0x20 + car, 0x01);
    opl_poke(0x40 + car, 0x00);
    opl_poke(0x60 + car, 0xF0);
    opl_poke(0x80 + car, 0x77);
}

static void note_on(unsigned c, unsigned hz)
{
    unsigned f = OPL_FNUM(hz);

    opl_poke(0xA0 + c, f & 0xFF);
    opl_poke(0xB0 + c, KEY_ON | (OPL_BLOCK << 2) | (f >> 8));
}

static void note_off(unsigned c)
{
    opl_poke(0xB0 + c, OPL_BLOCK << 2);
}

/* Waits n frames of VSYNC. The register is 8 bits, so the difference wraps. */
static void wait_frames(unsigned n)
{
    unsigned char start = ria_vsync();

    while ((unsigned char)(ria_vsync() - start) < n)
        ;
}

static void play(unsigned c, const note_t *notes, unsigned count)
{
    unsigned i;

    for (i = 0; i < count; i++) {
        note_on(c, notes[i].hz);
        wait_frames(notes[i].frames);
        note_off(c);
    }
}

void sound_init(void)
{
    xreg_ria_opl(XRAM_OPL);
    banjo_patch(TITLE_CHANNEL);
    effect_patch(EFFECT_CHANNEL);
}

void sound_title(void)
{
    play(TITLE_CHANNEL, title_tune, sizeof title_tune / sizeof title_tune[0]);
}

void sound_effect(sfx_t sfx)
{
    switch (sfx) {
    case SFX_RIVER:
        play(EFFECT_CHANNEL, river_fx, sizeof river_fx / sizeof river_fx[0]);
        break;
    case SFX_FORT:
        play(EFFECT_CHANNEL, fort_fx, sizeof fort_fx / sizeof fort_fx[0]);
        break;
    case SFX_DEATH:
        play(EFFECT_CHANNEL, death_fx, sizeof death_fx / sizeof death_fx[0]);
        break;
    case SFX_ARRIVE:
        play(EFFECT_CHANNEL, arrive_fx, sizeof arrive_fx / sizeof arrive_fx[0]);
        break;
    case SFX_CREAK:
        play(EFFECT_CHANNEL, creak_fx, sizeof creak_fx / sizeof creak_fx[0]);
        break;
    }
}
