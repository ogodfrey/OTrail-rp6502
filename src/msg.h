#ifndef MSG_H
#define MSG_H

/* Returns message number id, from the msg asset. The text is copied into a
   buffer that the next call overwrites, so use it in one call at a time. */
const char *msg_get(int id);

/* Short labels (up to 31 characters) use a separate buffer, so one can be
   passed alongside msg_get(). The next msg_label() call overwrites it. */
const char *msg_label(int id);

#endif
