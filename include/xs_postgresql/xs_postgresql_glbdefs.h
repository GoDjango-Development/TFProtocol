#ifndef XS_POSTGRESQL_GLBDEFS_H
#define XS_POSTGRESQL_GLBDEFS_H

#include <inttypes.h>
#include <limits.h>

#define swapbo64(value) value = (value >> 56 & 0xFF) | (value >> 48 & 0xFF) << \
    8 | (value >> 40 & 0xFF) << 16 | (value >> 32 & 0xFF) << 24 | (value >> 24 \
    & 0xFF) << 32 | (value >> 16 & 0xFF) << 40 | (value >> 8 & 0xFF) << 48 | \
    (value & 0xFF) << 56

#define RESPONSE_MAX 200
#define CONNECTION_MAX 200
#define SQL_MAX 200
#define INIT_BUFFER_SIZE 10

typedef int64_t (*IO_R)(char *buf, int64_t len);
typedef int64_t (*IO_W)(char *buf, int64_t len);
typedef int (*Jaildir)(const char *src, char *dst);

#endif
