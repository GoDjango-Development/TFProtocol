#ifndef POSIX_XSILY_H
#define POSIX_XSILY_H

/* This header provides a POSIX-XSI compatibility layer for some unix
	extensions used by TFProtocol. Definitions in this header may overlap
	with system declarations unless _POSIX_C_SOURCE=200809L and
	_XOPEN_SOURCE=700 are defined before including any system headers. */

/* Provide a POSIX compatibility layer for the BSD usleep() extension. */
int usleep(int usec);

#endif
