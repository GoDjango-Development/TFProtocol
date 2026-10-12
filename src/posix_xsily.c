/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <posix_xsily.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>

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
	struct timespec rm = { 0 };
	ts.tv_sec = 0;
	ts.tv_nsec = usec * 1000;
	int rc = nanosleep(&ts, &rm);
	if (rc == -1 || (rm.tv_sec | rm.tv_nsec))
		return -1;
	return 0;
}

enum DTYPE gdt(const char *bname, const char *fname)
{
	char path[PATH_MAX];
	strcpy(path, bname);
	strcat(path, "/");
	strcat(path, fname);
	struct stat st;
	if (stat(path, &st) == -1)
		return -1;
	if (S_ISDIR(st.st_mode))
		return DT_DIR;
	if (S_ISCHR(st.st_mode))
		return DT_CHR;
	if (S_ISBLK(st.st_mode))
		return DT_BLK;
	if (S_ISREG(st.st_mode))
		return DT_REG;
	if (S_ISFIFO(st.st_mode))
		return DT_FIFO;
	if (S_ISLNK(st.st_mode))
		return DT_LNK;
	if (S_ISSOCK(st.st_mode))
		return DT_SOCK;
	return DT_UNKNOWN;
}
