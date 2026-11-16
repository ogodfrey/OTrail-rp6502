#include <fcntl.h>
#include <rp6502.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "sound.h"
#include "xram.h"

/* Start at block 4; higher notes move up a block rather than overflowing FNUM. */
#define OPL_BLOCK 4
#define OPL_FNUM(hz) ((unsigned)((hz) * 65536L / 49716L))
#define KEY_ON 0x20
#define xreg_ria_opl(...) xreg(0x0, 0x1, 0x01, __VA_ARGS__)

#define TITLE_CHANNEL 0
#define EFFECT_CHANNEL 1

static void opl_poke(unsigned reg, unsigned char val)
{
    xram0_write(XRAM_OPL + reg, &val, 1);
}

enum { VOICE_BANJO, VOICE_BUGLE, VOICE_RIVER, VOICE_CREAK, VOICE_SHOT, VOICE_DRIVER };

/* Paired operator registers followed by channel feedback. Only channels 0 and 1
   are used, whose modulator/carrier slots are c and c+3. */
static void patch(uint8_t c, uint8_t voice)
{
    static const unsigned char regs[] = {
        0x20, 0x23, 0x40, 0x43, 0x60, 0x63, 0x80, 0x83, 0xE0, 0xE3, 0xC0
    };
    static const unsigned char voices[][11] = {
        {0x21, 0x21, 0x1A, 0x08, 0xF9, 0xD6, 0xF0, 0xF4, 0, 0, 0x0A},
        {0x21, 0x21, 0x24, 0x08, 0xA3, 0x92, 0x34, 0x27, 0, 0, 0x04},
        {0x01, 0x02, 0x08, 0x10, 0x84, 0x74, 0x47, 0x47, 0, 0, 0x0E},
        {0x01, 0x01, 0x08, 0x14, 0xB4, 0xD5, 0x46, 0x56, 0, 0, 0x0E},
        {0x0F, 0x01, 0x00, 0x08, 0xF8, 0xF8, 0x08, 0x08, 0, 0, 0x0E},
        {0x22, 0x61, 0x18, 0x0C, 0xE2, 0xC2, 0x47, 0x27, 0, 0, 0x06},
    };
    uint8_t i;

    for (i = 0; i < sizeof regs; i++)
        opl_poke(regs[i] + c, voices[voice][i]);
}

static unsigned char pitch[2];
static uint8_t travel_step;

static void note_on(uint8_t c, unsigned hz)
{
    unsigned f = OPL_FNUM(hz);
    uint8_t block = OPL_BLOCK;

    while (f > 1023) {
        f >>= 1;
        block++;
    }
    pitch[c] = (block << 2) | (f >> 8);

    opl_poke(0xA0 + c, f & 0xFF);
    opl_poke(0xB0 + c, KEY_ON | pitch[c]);
}

static void note_off(uint8_t c)
{
    opl_poke(0xB0 + c, pitch[c]);
}

/* Waits n frames of VSYNC. The register is 8 bits, so the difference wraps. */
static void wait_frames(unsigned n)
{
    unsigned char start = ria_vsync();

    while ((unsigned char)(ria_vsync() - start) < n)
        ;
}

void sound_init(void)
{
    xreg_ria_opl(XRAM_OPL);
    xram0_set(XRAM_OPL, 0, 256);
    patch(TITLE_CHANNEL, VOICE_BANJO);
    patch(EFFECT_CHANNEL, VOICE_RIVER);
    travel_step = 0;
}

/* Plays the tune of a landmark, read from the ROM:tunes asset (src/tunes.dat). Its
   entry is three bytes, the offset and count of the notes, and each note is three bytes. */
#define LANDMARK_COUNT 18
#define EVENT_COUNT 20
#define EFFECT_COUNT 10
#define EFFECT_BASE (LANDMARK_COUNT + EVENT_COUNT)
#define TUNE_ENTRIES (EFFECT_BASE + EFFECT_COUNT)
#define MAX_TUNE_NOTES 20
#define TUNE_ENTRY_BYTES 3
#define TUNE_NOTE_BYTES 3

static void play_tune(uint8_t c, unsigned index)
{
    unsigned char entry[TUNE_ENTRY_BYTES], note[TUNE_NOTE_BYTES];
    unsigned offset;
    uint8_t count, i;
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
        wait_frames(1);
    }
    close(fd);
}

void sound_title(void)
{
    play_tune(TITLE_CHANNEL, 0);
}

void sound_landmark(unsigned index)
{
    patch(TITLE_CHANNEL, VOICE_BUGLE);
    play_tune(TITLE_CHANNEL, index);
    patch(TITLE_CHANNEL, VOICE_BANJO);
}

void sound_event(unsigned index)
{
    play_tune(TITLE_CHANNEL, LANDMARK_COUNT + index);
}

void sound_shot(void)
{
    patch(EFFECT_CHANNEL, VOICE_SHOT);
    note_on(EFFECT_CHANNEL, 80);
    wait_frames(2);
    note_off(EFFECT_CHANNEL);
}

void sound_effect(sfx_t sfx)
{
    switch (sfx) {
    case SFX_RIVER:
        patch(EFFECT_CHANNEL, VOICE_RIVER);
        play_tune(EFFECT_CHANNEL, EFFECT_BASE + 0);
        break;
    case SFX_FORT:
        patch(EFFECT_CHANNEL, VOICE_BUGLE);
        play_tune(EFFECT_CHANNEL, EFFECT_BASE + 1);
        break;
    case SFX_CREAK:
        patch(EFFECT_CHANNEL, VOICE_CREAK);
        play_tune(EFFECT_CHANNEL, EFFECT_BASE + 2);
        if (!(++travel_step & 3)) {
            patch(EFFECT_CHANNEL, VOICE_DRIVER);
            play_tune(EFFECT_CHANNEL, EFFECT_BASE + 8 + ((travel_step >> 2) & 1));
        }
        break;
    case SFX_DEATH:
        play_tune(TITLE_CHANNEL, EFFECT_BASE + 3);
        break;
    case SFX_ARRIVE:
        patch(TITLE_CHANNEL, VOICE_BUGLE);
        play_tune(TITLE_CHANNEL, EFFECT_BASE + 4);
        patch(TITLE_CHANNEL, VOICE_BANJO);
        break;
    case SFX_WEEKLY:
        play_tune(TITLE_CHANNEL, EFFECT_BASE + 5);
        break;
    case SFX_BROKEN:
        play_tune(TITLE_CHANNEL, EFFECT_BASE + 6);
        break;
    case SFX_DYSENTERY:
        play_tune(TITLE_CHANNEL, EFFECT_BASE + 7);
        break;
    }
}
