#ifndef UI_H
#define UI_H

#include <stddef.h>

void ui_clear(void);
void ui_read_line(char *buf, size_t size);
int ui_ask_int(const char *prompt, int min, int max);
void ui_pause(void);
void ui_print_money(long cents);

#endif
