/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <xs_gateway/xs_gateway.h>
#include <cmd.h>
#include <unistd.h>
#include <util.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <libgen.h>
#include <fcntl.h>

/* XS_GATEWAY error codes. */

#define XS_EGWINTERNAL "1 : Internal error occurred."
#define XS_EGWIDENT "2 : Failed creating identity."
#define UXSO_TMOUT 10
#define USLEEP 100000
/* XS_GATEWAY subsystem reserved code. */
#define GWCODE_UNUSED 0
/* XS_GATEWAY subsystem exit code. */
#define GWCODE_EXIT -1
/* Thread sync flags. */
#define READYSUCCESS 1
#define READYFAIL -1
#define READYUXTH 2


/* Comm data structure to be used by comm functions. */
struct gw {
    struct uxdata {
        int socli;
    } *ux;
    int tfp_sock;
    volatile int sem;
    volatile int exited;
    char ident[PATH_MAX];
    pthread_mutex_t mutsem;
    pthread_mutex_t wrmut;
    char *buf;
    int volatile ready;
};

static int start_ux(struct gw **gwpt);
static void *uxlisten(void *prms);
static void *uxcomm(void *prms);
static void start_net(void);
static void freegw(struct gw *gw);

void xs_gateway(void)
{
    cmd_ok();
    if (readbuf(comm.buf, sizeof comm.buf) == -1) {
        endcomm();
        return;
    }
    struct gw *gw = malloc(sizeof(struct gw));
    if (!gw) {
        cmd_fail(CMD_EMEMINT);
        return;
    }
    memset(gw, 0, sizeof(struct gw));
    if (jaildir(comm.buf, gw->ident)) {
        cmd_fail(CMD_EACCESS);
        free(gw);
        return;
    }
    if (dup2(comm.sock, gw->tfp_sock) == -1) {
        cmd_fail(XS_EGWINTERNAL);
        free(gw);
        return;
    }
    volatile int *ready = &gw->ready;
    volatile int *pexit = &gw->exited;
    int tfp_sock = gw->tfp_sock;
    if (start_ux(&gw)) {
        cmd_fail(XS_EGWINTERNAL);
        free(gw);
        return;
    }
    while (!*ready)
        usleep(USLEEP);
    if (*ready == READYSUCCESS) {
        cmd_ok();
        *ready = READYUXTH;
    }
    else {
        cmd_fail(XS_EGWIDENT);
        freegw(gw);
        return;
    }
    start_net();
    *pexit = 1;
    close(tfp_sock);
}

static int start_ux(struct gw **gwpt)
{
    pthread_t uxth;
    struct gw *gw = *gwpt;
    gw->buf = malloc(COMMBUFLEN);
    if (!gw->buf)
        return -1;
    int rc = pthread_mutex_init(&gw->mutsem, NULL);
    if (rc)
        return -1;
    rc = pthread_mutex_init(&gw->wrmut, NULL);
    if (rc)
        return -1;
    rc = pthread_create(&uxth, NULL, uxlisten, gwpt);
    if (rc)
        return -1;
    return 0;
}

