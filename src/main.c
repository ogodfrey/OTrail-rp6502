#include <fcntl.h>
#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "game.h"
#include "scene.h"
#include "ui.h"
#include "xram.h"

#define SCORE_FILE "oregon.sco"
#define SCORE_SLOTS 10

typedef struct
{
    long score;
    char name[NAME_LEN];
} score_entry_t;

static score_entry_t high_scores[SCORE_SLOTS];

static void scores_load(void)
{
    int fd = open(SCORE_FILE, O_RDONLY);

    if (fd < 0)
        return;
    read(fd, high_scores, sizeof(high_scores));
    close(fd);
}

static void scores_save(void)
{
    int fd = open(SCORE_FILE, O_WRONLY | O_CREAT | O_TRUNC);

    if (fd < 0)
        return;
    write(fd, high_scores, sizeof(high_scores));
    close(fd);
}

static void scores_add(long score, const char *name)
{
    int i, pos = -1;

    for (i = 0; i < SCORE_SLOTS; ++i)
    {
        if (score > high_scores[i].score)
        {
            pos = i;
            break;
        }
    }
    if (pos < 0)
        return;

    for (i = SCORE_SLOTS - 1; i > pos; --i)
        high_scores[i] = high_scores[i - 1];
    high_scores[pos].score = score;
    strcpy(high_scores[pos].name, name);
    scores_save();
}

static void show_high_scores(void)
{
    int i;

    ui_clear();
    printf("HIGH SCORES\n\n");
    for (i = 0; i < SCORE_SLOTS; ++i)
    {
        if (high_scores[i].score > 0)
            printf("%d) %-16s %ld\n", i + 1, high_scores[i].name, high_scores[i].score);
        else
            printf("%d) ---\n", i + 1);
    }
    ui_pause();
}

static void show_title(void)
{
    ui_clear();
    printf("THE OREGON TRAIL\n\n");
    printf("Independence, Missouri, 1848.\n");
    printf("Lead your party 2040 miles to the Willamette\n");
    printf("Valley in Oregon. Buy supplies, pick a pace,\n");
    printf("hunt, cross rivers, and keep everyone alive.\n\n");
    printf("1) Start a new journey\n2) High scores\n3) Quit\n\n");
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
        .xram_palette_ptr = 0xFFFF,
    };

    xreg_vga_canvas(CANVAS_640X480);
    xram0_write(XRAM_SCENE_CONFIG, &scene_config, sizeof(scene_config));
    xreg_vga_mode3(MODE3_4BPP, XRAM_SCENE_CONFIG, 0, 0, SCENE_HEIGHT_PX);
    xreg_vga_mode0(0, SCENE_HEIGHT_PX);
    srand((unsigned)ria_attr_get(RIA_ATTR_LRAND));
    scores_load();

    for (;;)
    {
        scene_draw(SCENE_PRAIRIE, false);
        show_title();
        switch (ui_ask_int("Choose: ", 1, 3))
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
