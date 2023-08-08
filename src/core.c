/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <core.h>

#define STACK_SIZE 16 * 1024 * 1024
#define MINNPROC 256

int setstacksize(void)
{
    struct rlimit rl;
    int rc = getrlimit(RLIMIT_STACK, &rl);
    if (rc)
        return -1;
    if (rl.rlim_cur < STACK_SIZE) {
        rl.rlim_cur = STACK_SIZE;
        rc = setrlimit(RLIMIT_STACK, &rl);
        if (rc)
            return -1;
    }
    return 0;
}

int setmaxchilds(int64_t num)
{
    /* This isn't an XSI rlimit compatible parameter. At least in solaris it 
       must be done by configuring the OS. In the rest of Unices it is possible
       that requires additional OS configurations in order to be effective, 
       like PID_MAX and THREAD/PROCES_MAX parameters. */
    struct rlimit rl;
    int rc = getrlimit(RLIMIT_NPROC, &rl);
    if (rc)
        return -1;
    if (num >= MINNPROC) {
        rl.rlim_cur = num;
        rl.rlim_max = num;
    } else if (num < MINNPROC && num > 0) {
        rl.rlim_cur = MINNPROC;
        rl.rlim_max = MINNPROC;
    } else if (num < 0) {
        rl.rlim_cur = RLIM_INFINITY;
        rl.rlim_max = RLIM_INFINITY;
    } else
        return 0;
    return setrlimit(RLIMIT_NPROC, &rl);
}
