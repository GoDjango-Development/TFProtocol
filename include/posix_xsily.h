/* This header provides a POSIX-XSI compatibility layer for some unix
	extensions used by TFProtocol. Definitions in this header may overlap
	with system declarations unless _POSIX_C_SOURCE=200809L and
	_XOPEN_SOURCE=700 are defined before including any system headers. */

#ifndef POSIX_XSILY_H
#define POSIX_XSILY_H

#include <sys/stat.h>

enum DTYPE {
	DT_UNKNOWN, DT_REG, DT_DIR, DT_FIFO, DT_SOCK, DT_CHR, DT_BLK, DT_LNK
};

/* Provide a POSIX compatibility layer for the BSD usleep() extension. */
int usleep(int usec);
/* Provide a POSIX compatible util to get mode_t of path (DIR, LINK, REG...) */
enum DTYPE gdt(const char *bname, const char *fname);

#endif
