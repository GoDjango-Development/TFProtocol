/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

/* The eXtended subsystem 1 is an intended API to provide greater granularity
    in file operations. This subsystem could be not present in some 
    implementations. */

#ifndef XS1_H
#define XS1_H

#include <stdint.h>

/* The file descriptors node structure. */
struct ds {
    int32_t *fds;
    int count;
};


void xs1_parse(const char *cmd);

#endif
