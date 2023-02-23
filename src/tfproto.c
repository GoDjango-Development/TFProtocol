/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <stdio.h>
#include <stdlib.h>
#include <tfproto.h>
#include <unistd.h>
#include <pthread.h>
#include <init.h>
#include <string.h>
#include <errno.h>
#include <sig.h>
#include <time.h>
#include <cmd.h>
#include <crypto.h>
#include <util.h>
#include <fcntl.h>
#include <log.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <pwd.h>
#include <trp.h>
#include <sys/stat.h>

/* Tha header that indicates the size of the expected messages. */
#pragma pack(push, 1)
struct tfhdr {
    int32_t sz;
} rxhdr, txhdr;
#pragma pack(pop)

/* Instance of "struct comm". global for entire programe usage. */
struct comm comm;
/* Loop condition variable. */
static int loop = 1;
/* Cryptography structures containing crypt and decrypt functions. */
struct crypto cryp_org;
struct crypto cryp_rx;
struct crypto cryp_tx;
/* Logged flag. */
extern int logged;
/* Secure FileSystem identity token. */
char fsid[LINE_MAX];
/* Thread handler for signals. */
static pthread_t oobth;
/* Secure FileSystem current operation. */
unsigned int volatile fsop;
volatile int fsop_ovrr;

/* Validate protocol version. non-zero return for ok. */
static int chkproto(void);
/* Main loop until client send "end" message to terminate comm. */
static void mainloop(void);
/* The command interface sent through the network by client-side. */
static void cmdifce(void);
/* Validate hash identity. non-zero return for ok. */
static int chkhash(void);
/* Receive the 2048 bit rsa public key encryptation from client. */
static int getkey(void);
/* Set size to the header. */
static void hdrsetsz(int32_t sz, struct tfhdr *hdr);
/* Get size to the header. */
static int32_t hdrgetsz(struct tfhdr *hdr);
/* The actual socket sender funtction. */
static int64_t sndbuf(int fd, char *buf, int64_t len, int enc);
/* The actual socket receiver funtction. */
static int64_t rcvbuf(int fd, char *buf, int64_t len, int enc);
/* Thread handler function to process OOB (out-of-band). */
static void *oobthread(void *prms);
/* Generete unique key for keepalive mechanism */
static void genkeepkey(void);

void begincomm(int sock, struct sockaddr_in6 *rmaddr, socklen_t *rmaddrsz)
{
    comm.sock = sock;
    comm.addr = *rmaddr;
    comm.addrsz = *rmaddrsz;
    fcntl(comm.sock, F_SETOWN, getpid());
    int r = pthread_create(&oobth, NULL, oobthread, NULL);
    if (r != 0) {
        wrlog(ELOGOOBTHREAD, LGC_CRITICAL);
        exit(EXIT_FAILURE);
    }
    if (tfproto.trp_tp != TRP_NONE)
        trp();
    genkeepkey();
    initcrypto(&cryp_rx);
    mainloop();
    cleanup();
    exit(EXIT_SUCCESS);
}

int chkproto(void)
{
    if (readbuf(comm.buf, sizeof comm.buf) == -1)
        return 0;
    else if (!strcmp(comm.buf, tfproto.proto))
        return 1;
    return 0;
}

void endcomm(void)
{
    loop = 0;
}

int64_t writebuf(char *buf, int64_t len) 
{
    hdrsetsz(len, &txhdr);
    if (sndbuf(comm.sock, (char *) &txhdr, sizeof txhdr, 1) == -1)
        return -1;
    return sndbuf(comm.sock, buf, len, 1);
}

int64_t readbuf(char *buf, int64_t len)
{
    if (rcvbuf(comm.sock, (char *) &rxhdr, sizeof rxhdr, 1) == -1)
        return -1;
    len = hdrgetsz(&rxhdr);
    if (buf == comm.buf && len < sizeof comm.buf)
        buf[len] = 0;
    return rcvbuf(comm.sock, buf, len, 1);
}

