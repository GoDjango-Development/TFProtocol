#include <posix_xsily.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define USLEEP_MAX 1000000

/* In accordance with BSD semantics, an early return caused by signal
	delivery is permitted. We therefore do not loop to ensure that the
	requested interval has elapsed.
	Implementation reference: https://man.openbsd.org/usleep.3 */
int usleep(int usec)
{
	if (usec >= USLEEP_MAX)
		return -1;
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = usec * 1000;
	int rc = nanosleep(&ts, NULL);
	if (rc == -1 && errno == EINTR)
		return -1;
	return 0;
}
