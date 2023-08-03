/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef CORE_H
#define CORE_H

#include <inttypes.h>
#include <sys/resource.h>

/* Set recommended working stack size. */
int setstacksize(void);
/* Set the maximun number of child processes. */
int setmaxchilds(int64_t num);

#endif
