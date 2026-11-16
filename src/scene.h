#ifndef SCENE_H
#define SCENE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    SCENE_PRAIRIE,
    SCENE_MOUNTAIN,
    SCENE_RIVER,
    SCENE_FORT,
    SCENE_STORE,
    SCENE_JOURNEY,
    SCENE_CHIMNEY,
    SCENE_INDEPENDENCE,
    SCENE_SODA_SPRINGS,
    SCENE_BLUE_MOUNTAINS,
    SCENE_BROKEN_WAGON,
    SCENE_WILLAMETTE_VALLEY,
    SCENE_INDEPENDENCE_TOWN,
    SCENE_KANSAS_RIVER,
    SCENE_BIG_BLUE_RIVER,
    SCENE_FORT_KEARNEY,
    SCENE_SOUTH_PASS,
    SCENE_GREEN_RIVER,
    SCENE_SNOW_BOUND,
    SCENE_SNAKE_BITE,
    SCENE_ILLNESS,
    SCENE_GRAVESITE,
    SCENE_INDIANS_FOOD,
    SCENE_BLIZZARD,
    SCENE_THUNDERSTORM,
    SCENE_HEAVY_FOG,
    SCENE_HAIL_STORM,
    SCENE_OXEN_DIED_OR_INJURED,
    SCENE_PARTY_MEMBER_AFFLICTED,
    SCENE_LOST_TRAIL,
    SCENE_WILD_FRUIT,
    SCENE_ROUGH_TRAIL,
    SCENE_WAGON_FIRE,
    SCENE_PARTY_MEMBER_LOST,
    SCENE_ABANDONED_WAGON,
    SCENE_THIEF_NIGHT,
    SCENE_LITTLE_WATER,
    SCENE_INADEQUATE_GRASS,
    SCENE_DYSENTERY_END
} scene_t;

void scene_draw(scene_t scene, bool winter);
void scene_journey_marker(unsigned miles);
void scene_hunt_clear(void);
#define HUNT_FACE_LEFT 0x80
void scene_hunt_sprite(uint8_t x, uint8_t y, uint8_t kind, bool erase);
void scene_hunt_trees(void);
bool scene_hunt_cover(uint8_t x, uint8_t y);

#endif
