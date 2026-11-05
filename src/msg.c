#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "msg.h"

/* Each message is one fixed-size record of the msg asset, NUL-padded. */
#define MSG_RECORD 80

static char msg_buf[MSG_RECORD];

const char *msg_get(int id)
{
    int fd = open("ROM:msg", O_RDONLY);

    msg_buf[0] = '\0';
    if (fd < 0)
        return msg_buf;
    lseek(fd, (long)id * MSG_RECORD, SEEK_SET);
    read(fd, msg_buf, MSG_RECORD);
    close(fd);
    msg_buf[MSG_RECORD - 1] = '\0';
    return msg_buf;
}
