#include <fcntl.h>
#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "game.h"
#include "scene.h"
#include "ui.h"
#include "msg.h"
#include "xram.h"

#define SCORE_FILE "oregon.sco"
#define SCORE_SLOTS 10

typedef struct
{
    long score;
    char name[NAME_LEN];
} score_entry_t;

static void scores_init(void)
{
    score_entry_t blank;
    int fd, i;

    fd = open(SCORE_FILE, O_RDONLY);
    if (fd >= 0) {
        close(fd);
        return;
    }
    memset(&blank, 0, sizeof(blank));
    fd = open(SCORE_FILE, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0)
        return;
    for (i = 0; i < SCORE_SLOTS; ++i)
        write(fd, &blank, sizeof(blank));
    close(fd);
}

static void scores_add(long score, const char *name)
{
    score_entry_t e;
    int i, pos = -1, fd;

    scores_init();
    fd = open(SCORE_FILE, O_RDWR);
    if (fd < 0)
        return;
    for (i = 0; i < SCORE_SLOTS && pos < 0; ++i) {
        read(fd, &e, sizeof(e));
        if (score > e.score)
            pos = i;
    }
    if (pos < 0) {
        close(fd);
        return;
    }
    for (i = SCORE_SLOTS - 1; i > pos; --i) {
        lseek(fd, (long)(i - 1) * sizeof(e), SEEK_SET);
        read(fd, &e, sizeof(e));
        lseek(fd, (long)i * sizeof(e), SEEK_SET);
        write(fd, &e, sizeof(e));
    }
    memset(&e, 0, sizeof(e));
    e.score = score;
    strcpy(e.name, name);
    lseek(fd, (long)pos * sizeof(e), SEEK_SET);
    write(fd, &e, sizeof(e));
    close(fd);
}

static void show_high_scores(void)
{
    score_entry_t e;
    int i, fd;

    scores_init();
    fd = open(SCORE_FILE, O_RDONLY);
    ui_clear();
    printf(msg_get(139));
    for (i = 0; i < SCORE_SLOTS; ++i)
    {
        if (fd >= 0 && read(fd, &e, sizeof(e)) == (int)sizeof(e) && e.score > 0)
            printf(msg_get(140), i + 1, e.name, e.score);
        else
            printf(msg_get(141), i + 1);
    }
    if (fd >= 0)
        close(fd);
    ui_pause();
}

static void show_title(void)
{
    ui_clear();
    printf(msg_get(142));
    printf(msg_get(143));
    printf(msg_get(144));
    printf(msg_get(145));
    printf(msg_get(146));
    printf(msg_get(147));
}

int main(void)
{
    mode3_config_t scene_config = {
        .x_wrap = false,
        .y_wrap = false,
        .x_pos_px = 0,
        .y_pos_px = 0,
        .width_px = SCENE_WIDTH_PX,
        .height_px = SCENE_HEIGHT_PX,
        .xram_data_ptr = XRAM_SCENE,
        .xram_palette_ptr = XRAM_SCENE_PALETTE,
    };

    xreg_vga_canvas(CANVAS_640X480);
    xram0_write(XRAM_SCENE_CONFIG, &scene_config, sizeof(scene_config));
    xreg_vga_mode3(MODE3_4BPP, XRAM_SCENE_CONFIG, 0, 0, SCENE_HEIGHT_PX);
    xreg_vga_mode0(0, SCENE_HEIGHT_PX);
    srand((unsigned)ria_attr_get(RIA_ATTR_LRAND));

    for (;;)
    {
        scene_draw(SCENE_PRAIRIE, false);
        show_title();
        switch (ui_ask_int(msg_get(148), 1, 3))
        {
        case 1:
        {
            game_result_t result;

            game_play(&result);
            scores_add(result.score, result.leader);
            show_high_scores();
            break;
        }
        case 2:
            show_high_scores();
            break;
        case 3:
            return 0;
        }
    }
}
