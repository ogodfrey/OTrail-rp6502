#include <fcntl.h>
#include <rp6502.h>
#include <stdio.h>
#include <unistd.h>
#include "scene.h"
#include "xram.h"

/* scenes.dat: the start of each scene, the summer and winter palettes, and
   then the PackBits pixels of each scene. */
#define SCENE_COUNT 5
#define PALETTE_BYTES (16 * 2)
#define HEADER_BYTES (4 * SCENE_COUNT + 2 * PALETTE_BYTES)
#define SCENE_BYTES (SCENE_ROW_BYTES * SCENE_HEIGHT_PX)

static unsigned char buf[HEADER_BYTES];
static unsigned pos;
static unsigned literal, repeat;
static unsigned char repeat_value;
static bool need_value;

/* Unpacks the n bytes in buf into XRAM. The state carries over from one chunk
   to the next, since a run can cross the end of a chunk. */
static void unpack(unsigned n)
{
    unsigned i = 0, k, c;

    while (pos < SCENE_BYTES) {
        if (need_value) {
            if (i >= n)
                return;
            repeat_value = buf[i++];
            need_value = false;
        }
        if (repeat) {
            k = repeat < SCENE_BYTES - pos ? repeat : SCENE_BYTES - pos;
            xram0_set(XRAM_SCENE + pos, repeat_value, k);
            pos += k;
            repeat -= k;
        } else if (literal) {
            if (i >= n)
                return;
            k = literal < n - i ? literal : n - i;
            k = k < SCENE_BYTES - pos ? k : SCENE_BYTES - pos;
            xram0_write(XRAM_SCENE + pos, buf + i, k);
            pos += k;
            i += k;
            literal -= k;
        } else {
            if (i >= n)
                return;
            c = buf[i++];
            if (c < 128)
                literal = c + 1;
            else {
                repeat = 257 - c;
                need_value = true;
            }
        }
    }
}

void scene_draw(scene_t scene, bool winter)
{
    int fd = open("ROM:scenes", O_RDONLY);
    unsigned long start;
    int n;

    if (fd < 0)
        return;
    if (read(fd, buf, HEADER_BYTES) != HEADER_BYTES) {
        close(fd);
        return;
    }
    xram0_write(XRAM_SCENE_PALETTE, buf + 4 * SCENE_COUNT + (winter ? PALETTE_BYTES : 0), PALETTE_BYTES);
    start = buf[4 * scene] | ((unsigned long)buf[4 * scene + 1] << 8) |
            ((unsigned long)buf[4 * scene + 2] << 16) | ((unsigned long)buf[4 * scene + 3] << 24);
    lseek(fd, (long)start, SEEK_SET);
    pos = 0;
    literal = repeat = 0;
    need_value = false;
    while (pos < SCENE_BYTES && (n = read(fd, buf, sizeof buf)) > 0)
        unpack((unsigned)n);
    close(fd);
}
