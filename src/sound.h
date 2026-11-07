#ifndef SOUND_H
#define SOUND_H

typedef enum
{
    SFX_RIVER,
    SFX_FORT,
    SFX_DEATH,
    SFX_ARRIVE,
    SFX_CREAK,
    SFX_WEEKLY,
    SFX_BROKEN
} sfx_t;

void sound_init(void);
void sound_title(void);
void sound_effect(sfx_t sfx);
void sound_landmark(unsigned index);
void sound_event(unsigned index);
#define EVENT_SNOW_BOUND 0
#define EVENT_SNAKE_BITE 1
#define EVENT_ILLNESS 2
#define EVENT_GRAVESITE 3
#define EVENT_INDIANS_FOOD 4
#define EVENT_BLIZZARD 5
#define EVENT_THUNDERSTORM 6
#define EVENT_HEAVY_FOG 7
#define EVENT_HAIL_STORM 8

#endif