static void mainloop(void)
{
    if (chkproto())
        cmd_ok();
    else {
        cmd_fail(EPROTO_BADVER);
        return;
    }
    if (getkey()) {
        int rs;
        if ((rs = derankey(&cryp_rx, tfproto.priv)) == -1) {
            cmd_fail(EPROTO_BADKEY);
            return;
        }
        if (dup_crypt(&cryp_tx, &cryp_rx)) {
            cmd_fail(EPROTO_BADKEY);
            return;
        }
        if (dup_crypt(&cryp_org, &cryp_rx)) {
            cmd_fail(EPROTO_BADKEY);
            return;
        }
        cmd_ok();
    } else {
        cmd_fail(EPROTO_BADKEY);
        return;
    }
    cryp_rx.st = CRYPT_ON;
    cryp_tx.st = CRYPT_ON;
    cryp_rx.pack = CRYPT_UNPACK;
    cryp_tx.pack = CRYPT_PACK;
    if (chkhash())
        cmd_ok();
    else {
        cmd_fail(EPROTO_BADHASH);
        return;
    }
    struct passwd *usr = getpwnam(tfproto.defusr);
    if (usr && !setgid(usr->pw_gid) && !setuid(usr->pw_uid))
        logged = 1;
    while (loop)
        cmdifce();
}

static void cmdifce(void)
{
    if (getcmd())
        return;
    comm.buf[COMMBUFLEN - 1] = 0;
    cmd_parse();
}

static int chkhash(void)hdrsetsz
{
    if (readbuf(comm.buf, sizeof comm.buf) == -1)
        return 0;
    else if (!strcmp(comm.buf, tfproto.hash))
        return 1;
    return 0;
}

void cleanup(void)
{
    shm_unlink(comm.srvid);
    close(comm.sock);
}

static int getkey(void)
{
    int rb = readbuf(comm.buf, sizeof comm.buf);
    if (rb == -1)
        return 0;
    memcpy(cryp_rx.enkey, comm.buf, rb);
    return 1;
}

int64_t readbuf_ex(char *buf, int64_t len)
{
    return rcvbuf(comm.sock, buf, len, 1);
}

int64_t writebuf_ex(char *buf, int64_t len)
{
    return sndbuf(comm.sock, buf, len, 1);
}

int64_t readbuf_exfd(int fd, char *buf, int64_t len, int enc)
{
    return rcvbuf(fd, buf, len, enc);
}

int64_t writebuf_exfd(int fd, char *buf, int64_t len, int enc)
{
    return sndbuf(fd, buf, len, enc);
}

static void hdrsetsz(int32_t sz, struct tfhdr *hdr)
{
    if (!isbigendian())
        swapbo32(sz);
    hdr->sz = sz;
}

static int32_t hdrgetsz(struct tfhdr *hdr)
{
    int32_t sz = hdr->sz;
    if (!isbigendian())
        swapbo32(sz);
    return sz;
}

static int64_t sndbuf(int fd, char *buf, int64_t len, int enc)
{
#ifdef DEBUGreadbuf_ex
    static int64_t dc = 0;
    int c = 0;
    printf("\n\t\tSend Counter %lld\n", (long long) dc);
    printf("\t\tSent Length %lld\n\n", (long long) len);
    printf("============= Data before been encrypted =============\n");
    printf("*** Printing in character format ***\n");
    for (; c < len; c++)
        printf("%c", *(buf + c));
    printf("\n\n");
    printf("*** Printing in signed decimal format ***\n");
    for (c = 0; c < len; c++)
        printf("%d ", *(buf + c));
    printf("\n\n");
#endif
    if (enc && cryp_tx.encrypt)
        cryp_tx.encrypt(&cryp_tx, buf, len);
#ifdef DEBUG
    printf("============= Data after been encrypted =============\n");
    printf("*** Printing in character format ***\n");
    for (c = 0; c < len; c++)
        printf("%c", *(buf + c));
    printf("\n\n");
    printf("*** Printing in signed decimal format ***\n");
    for (c = 0; c < len; c++)
        printf("%d ", *(buf + c));
    dc++;
    printf("\n");
#endif
    int64_t wb = 0;
    int64_t written = 0;
    while (len > 0) {
        do {
            wb = write(fd, buf + written, len);
            comm.act++;
        } while (wb == -1 && errno == EINTR);
        if (wb == -1)
            return -1;
        len -= wb;
        written += wb;
    }
    return written;
}

