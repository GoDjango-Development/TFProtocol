/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <udp_keep.h>
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
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define FORKCOUNT_FAC 2
#define THREADCOUNT 10
#define MSG_MAXLEN 400
#define TLB_TUPLEN 256

int udp_debug;
/* Thread pool's barrier. */
static pthread_barrier_t barrier;
/* Thread pool.*/
static pthread_t threads[THREADCOUNT];
/* Server inet address. */
struct sockaddr_in6 addr;

/* Start receiving udp datagrams with the pool of worker threads. */
static void rcvdgram(void);
/* Worker thread function. */
static void *thworker(void *prms);
/* Proccess message. */
static void procmsg(char *msg, int *msglen);
static void fillproxaddr(void);

void udpkeep_start(struct sockaddr_in6 *srvaddr)
{
    addr = *srvaddr;
#ifndef DEBUG
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocs <= 0)
        nprocs = 1;
    int c = 0;
    for (; c < nprocs * FORKCOUNT_FAC; c++) {
        pid_t pid = fork();
        if (!pid)
            rcvdgram();
    }
#else
    rcvdgram();
#endif
}

static void rcvdgram(void)
{
    pthread_barrier_init(&barrier, NULL, THREADCOUNT);
    int c = 0;
    for (; c < THREADCOUNT; c++)
        pthread_create(&threads[c], NULL, thworker, NULL);
    pthread_barrier_wait(&barrier);
    exit(0);
}

static void *thworker(void *prms)
{
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock == -1) {
        wrlog(ELOGDCSOCKDGRAM, LGC_CRITICAL);
        return NULL;
    }
    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
    int len = sizeof addr;
    int st = bind(sock, (struct sockaddr *) &addr, len);
    if (st == -1) {
        wrlog(ELOGDBSOCKUDP, LGC_CRITICAL);
        return NULL;
    }
    char msg[MSG_MAXLEN];
    int msglen = 0;
    int sock_prox = -1;
    int proxlen = sizeof comm.addr_prox;
    if (tfproto.trp_tp != TRP_NONE) {
        fillproxaddr();
        while (1) {
            memset(msg, 0, sizeof msg);
            if ((msglen = recvfrom(sock, msg, sizeof msg, 0, (struct sockaddr *) 
                &addr, &len)) < 0)
                continue;
            if (sock_prox >= 0)
                close(sock_prox);
            if (comm.addr_proxtp == IPTYPE_V4)
                sock_prox = socket(AF_INET, SOCK_DGRAM, 0);
            else if (comm.addr_proxtp == IPTYPE_V6)
                sock_prox = socket(AF_INET6, SOCK_DGRAM, 0);
            sendto(sock_prox, msg, msglen, 0, (struct sockaddr *) &comm.addr_prox,
                proxlen);
            memset(msg, 0, sizeof msg);
            if ((msglen = recvfrom(sock_prox, msg, sizeof msg, 0, &comm.addr_prox,
                &proxlen)) < 0)
                continue;
            sendto(sock, msg, msglen, 0, (struct sockaddr *) &addr, len);
        }
    } else {
        while (1) {
            memset(msg, 0, sizeof msg);
            if (recvfrom(sock, msg, sizeof msg, 0, (struct sockaddr *) &addr, 
                &len) < 0)
                continue;
            procmsg(msg, &msglen);
            if (msglen < 0)
                continue;
            sendto(sock, msg, msglen, 0, (struct sockaddr *) &addr, len);
        }
    }
    pthread_barrier_wait(&barrier);
    return NULL;
}

static void procmsg(char *msg, int *msglen)
{
    int shmfd;
    if (*msg == UDP_HOSTCHECK) {
        *msg = UDP_OK;
        *msglen = 1;
        return;
    } else if (*msg == UDP_PROCHECK) {
        shmfd = shm_open(msg + 1, O_RDWR, S_IRUSR | S_IWUSR);
        if (shmfd != -1) {
            *msg = UDP_OK;
            close(shmfd);
        } else
            *msg = UDP_FAILED;
        *msglen = 1;
        return;
    } else if (*msg == UDP_SOCKCHECK) {
        shmfd = shm_open(msg + 1, O_RDWR, S_IRUSR | S_IWUSR);
        struct shm_obj *shmobj; 
        if (shmfd != -1 && (shmobj = mmap(NULL, sizeof(struct shm_obj), 
            PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0))) {
            *msg = UDP_OK;
            sprintf(msg + 1, "%d", shmobj->oobcount);
            close(shmfd);
            *msglen = strlen(msg) + 1;
        } else {
            *msg = UDP_FAILED;
            *msglen = 1;
        }
        return;
    } else if (*msg == UDP_TLB) {
        FILE *tlb = fopen(tfproto.tlb, "r");
        static int64_t pos;
        struct stat st;
        stat(tfproto.tlb, &st);
        if (tlb && (fseek(tlb, pos, SEEK_SET) == -1 || pos == st.st_size))
            fseek(tlb, 0, SEEK_SET);
        if (tlb && fgets(msg + 1, TLB_TUPLEN, tlb)) {
            pos = ftell(tlb);
            char *pt = strchr(msg, '\n');
            if (pt)
                *pt = '\0';
            *msg = UDP_OK;
            *msglen = strlen(msg) + 1;
        } else {
            *msg = UDP_FAILED;
            *msglen = 1;
        }
        fclose(tlb);
        return;
    } 
    *msglen = -1;
}

static void fillproxaddr(void)
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
        comm.addr_prox = *(struct sockaddr *) &addr;
        comm.addr_proxtp = IPTYPE_V4;
    } else if (iptype == IPTYPE_V6) {
        struct sockaddr_in6 addr6;
        comm.sock_prox = socket(AF_INET6, SOCK_STREAM, 0);
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = htons(atoi(tfproto.port));
        inet_pton(AF_INET6, ip, &addr6.sin6_addr);
        comm.addr_prox = *(struct sockaddr *) &addr6;
        comm.addr_proxtp = IPTYPE_V6;
    }
}
