#include <rp6502.h>
#include "scene.h"
#include "xram.h"

/* Indexes into the built-in 16-color ANSI palette of 4-bit mode 3. */
#define C_BLACK 0
#define C_GREEN 2
#define C_BROWN 3
#define C_BLUE 4
#define C_DARK_GRAY 8
#define C_YELLOW 11
#define C_BRIGHT_BLUE 12
#define C_CYAN 14
#define C_WHITE 15

/* Scene rows, in scanlines of the canvas. */
#define HORIZON 80

/* The bitmap is 4 bits per pixel, so each byte is two pixels of one color.
   All x positions and widths here are in bytes. */
static void span(unsigned row, unsigned x, unsigned width, unsigned char color)
{
    unsigned char pair = (unsigned char)((color << 4) | color);

    if (row >= SCENE_HEIGHT_PX || x >= SCENE_ROW_BYTES)
        return;
    if (width > SCENE_ROW_BYTES - x)
        width = SCENE_ROW_BYTES - x;
    xram0_set(XRAM_SCENE + row * SCENE_ROW_BYTES + x, pair, width);
}

static void rect(unsigned x, unsigned y, unsigned width, unsigned height, unsigned char color)
{
    unsigned row;

    for (row = y; row < y + height; ++row)
        span(row, x, width, color);
}

/* A peak with its apex at peak_row, widening to half-width at base_row. */
static void peak(unsigned cx, unsigned peak_row, unsigned base_row, unsigned half,
                 unsigned char rock, unsigned char snow)
{
    unsigned row, height = base_row - peak_row, half_width;

    for (row = peak_row; row <= base_row; ++row)
    {
        half_width = (row - peak_row) * half / height;
        span(row, cx - half_width, half_width * 2 + 1,
             (row - peak_row) * 4 < height ? snow : rock);
    }
}

static void wagon(void)
{
    rect(154, 56, 32, 6, C_WHITE);   /* canvas top */
    rect(146, 62, 48, 22, C_WHITE);  /* canvas */
    rect(140, 84, 60, 6, C_BROWN);   /* bed */
    rect(120, 86, 22, 3, C_BROWN);   /* tongue */
    rect(146, 90, 10, 10, C_BLACK);  /* rear wheel */
    rect(186, 90, 10, 10, C_BLACK);  /* front wheel */
}

void scene_draw(scene_t scene, bool winter)
{
    unsigned char sky = winter ? C_BRIGHT_BLUE : C_CYAN;
    unsigned char ground = winter ? C_WHITE : C_GREEN;

    rect(0, 0, SCENE_ROW_BYTES, HORIZON, sky);
    rect(0, HORIZON, SCENE_ROW_BYTES, SCENE_HEIGHT_PX - HORIZON, ground);

    switch (scene)
    {
    case SCENE_PRAIRIE:
        rect(270, 8, 12, 12, C_YELLOW);
        break;
    case SCENE_MOUNTAIN:
        peak(100, 12, HORIZON, 80, C_DARK_GRAY, C_WHITE);
        peak(230, 22, HORIZON, 70, C_DARK_GRAY, C_WHITE);
        break;
    case SCENE_RIVER:
        rect(0, 84, SCENE_ROW_BYTES, 24, C_BLUE);
        break;
    case SCENE_FORT:
        rect(70, 40, 180, 40, C_BROWN);
        for (unsigned i = 0; i < 9; ++i)
            rect(70 + i * 22, 34, 12, 6, C_BROWN);
        rect(150, 60, 24, 20, C_BLACK);
        break;
    }

    wagon();
}
