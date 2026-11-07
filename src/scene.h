#ifndef SCENE_H
#define SCENE_H

#include <stdbool.h>

typedef enum
{
    SCENE_PRAIRIE,
    SCENE_MOUNTAIN,
    SCENE_RIVER,
    SCENE_FORT,
    SCENE_STORE,
    SCENE_JOURNEY,
    SCENE_CHIMNEY,
    SCENE_INDEPENDENCE
} scene_t;

void scene_draw(scene_t scene, bool winter);
void scene_journey_marker(unsigned miles);

#endif
