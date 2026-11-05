#ifndef MSG_H
#define MSG_H

/* Returns message number id, from the msg asset. The text is copied into a
   buffer that the next call overwrites, so use it in one call at a time. */
const char *msg_get(int id);

#endif
