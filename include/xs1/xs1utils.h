/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef XS1UTILS_H
#define XS1UTILS_H

#include <xs1/xs1.h>

/* This function get an unused descriptor hole from ds struct array and if
    there is no empty one, it extends the array and return the new created one.
    On failure, it returns NULL. */
int32_t *xs1_getfd(struct ds *ds);
/* Return the array index for fdp -file descriptor pointer- or -1 if can't
    find it. */
int xs1_getidx(int32_t *fdp, struct ds *ds);

#endif
