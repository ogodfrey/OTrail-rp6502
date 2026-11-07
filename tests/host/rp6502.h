/* Host stand-in for the RP6502 API, for the playthrough test. The game calls
   only these; the VGA and RIA hardware calls do nothing. */
#ifndef RP6502_HOST_H
#define RP6502_HOST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RIA_ATTR_LRAND 0x04

long ria_attr_get(unsigned char id);
int xreg(char device, char channel, unsigned char index, ...);
unsigned char ria_vsync(void);
void xram0_set(unsigned dest, unsigned char val, unsigned count);
void xram0_write(unsigned dest, const void *src, unsigned count);
void xram0_read(void *dest, unsigned src, unsigned count);

#endif
