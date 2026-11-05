/* Included before the game, so output is not buffered. The test reads each
   prompt as soon as the game prints it. */
#ifndef HOST_UNBUF_H
#define HOST_UNBUF_H

#include <stdio.h>

static void host_unbuffer(void) __attribute__((constructor));
static void host_unbuffer(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
}

#endif
