#include <fcntl.h>
#include <rp6502.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
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
    unsigned char frames;
} note_t;

/* Sixty frames is one second, the rate of VSYNC. */
static const note_t title_tune[] = {
    {330, 18}, {392, 18}, {440, 18}, {392, 18}, {330, 18}, {294, 18}, {262, 36},
    {330, 18}, {392, 18}, {523, 36}, {440, 18}, {392, 36},
};

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
    opl_poke(0xC0 + c, 0x00);
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
        if (notes[i].hz)
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

/* Plays the tune of a landmark, read from the ROM:tunes asset (src/tunes.dat). Its
   entry is three bytes, the offset and count of the notes, and each note is three bytes. */
#define LANDMARK_COUNT 18
#define EVENT_COUNT 16
#define EFFECT_COUNT 7
#define EFFECT_BASE (LANDMARK_COUNT + EVENT_COUNT)
#define TUNE_ENTRIES (EFFECT_BASE + EFFECT_COUNT)
#define MAX_TUNE_NOTES 20
#define TUNE_ENTRY_BYTES 3
#define TUNE_NOTE_BYTES 3

static void play_tune(unsigned c, unsigned index)
{
    unsigned char entry[TUNE_ENTRY_BYTES], note[TUNE_NOTE_BYTES];
    unsigned offset, count, i;
    int fd;

    if (index >= TUNE_ENTRIES)
        return;
    fd = open("ROM:tunes", O_RDONLY);
    if (fd < 0)
        return;
    lseek(fd, (long)(index * TUNE_ENTRY_BYTES), SEEK_SET);
    if (read(fd, entry, sizeof entry) != sizeof entry) {
        close(fd);
        return;
    }
    offset = entry[0] | ((unsigned)entry[1] << 8);
    count = entry[2];
    if (count == 0 || count > MAX_TUNE_NOTES) {
        close(fd);
        return;
    }
    lseek(fd, (long)offset, SEEK_SET);
    for (i = 0; i < count; i++) {
        unsigned hz;

        if (read(fd, note, sizeof note) != sizeof note)
            break;
        hz = note[0] | ((unsigned)note[1] << 8);
        if (hz)
            note_on(c, hz);
        wait_frames(note[2]);
        note_off(c);
    }
    close(fd);
}

void sound_landmark(unsigned index)
{
    play_tune(TITLE_CHANNEL, index);
}

void sound_event(unsigned index)
{
    play_tune(TITLE_CHANNEL, LANDMARK_COUNT + index);
}

/* The wheel creak: wooden friction and resonance on channel 1, from the RP6502 OPL2
   example. Each pair is an OPL2 register and its value: slow attack and long decay with
   sustain on, and feedback of level 7 for the wooden grit. The notes of the creak are the
   wheel creak tune in tunes.dat, played by play_tune. */
static const unsigned char creak_regs[] = {
    0x21, 0x20, 0x24, 0x21, 0x41, 0x0A, 0x44, 0x05, 0x61, 0x28,
    0x64, 0x4A, 0x81, 0x22, 0x84, 0x24, 0xC1, 0x0E,
};

static void wheel_creak(void)
{
    unsigned i;

    for (i = 0; i < sizeof creak_regs; i += 2)
        opl_poke(creak_regs[i], creak_regs[i + 1]);
    play_tune(EFFECT_CHANNEL, EFFECT_BASE + 2);
    effect_patch(EFFECT_CHANNEL);
}

void sound_effect(sfx_t sfx)
{
    switch (sfx) {
    case SFX_RIVER:
        play_tune(EFFECT_CHANNEL, EFFECT_BASE + 0);
        break;
    case SFX_FORT:
        play_tune(EFFECT_CHANNEL, EFFECT_BASE + 1);
        break;
    case SFX_CREAK:
        wheel_creak();
        break;
    case SFX_DEATH:
        play_tune(TITLE_CHANNEL, EFFECT_BASE + 3);
        break;
    case SFX_ARRIVE:
        play_tune(TITLE_CHANNEL, EFFECT_BASE + 4);
        break;
    case SFX_WEEKLY:
        play_tune(TITLE_CHANNEL, EFFECT_BASE + 5);
        break;
    case SFX_BROKEN:
        play_tune(TITLE_CHANNEL, EFFECT_BASE + 6);
        break;
    }
}
