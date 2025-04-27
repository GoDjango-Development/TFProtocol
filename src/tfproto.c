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
#include <sys/wait.h>
#include <net.h>

/* FAI token sustitute for TFProtocol version. */
#define FAIACCESS_TOK "FAI://"
/* Return value of checkproto for OK version number. */
#define OKPROTO_VER 1
/* Return value of checkproto for FAI granted. */
#define FAIPROTO_GRANTED 2
/* Return value of checkproto for FAI denial. */
#define FAIPROTO_DENIED 3

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
/* Cryptography structures for block cipher crypt and decrypt. */
struct blkcipher cipher_rx;
struct blkcipher cipher_tx;
/* Logged flag. */
extern int logged;
/* Secure FileSystem identity token. */
char fsid[LINE_MAX];
/* Thread handler for signals. */
static pthread_t oobth;
/* Secure FileSystem current operation. */
unsigned int volatile fsop;
volatile int fsop_ovrr;
/* Define if block cipher should be used. */
int blkstatus;
/* FAI key and key length. */
static void *faikey;
static int faikeylen;
/* FAI token time expiration. */
static int64_t faitexp;
/* FAI file token. */
static char faifile[PATH_MAX];
/* UUID for client impersonation avoidance */
static char cia[UUIDCHAR_LEN];
/* Client impersonation avoidance state */
static int cia_st;

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
/* Block cipher sender function. */ 
static int blk_sndbuf(int fd, char *buf, int64_t len, int enc);
/* Block cipher receiver function. */
static int blk_rcvbuf(int fd, char *buf, int64_t len, int enc);
/* Write data to file descriptor. */
static int wrfd(int fd, char *buf, int64_t len);
/* Read data to file descriptor. */
static int rdfd(int fd, char *buf, int64_t len);
/* Check FAI token validity. */
static int chkfaitok(const char *path);
/* Main loop for FAI entrance. */
static void mainloop_fai(void);
/* Generate new symmetric encryptio key for the next FAI access. */
static int renewfaikey(void);
/* Generate UUID Identity for client impersonation avoidance */
void genid(void);
/* Return UUID identity for client impersonation avoidance */
void getcia(char *ciabuf);

void begincomm(int sock, struct sockaddr_in6 *rmaddr, socklen_t *rmaddrsz)
{
    comm.sock = sock;
    comm.addr = *rmaddr;
    comm.addrsz = *rmaddrsz;
    fcntl(comm.sock, F_SETOWN, getpid());
    int r = pthread_create(&oobth, NULL, oobthread, NULL);
    if (r != 0) {
        wrlog(ELOGOOBTHREAD, LGC_CRITICAL);
        avoidz();
        exit(EXIT_FAILURE);
    }
    if (tfproto.trp_tp != TRP_NONE)
        trp();
    genkeepkey();
    initcrypto(&cryp_rx);
	genid();
#ifndef DEBUG
    mainloop();
    cleanup();
    avoidz();
    exit(EXIT_SUCCESS);
#else
    mainloop();
    cleanup();
    execv(*tfproto.argv, (char *const *) tfproto.argv);
    /* Extra protection bit in case execv fails. */
    exit(EXIT_SUCCESS);
#endif
}

