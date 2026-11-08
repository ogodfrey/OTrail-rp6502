#include <fcntl.h>
#include <rp6502.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <rp6502.h>
#include <stdio.h>
#include <unistd.h>
#include "scene.h"
#include "xram.h"

/* The miles of the whole trail. The journey map's route covers all of them. */
#define JOURNEY_MILES 2040

/* scenes.dat: the start of each scene, the summer and winter palettes, the
   journey route, and then the LZ-packed pixels of each scene. The route is
   24 points of two 16-bit values, from Independence to Willamette. The header
   is read in pieces of buf, so buf is no bigger than the largest piece. */
#define SCENE_COUNT 39
#define PALETTE_BYTES (16 * 2)
#define OFFSETS_BYTES (4 * SCENE_COUNT)
#define ROUTE_OFFSET (OFFSETS_BYTES + 2 * PALETTE_BYTES)
#define JOURNEY_POINTS 24
#define SCENE_BYTES (SCENE_ROW_BYTES * SCENE_HEIGHT_PX)

static unsigned char buf[32];
static unsigned char copy_buf[16];
static unsigned pos;
static unsigned literal, match_len, match_dist;
static unsigned char dist_bytes;

/* Unpacks the n bytes in buf into XRAM. The state carries over from one chunk
   to the next, since a token, a literal, or a distance can cross the end of a chunk.
   A match copies bytes already in XRAM, in pieces no longer than its distance, so
   an overlapping match repeats its bytes correctly. */
static void unpack(unsigned n)
{
    unsigned i = 0, k, c;

    while (pos < SCENE_BYTES) {
        if (literal) {
            if (i >= n)
                return;
            k = literal < n - i ? literal : n - i;
            k = k < SCENE_BYTES - pos ? k : SCENE_BYTES - pos;
            xram0_write(XRAM_SCENE + pos, buf + i, k);
            pos += k;
            i += k;
            literal -= k;
        } else if (match_len) {
            if (dist_bytes < 2) {
                if (i >= n)
                    return;
                match_dist |= (unsigned)buf[i++] << (8 * dist_bytes);
                dist_bytes++;
                continue;
            }
            k = match_len;
            if (k > SCENE_BYTES - pos)
                k = SCENE_BYTES - pos;
            if (k > sizeof copy_buf)
                k = sizeof copy_buf;
            if (k > match_dist)
                k = match_dist;
            xram0_read(copy_buf, XRAM_SCENE + pos - match_dist, k);
            xram0_write(XRAM_SCENE + pos, copy_buf, k);
            pos += k;
            match_len -= k;
        } else {
            if (i >= n)
                return;
            c = buf[i++];
            if (c < 128)
                literal = c + 1;
            else {
                match_len = c - 128 + 4;
                match_dist = 0;
                dist_bytes = 0;
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
    lseek(fd, 4L * scene, SEEK_SET);
    if (read(fd, buf, 4) != 4) {
        close(fd);
        return;
    }
    start = buf[0] | ((unsigned long)buf[1] << 8) |
            ((unsigned long)buf[2] << 16) | ((unsigned long)buf[3] << 24);
    lseek(fd, OFFSETS_BYTES + (winter ? PALETTE_BYTES : 0), SEEK_SET);
    if (read(fd, buf, PALETTE_BYTES) == PALETTE_BYTES)
        xram0_write(XRAM_SCENE_PALETTE, buf, PALETTE_BYTES);
    lseek(fd, (long)start, SEEK_SET);
    pos = 0;
    literal = match_len = match_dist = 0;
    dist_bytes = 2;
    while (pos < SCENE_BYTES && (n = read(fd, buf, sizeof buf)) > 0)
        unpack((unsigned)n);
    close(fd);
}

/* Palette indexes of the marker: the ink border and the white center. */
#define MARK_INK 0
#define MARK_WHITE 4

/* Fills count bytes of one scanline from byte column xb with a color. */
static void fill_bytes(int row, int xb, int count, unsigned char color)
{
    unsigned char pair = (unsigned char)((color << 4) | color);

    if (row < 0 || row >= SCENE_HEIGHT_PX || xb < 0 || xb >= SCENE_ROW_BYTES)
        return;
    if (count > SCENE_ROW_BYTES - xb)
        count = SCENE_ROW_BYTES - xb;
    xram0_set(XRAM_SCENE + row * SCENE_ROW_BYTES + xb, pair, count);
}

/* Draws the wagon marker on the journey map, at the point of the route that
   the miles traveled reach. Call it after scene_draw(SCENE_JOURNEY). */
void scene_journey_marker(unsigned miles)
{
    unsigned t, s, rem, frac, i;
    unsigned char p[8];
    int x0, y0, x1, y1, x, y, row, bx, fd;

    if (miles > JOURNEY_MILES)
        miles = JOURNEY_MILES;
    /* Sixteen-bit arithmetic only, which keeps the marker out of the 32-bit
       library routines. t spans 0 to 46920, and frac is 0 to 255. */
    t = miles * (JOURNEY_POINTS - 1);
    s = t / JOURNEY_MILES;
    rem = t % JOURNEY_MILES;
    frac = rem / 8;
    if (s >= JOURNEY_POINTS - 1) {
        s = JOURNEY_POINTS - 2;
        frac = 255;
    }

    fd = open("ROM:scenes", O_RDONLY);
    if (fd < 0)
        return;
    lseek(fd, (long)(ROUTE_OFFSET + 4 * s), SEEK_SET);
    if (read(fd, p, sizeof p) != sizeof p) {
        close(fd);
        return;
    }
    close(fd);
    x0 = p[0] | (p[1] << 8);
    y0 = p[2] | (p[3] << 8);
    x1 = p[4] | (p[5] << 8);
    y1 = p[6] | (p[7] << 8);
    x = x0 + (((x1 - x0) * (int)(frac >> 3)) >> 5);
    y = y0 + (((y1 - y0) * (int)(frac >> 3)) >> 5);

    bx = x / 2 - 2;
    for (i = 0; i < 8; i++) {
        row = y - 4 + (int)i;
        if (i == 0 || i == 7) {
            fill_bytes(row, bx, 4, MARK_INK);
        } else {
            fill_bytes(row, bx, 1, MARK_INK);
            fill_bytes(row, bx + 1, 2, MARK_WHITE);
            fill_bytes(row, bx + 3, 1, MARK_INK);
        }
    }
}
