#include "rp6502.h"
#include <assert.h>
#include <unistd.h>

static unsigned char ram[65536];

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

/* Host stdin is read by stdio; no raw UART keystrokes are queued. */
struct host_ria RIA = {0};

int xreg(char device, char channel, unsigned char index, ...)
{
    (void)device;
    (void)channel;
    (void)index;
    return 0;
}

void xram0_set(unsigned dest, unsigned char val, unsigned count)
{
    assert(dest + count <= sizeof ram);
    memset(ram + dest, val, count);
}

void xram0_write(unsigned dest, const void *src, unsigned count)
{
    assert(dest + count <= sizeof ram);
    memcpy(ram + dest, src, count);
}

void xram0_read(void *dest, unsigned src, unsigned count)
{
    assert(src + count <= sizeof ram);
    memcpy(dest, ram + src, count);
}

int read_xram(unsigned xaddr, unsigned count, int fildes)
{
    assert(xaddr + count <= sizeof ram);
    return read(fildes, ram + xaddr, count);
}

int write_xram(unsigned xaddr, unsigned count, int fildes)
{
    assert(xaddr + count <= sizeof ram);
    return write(fildes, ram + xaddr, count);
}
