#include <fcntl.h>
#include <rp6502.h>
#include <stdio.h>
#include <unistd.h>
#include "scene.h"
#include "xram.h"

/* scenes.dat: the start of each scene, the summer and winter palettes, and
   then the run-length pixels of each scene. */
#define SCENE_COUNT 5
#define PALETTE_BYTES (16 * 2)
#define HEADER_BYTES (4 * SCENE_COUNT + 2 * PALETTE_BYTES)
#define SCENE_BYTES (SCENE_ROW_BYTES * SCENE_HEIGHT_PX)

static unsigned char buf[HEADER_BYTES];

void scene_draw(scene_t scene, bool winter)
{
    int fd = open("ROM:scenes", O_RDONLY);
    unsigned long start;
    unsigned pos = 0;
    int n, i;

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
    while (pos < SCENE_BYTES && (n = read(fd, buf, sizeof buf)) > 0) {
        for (i = 0; i + 1 < n && pos < SCENE_BYTES; i += 2) {
            xram0_set(XRAM_SCENE + pos, buf[i + 1], buf[i]);
            pos += buf[i];
        }
    }
    close(fd);
}
