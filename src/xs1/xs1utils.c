/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <xs1/xs1utils.h>
#include <stdlib.h>
#include <limits.h>

int *xs1_getfd(struct ds *ds)
{
    if (!ds)
        return NULL;
    if (ds->fds) {
        int c = 0;
        for (; c < ds->count; c++)
            if (*(ds->fds + c) == -1)
                return (ds->fds + c);
    }
    if (ds->count == INT_MAX)
        return NULL;
    void *mem = realloc(ds->fds, (ds->count + 1) * sizeof *ds->fds);
    if (mem) {
        ds->fds = mem;
        *(ds->fds + ds->count) = -1;
        return (ds->fds + ds->count++);
    }
    return NULL;
}

int xs1_getidx(int *fdp, struct ds *ds)
{
    int c = 0;
    for (; c < ds->count; c++)
        if (*(ds->fds + c) == *fdp)
            return c;
    return -1;
}

