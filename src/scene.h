#ifndef SCENE_H
#define SCENE_H

#include <stdbool.h>

typedef enum
{
    SCENE_PRAIRIE,
    SCENE_MOUNTAIN,
    SCENE_RIVER,
    SCENE_FORT
} scene_t;

void scene_draw(scene_t scene, bool winter);

#endif
