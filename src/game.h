#ifndef GAME_H
#define GAME_H

#define NAME_LEN 16

typedef struct
{
    long score;
    char leader[NAME_LEN];
} game_result_t;

void game_play(game_result_t *result);

#endif
