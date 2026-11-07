#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rp6502.h>
#include "ui.h"
#include "msg.h"
#include "xram.h"

#define xreg_ria_keyboard(...) xreg(0x0, 0x0, 0x00, __VA_ARGS__)
#define KEYBOARD_PRESSED(keys, code) ((keys)[(code) >> 3] & (1 << ((code) & 7)))
#define RIA_READY_RX 0x40

/* The keypad keys, by USB HID code, and the character each one types: digits 1 to 9,
   then 0, then Enter. The keys are read from the keyboard map, because stdin never gets
   them. */
static const unsigned char keypad_codes[] = {0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x58};
static const char keypad_chars[] = "1234567890\n";
static unsigned char keypad_down[sizeof keypad_codes];

/* Returns the character of a keypad key that went down since the last call, or 0. */
static int keypad_key(void)
{
    static bool mapped;
    unsigned char keys[32];
    int key = 0;
    unsigned i;

    if (!mapped) {
        xreg_ria_keyboard(XRAM_KEYBOARD);
        mapped = true;
    }
    xram0_read(keys, XRAM_KEYBOARD, sizeof keys);
    for (i = 0; i < sizeof keypad_codes; i++) {
        unsigned char down = KEYBOARD_PRESSED(keys, keypad_codes[i]) ? 1 : 0;

        if (down && !keypad_down[i] && !key)
            key = keypad_chars[i];
        keypad_down[i] = down;
    }
    return key;
}

static void wait_frame(void)
{
    unsigned char start = ria_vsync();

    while (ria_vsync() == start)
        ;
}

void ui_clear(void)
{
    printf(msg_get(149));
}

/* Reads a line from the console, or from the keypad. A console byte is read when one is
   waiting, so the keypad is checked only while the console is idle. End of input ends the
   program, so piped test runs stop cleanly. */
void ui_read_line(char *buf, size_t size)
{
    size_t n = 0;

    fflush(stdout);
    for (;;) {
        int c;

        if (RIA.ready & RIA_READY_RX) {
            char ch;

            if (read(STDIN_FILENO, &ch, 1) != 1)
                exit(0);
            c = ch;
        } else {
            c = keypad_key();
            if (!c) {
                wait_frame();
                continue;
            }
        }
        if (c == '\r')
            continue;
        if (c == '\n')
            break;
        if (n + 1 < size)
            buf[n++] = (char)c;
    }
    buf[n] = '\0';
}

int ui_ask_int(const char *prompt, int min, int max)
{
    char line[32];
    char *end;
    long value;

    for (;;)
    {
        fputs(prompt, stdout);
        ui_read_line(line, sizeof(line));
        value = strtol(line, &end, 10);
        if (end != line && *end == '\0' && value >= min && value <= max)
            return (int)value;
        printf("Enter a number from %d to %d.\n", min, max);
    }
}

void ui_pause(void)
{
    char line[64];

    fputs("\nPress Enter to continue.", stdout);
    ui_read_line(line, sizeof(line));
}

void ui_print_money(long cents)
{
    printf(msg_get(151), cents / 100, cents % 100);
}
