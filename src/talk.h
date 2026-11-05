#ifndef TALK_H
#define TALK_H

#include <stdbool.h>

#define TALK_LANDMARKS 17

/* Prints the next speaker at the landmark. turn counts how many were heard. */
bool talk_to_people(int landmark, int *turn);

#endif
