/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <pthread.h>
#include <sig.h>
#include <stdio.h>
#include <stdlib.h>
#include <log.h>
#include <net.h>
#include <tfproto.h>
#include <errno.h>
#include <init.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <wait.h>

sigset_t mask;

void setsighandler(int signo, void (*handler)(int))
{
    struct sigaction sa;
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(signo, &sa, NULL);
}

void sigint(int signo)
{
    cleanup();
    endnet();
    wrlog(SLOGSIGINT, LGC_INFO);
    exit(EXIT_SUCCESS);
}

void sigusr1(int signo)
{
    endnet();
    wrlog(SLOGSIGUSR1, LGC_INFO);
    exit(EXIT_SUCCESS);
}

void rlwait(char *const *argv, pid_t pid)
{
    int signop;
    sigset_t waitmask;
    sigemptyset(&waitmask);
    sigaddset(&waitmask, SIGUSR1);
    while (1) {
        sigwait(&waitmask, &signop);
        if (signop == SIGUSR1) {
            waitpid(pid, NULL, 0);
            execv(*argv, argv);
        }
    }
}
