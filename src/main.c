/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <net.h>
#include <init.h>
#include <log.h>
#include <sys/stat.h>
#include <errno.h>
#include <sig.h>
#include <tfproto.h>
#include <udp_keep.h>
#include <string.h>
#include <core.h>

#include <sys/resource.h>

#define MINARGS 2

/* Function to become daemon */
static void mkdaemon(void);

int main(int argc, char **argv)
{
    if (argc < 2) {
        wrlog(ELOGDARG, LGC_CRITICAL);
        exit(EXIT_FAILURE);
    }
    int rc = init(*(argv + 1));
    if (rc) {
        wrlog(ELOGDINIT, LGC_CRITICAL);
        exit(EXIT_FAILURE);
    }
    wrlog(SLOGDINIT, LGC_INFO);
    if (access(tfproto.dbdir, F_OK | R_OK | W_OK | X_OK)) {
        wrlog(ELOGDDIR, LGC_CRITICAL);
        exit(EXIT_FAILURE);
    }
    if (setstacksize()) {
        wrlog(ELOGSTACKSIZE, LGC_CRITICAL);
        exit(EXIT_FAILURE);
    }
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);
    setsighandler(SIGINT, sigint);
    chdir(tfproto.dbdir);
#ifndef DEBUG
    if (argc >= 3)
        mkdaemon();
#else
    if (argc >= 3 && !strcmp(*(argv + 2), UDPARGVNAM))
        udp_debug = 1;
#endif
    wrlog(SLOGDRUNNING, LGC_INFO);
    startnet();
    return 0;
}

static void mkdaemon(void)
{
    pid_t pid = fork();
    if (pid)
        exit(EXIT_SUCCESS);
    else if (pid == -1) {
        wrlog(ELOGDFORK, LGC_CRITICAL);
        exit(EXIT_FAILURE);
    }
    setsid();
    int fdmax = sysconf(_SC_OPEN_MAX);
    for (; fdmax >= 0; fdmax--)
        close(fdmax);
    umask(0);
    int nulld = open("/dev/null", O_RDWR);
    dup(nulld);
    dup(nulld);
    dup(nulld);
    /* Redundancy call just in case something happened beetwen fork */
    chdir(tfproto.dbdir);
    pid = fork();
    if (pid)
        exit(EXIT_SUCCESS);
    else if (pid == -1) {
        wrlog(ELOGDFORK, LGC_CRITICAL);
        exit(EXIT_FAILURE);
    }
}
