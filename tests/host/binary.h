/* The RP6502 reads files as bytes. Windows opens files as text by default,
   which changes CR LF pairs, so the host builds open files in binary mode. */
#include <fcntl.h>

#ifdef O_BINARY
#define open(path, flags) open((path), (flags) | O_BINARY)
#endif
