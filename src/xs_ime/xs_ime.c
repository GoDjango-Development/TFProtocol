/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <xs_ime/xs_ime.h>
#include <xs_ime/xs_imeopc1.h>
#include <tfproto.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <util.h>
#include <cmd.h>

/* Writer thread sleep interval.*/
#define WRSLEEP_TM 25000000
/* Send opcode retray interval. */
#define SNDLEEP 10000000

/* opcsnd structure instance. */
struct opcsnd opcsnd;
/* Nanosleep structure variable. */
struct timespec *tm;
/* Read header of XSIME packet. */
struct xsime_hdr rdhdr;
/* Write header of XSIME packet. */
struct xsime_hdr wrhdr;
/* Input buffer. */
char inbuf[BUFLEN];
/* Output buffer. */
char outbuf[BUFLEN];
/* Flag to signal when the protocol its ready to send and receive messages. */
volatile sig_atomic_t ready;
/* Path for the organizing unit. */
char orgunit[PATH_MAX];
/* Path for the user directory. */
char usrpath[PATH_MAX];
/* Path for the user's messages directory. */
char usrmsg[PATH_MAX];
/* Path for the user's blacklist directory. */
char usrblk[PATH_MAX];
/* User name. */
char usrnam[USRNAME_LEN];
/* Unique user instance. */
char unusr;
/* Auto delete messages after sending. */
char autodel;
/* Starting timestamp of resquested messages. */
unsigned long long tmstmp;

/* Writer thread data variable. */
static pthread_t wrth;
/* Reader thread data variable. */
static pthread_t rdth;
/* Mutex to protect integrity at the OPCODE OP_EXIT. */
static pthread_mutex_t exmut = PTHREAD_MUTEX_INITIALIZER;

/* Writer thread function. */
static void *wrthread(void *args);
/* Reader thread  function. */
static void *rdthread(void *args);
/* Clean IME header. */
static void cleanhdr(struct xsime_hdr *hdr);
/* Write down the socket pending opcode. */
static int writeopc(void);
/* Do clean-up at exit module. */
static void cleanmod(void);

void xsime_start()
{
    ready = 0;
    static struct timespec tmout = { 0, WRSLEEP_TM };
    tm = &tmout;
    opcsnd.tm.tv_sec = 0;
    opcsnd.tm.tv_nsec = SNDLEEP;
    int err = pthread_create(&wrth, NULL, wrthread, NULL);
    err += pthread_create(&rdth, NULL, rdthread, NULL);
    err += pthread_mutex_init(&opcsnd.mut, NULL);
    if (!err)
        cmd_ok();
    else {
        cmd_fail(XSIME_THREAD_ALLOC);
        return;
    }
    pthread_join(rdth, NULL);
    pthread_join(wrth, NULL);
    cleanmod();
}

static void *wrthread(void *args)
{   
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while (1) {
        nanosleep(tm, NULL);
        if (!chkrdy())
            continue;
        pthread_mutex_lock(&exmut);
        opc_sndmsg2();
        pthread_mutex_unlock(&exmut);
    }
    pthread_cancel(rdth);
    return NULL;
}

static void *rdthread(void *args)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while (readbuf_ex((char *) &rdhdr, sizeof rdhdr) != -1) {
        if (rdhdr.opcode == OPC_UNIT)
            opc_orgunit();
        else if (rdhdr.opcode == OPC_USR)
            opc_usr();
        else if (rdhdr.opcode == OPC_TIMER)
            opc_timer();
        else if (rdhdr.opcode == OPC_SNDMSG)
            opc_sndmsg();
        else if (rdhdr.opcode == OPC_SETUP)
            opc_setup();
        else if (rdhdr.opcode == OPC_EXIT) {
            opc_cleanup();
            break;
        }
        pthread_mutex_lock(&exmut);
        if (writeopc() == -1) {
            endcomm();
            break;
        } 
        pthread_mutex_unlock(&exmut);
    }
    pthread_mutex_lock(&exmut);
    pthread_cancel(wrth);
    sendopc(OPC_EXIT, OPC_SUCCESS);
    if (writeopc() == -1)
        endcomm();
    pthread_mutex_unlock(&exmut);
    return NULL;
}

void sendopc(int opc, int sz)
{
RETRY:
    pthread_mutex_lock(&opcsnd.mut);
    if (opcsnd.send) {
        pthread_mutex_unlock(&opcsnd.mut);
        nanosleep(&opcsnd.tm, NULL);
        goto RETRY;
    }
    opcsnd.hdr.opcode = opc;
    setbufsz(sz, &opcsnd.hdr);
    opcsnd.send = 1;
    pthread_mutex_unlock(&opcsnd.mut);
}

static void cleanhdr(struct xsime_hdr *hdr)
{
    memset(&hdr, 0, sizeof hdr);
}

int32_t getbufsz(struct xsime_hdr *hdr)
{
    int32_t v = hdr->sz;
    if (!isbigendian()) {
        swapbo32(v);
        return v;
    }
    return v;
}

void setbufsz(int32_t sz, struct xsime_hdr *hdr)
{
    if (!isbigendian())
        swapbo32(sz);
    hdr->sz = sz;
}

int chkrdy(void)
{
    return (ready & (ORUN | USR | SETUP)) == (ORUN | USR | SETUP) ? 1 : 0;
}

static int writeopc(void)
{
    pthread_mutex_lock(&opcsnd.mut);
    if (opcsnd.send) {
        if (writebuf_ex((char *) &opcsnd.hdr, sizeof opcsnd.hdr) == -1) {
            pthread_mutex_unlock(&opcsnd.mut);
            return -1;
        }
        opcsnd.send = 0;
    }
    pthread_mutex_unlock(&opcsnd.mut);
    return 0;
}

static void cleanmod(void)
{
    cleanhdr(&rdhdr);
    cleanhdr(&wrhdr);
    pthread_mutex_destroy(&opcsnd.mut);
    memset((char *) &opcsnd, 0, sizeof opcsnd);
}