int chkproto(void)
{
    if (readbuf(comm.buf, sizeof comm.buf) == -1)
        return 0;
    if (!strcmp(comm.buf, tfproto.proto))
        return OKPROTO_VER;
    if (strstr(comm.buf, FAIACCESS_TOK) && !chkfaitok(comm.buf + 
        strlen(FAIACCESS_TOK)))
        return FAIPROTO_GRANTED;
    else
        return FAIPROTO_DENIED;
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
    int rc;
    if ((rc = chkproto()) == OKPROTO_VER)
        cmd_ok();
    else if (rc == FAIPROTO_GRANTED) {
        /* Run the mainloop for FAI entrance version. */
        //cmd_ok();
        mainloop_fai();
        return;
    } else if (rc == FAIPROTO_DENIED) {
        cmd_fail(EPROTO_FAITOKEXPIRED);
        return;
    } else {
        cmd_fail(EPROTO_BADVER);
        return;
    }
    if (getkey()) {
        if (derankey(&cryp_rx, tfproto.priv) == -1) {
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
    comm.buf[COMMBUFLEN - 1] = '\0';
     if (blkstatus)
        comm.buf[comm.buflen < COMMBUFLEN - 1 ? comm.buflen:
            COMMBUFLEN -1] = '\0';
    cmd_parse();
}

static int chkhash(void)
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
    if (blkstatus)
        return blk_sndbuf(fd, buf, len, enc);
#ifdef DEBUG
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
    return wrfd(fd, buf, len);
}

static int64_t rcvbuf(int fd, char *buf, int64_t len, int enc)
{
    if (blkstatus)
        return blk_rcvbuf(fd, buf, len, enc);
    int64_t readed;
    if (cia_st) {
        char cia_rcv[UUIDCHAR_LEN];
        cia_rcv[UUIDCHAR_LEN - 1] = '\0';
        readed = rdfd(fd, cia_rcv, sizeof cia_rcv - 1);
        if (readed == -1)
            return -1;
        if (enc && cryp_rx.encrypt)
            cryp_rx.encrypt(&cryp_rx, cia_rcv, readed);
        if (strcmp(cia, cia_rcv))
            return -1;
    }
    readed = rdfd(fd, buf, len);
    if (readed == -1)
        return -1;
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

static int blk_sndbuf(int fd, char *buf, int64_t len, int enc)
{
    if (len <= 0)
        return 0;
   /* int64_t stps =  len / BLK_SIZE;
    int64_t i = 0;
    if (stps > 0) {
        for (; i < stps; i++) {
            memcpy(cipher_tx.data, buf + i * BLK_SIZE, BLK_SIZE);
            if (blkencrypt(&cipher_tx, buf + i * BLK_SIZE, cipher_tx.data,
                BLK_SIZE) == -1)
                return -1;
        }
        if (wrfd(fd, buf, stps * BLK_SIZE) == -1)
            return -1;
    }
    if (len % BLK_SIZE) {
        int rc;
        if ((rc = blkencrypt(&cipher_tx, cipher_tx.data, buf + i * BLK_SIZE,
            len % BLK_SIZE)) == -1)
            return -1;
        if (wrfd(fd, cipher_tx.data, BLK_SIZE) == -1)
            return -1;
    }*/
    int64_t aes_len = (len / BLK_SIZE + 1) * BLK_SIZE;
    char *aes_buf = malloc(aes_len);
    int rc = blkencrypt(&cipher_tx, aes_buf, buf, len);
    if (rc == -1) {
        free(aes_buf);
        return -1;
    }
    if (wrfd(fd, aes_buf, rc) == -1) {
        free(aes_buf);
        return -1;
    }
    return len;
}

static int blk_rcvbuf(int fd, char *buf, int64_t len, int enc)
{
    if (len <= 0)
        return 0;
    /*int64_t stps = len / BLK_SIZE;
    int64_t i = 0;
    if (stps > 0) {
        if (rdfd(fd, buf, stps * BLK_SIZE) == -1)
            return -1;
        for (; i < stps; i++) {    
            memcpy(cipher_rx.data, buf + i * BLK_SIZE, BLK_SIZE);
            if (blkdecrypt(&cipher_rx, buf + i * BLK_SIZE, cipher_rx.data,
                BLK_SIZE) == -1)
                return -1;
        }
    }
    if (len % BLK_SIZE) {
        if (rdfd(fd, cipher_rx.data, BLK_SIZE) == -1)
            return -1;
        int rc;
        if ((rc = blkdecrypt(&cipher_rx, cipher_rx.tmpbuf, cipher_rx.data,
            BLK_SIZE)) == -1)
            return -1;
        memcpy(buf + i * BLK_SIZE, cipher_rx.tmpbuf, rc);
    }*/
    int64_t aes_len = (len / BLK_SIZE + 1) * BLK_SIZE;
    char *aes_buf = malloc(aes_len);
    if (!aes_buf)
        return -1;
    if (rdfd(fd, aes_buf, aes_len) == -1) {
        free(aes_buf);
        return -1;
    }
    if (blkdecrypt(&cipher_rx, buf, aes_buf, aes_len) == -1) {
        free(aes_buf);
        return -1;
    }
    return len;
}

int setblkon(void)
{
    if (blkinit_en(&cipher_tx))
        return -1;
    if (blkinit_de(&cipher_rx)) {
        blkfin(&cipher_tx);
        return -1;
    }
    return 0;
}

void setblkoff(void)
{
    blkstatus = 0;
    blkfin(&cipher_tx);
    blkfin(&cipher_rx);
}

void startblk(void)
{
    blkstatus = 1;
}

static int wrfd(int fd, char *buf, int64_t len)
{
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

static int rdfd(int fd, char *buf, int64_t len)
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
    return readed;
}

static int chkfaitok(const char *path)
{
    strcpy(faifile, tfproto.faipath);
    strcat(faifile, path);
    FILE *fs = fopen(faifile, "r");
    if (!fs)
        return -1;
    char line[LINE_MAX];
    if (!fgets(line, sizeof line, fs)) {
        fclose(fs);
        return -1;
    }
    fclose(fs);
    char *pt = strchr(line, ' ');
    if (!pt)
        return -1;
    *pt++ = '\0';
    faitexp = atoll(line);
    if (time(0) >= faitexp) {
        unlink(faifile);
        return -1;
    }
    char *nl = strchr(pt, '\n');
    if (nl)
        *nl = '\0';
    faikey = base64dec(pt, strlen(pt), &faikeylen);
    if (!faikey)
        return -1;
    return 0;
}

static void mainloop_fai(void)
{
    /* Main loop version for FAI entrance. */
    cryp_rx.rndlen = faikeylen;
    cryp_rx.rndkey = faikey;
    if (faikeylen >= KEYMIN)
        cryp_rx.seed = *(int64_t *) cryp_rx.rndkey;
    else {
        cmd_fail(EPROTO_FAIKEYINVALID);
        return;
    }
    if (dup_crypt(&cryp_tx, &cryp_rx)) {
        cmd_fail(EPROTO_FAIKEYINVALID);
        return;
    }
    if (dup_crypt(&cryp_org, &cryp_rx)) {
        cmd_fail(EPROTO_FAIKEYINVALID);
        return;
    }
    cmd_ok();
    cryp_rx.st = CRYPT_ON;
    cryp_tx.st = CRYPT_ON;
    cryp_rx.pack = CRYPT_UNPACK;
    cryp_tx.pack = CRYPT_PACK;
    if (renewfaikey() == -1)
        return;
    struct passwd *usr = getpwnam(tfproto.defusr);
    if (usr && !setgid(usr->pw_gid) && !setuid(usr->pw_uid))
        logged = 1;
    while (loop)
        cmdifce();
}

static int renewfaikey(void)
{
    FILE *fs = fopen(faifile, "w");
    if (!fs)
        return -1;
    int keysz = random() % (FAIMAX_KEYLEN - FAIMIN_KEYLEN + 1) + FAIMIN_KEYLEN;
    char *key = genkey(keysz);
    if (!key) {
        fclose(fs);
        return -1;
    }
    char *b64 = base64en(key, keysz);
    free(key);
    if (!b64) {
        fclose(fs);
        return -1;
    }    
    strcpy(comm.buf, b64);
    fprintf(fs, "%lld %s\n", (long long) faitexp, b64);
    free(b64);
    fclose(fs);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        return -1;
    return 0;
}

void genid(void)
{
    uuidgen(cia);
}

void setcia(int st)
{
    if (st)
        cia_st = 1;
    else
        cia_st = 0;
}

void getcia(char *ciabuf)
{
    strcpy(ciabuf, cia);
}
