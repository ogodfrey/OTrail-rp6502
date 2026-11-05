#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"

void ui_clear(void)
{
    printf("\f");
}

/* End of input ends the program, so piped test runs stop cleanly. */
void ui_read_line(char *buf, size_t size)
{
    if (!fgets(buf, (int)size, stdin))
        exit(0);
    buf[strcspn(buf, "\r\n")] = '\0';
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
    printf("$%ld.%02ld", cents / 100, cents % 100);
}