static int64_t rcvbuf(int fd, char *buf, int64_t len, int enc)
{
    int64_t rb = 0;
    int64_t readed = 0;
    while (len > 0) {
        do {
            rb = read(fd, buf + readed, len);
            comm.act++;
        } while (rb == -1 && errno == EINTR);
        if (rb == 0 || rb == -1)
            return -1;
        len -= rb;
        readed += rb;
    }
#ifdef DEBUG
    static int64_t dc = 0;
    int c = 0;
    printf("\n\t\tReceive Counter %lld\n", (long long) dc);
    printf("\t\tReceived Length %lld\n\n", (long long) readed);
    printf("============= Data before been decrypted =============\n");
    printf("*** Printing in character format ***\n");
    for (; c < readed; c++)
        printf("%c", *(buf + c));
    printf("\n\n");
    printf("*** Printing in signed decimal format ***\n");
    for (c = 0; c < readed; c++)
        printf("%d ", *(buf + c));
    printf("\n\n");
#endif
    if (enc && cryp_rx.encrypt)
        cryp_rx.encrypt(&cryp_rx, buf, readed);
#ifdef DEBUG
    printf("============= Data after been decrypted =============\n");
    printf("*** Printing in character format ***\n");
    for (c = 0; c < readed; c++)
        printf("%c", *(buf + c));
    printf("\n\n");
    printf("*** Printing in signed decimal format ***\n");
    for (c = 0; c < readed; c++)
        printf("%d ", *(buf + c));
    dc++;
    printf("\n");
#endif
    return readed;
}

static void *oobthread(void *prms)
{
    sigset_t mask;
    sigfillset(&mask);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);
    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGURG);
    int signo;
    char oobc = 1;
    while (1) {
        sigwait(&sigs, &signo);
        if (signo == SIGURG) {
            recv(comm.sock, &oobc, sizeof oobc, MSG_OOB);
            if (tfproto.trp_tp != TRP_NONE) {
                send(comm.sock_prox, &oobc, sizeof oobc, MSG_OOB);
                recv(comm.sock_prox, &oobc, sizeof oobc, MSG_OOB);
            }
            send(comm.sock, &oobc, sizeof oobc, MSG_OOB);
            if (comm.shmobj != NULL)
                comm.shmobj->oobcount++;
        }
    }
    return NULL;
}

static void genkeepkey(void)
{
    char date[UXTIMELEN] = "";
    time_t t = time(NULL);
    gettm(&t, date);
    char pid[INTDIGITS] = "";
    sprintf(pid, "%d", getpid());
    strcpy(comm.srvid, "/");
    strcat(comm.srvid, date);
    strcat(comm.srvid, pid);
    comm.shmfd = shm_open(comm.srvid, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (comm.shmfd == -1) {
        strcpy(comm.srvid, "");
        return;
    }
    if (ftruncate(comm.shmfd, sizeof(struct shm_obj)) == -1) {
        strcpy(comm.srvid, "");
        close(comm.shmfd);
        return;
    }
    comm.shmobj = mmap(NULL, sizeof(struct shm_obj), PROT_READ | PROT_WRITE,
        MAP_SHARED, comm.shmfd, 0);
    if (!comm.shmobj) {
        strcpy(comm.srvid, "");
        close(comm.shmfd);
        return;
    }
    comm.shmobj->oobcount = 0;
}

int secfs_proc(const char *src)
{
    char path[PATH_MAX];
    strcpy(path, src);
    unsigned int fsopold = fsop;
    fsop = 0;
    if (strstr(path, FSMETA))
        return -1;
    if (fsop_ovrr) {
        fsop_ovrr = 0;
        return 0;
    }
    struct stat st = { 0 };
    stat(path, &st);
    if (!S_ISDIR(st.st_mode))
        rmtrdir(path);
    strcat(path, "/");
    strcat(path, FSMETA);
    strcat(path, FSMETARD_EXT);
    if (access(path, F_OK))
        return 0;
    if (!fsopold)
        return -1;
    unsigned int perm = getfsidperm(path, fsid);
    if (!perm)
        perm = getfsidperm(path, FSOTHERUSR);
    if ((perm & fsopold) != fsopold)
        return -1;
    return 0;
}

unsigned int getfsidperm(const char *path, const char *id)
{
    char line[LINE_MAX];
    FILE *fs = fopen(path, "r");
    if (!fs)
        return 0;
    char *pt;
    int found = 0;
    while (fgets(line, sizeof line, fs)) {
        pt = strchr(line, FSPERM_TOK);
        if (pt) {
            *pt = '\0';
            if (!strcmp(id, line)) {
                found = 1;
                break;
            }
        }
    }
    fclose(fs);
    if (found)
        return atoi(++pt);
    return 0;
}
