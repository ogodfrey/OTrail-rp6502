/* RP6502 XRAM Mapping */

#ifndef XRAM_H
#define XRAM_H

#include <rp6502.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* VGA canvas, from https://picocomputer.github.io/vga.html#canvas-types */
#define xreg_vga_canvas(...) xreg(0x1, 0x0, 0x00, __VA_ARGS__)

#define CANVAS_CONSOLE 0
#define CANVAS_320X240 1
#define CANVAS_320X180 2
#define CANVAS_640X480 3
#define CANVAS_640X360 4
#define CANVAS_320X200 5
#define CANVAS_640X400 6

/* VGA mode 0, from https://picocomputer.github.io/vga.html#mode-0-console */
#define xreg_vga_mode0(...) xreg(0x1, 0x0, 0x01, 0, __VA_ARGS__)

/* VGA mode 3, from https://picocomputer.github.io/vga.html#mode-3-bitmap */
#define xreg_vga_mode3(...) xreg(0x1, 0x0, 0x01, 3, __VA_ARGS__)

#define MODE3_1BPP 0x00
#define MODE3_2BPP 0x01
#define MODE3_4BPP 0x02
#define MODE3_8BPP 0x03
#define MODE3_16BPP 0x04

#define MODE3_REVERSE_BITS 0x08

typedef struct
{
    bool x_wrap;
    bool y_wrap;
    int16_t x_pos_px;
    int16_t y_pos_px;
    int16_t width_px;
    int16_t height_px;
    uint16_t xram_data_ptr;
    uint16_t xram_palette_ptr;
} mode3_config_t; /* layout */

/* The scene is a 640x128 bitmap at 4 bits per pixel, above the console. */
#define SCENE_WIDTH_PX 640
#define SCENE_HEIGHT_PX 128
#define SCENE_ROW_BYTES (SCENE_WIDTH_PX / 2u)

typedef struct
{
    mode3_config_t scene_config;
    uint8_t scene[SCENE_ROW_BYTES * SCENE_HEIGHT_PX];
} xram_layout_t;

#define XRAM_SCENE_CONFIG offsetof(xram_layout_t, scene_config)
#define XRAM_SCENE offsetof(xram_layout_t, scene)

#endif
