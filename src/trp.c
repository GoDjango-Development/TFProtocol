/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <trp.h>
#include <util.h>
#include <init.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <tfproto.h>
#include <pthread.h>
#include <signal.h>

#define CTMOUT 10

static int connecttm(struct sockaddr *addr);
static void *rdth(void *prms);
static void *wrth(void *prms);

void trp(void)
{
    int rc;
    int iptype;
    char ip[IPV6LEN];
    if (tfproto.trp_tp == TRP_DNS) {
        rc = resolvhn(tfproto.trp, ip, IPTYPE_V6, DNSTMOUT);
        iptype = IPTYPE_V6;
        if (rc != 0) {
            rc = resolvhn(tfproto.trp, ip, IPTYPE_V4, DNSTMOUT);
            iptype = IPTYPE_V4;
        }
    } else if (tfproto.trp_tp == TRP_IPV4) {
        strcpy(ip, tfproto.trp);
        iptype = IPTYPE_V4;
    } else if (tfproto.trp_tp == TRP_IPV6) {
        strcpy(ip, tfproto.trp);
        iptype = IPTYPE_V6;
    }
    if (iptype == IPTYPE_V4) {
        struct sockaddr_in addr;
        comm.sock_prox = socket(AF_INET, SOCK_STREAM, 0);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(atoi(tfproto.port));
        inet_pton(AF_INET, ip, &addr.sin_addr);        
        if (connecttm((struct sockaddr *) &addr) != 0)
            exit(EXIT_SUCCESS);
    } else if (iptype == IPTYPE_V6) {
        struct sockaddr_in6 addr6;
        comm.sock_prox = socket(AF_INET6, SOCK_STREAM, 0);
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = htons(atoi(tfproto.port));
        inet_pton(AF_INET6, ip, &addr6.sin6_addr);
        if (connecttm((struct sockaddr *) &addr6) != 0)
            exit(EXIT_SUCCESS);
    }
    pthread_t rt, wt;
    rc = pthread_create(&rt, NULL, rdth, NULL);
    rc =+ pthread_create(&wt, NULL, wrth, NULL);
    pthread_join(rt, NULL);
    pthread_join(wt, NULL);
    if (rc)
        exit(EXIT_SUCCESS);
}

static int connecttm(struct sockaddr *addr)
{
    int flags = fcntl(comm.sock_prox, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(comm.sock_prox, F_SETFL, flags);
    fd_set set;
    FD_ZERO(&set);
    FD_SET(comm.sock_prox, &set);
    struct timeval timeout;
    timeout.tv_sec = CTMOUT;
    timeout.tv_usec = 0;
    if (connect(comm.sock_prox, addr, sizeof *addr)) {
        int rc = select(FD_SETSIZE, NULL, &set, NULL, &timeout);
        if (!rc)
            return -1;
        else if (FD_ISSET(comm.sock_prox, &set)) {
            if (getpeername(comm.sock_prox, NULL, NULL) == -1 && errno == ENOTCONN)
                return -1;
            flags &= ~O_NONBLOCK;
            fcntl(comm.sock_prox, F_SETFL, flags);
        }
    }
    return 0;
}

static void *rdth(void *prms)
{
    sigset_t mask;
    sigfillset(&mask);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);
    int64_t rd, wr;
    int rc;
    comm.rxbuf_prox = malloc(COMMBUFLEN);
    if (!comm.rxbuf_prox)
        exit(EXIT_SUCCESS);
    while ((rd = read(comm.sock, comm.rxbuf_prox, COMMBUFLEN)) > 0) {
        wr = 0;
        while (rd > 0) {
            rc = write(comm.sock_prox, comm.rxbuf_prox + wr, rd);
            if (rc == -1)
                break;
            rd -= rc;
            wr += rc;
        }
    }
    exit(EXIT_SUCCESS);
    return NULL;
}

static void *wrth(void *prms) 
{
    sigset_t mask;
    sigfillset(&mask);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);
    int64_t rd, wr;
    int rc;
    comm.txbuf_prox = malloc(COMMBUFLEN);
    if (!comm.txbuf_prox)
        exit(EXIT_SUCCESS);
    while ((rd = read(comm.sock_prox, comm.txbuf_prox, COMMBUFLEN)) > 0) {
        wr = 0;
        while (rd > 0) {
            rc = write(comm.sock, comm.txbuf_prox + wr, rd);
            if (rc == -1)
                break;
            rd -= rc;
            wr += rc;
        }
    }
    exit(EXIT_SUCCESS);
    return NULL;
}
