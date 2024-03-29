/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <net.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <init.h>
#include <log.h>
#include <tfproto.h>
#include <sys/wait.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <util.h>
#include <errno.h>
#include <udp_keep.h>
#include <sig.h>

/* Idle time -in seconds- after probes begins. */
#define KEEPALIVE_TIME 3600
/* Interval time between probes. */
#define KEEPALIVE_INTVL 75
/* Number of probes before peer's dead is assumed. */
#define KEEPALIVE_PROBES 9

/* IPv6 any address initialization */
extern const struct in6_addr in6addr_any;

/* PID value for reload config instance. */
extern pid_t rlwait_pid;

/* IPC PIPE to avoid child zombie. */
int zfd[2];

static int srvsock;
/* Set standard tcp keepalive timeouts. */
static void setkeepalive(int so);

void startnet(void)
{
    unsigned short port = atoi(tfproto.port);
    struct sockaddr_in6 addr = { 0 };
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    addr.sin6_addr = in6addr_any;
    srvsock = socket(AF_INET6, SOCK_STREAM, 0);
    if (srvsock == -1) {
        wrlog(ELOGDCSOCK, LGC_CRITICAL);
        exit(EXIT_FAILURE);
    }
    int optval = 1;
    /* This socket option allows fast config reload and re-binding. */
    setsockopt(srvsock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
    int st = bind(srvsock, (struct sockaddr *) &addr, sizeof addr);
    if (st == -1) {
        wrlog(ELOGDBSOCK, LGC_CRITICAL);
        kill(rlwait_pid, SIGINT);
        exit(EXIT_FAILURE);
    }
    st = listen(srvsock, SOMAXCONN);
    if (st == -1) {
        wrlog(ELOGDLSOCK, LGC_CRITICAL);
        kill(rlwait_pid, SIGINT);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in6 rmaddr;
    socklen_t rmaddrsz = 0;
    wrlog(SLOGDSRVS, LGC_INFO);
#ifdef DEBUG
    if (udp_debug) {
        close(srvsock);
        udpkeep_start(&addr);
    }
#else
    udpkeep_start(&addr);
#endif
#ifndef DEBUG
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocs <= 0)
        nprocs = 1;
    int c = 0;
    for (; c < nprocs - 1; c++) {
        pid_t pid = fork();
        if (pid)
           break;
    }
#endif
    while (1) {
        int sockcomm;
        do
            sockcomm = accept(srvsock, (struct sockaddr *) &rmaddr, &rmaddrsz);
        while (sockcomm == -1 && errno == EINTR);
        if (sockcomm == -1) {
            wrlog(ELOGDASOCK, LGC_WARNING);
            continue;
        }
#ifndef DEBUG
        pid_t pid = fork();
        if (!pid) {
            sigaddset(&mask, SIGUSR1);
            pthread_sigmask(SIG_SETMASK, &mask, NULL);
            if (pipe(zfd) == -1) {
                wrlog(EZOMBIECREAT, LGC_WARNING);
                zfd[0] = -1;
            }
            pid = fork();
            if (!pid) {
                if (zfd[0] != -1)
                    close(zfd[1]);
                close(srvsock);
                setkeepalive(sockcomm);
                begincomm(sockcomm, &rmaddr, &rmaddrsz);
                /* A bit extra protection for terminating child process in
                    case of error. */
                exit(EXIT_SUCCESS);
            } else if (pid == -1)
                wrlog(ELOGDFORK, LGC_WARNING);
            if (zfd[0] != -1)
                close(zfd[0]);
            exit(EXIT_SUCCESS);
        } else if (pid == -1)
            wrlog(ELOGDFORK, LGC_WARNING);
        close(sockcomm);
        waitpid(pid, NULL, 0);
#else
        close(srvsock);
        setkeepalive(sockcomm);
        begincomm(sockcomm, &rmaddr, &rmaddrsz);
#endif
    }
    close(srvsock);
}

void endnet(void)
{
    close(srvsock);
}

static void setkeepalive(int so)
{
    int optval = 1;
    int r = setsockopt(so, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
    optval = KEEPALIVE_TIME;
    r += setsockopt(so, IPPROTO_TCP, TCP_KEEPIDLE, &optval, sizeof optval);
    optval = KEEPALIVE_INTVL;
    r += setsockopt(so, IPPROTO_TCP, TCP_KEEPINTVL, &optval, sizeof optval);
    optval = KEEPALIVE_PROBES;
    r += setsockopt(so, IPPROTO_TCP, TCP_KEEPCNT, &optval, sizeof optval);
    optval = (KEEPALIVE_TIME + KEEPALIVE_INTVL * KEEPALIVE_PROBES) * MILLISEC - 1;
    r += setsockopt(so, IPPROTO_TCP, TCP_USER_TIMEOUT, &optval, sizeof optval);
    if (r) {
        wrlog(ELOGKEEPALIVE, LGC_CRITICAL);
        exit(EXIT_FAILURE);
    }
}

void avoidz(void)
{
    char byte;
    if (zfd[0] != -1)
        read(zfd[0], &byte, sizeof byte);
}
