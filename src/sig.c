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

sigset_t mask;
int rlflag;

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
    if (!rlflag) {
        endnet();
        wrlog(SLOGSIGUSR1, LGC_INFO);
        exit(EXIT_SUCCESS);
    }
}

void rlwait(char *const *argv)
{
    int signop;
    sigaddset(&mask, SIGUSR1);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);
    sigset_t waitmask;
    sigemptyset(&waitmask);
    sigaddset(&waitmask, SIGUSR1);
    while (1) {
        sigwait(&waitmask, &signop);
        if (signop == SIGUSR1) {
            pid_t pid = fork();
            if (!pid)
                execv(*argv, argv);
            else if (pid == -1) {
                wrlog(ELOGDFORK, LGC_CRITICAL);
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
    }
}
