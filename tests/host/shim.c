#include "rp6502.h"

/* Fixed seed, so every run of the playthrough takes the same route. */
long ria_attr_get(unsigned char id)
{
    (void)id;
    return 42;
}

unsigned char ria_vsync(void)
{
    static unsigned char frame;

    return frame++;
}

struct host_ria RIA = {0x40};

int xreg(char device, char channel, unsigned char index, ...)
{
    (void)device;
    (void)channel;
    (void)index;
    return 0;
}

void xram0_set(unsigned dest, unsigned char val, unsigned count)
{
    (void)dest;
    (void)val;
    (void)count;
}

void xram0_write(unsigned dest, const void *src, unsigned count)
{
    (void)dest;
    (void)src;
    (void)count;
}

void xram0_read(void *dest, unsigned src, unsigned count)
{
    (void)dest;
    (void)src;
    (void)count;
}
