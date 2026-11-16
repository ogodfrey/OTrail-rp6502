#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "msg.h"

/* Each message is one fixed-size record of the msg asset, NUL-padded. */
#define MSG_RECORD 80

static char msg_buf[MSG_RECORD];
static char label_buf[32];

static const char *msg_read(int id, char *buf, unsigned char size)
{
    int fd = open("ROM:msg", O_RDONLY);

    buf[0] = '\0';
    if (fd < 0 || lseek(fd, (long)id * MSG_RECORD, SEEK_SET) < 0 ||
        read(fd, buf, size) != size) {
        puts("Cannot read ROM:msg.");
        buf[0] = '\0';
    }
    if (fd >= 0)
        close(fd);
    buf[size - 1] = '\0';
    return buf;
}

const char *msg_get(int id)
{
    return msg_read(id, msg_buf, sizeof msg_buf);
}

const char *msg_label(int id)
{
    return msg_read(id, label_buf, sizeof label_buf);
}
