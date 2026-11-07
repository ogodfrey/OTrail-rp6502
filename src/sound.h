#ifndef SOUND_H
#define SOUND_H

typedef enum
{
    SFX_RIVER,
    SFX_FORT,
    SFX_DEATH,
    SFX_ARRIVE,
    SFX_CREAK,
    SFX_WEEKLY
} sfx_t;

void sound_init(void);
void sound_title(void);
void sound_effect(sfx_t sfx);

#endif
