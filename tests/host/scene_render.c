#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scene.h"
#include "xram.h"

extern unsigned char xram[65536];

/* Usage: scene_render SCENE WINTER OUTFILE. Writes the palette (32 bytes)
   and the scene pixels (packed, two to a byte) that scene_draw put in XRAM. */
int main(int argc, char **argv)
{
    FILE *f;

    if (argc != 4)
        return 2;
    memset(xram, 0, sizeof xram);
    scene_draw((scene_t)atoi(argv[1]), atoi(argv[2]) != 0);
    f = fopen(argv[3], "wb");
    if (!f)
        return 1;
    fwrite(xram + XRAM_SCENE_PALETTE, 1, 2 * 16, f);
    fwrite(xram + XRAM_SCENE, 1, SCENE_ROW_BYTES * SCENE_HEIGHT_PX, f);
    fclose(f);
    return 0;
}
