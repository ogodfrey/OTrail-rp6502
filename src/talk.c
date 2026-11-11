#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "talk.h"

#define TALK_COUNT 3
#define TALK_LINE_MAX 320
#define WRAP_WIDTH 76

/* Each line of the talk asset is: landmark|speaker number|speaker|quote. */
static void print_wrapped(const char *text)
{
    int col = 0;
    const char *word = text;

    while (*word) {
        const char *end = word;
        int len;

        while (*end && *end != ' ' && *end != '\n')
            end++;
        len = (int)(end - word);
        if (col > 0 && col + 1 + len > WRAP_WIDTH) {
            putchar('\n');
            col = 0;
        }
        if (col > 0) {
            putchar(' ');
            col++;
        }
        fwrite(word, 1, (size_t)len, stdout);
        col += len;
        word = end;
        while (*word == ' ')
            word++;
    }
}

bool talk_to_people(int landmark, int *turn)
{
    int want = *turn % TALK_COUNT;
    char line[TALK_LINE_MAX];
    int fd;
    bool shown = false;

    if (landmark < 0 || landmark >= TALK_LANDMARKS)
        return false;
    fd = open("ROM:talk", O_RDONLY);
    if (fd < 0) {
        perror("Opening dialogue");
        return false;
    }
    while (!shown) {
        int count = read(fd, line, sizeof(line) - 1);
        char *nl, *bar1;
        char *bar2, *bar3;

        if (count < 0) {
            perror("Reading dialogue");
            break;
        }
        if (count == 0)
            break;
        line[count] = '\0';
        nl = strchr(line, '\n');
        if (nl) {
            /* Leave the next record for the next read, without a heap-backed FILE. */
            if (lseek(fd, (long)(nl + 1 - line) - count, SEEK_CUR) < 0) {
                perror("Seeking dialogue");
                break;
            }
            *nl = '\0';
        } else if (count == sizeof(line) - 1) {
            fputs("Dialogue line is too long.\n", stdout);
            break;
        }
        bar1 = strchr(line, '|');
        if (!bar1 || atoi(line) != landmark || atoi(bar1 + 1) != want)
            continue;
        bar2 = strchr(bar1 + 1, '|');
        bar3 = bar2 ? strchr(bar2 + 1, '|') : NULL;
        if (!bar3)
            continue;
        *bar3 = '\0';
        printf("%s says:\n\"", bar2 + 1);
        {
            char *quote = bar3 + 1;
            char *nl = strpbrk(quote, "\r\n");
            if (nl)
                *nl = '\0';
            print_wrapped(quote);
        }
        printf("\"\n");
        shown = true;
    }
    close(fd);
    (*turn)++;
    return shown;
}
