#include <string.h>
#include "rp6502.h"

/* A 64 KB XRAM, so the scene code can be checked on the host. */
unsigned char xram[65536];

void xram0_set(unsigned dest, unsigned char val, unsigned count)
{
    memset(xram + dest, val, count);
}

void xram0_write(unsigned dest, const void *src, unsigned count)
{
    memcpy(xram + dest, src, count);
}

void xram0_read(void *dest, unsigned src, unsigned count)
{
    memcpy(dest, xram + src, count);
}