static void *uxlisten(void *prms)
{
    struct gw *gw = *(struct gw **) prms;
    unlink(gw->ident);
    struct sockaddr_un addr = { 0 };
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, gw->ident);
    int addrsz;
    pthread_t uxth;
    size_t size = (size_t) &((struct sockaddr_un *) NULL)->sun_path + 
        strlen(addr.sun_path);
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        gw->ready = READYFAIL;
        return NULL;
    }
    if (bind(sock, (struct sockaddr *) &addr, size) < 0) {
        gw->ready = READYFAIL;
        return NULL;
    }
    if (listen(sock, SOMAXCONN) == -1) {
        gw->ready = READYFAIL;
        return NULL;
    }
    struct timeval tmout;
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        gw->ready = READYFAIL;
        return NULL;
    }
    *(struct gw **) prms = NULL;
    gw->ready = READYSUCCESS;
    flags |= O_NONBLOCK;
    fcntl(sock, F_SETFL, flags);
    fd_set set;
    FD_ZERO(&set);
    while (gw->ready != READYUXTH)
        usleep(USLEEP);
    while (1) {
        FD_SET(sock, &set);
        tmout.tv_sec = UXSO_TMOUT;
        tmout.tv_usec = 0;
        int rc = select(FD_SETSIZE, &set, NULL, NULL, &tmout);
        if (gw->exited)
            break;
        if (!rc || rc == -1)
            continue;
        int socli = accept(sock, (struct sockaddr *) &addr, &addrsz);
        while (gw->ux)
            usleep(USLEEP);
        gw->ux = malloc(sizeof(struct uxdata));
        if (!gw->ux)
            continue;
        gw->ux->socli = socli;
        if (gw->ux->socli == -1) {
            close(gw->ux->socli);
            free(gw->ux);
            gw->ux = NULL;
        } else if (pthread_create(&uxth, NULL, uxcomm, gw)) {
            close(gw->ux->socli);
            free(gw->ux);
            gw->ux = NULL;
        }
    }
    while (gw->sem > 0)
        sleep(UXSO_TMOUT);
    close(sock);
    freegw(gw);
    return NULL;
}

static void *uxcomm(void *prms)
{
    struct gw *gw = prms;
    struct uxdata *ux = gw->ux;
    gw->ux = NULL;
    int rc;
    int32_t hdr;
    pthread_mutex_lock(&gw->mutsem);
    gw->sem++;
    pthread_mutex_unlock(&gw->mutsem);
    pthread_mutex_lock(&gw->wrmut);
    rc = readbuf_exfd(ux->socli, (char *) &hdr, sizeof hdr, 0);
    if (rc != -1) {
        rc = readbuf_exfd(ux->socli, gw->buf, hdr, 0);
        if (rc != -1) {
            int32_t len = hdr;
            if (!isbigendian())
                swapbo32(hdr);
            if (writebuf_exfd(gw->tfp_sock, (char *) &hdr, sizeof hdr, 1) != -1)
                writebuf_exfd(gw->tfp_sock, gw->buf, len, 1);
        }
    }
    pthread_mutex_unlock(&gw->wrmut);
    pthread_mutex_lock(&gw->mutsem);
    close(ux->socli);
    free(ux);
    gw->sem--;
    pthread_mutex_unlock(&gw->mutsem);
    return NULL;
}

static void start_net(void)
{
    int32_t hdr;
    int loop = 1;
    while (loop) {
        if (readbuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        if (!isbigendian())
            swapbo32(hdr);
        if (hdr > 0) {
            if (readbuf_ex(comm.buf, hdr) == -1) {
                endcomm();
                return;
            }
            comm.buf[hdr] = '\0';
            struct sockaddr_un addr = { 0 };
            addr.sun_family = AF_UNIX;
            strcpy(addr.sun_path, comm.buf);
            if (readbuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
            if (!isbigendian())
                swapbo32(hdr);
            memset(comm.buf, 0, sizeof comm.buf);
            if (readbuf_ex(comm.buf, hdr) == -1) {
                endcomm();
                return;
            }
            int sock = socket(AF_UNIX, SOCK_STREAM, 0);
            size_t size = (size_t) &((struct sockaddr_un *) NULL)->sun_path + 
                strlen(addr.sun_path);
            int rc = connect(sock, (struct sockaddr *) &addr, size);
            if (rc == -1)
                continue;
            writebuf_exfd(sock, (char *) &hdr, sizeof hdr, 0);
            writebuf_exfd(sock, comm.buf, hdr, 0);
        } else
            switch (hdr) {
            case GWCODE_EXIT:
                loop = 0;
                break;
            };
    }
}

static void freegw(struct gw *gw)
{
    free(gw->buf);
    pthread_mutex_destroy(&gw->mutsem);
    pthread_mutex_destroy(&gw->wrmut);
    free(gw);
}
