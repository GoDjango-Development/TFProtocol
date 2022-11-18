#include <core.h>
#include <sys/resource.h>

#define STACK_SIZE 16 * 1024 * 1024

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
