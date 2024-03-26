/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <cmd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <init.h>
#include <util.h>
#include <ntfy.h>
#include <mempool.h>
#include <fcntl.h>
#include <utime.h>
#include <xmods.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <pwd.h>
#include <grp.h>
#include <sys/time.h>
#include <tfproto.h>

/* H-P command signal for command operation release. */
#define HPFFIN -127
/* H-P command flow header indicating end-of-file. */
#define HPFEND 0
/* H-P command flow header signaling operation stop (no file deletion). */
#define HPFSTOP -1
/* H-P command flow header signaling operation cancellation (file deletion). */
#define HPFCANCEL -2
/* H-P command flow header signaling operation continuity. */
#define HPFCONT -3
/* H-P command buffer percent from free ram. */
#define HPFBUFP 10
/* FApi overwrite character indicator. */
#define FAPI_OWCH '1'
/* Multi path separator token. */
#define MULTIP_SEPS " | "
/* Buffer length to allocate a signed 64 bit integer. */
#define INT64LEN sizeof(long long) / 2 * 3 + sizeof(long long) + 2
/* Secure directory extension. */
#define SDEXT ".sd/"
/* Length for the user's password. */
#define PASSWDLEN 256
/* Length for the username. */
#define USRNAMLEN 256
/* Line's length of the pswdb file. */
#define PWDBLINE 1024
/* User/Password separator. */
#define USRPWDSEP " "
/* Time separator for higher time resolution timestamps. */
#define TMSEP "."
/* Nigma min session key acceptable length. */
#define NIGMA_MIN 8
/* Jail security tokens file. */
#define JAIL_FILE "jail.key"
/* NULL file path. */
#define NULLFILE "/dev/null"
/* FSIZE fail command flag. */
#define FSIZE_FAIL -1
/* FSIZELS not regular file flag. */
#define FSIZE_NOTREG -2
/* FSIZELS end flag. */
#define FSIZELS_END -3
/* LS command path separator token. */
#define LSV2TOK_SEP "@||@"
/* FTYPE command constants. */
/* FTYPELS end. */
#define FTYPE_END -2
/* Failed. */
#define FTYPE_FAIL -1
/* File type constants. */
/* Directory. */
#define FTYPE_DIR 0
/* Character device. */
#define FTYPE_CHR 1
/* Block device. */
#define FTYPE_BLK 2
/* Regular file. */
#define FTYPE_REG 3
/* FIFO fifo or pipe. */
#define FTYPE_FIFO 4
/* Symbolic link. */
#define FTYPE_SYM 5
/* Socket. */
#define FTYPE_SOCK 6
/* Other. */
#define FTYPE_OTHER 7
/* FSTATLS fail code. */
#define FSTATLS_FAIL -1
/* FSTATLS end code. */
#define FSTATLS_END -2
/* Value for NETLOCK command. */
#define NETLOCK_SWITCH 0
/* Value for NETLOCK_TRY command. */
#define NETLOCKTRY_SWITCH 1
/* NETLOCK descriptor table length. */
#define NETLOCK_LEN 8
/* Standard string length. */
#define STDSTR 255
/* LSV2DOWN success header value. */
#define LSV2DOWN_HDR_SUCCESS -10
/* LSV2DOWN failed header value. */
#define LSV2DOWN_HDR_FAILED -11
/* Max and Min session key lengths for FAITOK interface. */
#define MAX_KEYLEN 256
#define MIN_KEYLEN 128
/* Seconds per minute. */
#define SPM 60

/* Hi-Performance file operations structure. */
struct hpfile {
    uint64_t offst;
    int64_t bufsz;
} static hpf;

/* Hi-Performance file -with cancellations- operations structure. */
struct hpfilecan {
    uint64_t offst;
    int64_t bufsz;
    uint64_t canpt;
} static hpfcan;

/* FSTATLS header. */
#pragma pack(push, 1)
struct fstathdr {
    char code;
    char type;
    uint64_t size;
    uint64_t atime;
    uint64_t mtime;
};
#pragma pack(pop)

struct netlock_d {
    int fd;
    pthread_t th;
    char used;
    unsigned int tmout;
} netlck[NETLOCK_LEN];

/* Flag to signaling GET command flow to stop. */
static volatile int hpfcont;
/* Lock filename. */
char lcknam[PATH_MAX];
/* Logged flag. */
int logged;
/* Global File Stream pointer. */
static FILE *fs;

/* Send file over the network. "del" parameter indicate deletion after 
    finish. */
static void sendfile(const char *path, int del);
/* Receive file over the network. "ow" paramter allow to overwrite file */
static void rcvfile(const char *path, int ow);
/* Get available size for the PUT command buffer.  */
static int64_t getputbuf(void);
/* GET command thread for cancellation requests. */
static void *getstream_cancelth(void *prms);
/* GET command streamer. This function returns -1 only if can't write to the
    socket.*/
static int getstream(int fd, char *buf, int64_t bufsz);
/* PUT command streamer. This function returns -1 only if can't read from the 
    socket.*/
static int putstream(int fd, char *buf, int64_t bufsz, int *del);
/* PUTCAN command streamer -with cancellations-. This function returns -1 only if 
    can't read from the socket.*/
static int putstream_can(int fd, char *buf, int64_t bufsz, int *del, 
    const uint64_t canpt);
/* GETCAN command streamer -with cancellations-. This function returns -1 only if 
    can't read from the socket.*/
static int getstream_can(int fd, char *buf, int64_t bufsz, 
    const uint64_t canpt);
/* LSR_ITER callback. */
static void lsr_callback(const char *root, const char *filename, int isdir);
/* NETLOCK/NETLOCK_TRY switch helper function. */
static void netlock_switch(int s);
/* NETLOCK Timeout watchdog thread. */
static void *netlock_wdg(void *prms);
/* LSRDOWN_ITER callback. */
static void lsrdown_callback(const char *root, const char *filename, int isdir);

void cmd_echo(void)
{
    int len = strlen(CMD_ECHO) + 1;
    if (writebuf(comm.buf + len, strlen(comm.buf) - len) == -1)
        endcomm();
}

void cmd_ok(void)
{
    strcpy(comm.buf, CMD_OK);
    if (writebuf(comm.buf, strlen(CMD_OK)) == -1)
        endcomm();
}

void cmd_fail(const char *arg)
{
    strcpy(comm.buf, CMD_FAILED);
    strcat(comm.buf, CMD_SEPSTR);
    int arglen = 0;
    if (arg) {
        strcat(comm.buf, arg);
        arglen = strlen(arg);
    }
    if (writebuf(comm.buf, strlen(CMD_FAILED) + arglen + 1) == -1)
        endcomm();
}

void cmd_unknown(void)
{
    strcpy(comm.buf, CMD_UNKNOWN);
    if (writebuf(comm.buf, strlen(CMD_UNKNOWN)) == -1)
        endcomm();
}

void cmd_parse(void)
{
    char cmd[CMD_NAMELEN];
    excmd(comm.buf, cmd);
    cmdtoupper(cmd);
    if (!logged)
        if (!strcmp(cmd, CMD_END)) {
            endcomm();
            return;
        } else if (!strcmp(cmd, CMD_LOGIN)) {
            cmd_login();
            return;
        } else if (!strcmp(cmd, CMD_KEEPALIVE)) {
            cmd_keepalive();
            return;
        } else if (!strcmp(cmd, CMD_PROCKEY)) {
            cmd_prockey();
            return;
        } else {
            cmd_fail(EPROTO_ENOTLOGGED);
            return;
        }
    if (!strcmp(cmd, CMD_END)) {
        endcomm();
        return;
    } 
    if (tfproto.injail) {
        if (!strcmp(cmd, CMD_INJAIL)) {
            cmd_injail();
            return;
        } else {
            cmd_fail(EPROTO_ENOTINJAILED);
            return;
        }
    }
    if (tfproto.locksys) {
        if (!strcmp(cmd, CMD_LOCKSYS)) {
            cmd_locksys();
            return;
        } else {
            cmd_fail(EPROTO_ELOCKSYS);
            return;
        }
    }
    if (tfproto.flycontext) {
        if (!strcmp(cmd, CMD_FLYCONTEXT)) {
            cmd_flycontext();
            return;
        } else {
            cmd_fail(CMD_EFLYCONTEXT);
            return;
        }
    } 
    if (!strcmp(cmd, CMD_ECHO))
        cmd_echo();
    else if (!strcmp(cmd, CMD_MKDIR))
        cmd_mkdir();
    else if (!strcmp(cmd, CMD_LS))
        cmd_ls();
    else if (!strcmp(cmd, CMD_ADDNTFY))
        cmd_addntfy();
    else if (!strcmp(cmd, CMD_STARTNTFY))
        cmd_startntfy();
    else if (!strcmp(cmd, CMD_RCVFILE))
        cmd_sendfile();
    else if (!strcmp(cmd, CMD_SNDFILE))
        cmd_rcvfile();
    else if (!strcmp(cmd, CMD_DEL))
        cmd_del();
    else if (!strcmp(cmd, CMD_RMDIR))
        cmd_rmdir();
    else if (!strcmp(cmd, CMD_LSR))
        cmd_lsr();
    else if (!strcmp(cmd, CMD_COPY))
        cmd_copy();
    else if (!strcmp(cmd, CMD_CPDIR))
        cmd_cpdir();
    else if (!strcmp(cmd, CMD_XCOPY))
        cmd_xcopy();
    else if (!strcmp(cmd, CMD_XCPDIR))
        cmd_xcpdir();
    else if (!strcmp(cmd, CMD_XDEL))
        cmd_xdel();
    else if (!strcmp(cmd, CMD_XRMDIR))
        cmd_xrmdir();
    else if (!strcmp(cmd, CMD_LOCK))
        cmd_lock();
    else if (!strcmp(cmd, CMD_TOUCH))
        cmd_touch();
    else if (!strcmp(cmd, CMD_DATE))
        cmd_date();
    else if (!strcmp(cmd, CMD_DATEF))
        cmd_datef();
    else if (!strcmp(cmd, CMD_DTOF))
        cmd_dtof();
    else if (!strcmp(cmd, CMD_FTOD))
        cmd_ftod();
    else if (!strcmp(cmd, CMD_FSTAT))
        cmd_fstat();
    else if (!strcmp(cmd, CMD_FUPD))
        cmd_fupd();
    else if (!strcmp(cmd, CMD_RENAM))
        cmd_renam();
    else if (!strcmp(cmd, CMD_KEEPALIVE))
        cmd_keepalive();
    else if (!strcmp(cmd, CMD_PROCKEY))
        cmd_prockey();
    else if (!strcmp(cmd, CMD_PUT))
        cmd_put();
    else if (!strcmp(cmd, CMD_GET))
        cmd_get();
    else if (!strcmp(cmd, CMD_FREESP))
        cmd_freesp();
    else if (!strcmp(cmd, CMD_PUTCAN))
        cmd_putcan();
    else if (!strcmp(cmd, CMD_GETCAN))
        cmd_getcan();
    else if (!strcmp(cmd, CMD_LOGIN))
        cmd_login();
    else if (!strcmp(cmd, CMD_CHMOD))
        cmd_chmod();
    else if (!strcmp(cmd, CMD_CHOWN))
        cmd_chown();
    else if (!strcmp(cmd, CMD_UDATE))
        cmd_udate();
    else if (!strcmp(cmd, CMD_NDATE))
        cmd_ndate();
    else if (!strcmp(cmd, CMD_SHA256))
        cmd_sha256();
    else if (!strcmp(cmd, CMD_NIGMA))
        cmd_nigma();
    else if (!strcmp(cmd, CMD_RMSECDIR))
        cmd_rmsecdir();
    else if (!strcmp(cmd, CMD_INJAIL))
        cmd_injail();
    else if (!strcmp(cmd, CMD_TLB))
        cmd_tlb();
    else if (!strcmp(cmd, CMD_SDOWN))
        cmd_sdown();
    else if (!strcmp(cmd, CMD_SUP))
        cmd_sup();
    else if (!strcmp(cmd, CMD_FSIZE))
        cmd_fsize();
    else if (!strcmp(cmd, CMD_FSIZELS))
        cmd_fsizels();
    else if (!strcmp(cmd, CMD_LSV2))
        cmd_lsrv2(LSRITER_NONR);
    else if (!strcmp(cmd, CMD_LSRV2))
        cmd_lsrv2(LSRITER_R);
    else if (!strcmp(cmd, CMD_FTYPE))
        cmd_ftype();
    else if (!strcmp(cmd, CMD_FTYPELS))
        cmd_ftypels();
    else if (!strcmp(cmd, CMD_FSTATLS))
        cmd_fstatls();
    else if (!strcmp(cmd, CMD_INTREAD))
        cmd_intread();
    else if (!strcmp(cmd, CMD_INTWRITE))
        cmd_intwrite();
    else if (!strcmp(cmd, CMD_NETLOCK))
        cmd_netlock();
    else if (!strcmp(cmd, CMD_NETUNLOCK))
        cmd_netunlock();
    else if (!strcmp(cmd, CMD_NETLOCKTRY))
        cmd_netlocktry();
    else if (!strcmp(cmd, CMD_NETMUTACQ_TRY))
        cmd_netmutacqtry();
    else if (!strcmp(cmd, CMD_NETMUTREL))
        cmd_netmutrel();
    else if (!strcmp(cmd, CMD_SETFSID))
        cmd_setfsid();
    else if (!strcmp(cmd, CMD_SETFSPERM))
        cmd_setfsperm();
    else if (!strcmp(cmd, CMD_REMFSPERM))
        cmd_remfsperm();
    else if (!strcmp(cmd, CMD_GETFSPERM))
        cmd_getfsperm();
    else if (!strcmp(cmd, CMD_ISSECFS))
        cmd_issecfs();
    else if (!strcmp(cmd, CMD_TASFS))
        cmd_tasfs();
    else if (!strcmp(cmd, CMD_RMKDIR))
        cmd_rmkdir();
    else if (!strcmp(cmd, CMD_GETTZ))
        cmd_gettz();
    else if (!strcmp(cmd, CMD_SETTZ))
        cmd_settz();
    else if (!strcmp(cmd, CMD_LOCALTIME))
        cmd_localtime();
    else if (!strcmp(cmd, CMD_DATEFTZ))
        cmd_dateftz();
    else if (!strcmp(cmd, CMD_LSV2DOWN))
        cmd_lsrv2down(LSRITER_NONR);
    else if (!strcmp(cmd, CMD_LSRV2DOWN))
        cmd_lsrv2down(LSRITER_R);
    else if (!strcmp(cmd, CMD_RUNBASH))
        cmd_runbash();
    else if (!strcmp(cmd, CMD_FLYCONTEXT))
        cmd_flycontext();
    else if (!strcmp(cmd, CMD_GOAES))
        cmd_goaes();
    else if (!strcmp(cmd, CMD_GENUUID))
        cmd_genuuid();
    else if (!strcmp(cmd, CMD_TFPCRYPTO))
        cmd_tfpcrypto();
    else if (!strcmp(cmd, CMD_FAITOK))
        cmd_faitok();
    else if (strstr(cmd, CMD_XS))
        run_xmods(cmd);
    else
        cmd_unknown();
}

void cmd_mkdir(void)
{
    char *pt = comm.buf + strlen(CMD_MKDIR) + 1;
    char path[PATH_MAX] = "";
    fsop = SECFS_MKDIR;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    int rv = mkdir(path, DEFDIR_PERM);
    if (rv == -1)
        if (errno == EEXIST) {
            cmd_fail(CMD_EDIREXIST);
            return;
        } else {
            cmd_fail(CMD_EACCESS);
            return;
        }
    cmd_ok();
}

void cmd_ls(void)
{
    char *pt = comm.buf + strlen(CMD_LS) + 1;
    char path[PATH_MAX] = "";
    char cpypath[PATH_MAX];
    fsop = SECFS_LDIR;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 0)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    DIR *dir = opendir(path);
    if (dir == NULL) {
        if (errno == EACCES)
            cmd_fail(CMD_EACCESS);  
        else
            cmd_fail(CMD_ENOTDIR);  
        return;
    }
    closedir(dir);
    realpath(path, cpypath);
    strcat(cpypath, "/");
    struct mempool list;
    mpool_create(&list, PATH_MAX);
    listdir(path, &list, 0);
    const char *const lsnam = "_ls.tmp";
    Tmp tmp = mktmp(lsnam);
    if (!tmp) {
        mpool_destroy(&list);
        cmd_fail(CMD_EMKTMP);  
        return;
    }
    struct {
        char path[PATH_MAX];
    } *prec = list.data;
    int i = 0;
    struct stat st = { 0 };
    while (i < list.count) {
        int scdir = 0;
        lstat(prec->path, &st);
        char rp[PATH_MAX] = "";
        char *rpt = NULL;
        if (S_ISDIR(st.st_mode)) {
            strcpy(path, "D: ");
            realpath(prec->path, rp);
            rpt = strstr(rp, cpypath) + strlen(cpypath);
            if (rpt) {
                strcat(path, rpt);
                strcat(path, "/");
                strcat(path, "\n");
                if (strstr(path, SDEXT))
                    scdir = 1;
            }
        } else if (S_ISREG(st.st_mode)) {
            strcpy(path, "F: ");
            realpath(prec->path, rp);
            rpt = strstr(rp, cpypath) + strlen(cpypath);
            if (rpt) {
                strcat(path, rpt);
                strcat(path, "\n");
            }
        } else {
            strcpy(path, "U: ");
            normpath(prec->path, rp);
            rpt = strstr(rp, cpypath) + strlen(cpypath);
            if (rpt) {
                strcat(path, rpt);
                strcat(path, "\n");
            }
        }
        if (strstr(path, FSMETA))
                scdir = 1;
        if (!scdir)
            writetmp(tmp, path);
        prec++, i++;
    }
    mpool_destroy(&list);
    sendfile(tmppath(tmp), 0);
    freetmp(tmp);
}

static void sendfile(const char *path, int del)
{
    Fapi fapi = fapinit(path, comm.buf, FAPI_READ);
    if (!fapi) {
        cmd_fail(fapierr());
        return;
    }
    int rb;
    int cmdlen = strlen(CMD_CONT);
    while (rb = fapiread(fapi, cmdlen + 1, sizeof comm.buf - cmdlen - 1)) {
        if (rb == -1) {
            fapifree(fapi);
            cmd_fail(fapierr());
            return;
        } 
        strcpy(comm.buf, CMD_CONT);
        comm.buf[cmdlen] = ' ';
        if (writebuf(comm.buf, rb + cmdlen + 1) == -1) {
            endcomm();
            fapifree(fapi);
            return;
        }
        if (getcmd()) {
            fapifree(fapi);
            return;
        }
        if (!strcmp(comm.buf, CMD_BREAK)) {
            fapifree(fapi);
            cmd_ok();
            return;
        }
    }
    if (del)
        unlink(path);
    fapifree(fapi);
    cmd_ok();
}

int getcmd(void)
{
    if ((comm.buflen = readbuf(comm.buf, sizeof comm.buf)) == -1) {
        endcomm();
        return -1;
    }
    return 0;
}

void cmd_sendfile(void)
{
    char *pt = comm.buf + strlen(CMD_RCVFILE) + 1;
    int del = 0;
    if (*pt == FAPI_OWCH)
        del = 1;
    if (!(pt = strstr(pt, CMD_SEPSTR))) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    char path[PATH_MAX] = "";
    fsop = SECFS_RFILE;
    if (jaildir(pt + 1, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    sendfile(path, del);
}

void cmd_rcvfile(void)
{
    char *pt = comm.buf + strlen(CMD_SNDFILE) + 1;
    int ow = 0;
    if (*pt == FAPI_OWCH)
        ow = 1;
    if (!(pt = strstr(pt, CMD_SEPSTR))) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    char path[PATH_MAX] = "";
    fsop = SECFS_WFILE;
    if (jaildir(pt + 1, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    rcvfile(path, ow);
}

static void rcvfile(const char *path, int ow)
{
    Fapi fapi = NULL;
    if (ow)
        fapi = fapinit(path, comm.buf, FAPI_OVERWRITE);
    else
        fapi = fapinit(path, comm.buf, FAPI_WRITE);
    if (!fapi) {
        cmd_fail(fapierr());
        return;
    }
    int cmdlen = 0;
    char cmd[CMD_NAMELEN];
    int rb;
    while (1) {
        strcpy(comm.buf, CMD_CONT);
        if (writebuf(comm.buf, strlen(CMD_CONT)) == -1) {
            endcomm();
            fapifree(fapi);
            return;
        }
        if ((rb = readbuf(comm.buf, sizeof comm.buf)) == -1) {
            endcomm();
            fapifree(fapi);
            return;
        }
        excmd(comm.buf, cmd);
        if (!strcmp(cmd, CMD_BREAK)) {
            unlink(path);
            break;
        } else if (!strcmp(cmd, CMD_OK))
            break;
        cmdlen = strlen(cmd);
        if (fapiwrite(fapi, cmdlen + 1, rb - cmdlen - 1) == -1) {
            if (errno == ENOSPC) {
                fapifree(fapi);
                unlink(path);
                cmd_fail(CMD_EQUOTA);
                return;
            }
            endcomm();
            fapifree(fapi);
            return;
        }
    }
    fapifree(fapi);
    cmd_ok();
}

void excmd(const char *src, char *cmd)
{
    int c = 0;
    memset(cmd, 0, CMD_NAMELEN);
    while (*src != CMD_SEPCH && *src && c < CMD_NAMELEN - 1)
        *cmd++ = *src++, c++;
    *cmd = '\0';
}

void cmd_del(void)
{
    char *pt = comm.buf + strlen(CMD_DEL) + 1;
    char path[PATH_MAX] = "";
    fsop = SECFS_DFILE;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    char *trail = strstr(path, lcknam);
    if (trail && !strcmp(trail, lcknam))
        goto SKIPLOCK;
    if (!trylck(path, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
SKIPLOCK:
    if (access(path, F_OK)) {
        cmd_fail(CMD_EFILENOENT);
        return;
    }
    struct stat st = { 0 };
    stat(path, &st);
    if (S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_EISDIR);
        return;
    }
    if (unlink(path) == -1) {
        if (errno == EACCES)
            cmd_fail(CMD_EACCESS);
        else if (errno == ENOENT)
            cmd_fail(CMD_EFILENOENT);
        else
            cmd_fail(NULL);
        return;
    }
    cmd_ok();
}

void cmd_rmdir(void)
{
    char *pt = comm.buf + strlen(CMD_RMDIR) + 1;
    char path[PATH_MAX] = "";
    fsop = SECFS_RMDIR;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    if (rmr(path, SDEXT) == -1) {
        cmd_fail(CMD_EDIRSTILL);
        return;
    }
    if (access(tfproto.dbdir, F_OK) == -1)
        mkdir(tfproto.dbdir, DEFDIR_PERM);
    cmd_ok();
}

void cmd_lsr(void)
{
    char *pt = comm.buf + strlen(CMD_LSR) + 1;
    char path[PATH_MAX] = "";
    char cpypath[PATH_MAX];
    fsop = SECFS_LRDIR;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 0)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    DIR *dir = opendir(path);
    if (dir == NULL) {
        if (errno == EACCES)
            cmd_fail(CMD_EACCESS);  
        else
            cmd_fail(CMD_ENOTDIR);  
        return;
    }
    closedir(dir);
    realpath(path, cpypath);
    strcat(cpypath, "/");
    struct mempool list;
    mpool_create(&list, PATH_MAX);
    listdir(path, &list, 1);
    struct {
        char path[PATH_MAX];
    } *prec = list.data;
    int i = 0;
    const char *const lsnam = "_ls.tmp";
    Tmp tmp = mktmp(lsnam);
    if (!tmp) {
        mpool_destroy(&list);
        cmd_fail(CMD_EMKTMP);
        return;
    }
    struct stat st = { 0 };
    memset(path, 0, sizeof path);
    while (i < list.count) {
        int scdir = 0;
        lstat(prec->path, &st);
        char rp[PATH_MAX] = "";
        char *rpt = NULL;
        if (S_ISDIR(st.st_mode)) {
            strcpy(path, "D: ");
            realpath(prec->path, rp);
            rpt = strstr(rp, cpypath) + strlen(cpypath);
            if (rpt) {
                strcat(path, rpt);
                strcat(path, "/");
                strcat(path, "\n");
                if (strstr(path, SDEXT))
                    scdir = 1;
            }
        } else if (S_ISREG(st.st_mode)) {
            strcpy(path, "F: ");
            realpath(prec->path, rp);
            rpt = strstr(rp, cpypath) + strlen(cpypath);
            if (rpt) {
                strcat(path, rpt);
                strcat(path, "\n");
                if (strstr(path, SDEXT))
                    scdir = 1;
            }
        } else {
            strcpy(path, "U: ");
            normpath(prec->path, rp);
            rpt = strstr(rp, cpypath) + strlen(cpypath);
            if (rpt) {
                strcat(path, rpt);
                strcat(path, "\n");
                if (strstr(path, SDEXT))
                    scdir = 1;
            }
        }
        if (strstr(path, FSMETA))
            scdir = 1;
        if (!scdir)
            writetmp(tmp, path);
        prec++, i++;
    }
    mpool_destroy(&list);
    sendfile(tmppath(tmp), 0);
    freetmp(tmp);
}

void cmd_copy(void)
{
    char *pt = comm.buf + strlen(CMD_COPY) + 1;
    char srcp[PATH_MAX] = "", dstp[PATH_MAX] = "";
    char *ppt;
    if (!(ppt = strstr(pt, MULTIP_SEPS))) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    strcpy(dstp, ppt + strlen(MULTIP_SEPS));
    *ppt = '\0';
    strcpy(srcp, pt);
    char path[PATH_MAX] = "";
    fsop = SECFS_RFILE;
    if (jaildir(srcp, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    struct stat st = { 0 };
    stat(path, &st);
    if (S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_ELINKDIR);
        return;
    }
    strcpy(srcp, path);
    fsop = SECFS_WFILE;
    if (jaildir(dstp, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    strcpy(dstp, path);
    if (link(srcp, dstp) == -1) 
        if (errno == EACCES) {
            cmd_fail(CMD_EACCESS);
            return;
        } else if (errno == EEXIST) {
            cmd_fail(CMD_EFILEXIST);
            return;
        } else if (errno == ENOENT) {
            cmd_fail(CMD_ESRCFILE);
            return;
        } else {
            cmd_fail(CMD_ECPYERR);
            return;
        }
    cmd_ok();
}

void cmd_cpdir(void)
{
    char *pt = comm.buf + strlen(CMD_CPDIR) + 1;
    char srcp[PATH_MAX] = "", dstp[PATH_MAX] = "";
    char *ppt;
    if (!(ppt = strstr(pt, MULTIP_SEPS))) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    strcpy(dstp, ppt + strlen(MULTIP_SEPS));
    *ppt = '\0';
    strcpy(srcp, pt);
    char path[PATH_MAX] = "";
    fsop = SECFS_RFILE;
    if (jaildir(srcp, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 0)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    struct stat st = { 0 };
    stat(path, &st);
    if (!S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_ESRCNODIR);
        return;
    }
    strcpy(srcp, path);
    fsop = SECFS_WFILE | SECFS_MKDIR;
    if (jaildir(dstp, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    if (!access(path, F_OK)) {
        cmd_fail(CMD_EDIREXIST);
        return;
    }
    strcpy(dstp, path);
    if (cpr(srcp, dstp) == -1) {
        cmd_fail(CMD_EDIRTREE);
        return;   
    }
    cmd_ok();
}

void cmd_xcopy(void)
{
    char *pt = comm.buf + strlen(CMD_XCOPY) + 1;
    char newn[PATH_MAX] = "", dstpat[PATH_MAX] = "", srcp[PATH_MAX] = "";
    char *ppt;
    if (!(ppt = strstr(pt, CMD_SEPSTR))) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt = '\0';
    strcpy(newn, pt);
    pt += strlen(newn) + 1;
    ppt = strstr(pt, MULTIP_SEPS);
    if (!ppt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt = '\0';
    strcpy(srcp, pt);
    pt += strlen(srcp) + strlen(MULTIP_SEPS);
    strcpy(dstpat, pt);
    char path[PATH_MAX] = "";
    if (jaildir(srcp, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    strcpy(srcp, path);
    if (access(srcp, F_OK)) {
        cmd_fail(CMD_ESRCFILE);
        return;
    }
    struct stat st = { 0 };
    stat(srcp, &st);
    if (!S_ISREG(st.st_mode)) {
        cmd_fail(CMD_ESRCNOFILE);
        return;
    }
    struct mempool list;
    mpool_create(&list, PATH_MAX);
    listdir(tfproto.dbdir, &list, 1);
    struct {
        char path[PATH_MAX];
    } *prec = list.data;
    int  i = 0;
    while (i < list.count) {
        if ((ppt = strstr(prec->path, dstpat)) && strlen(ppt) == 
            strlen(dstpat)) {
            strcpy(path, prec->path);
            strcat(path, "/");
            strcat(path, newn);
            if (trylck(path, lcknam, 1))
                link(srcp, path);
        }
        prec++, i++;
    }
    mpool_destroy(&list);
    cmd_ok();
}

void cmd_xcpdir(void)
{
    char *pt = comm.buf + strlen(CMD_XCPDIR) + 1;
    char newn[PATH_MAX] = "", dstpat[PATH_MAX] = "", srcp[PATH_MAX] = "";
    char *ppt;
    if (!(ppt = strstr(pt, CMD_SEPSTR))) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt = '\0';
    strcpy(newn, pt);
    pt += strlen(newn) + 1;
    ppt = strstr(pt, MULTIP_SEPS);
    if (!ppt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt = '\0';
    strcpy(srcp, pt);
    pt += strlen(srcp) + strlen(MULTIP_SEPS);
    strcpy(dstpat, pt);
    char path[PATH_MAX] = "";
    if (jaildir(srcp, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 0)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    strcpy(srcp, path);
    if (access(srcp, F_OK)) {
        cmd_fail(CMD_ESRCPATH);
        return;
    }
    struct stat st = { 0 };
    stat(srcp, &st);
    if (!S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_ESRCNODIR);
        return;
    }
    struct mempool list;
    mpool_create(&list, PATH_MAX);
    listdir(tfproto.dbdir, &list, 1);
    struct {
        char path[PATH_MAX];
    } *prec = list.data;
    int i = 0;
    while (i < list.count) {
        if ((ppt = strstr(prec->path, dstpat)) && strlen(ppt) == 
            strlen(dstpat)) {
            strcpy(path, prec->path);
            strcat(path, "/");
            strcat(path, newn);
            if (trylck(path, lcknam, 1))
                cpr(srcp, path);
        }
        i++; prec++;
    }
    mpool_destroy(&list);
    cmd_ok();
}

void cmd_xdel(void)
{
    char *pt = comm.buf + strlen(CMD_XDEL) + 1;
    char nam[PATH_MAX] = "", srcp[PATH_MAX] = "";
    char *ppt;
    if (!(ppt = strstr(pt, CMD_SEPSTR))) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt = '\0';
    strcpy(srcp, pt);
    pt += strlen(srcp) + 1;
    strcpy(nam, pt);
    char path[PATH_MAX] = "";
    if (jaildir(srcp, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 0)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    strcpy(srcp, path);
    if (access(srcp, F_OK)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    struct stat st = { 0 };
    stat(srcp, &st);
    if (!S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_ESRCNODIR);
        return;
    }
    struct mempool list;
    mpool_create(&list, PATH_MAX);
    listdir(path, &list, 1);
    struct {
        char path[PATH_MAX];
    } *prec = list.data;
    int i = 0;
    while (i < list.count) {
        if ((ppt = strstr(prec->path, nam)) && strlen(ppt) == strlen(nam))
            if (trylck(prec->path, lcknam, 1))
                unlink(prec->path);
        i++; prec++;
    }
    mpool_destroy(&list);
    cmd_ok();
}

void cmd_xrmdir(void)
{
    char *pt = comm.buf + strlen(CMD_XRMDIR) + 1;
    char nam[PATH_MAX] = "", srcp[PATH_MAX] = "";
    char *ppt;
    if (!(ppt = strstr(pt, CMD_SEPSTR))) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt = '\0';
    strcpy(srcp, pt);
    pt += strlen(srcp) + 1;
    strcpy(nam, pt);
    char path[PATH_MAX] = "";
    if (jaildir(srcp, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 0)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    strcpy(srcp, path);
    if (access(srcp, F_OK)) {
        cmd_fail(CMD_ENOTDIR);
        return;
    }
    struct stat st = { 0 };
    stat(srcp, &st);
    if (!S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_ESRCNODIR);
        return;
    }
    struct mempool list;
    mpool_create(&list, PATH_MAX);
    listdir(path, &list, 1);
    struct {
        char path[PATH_MAX];
    } *prec = list.data;
    int i = 0;
    while (i < list.count) {
        if ((ppt = strstr(prec->path, nam)) && strlen(ppt) == strlen(nam)) {
            stat(prec->path, &st);
            if (S_ISDIR(st.st_mode) && trylck(prec->path, lcknam, 0))
                rmr(prec->path, SDEXT);
        }
        i++; prec++;
    }
    mpool_destroy(&list);
    cmd_ok();
}

void cmd_lock(void)
{
    char *pt = comm.buf + strlen(CMD_LOCK) + 1;
    strcpy(lcknam, pt);
    if (strstr(lcknam, ".") || strstr(lcknam, "..") || strstr(lcknam, "/")) {
        cmd_fail(CMD_EINVLCK);
        return;
    }
    cmd_ok();
}

void cmd_touch(void)
{
    char *pt = comm.buf + strlen(CMD_TOUCH) + 1;
    char file[PATH_MAX] = "";
    fsop = SECFS_WFILE;
    if (jaildir(pt, file)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(file, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    if (!access(file, F_OK)) {
        cmd_fail(CMD_EFILEXIST);
        return;
    }
    int fd = open(file, O_RDWR | O_CREAT, DEFFILE_PERM);
    if (!fd) {
        cmd_fail(CMD_ETOUCH);
        return;
    }
    close(fd);
    cmd_ok();
}

void cmd_date(void)
{
    char date[UXTIMELEN] = "";
    time_t t = time(NULL);
    gettm(&t, date);
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, date);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_datef(void)
{
    char date[HTIMELEN] = "";
    time_t t = time(NULL);
    gettmf(&t, date);
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, date);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_dtof(void)
{
    char *pt = comm.buf + strlen(CMD_DTOF) + 1;
    char date[HTIMELEN] = "";
    strcpy_sec(date, pt, UXTIMELEN);
    if (!strcmp(date, "")) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    time_t t = atoll(date);
    if (gettmf(&t, date) == -1) {
        cmd_fail(CMD_EBADDATE);
        return;
    }
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, date);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_ftod(void)
{
    char *pt = comm.buf + strlen(CMD_FTOD) + 1;
    char date[HTIMELEN] = "";
    strcpy_sec(date, pt, HTIMELEN);
    if (!strcmp(date, "")) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    strtotm(date);
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, date);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_fstat(void)
{
    char *pt = comm.buf + strlen(CMD_FSTAT) + 1;
    char path[PATH_MAX] = "";
    if (!strcmp(pt, "")) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    fsop = SECFS_STAT;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    struct stat st = { 0 };
    stat(path, &st);
    if (S_ISDIR(st.st_mode)) {
        if (!trylck(path, lcknam, 0)) {
            cmd_fail(CMD_ELOCKED);
            return;
        }
    } else if (!trylck(path, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    if (access(path, F_OK)) {
        cmd_fail(CMD_EFILENOENT);
        return;
    }
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    stat(path, &st);
    if (S_ISDIR(st.st_mode))
        strcat(comm.buf, "D ");
    else if (S_ISREG(st.st_mode))
        strcat(comm.buf, "F ");
    else
        strcat(comm.buf, "U ");
    char int64b[INT64LEN] = "";
    sprintf(int64b, "%llu", (unsigned long long) st.st_size);
    strcat(comm.buf, int64b);
    strcat(comm.buf, CMD_SEPSTR);
    sprintf(int64b, "%lld", (long long int) st.st_atime);
    strcat(comm.buf, int64b);
    strcat(comm.buf, CMD_SEPSTR);
    sprintf(int64b, "%lld", (long long int) st.st_mtime);
    strcat(comm.buf, int64b);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_fupd(void)
{
    char *pt = comm.buf + strlen(CMD_FUPD) + 1;
    char path[PATH_MAX] = "";
    if (!strcmp(pt, "")) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    fsop = SECFS_FDUPD;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    struct stat st = { 0 };
    stat(path, &st);
    if (S_ISDIR(st.st_mode)) {
        if (!trylck(path, lcknam, 0)) {
            cmd_fail(CMD_ELOCKED);
            return;
        }
    } else if (!trylck(path, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    if (access(path, F_OK)) {
        cmd_fail(CMD_EFILENOENT);
        return;
    }
    if (utime(path, NULL) == -1) {
        cmd_fail(CMD_EBADUPD);
        return;
    }
    cmd_ok();
}

void cmd_renam(void)
{
    char *pt = comm.buf + strlen(CMD_RENAM) + 1;
    char srcp[PATH_MAX] = "", dstp[PATH_MAX] = "";
    char *ppt;
    if (!(ppt = strstr(pt, MULTIP_SEPS))) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    strcpy(dstp, ppt + strlen(MULTIP_SEPS));
    if (!strcmp(dstp, "")) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt = '\0';
    strcpy(srcp, pt);
    char path[PATH_MAX] = "";
    fsop = SECFS_RFILE;
    if (jaildir(srcp, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    struct stat st = { 0 };
    stat(path, &st);
    if (S_ISDIR(st.st_mode)) {
        if (!trylck(path, lcknam, 0)) {
            cmd_fail(CMD_ELOCKED);
            return;
        }
    } else {
        if (!trylck(path, lcknam, 1)) {
            cmd_fail(CMD_ELOCKED);
            return;
        }
    }
    strcpy(srcp, path);
    fsop = SECFS_WFILE;
    if (jaildir(dstp, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    stat(path, &st);
    if (S_ISDIR(st.st_mode)) {
        if (!trylck(path, lcknam, 0)) {
            cmd_fail(CMD_ELOCKED);
            return;
        }
    } else {
        if (!trylck(path, lcknam, 1)) {
            cmd_fail(CMD_ELOCKED);
            return;
        }
    }
    strcpy(dstp, path);
    if (rename(srcp, dstp) == -1) {
        cmd_fail(CMD_ERENAM);
        return;
    }
    cmd_ok();
}

void cmd_keepalive(void) 
{
    char *pt = comm.buf + strlen(CMD_KEEPALIVE) + 1;
    char alive[INTDIGITS], idle[INTDIGITS] = "", intvl[INTDIGITS] = "",
        cnt[INTDIGITS] = "";
    char *ppt;
    if (!(ppt = strstr(pt, CMD_SEPSTR))) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt = '\0';
    strcpy(alive, pt);
    pt += strlen(alive) + 1;
    ppt = strstr(pt, MULTIP_SEPS);
    if (!ppt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt = '\0';
    strcpy(idle, pt);
    pt += strlen(idle) + strlen(MULTIP_SEPS);
    ppt = strstr(pt, MULTIP_SEPS);
    if (!ppt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt = '\0';
    strcpy(intvl, pt);
    pt += strlen(intvl) + strlen(MULTIP_SEPS);
    strcpy(cnt, pt);
    int optval = atoi(alive);
    int r;
    if (optval) {
        r = setsockopt(comm.sock, SOL_SOCKET, SO_KEEPALIVE, &optval, 
            sizeof optval);
        optval = atoi(idle);
        r += setsockopt(comm.sock, IPPROTO_TCP, TCP_KEEPIDLE, &optval, 
            sizeof optval);
        optval = atoi(intvl);
        r += setsockopt(comm.sock, IPPROTO_TCP, TCP_KEEPINTVL, &optval, 
            sizeof optval);
        optval = atoi(cnt);
        r += setsockopt(comm.sock, IPPROTO_TCP, TCP_KEEPCNT, &optval, 
            sizeof optval);
        optval = (atoi(idle) + atoi(intvl) * atoi(cnt)) * MILLISEC - 1;
        r += setsockopt(comm.sock, IPPROTO_TCP, TCP_USER_TIMEOUT, &optval, 
            sizeof optval);
    } else {
        r = setsockopt(comm.sock, SOL_SOCKET, SO_KEEPALIVE, &optval, 
            sizeof optval);
        r += setsockopt(comm.sock, IPPROTO_TCP, TCP_USER_TIMEOUT, &optval, 
            sizeof optval);
    }
    if (r) {
        optval = 0;
        setsockopt(comm.sock, SOL_SOCKET, SO_KEEPALIVE, &optval, 
            sizeof optval);
        cmd_fail(CMD_EKEEPALIVE);
        return;
    }
    cmd_ok();
}

void cmd_prockey(void)
{
    if (!strcmp(comm.srvid, "")) {
        cmd_fail(CMD_EPROCKEY);
        return;
    } 
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, comm.srvid);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_put(void)
{
    char path[PATH_MAX];
    char *pt = comm.buf + strlen(CMD_PUT) + 1;
    *(pt + (comm.buflen - (strlen(CMD_PUT) + 1) - sizeof hpf - 1)) = '\0';
    fsop = SECFS_WFILE;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    pt = strchr(pt, '\0');
    if (!pt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    memcpy((char *) &hpf, pt + 1, sizeof hpf);
    int fd = open(path, O_WRONLY | O_CREAT, DEFFILE_PERM);
    if (fd == -1) {
        cmd_fail(CMD_EHPFFD);
        return;
    }
    if (!isbigendian())
        swapbo64(hpf.offst);
    if (!isbigendian())
        swapbo64(hpf.bufsz);
    if (lseek(fd, hpf.offst, SEEK_SET) == -1) {
        close(fd);
        cmd_fail(CMD_EHPFSEEK);
        return;
    }
    int64_t bufsz = getputbuf();
    if (hpf.bufsz > bufsz)
        hpf.bufsz = bufsz;
    else
        bufsz = hpf.bufsz;
    char *buf = malloc(bufsz);
    if (!buf) {
        close(fd);
        cmd_fail(CMD_EHPFBUF);
        return;
    }
    if (!isbigendian())
        swapbo64(hpf.bufsz);
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    int wrsz = strlen(comm.buf);
    memcpy(comm.buf + wrsz, (char *) &hpf.bufsz, sizeof hpf.bufsz);
    if (writebuf(comm.buf, wrsz + sizeof hpf.bufsz) == -1) {
        free(buf);
        close(fd);
        endcomm();
        return;
    }
    int del = 0;
    if (putstream(fd, buf, bufsz, &del) == -1)
        endcomm();
    if (del)
        unlink(path);
    free(buf);
    close(fd);
}

static int64_t getputbuf(void)
{
    int64_t apages = sysconf(_SC_AVPHYS_PAGES);
    int64_t pagesz = sysconf(_SC_PAGE_SIZE);
    return apages * pagesz * HPFBUFP / 100;
}

void cmd_get(void)
{
    char path[PATH_MAX];
    char *pt = comm.buf + strlen(CMD_GET) + 1;
    *(pt + (comm.buflen - (strlen(CMD_GET) + 1) - sizeof hpf - 1)) = '\0';
    fsop = SECFS_RFILE;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    pt = strchr(pt, '\0');
    if (!pt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    memcpy((char *) &hpf, pt + 1, sizeof hpf);
    int fd = open(path, O_RDONLY, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        cmd_fail(CMD_EHPFFD);
        return;
    }
    if (!isbigendian())
        swapbo64(hpf.offst);
    if (!isbigendian())
        swapbo64(hpf.bufsz);
    if (lseek(fd, hpf.offst, SEEK_SET) == -1) {
        close(fd);
        cmd_fail(CMD_EHPFSEEK);
        return;
    }
    int64_t bufsz = getputbuf();
    if (hpf.bufsz > bufsz)
        hpf.bufsz = bufsz;
    else
        bufsz = hpf.bufsz;
    char *buf = malloc(bufsz);
    if (!buf) {
        close(fd);
        cmd_fail(CMD_EHPFBUF);
        return;
    }
    if (!isbigendian())
        swapbo64(hpf.bufsz);
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    int wrsz = strlen(comm.buf);
    memcpy(comm.buf + wrsz, (char *) &hpf.bufsz, sizeof hpf.bufsz);
    if (writebuf(comm.buf, wrsz + sizeof hpf.bufsz) == -1) {
        free(buf);
        close(fd);
        endcomm();
        return;
    }
    if (getstream(fd, buf, bufsz) == -1) 
        endcomm();
    free(buf);
    close(fd);
}

static void *getstream_cancelth(void *prms)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    int64_t code;
    while (1) {
        readbuf_ex((char *) &code, sizeof code);
        if (!isbigendian())
            swapbo64(code);
        if (code == HPFCANCEL) {
            hpfcont = 0;
            continue;
        }
        if (code == HPFFIN)
            break;
    }
    return NULL;
}

void cmd_freesp(void)
{
    unsigned long long free = freespace(tfproto.dbdir);
    char fsp[INT64LEN];
    sprintf(fsp, "%llu", free);
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, fsp);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

static int getstream(int fd, char *buf, int64_t bufsz)
{
    hpfcont = 1;
    pthread_t getth;
    int tid = pthread_create(&getth, NULL, getstream_cancelth, NULL);
    int64_t rb;
    int64_t code;
    while (hpfcont) {
        rb = readchunk(fd, buf, bufsz);
        if (rb > 0) {
            int64_t rdhdr = rb;
            if (!isbigendian())
                swapbo64(rdhdr);
            if (writebuf_ex((char *) &rdhdr, sizeof rdhdr) == -1) {
                pthread_cancel(tid);
                return -1;
            }
            if (writebuf_ex(buf, rb) == -1) {
                pthread_cancel(tid);
                return -1;
            }
        } else if (rb == 0) {
            code = HPFEND;
            if (!isbigendian())
                swapbo64(code);
            if (writebuf_ex((char *) &code, sizeof code) == -1) {
                pthread_cancel(tid);
                return -1;
            }
            break;
        } else {
            code = HPFCANCEL;
            if (!isbigendian())
                swapbo64(code);
            if (writebuf_ex((char *) &code, sizeof code) == -1) {
                pthread_cancel(tid);
                return -1;
            }
            break;
        }
    }
    pthread_join(getth, NULL);
    code = HPFFIN;
    if (!isbigendian())
        swapbo64(code);
    if (writebuf_ex((char *) &code, sizeof code) == -1)
        return -1;
    return 0;
}

static int putstream(int fd, char *buf, int64_t bufsz, int *del)
{
    int64_t len;
    int64_t code;
    while (1) {
        if (readbuf_ex((char *) &len, sizeof len) == -1) {
            *del = 1;
            return -1;
        }
        if (!isbigendian())
            swapbo64(len);
        if (len == HPFEND)
            break;
        else if (len == HPFSTOP)
            break;
        else if (len == HPFCANCEL) {
            *del = 1;
            break;
        } else {
            if (readbuf_ex(buf, len) == -1) {
                *del = 1;
                return -1;
            }
            int wb = writechunk(fd, buf, len);
            if (wb != len) {
                code = HPFCANCEL;
                if (!isbigendian())
                    swapbo64(code);
                if (writebuf_ex((char *) &code, sizeof code) == -1) {
                    *del = 1;
                    return -1;
                }
                break;
            }
        }
    }
    code = HPFFIN;
    if (!isbigendian())
        swapbo64(code);
    if (writebuf_ex((char *) &code, sizeof code) == -1)
        return -1;
    while (1) {
        if (readbuf_ex((char *) &len, sizeof len) == -1)
            return -1;
        if (!isbigendian())
            swapbo64(len);
        if (len == HPFCANCEL || len == HPFEND || len == HPFSTOP)
            continue;
        if (len == HPFFIN)
            break;
        else {
            if (readbuf_ex(buf , len) == -1)
                return -1;
        }
    }
    return 0;
}

void cmd_putcan(void)
{
    char path[PATH_MAX];
    char *pt = comm.buf + strlen(CMD_PUTCAN) + 1;
    *(pt + (comm.buflen - (strlen(CMD_PUTCAN) + 1) - sizeof hpfcan - 1)) = '\0';
    fsop = SECFS_WFILE;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    pt = strchr(pt, '\0');
    if (!pt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    memcpy((char *) &hpfcan, pt + 1, sizeof hpfcan);
    int fd = open(path, O_WRONLY | O_CREAT, DEFFILE_PERM);
    if (fd == -1) {
        cmd_fail(CMD_EHPFFD);
        return;
    }
    if (!isbigendian())
        swapbo64(hpfcan.offst);
    if (!isbigendian())
        swapbo64(hpfcan.bufsz);
    if (!isbigendian())
        swapbo64(hpfcan.canpt);
    if (lseek(fd, hpfcan.offst, SEEK_SET) == -1) {
        close(fd);
        cmd_fail(CMD_EHPFSEEK);
        return;
    }
    int64_t bufsz = getputbuf();
    if (hpfcan.bufsz > bufsz)
        hpfcan.bufsz = bufsz;
    else
        bufsz = hpfcan.bufsz;
    char *buf = malloc(bufsz);
    if (!buf) {
        close(fd);
        cmd_fail(CMD_EHPFBUF);
        return;
    }
    if (!isbigendian())
        swapbo64(hpfcan.bufsz);
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    int wrsz = strlen(comm.buf);
    memcpy(comm.buf + wrsz, (char *) &hpfcan.bufsz, sizeof hpfcan.bufsz);
    if (writebuf(comm.buf, wrsz + sizeof hpfcan.bufsz) == -1) {
        free(buf);
        close(fd);
        endcomm();
        return;
    }
    int del = 0;
    if (putstream_can(fd, buf, bufsz, &del, hpfcan.canpt) == -1)
        endcomm();
    if (del)
        unlink(path);
    free(buf);
    close(fd);
}

static int putstream_can(int fd, char *buf, int64_t bufsz, int *del, 
    const uint64_t canpt)
{
    int64_t len;
    int64_t code;
    uint64_t canstep = 0;
    int end = 0;
    while (1) {
        if (readbuf_ex((char *) &len, sizeof len) == -1) {
            *del = 1;
            return -1;
        }
        if (!isbigendian())
            swapbo64(len);
        if (len == HPFEND)
            break;
        else if (len == HPFSTOP)
            break;
        else if (len == HPFCANCEL) {
            *del = 1;
            break;
        } else {
            if (readbuf_ex(buf, len) == -1) {
                *del = 1;
                return -1;
            }
            if (!end) {
                int wb = writechunk(fd, buf, len);
                if (wb != len) {
                    code = HPFCANCEL;
                    end = 1;
                } else 
                    code = HPFCONT;
            }
        }
        if (canpt >= 1) {
            canstep++;
            if (canstep == canpt) {
                if (!isbigendian())
                    swapbo64(code);
                if (writebuf_ex((char *) &code, sizeof code) == -1) {
                    *del = 1;
                    return -1;
                }
                if (end)
                    break;
                canstep = 0;
            }
        }
    }
    return 0;
}

void cmd_getcan(void)
{
    char path[PATH_MAX];
    char *pt = comm.buf + strlen(CMD_GETCAN) + 1;
    *(pt + (comm.buflen - (strlen(CMD_GETCAN) + 1) - sizeof hpfcan - 1)) = '\0';
    fsop = SECFS_RFILE;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    pt = strchr(pt, '\0');
    if (!pt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    memcpy((char *) &hpfcan, pt + 1, sizeof hpfcan);
    int fd = open(path, O_RDONLY, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        cmd_fail(CMD_EHPFFD);
        return;
    }
    if (!isbigendian())
        swapbo64(hpfcan.offst);
    if (!isbigendian())
        swapbo64(hpfcan.bufsz);
    if (!isbigendian())
        swapbo64(hpfcan.canpt);
    if (lseek(fd, hpfcan.offst, SEEK_SET) == -1) {
        close(fd);
        cmd_fail(CMD_EHPFSEEK);
        return;
    }
    int64_t bufsz = getputbuf();
    if (hpfcan.bufsz > bufsz)
        hpfcan.bufsz = bufsz;
    else
        bufsz = hpfcan.bufsz;
    char *buf = malloc(bufsz);
    if (!buf) {
        close(fd);
        cmd_fail(CMD_EHPFBUF);
        return;
    }
    if (!isbigendian())
        swapbo64(hpfcan.bufsz);
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    int wrsz = strlen(comm.buf);
    memcpy(comm.buf + wrsz, (char *) &hpfcan.bufsz, sizeof hpfcan.bufsz);
    if (writebuf(comm.buf, wrsz + sizeof hpfcan.bufsz) == -1) {
        free(buf);
        close(fd);
        endcomm();
        return;
    }
    if (getstream_can(fd, buf, bufsz, hpfcan.canpt) == -1) 
        endcomm();
    free(buf);
    close(fd);
}

static int getstream_can(int fd, char *buf, int64_t bufsz, 
    const uint64_t canpt)
{
    int64_t rb;
    int64_t code;
    uint64_t canstep = 0;
    while (1) {
        rb = readchunk(fd, buf, bufsz);
        if (rb > 0) {
            int64_t rdhdr = rb;
            if (!isbigendian())
                swapbo64(rdhdr);
            if (writebuf_ex((char *) &rdhdr, sizeof rdhdr) == -1)
                return -1;
            if (writebuf_ex(buf, rb) == -1)
                return -1;
        } else if (rb == 0) {
            code = HPFEND;
            if (!isbigendian())
                swapbo64(code);
            if (writebuf_ex((char *) &code, sizeof code) == -1)
                return -1;
            break;
        } else {
            code = HPFCANCEL;
            if (!isbigendian())
                swapbo64(code);
            if (writebuf_ex((char *) &code, sizeof code) == -1)
                return -1;
            break;
        }
        if (canpt >= 1) {
            canstep++;
            if (canstep == canpt) {
                if (readbuf_ex((char *) &code, sizeof code) == -1)
                    return -1;
                if (!isbigendian())
                    swapbo64(code);
                if (code != HPFCONT)
                    break;
                canstep = 0;
            }
        }
    }
    return 0;
}

void cmd_login(void)
{
    if (logged) {
        cmd_fail(CMD_EISINJAILED);
        return;
    }
    char *pt = comm.buf + strlen(CMD_LOGIN) + 1;
    char usr[PATH_MAX], pwd[PATH_MAX];
    char *ppt;
    ppt = strstr(pt, CMD_SEPSTR);
    if (!ppt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt = '\0';
    strcpy(usr, pt);
    strcat(usr, USRPWDSEP);
    pt += strlen(usr);
    strcpy(pwd, pt);
    FILE *pwdb = fopen(tfproto.userdb, "r");
    if (!pwdb) {
        cmd_fail(CMD_ELOGINDB);
        return;
    }
    char buf[PWDBLINE];
    while (fgets(buf, sizeof buf, pwdb))
        if (pt = strstr(buf, usr))
            break;
    *strchr(usr, ' ') = '\0';
    if (!pt) {
        fclose(pwdb);
        cmd_fail(CMD_ELOGIN);
        return;
    }
    fclose(pwdb);
    pt += strlen(usr) + 1;
    char *ptend = strchr(pt, '\n');
    if (ptend)
        *ptend = '\0';
    if (!strcmp(pt, pwd)) {
        struct passwd *sysusr = getpwnam(usr);
        if (!sysusr || setgid(sysusr->pw_gid) == -1 || setuid(sysusr->pw_uid) ==
            -1) {
            cmd_fail(CMD_ELOGIN);
            return;
        }
        logged = 1;
    }
    if (!logged) {
        cmd_fail(CMD_ELOGIN);
        return;
    }
    cmd_ok();
}

void cmd_chmod(void)
{
    char *pt = comm.buf + strlen(CMD_CHMOD) + 1;
    char path[PATH_MAX], bits[INTDIGITS];
    char *ppt;
    ppt = strstr(pt, CMD_SEPSTR);
    if (!ppt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt++ = '\0';
    fsop = SECFS_UXPERM;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    strcpy(bits, ppt);
    int mode = strtol(bits, NULL, 8);
    if (chmod(path, mode) == -1)
        if (errno == ENOENT) {
            cmd_fail(CMD_EFILENOENT);
            return;
        } else if (errno == EPERM) {
            cmd_fail(CMD_ECHMOD);
            return;
        }
    cmd_ok();
}

void cmd_chown(void)
{
    char *pt = comm.buf + strlen(CMD_CHOWN) + 1;
    char path[PATH_MAX], usr[USRNAMLEN], grp[USRNAMLEN];
    char *ppt;
    ppt = strstr(pt, CMD_SEPSTR);
    if (!ppt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt++ = '\0';
    fsop = SECFS_UXPERM;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    pt = ppt++;
    ppt = strstr(ppt, CMD_SEPSTR);
    if (!ppt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *ppt++ = '\0';
    strcpy(usr, pt);
    strcpy(grp, ppt);
    struct passwd *sysusr = getpwnam(usr);
    if (!sysusr) {
        cmd_fail(CMD_ECHOWNUSR);
        return;
    }
    struct group *sysgrp = getgrnam(grp);
    if (!sysgrp) {
        cmd_fail(CMD_ECHOWNGRP);
        return;
    }
    if (chown(path, sysusr->pw_uid, sysgrp->gr_gid) == -1) {
        cmd_fail(CMD_ECHOWN);
        return;
    }
    cmd_ok();
}

void cmd_udate(void)
{
    char sec[UXTIMELEN] = "";
    char usec[UXTIMELEN] = "";
    struct timeval tv;
    gettimeofday(&tv, NULL);
    sprintf(sec, "%llu", (unsigned long long) tv.tv_sec);
    sprintf(usec, "%lld", (long long) tv.tv_usec);
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, sec);
    strcat(comm.buf, TMSEP);
    strcat(comm.buf, usec);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_ndate(void)
{
    char sec[UXTIMELEN] = "";
    char nsec[UXTIMELEN] = "";
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    sprintf(sec, "%llu", (unsigned long long) tv.tv_sec);
    sprintf(nsec, "%lld", (long long) tv.tv_nsec);
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, sec);
    strcat(comm.buf, TMSEP);
    strcat(comm.buf, nsec);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_sha256(void)
{
    char *pt = comm.buf + strlen(CMD_SHA256) + 1;
    char path[PATH_MAX];
    strcpy(path, pt);
    if (!strcmp(path, "")) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    fsop = SECFS_RFILE;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    char sha256_str[SHA256STR_LEN];
    if (sha256sum(path, sha256_str) == -1) {
        cmd_fail(CMD_ESHA256);
        return;
    }
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, HEXPREFIX);
    strcat(comm.buf, sha256_str);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_nigma(void)
{
    char *pt = comm.buf + strlen(CMD_NIGMA) + 1;
    int keylen = atoi(pt);
    if (keylen < NIGMA_MIN) {
        cmd_fail(CMD_ENIMGA);
        return;
    }
    char *key_rx = malloc(keylen);
    if (!key_rx) {
        cmd_fail(CMD_ENIMGA);
        return;
    }
    char *key_tx = malloc(keylen);
    if (!key_tx) {
        cmd_fail(CMD_ENIMGA);
        return;
    }
    randent(key_rx, keylen);
    char *keycp = malloc(keylen);
    if (!keycp) {
        free(key_rx);
        free(key_tx);
        cmd_fail(CMD_ENIMGA);
        return;
    }
    memcpy(key_tx, key_rx, keylen);
    memcpy(keycp, key_rx, keylen);
    cmd_ok();
    int hdr = keylen;
    if (!isbigendian())
        swapbo32(hdr);
    if (writebuf_ex((char *) &hdr, sizeof hdr) == -1)
        endcomm();
    if (writebuf_ex(keycp, keylen) == -1)
        endcomm();
    swapkey(&cryp_rx, key_rx, keylen);
    swapkey(&cryp_tx, key_tx, keylen);
    free(keycp);
}

void cmd_rmsecdir(void)
{
    char *pt = comm.buf + strlen(CMD_RMSECDIR) + 1;
    char sectok[LINE_MAX] = "", dstp[PATH_MAX] = "";
    char *ppt;
    if (!(ppt = strstr(pt, MULTIP_SEPS))) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    strcpy(dstp, ppt + strlen(MULTIP_SEPS));
    *ppt = '\0';
    char path[PATH_MAX] = "";
    fsop = SECFS_RMDIR;
    if (jaildir(dstp, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 0)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    strcpy(sectok, path);
    strcat(sectok, "/");
    strcat(sectok, pt);
    if (access(sectok, F_OK)) {
        cmd_fail(CMD_ERMSEC);
        return;
    }
    if (rmr(path, NULL) == -1) {
        cmd_fail(CMD_EDIRSTILL);
        return;
    }
    cmd_ok();
}

void cmd_injail(void)
{
    if (!tfproto.injail) {
        cmd_fail(CMD_EISINJAILED);
        return;
    }
    char *pt = comm.buf + strlen(CMD_INJAIL) + 1;
    char sectok[LINE_MAX] = "", dstp[PATH_MAX] = "";
    char *ppt;
    if (!(ppt = strstr(pt, MULTIP_SEPS))) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    strcpy(dstp, ppt + strlen(MULTIP_SEPS));
    *ppt = '\0';
    strcpy(sectok, pt);
    char path[PATH_MAX] = "";
    if (jaildir(dstp, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    strcat(path, "/");
    strcpy(dstp, tfproto.dbdir);
    strcpy(tfproto.dbdir, path);
    strcat(path, JAIL_FILE);
    FILE *tokf = fopen(path, "r");
    if (!tokf) {
        strcpy(tfproto.dbdir, dstp);
        cmd_fail(CMD_EJAILACL);
        return;
    }
    char ln[LINE_MAX];
    while (fgets(ln, sizeof ln, tokf)) {
        pt = strchr(ln, '\n');
        if (pt)
            *pt = '\0';
        if (!strcmp(ln, sectok)) {
            tfproto.injail = 0;
            cmd_ok();
            return;
        }
    }
    strcpy(tfproto.dbdir, dstp);
    cmd_fail(CMD_EJAILTOK);
}

void cmd_tlb(void)
{
    FILE *tlb = fopen(tfproto.tlb, "r");
    if (!tlb) {
        cmd_fail(CMD_ETLP);
        return;
    }
    static int64_t pos;
    struct stat st;
    if (stat(tfproto.tlb, &st) == -1) {
        cmd_fail(CMD_ETLP);
        return;
    }
    if (fseek(tlb, pos, SEEK_SET) == -1 || pos == st.st_size)
        fseek(tlb, 0, SEEK_SET);
    int cmdok_len = strlen(CMD_OK) + 1;
    if (tlb && fgets(comm.buf + cmdok_len, sizeof comm.buf - cmdok_len, tlb)) {
        pos = ftell(tlb);
        char *pt = strchr(comm.buf, '\n');
        if (pt)
            *pt = '\0';
        memcpy(comm.buf, CMD_OK, cmdok_len - 1);
        *(comm.buf + cmdok_len - 1) = ' ';
        if (writebuf(comm.buf, strlen(comm.buf)) == -1)
            endcomm();
        fclose(tlb);
        return;
    }
    fclose(tlb);
    cmd_fail(CMD_ETLP);
}

void cmd_sdown(void)
{
    char *pt = comm.buf + strlen(CMD_SDOWN) + 1;
    int32_t hdr;
    char path[PATH_MAX];
    fsop = SECFS_RFILE;
    if (jaildir(pt, path)) {
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    int rc;
    while (1) {
        rc = readchunk(fd, comm.buf, sizeof comm.buf);
        if (rc == -1) {
            hdr = -1;
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
            break;
        } else if (rc == 0) {
            hdr = 0;
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
            break;
        } else {
            hdr = rc;
            if (!isbigendian())
                swapbo32(hdr);
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
            if (writebuf_ex(comm.buf, rc) == -1) {
                endcomm();
                return;
            }
        }
    }
    close(fd);
}

void cmd_sup(void)
{
    char *pt = comm.buf + strlen(CMD_SUP) + 1;
    int32_t hdr;
    char path[PATH_MAX];
    int err = 0;
    fsop = SECFS_WFILE;
    if (jaildir(pt, path))
        err = 1;
    int fd;
    int rc;
    if (!err) {
        fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, DEFFILE_PERM);
        if (fd == -1) {
            fd = open(NULLFILE, O_WRONLY);
            err = 1;
        }
    } else
        fd = open(NULLFILE, O_WRONLY);
    while (1) {
        if (readbuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        if (!isbigendian())
            swapbo32(hdr);
        if (readbuf_ex(comm.buf, hdr) == -1) {
            endcomm();
            return;
        }
        if (hdr > 0) {
            if (writechunk(fd, comm.buf, hdr) == -1)
                rc = -1;
        } else
            break;
    }
    if (!err && rc == -1 || hdr && !err)
        unlink(path);
    if (!hdr)
        if (err || rc == -1) {
            hdr = -1;
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
        } else {
            hdr = 0;
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
        }
    close(fd);
}

void cmd_fsize(void)
{
    char *pt = comm.buf + strlen(CMD_FSIZE) + 1;
    char path[PATH_MAX];
    int64_t hdr;
    fsop = SECFS_STAT;
    if (jaildir(pt, path)) {
        hdr = FSIZE_FAIL;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    struct stat st;
    int rc = stat(path, &st);
    if (rc == -1) {
        hdr = FSIZE_FAIL;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
    } else {
        if (!S_ISREG(st.st_mode))
            hdr = FSIZE_NOTREG;
        else
            hdr = st.st_size;
        if (!isbigendian())
            swapbo64(hdr);
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
    }
}

void cmd_fsizels(void)
{
    char *pt = comm.buf + strlen(CMD_FSIZELS) + 1;
    char path[PATH_MAX];
    char line[LINE_MAX];
    int64_t hdr;
    fsop = SECFS_RFILE;
    if (jaildir(pt, path)) {
        hdr = FSIZELS_END;
        if (!isbigendian())
            swapbo64(hdr);
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    FILE *fs = fopen(path, "r");
    if (!fs) {
        hdr = FSIZELS_END;
        if (!isbigendian())
            swapbo64(hdr);
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    struct stat st;
    while (fgets(line, sizeof line, fs)) {
        pt = strchr(line, '\n');
        if (pt)
            *pt = '\0';
        fsop = SECFS_STAT;
        if (jaildir(line, path)) {
            hdr = FSIZE_FAIL;
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
            continue;
        }
        int rc = stat(path, &st);
        if (rc == -1) {
            hdr = FSIZE_FAIL;
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
        } else {
            if (!S_ISREG(st.st_mode))
                hdr = FSIZE_NOTREG;
            else
                hdr = st.st_size;
            if (!isbigendian())
                swapbo64(hdr);
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
        }
    }
    hdr = FSIZELS_END;
    if (!isbigendian())
        swapbo64(hdr);
    if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
        endcomm();
        return;
    }
    fclose(fs);
}

void cmd_lsrv2(int mode)
{
    char *pt;
    if (mode == LSRITER_NONR)
        pt = comm.buf + strlen(CMD_LSV2) + 1;
    else if (mode == LSRITER_R)
        pt = comm.buf + strlen(CMD_LSRV2) + 1;
    char path[PATH_MAX];
    *path = '\0';
    char file[PATH_MAX];
    char *pt2 = strstr(comm.buf, LSV2TOK_SEP);
    if (!pt2) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *pt2 = '\0';
    if (mode == LSRITER_NONR)
        fsop = SECFS_LDIR;
    else if (mode == LSRITER_R)
        fsop = SECFS_LRDIR;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 0)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    pt2 += strlen(LSV2TOK_SEP);
    fsop = SECFS_WFILE;
    if (jaildir(pt2, file)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    struct stat st = { 0 };
    if (stat(path, &st) == -1) {
        cmd_fail(CMD_ELSRV2);
        return;
    }
    if (!S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_ESRCNODIR);
        return;
    }
    if (path[strlen(path) - 1] != '/')
        strcat(path, "/");
    char root[PATH_MAX];
    normpath(path, root);
    fs = fopen(file, "w+");
    if (!fs) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    int rc = lsr_iter(root, mode, lsr_callback);
    if (rc) {
        fclose(fs);
        unlink(file);
        cmd_fail(CMD_ELSRV2);
        return;
    }
    fclose(fs);
    cmd_ok();
}

static void lsr_callback(const char *root, const char *filename, int isdir)
{
    char stdpath[PATH_MAX];
    normpath(filename, stdpath);
    if (strstr(stdpath, FSMETA))
        return;
    char *pt = stdpath + strlen(tfproto.dbdir); 
    if (isdir)
        strcat(stdpath, "/");
    if (!strstr(stdpath + strlen(root), SDEXT)) {
        strcat(stdpath, "\n");
        fputs(pt, fs);
    }
}

void cmd_ftype(void)
{
    char *pt = comm.buf + strlen(CMD_FTYPE) + 1;
    char path[PATH_MAX];
    *path = '\0';
    char hdr = 0;
    fsop = SECFS_STAT;
    if (jaildir(pt, path)) {
        hdr = FTYPE_FAIL;
        if (writebuf_ex(&hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    struct stat st = { 0 };
    if (stat(path, &st) == -1) {
        hdr = FTYPE_FAIL;
        if (writebuf_ex(&hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    if (S_ISDIR(st.st_mode))
        hdr = FTYPE_DIR;
    else if (S_ISCHR(st.st_mode))
        hdr = FTYPE_CHR;
    else if (S_ISBLK(st.st_mode))
        hdr = FTYPE_BLK;
    else if (S_ISREG(st.st_mode))
        hdr = FTYPE_REG;
    else if (S_ISFIFO(st.st_mode))
        hdr = FTYPE_FIFO;
    else if (S_ISLNK(st.st_mode))
        hdr = FTYPE_SYM;
    else if (S_ISSOCK(st.st_mode))
        hdr = FTYPE_SOCK;
    else 
        hdr = FTYPE_OTHER;
    if (writebuf_ex(&hdr, sizeof hdr) == -1) {
        endcomm();
        return;
    }
}

void cmd_ftypels(void)
{
    char *pt = comm.buf + strlen(CMD_FTYPELS) + 1;
    char path[PATH_MAX];
    char line[LINE_MAX];
    *path = '\0';
    char hdr = 0;
    fsop = SECFS_RFILE;
    if (jaildir(pt, path)) {
        hdr = FTYPE_END;
        if (writebuf_ex(&hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    FILE *fs = fopen(path, "r");
    if (!fs) {
        hdr = FTYPE_END;
        if (writebuf_ex(&hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    struct stat st;
    while (fgets(line, sizeof line, fs)) {
        pt = strchr(line, '\n');
        if (pt)
            *pt = '\0';
        fsop = SECFS_STAT;
        if (jaildir(line, path)) {
            hdr = FTYPE_FAIL;
            if (writebuf_ex(&hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
            continue;
        }
        int rc = stat(path, &st);
        if (rc == -1) {
            hdr = FTYPE_FAIL;
            if (writebuf_ex(&hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
        } else {
            if (S_ISDIR(st.st_mode))
                hdr = FTYPE_DIR;
            else if (S_ISCHR(st.st_mode))
                hdr = FTYPE_CHR;
            else if (S_ISBLK(st.st_mode))
                hdr = FTYPE_BLK;
            else if (S_ISREG(st.st_mode))
                hdr = FTYPE_REG;
            else if (S_ISFIFO(st.st_mode))
                hdr = FTYPE_FIFO;
            else if (S_ISLNK(st.st_mode))
                hdr = FTYPE_SYM;
            else if (S_ISSOCK(st.st_mode))
                hdr = FTYPE_SOCK;
            else 
                hdr = FTYPE_OTHER;
            if (writebuf_ex(&hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
        }
    }
    hdr = FTYPE_END;
    if (writebuf_ex(&hdr, sizeof hdr) == -1) {
        endcomm();
        return;
    }
    fclose(fs);
}

void cmd_fstatls(void)
{
    char *pt = comm.buf + strlen(CMD_FSTATLS) + 1;
    char path[PATH_MAX];
    char line[LINE_MAX];
    *path = '\0';
    struct fstathdr hdr;
    fsop = SECFS_RFILE;
    if (jaildir(pt, path)) {
        hdr.code = FSTATLS_END;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    FILE *fs = fopen(path, "r");
    if (!fs) {
        hdr.code = FSTATLS_END;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    struct stat st;
    while (fgets(line, sizeof line, fs)) {
        pt = strchr(line, '\n');
        if (pt)
            *pt = '\0';
        fsop = SECFS_STAT;
        if (jaildir(line, path)) {
            memset(&hdr, 0, sizeof hdr);
            hdr.code = FSTATLS_FAIL;
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
            continue;
        }
        int rc = stat(path, &st);
        if (rc == -1) {
            memset(&hdr, 0, sizeof hdr);
            hdr.code = FSTATLS_FAIL;
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
        } else {
            hdr.code = 0;
            hdr.atime = st.st_atime;
            hdr.mtime = st.st_mtime;
            hdr.size = st.st_size;
            if (!isbigendian())
                swapbo64(hdr.atime);
            if (!isbigendian())
                swapbo64(hdr.mtime);
            if (!isbigendian())
                swapbo64(hdr.size);
            if (S_ISDIR(st.st_mode))
                hdr.type = FTYPE_DIR;
            else if (S_ISCHR(st.st_mode))
                hdr.type = FTYPE_CHR;
            else if (S_ISBLK(st.st_mode))
                hdr.type = FTYPE_BLK;
            else if (S_ISREG(st.st_mode))
                hdr.type = FTYPE_REG;
            else if (S_ISFIFO(st.st_mode))
                hdr.type = FTYPE_FIFO;
            else if (S_ISLNK(st.st_mode))
                hdr.type = FTYPE_SYM;
            else if (S_ISSOCK(st.st_mode))
                hdr.type = FTYPE_SOCK;
            else
                hdr.type = FTYPE_OTHER;
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                endcomm();
                return;
            }
        }
    }
    memset(&hdr, 0, sizeof hdr);
    hdr.code = FSTATLS_END;
    if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
        endcomm();
        return;
    }
    fclose(fs);
}

void cmd_intread(void)
{
    char *pt = comm.buf + strlen(CMD_INTREAD) + 1;
    int32_t hdr;
    char path[PATH_MAX];
    fsop = SECFS_RFILE;
    if (jaildir(pt, path)) {
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    int fd = open(path, O_RDWR);
    if (fd == -1 || crtlock(fd, BLK) == -1) {
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    struct stat st = { 0 };
    stat(path, &st);
    void *bytes = malloc(st.st_size);
    if (!bytes) {
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        crtlock(fd, UNLOCK);
        close(fd);
        return;
    }
    char sha256_str[SHA256STR_LEN - 1 + sizeof HEXPREFIX];
    if (sha256sum(path, sha256_str + strlen(HEXPREFIX)) == -1) {
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        crtlock(fd, UNLOCK);
        close(fd);
        free(bytes);
        return;
    }
    memcpy(sha256_str, HEXPREFIX, strlen(HEXPREFIX));
    int rc = readchunk(fd, bytes, st.st_size);
    crtlock(fd, UNLOCK);
    close(fd);
    if (rc == -1) {
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        free(bytes);
        return;
    } else {
        hdr = rc;
        if (!isbigendian())
            swapbo32(hdr);
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        if (writebuf_ex(sha256_str, strlen(sha256_str)) == -1) {
            endcomm();
            return;
        }
        if (writebuf_ex(bytes, rc) == -1) {
            endcomm();
            return;
        }
        free(bytes);
    }
}

void cmd_intwrite(void)
{
    char *pt = comm.buf + strlen(CMD_INTWRITE) + 1;
    int32_t hdr;
    char path[PATH_MAX];
    char sha256_src[SHA256STR_LEN - 1 + sizeof HEXPREFIX];
    if (readbuf_ex(sha256_src, sizeof sha256_src - 1) == -1) {
        endcomm();
        return;
    }
    *(sha256_src + SHA256STR_LEN - 1 + strlen(HEXPREFIX)) = '\0';
    if (readbuf_ex((char *) &hdr, sizeof hdr) == -1) {
        endcomm();
        return;
    }
    if (!isbigendian())
        swapbo32(hdr);
    int32_t byteslen = hdr;
    void *bytes = malloc(byteslen);
    if (!bytes) {
        rdnull(comm.sock, byteslen);
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        return;
    } else if (readbuf_ex(bytes, byteslen) == -1) {
        endcomm();
        return;
    }
    fsop = SECFS_WFILE;
    if (jaildir(pt, path)) {
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        free(bytes);
        return;
    }
    int fd = open(path, O_RDWR);
    if (fd == -1 || crtlock(fd, BLK) == -1) {
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        free(bytes);
        return;
    }
    char sha256_dst[SHA256STR_LEN - 1 + sizeof HEXPREFIX];
    if (sha256sum(path, sha256_dst + strlen(HEXPREFIX)) == -1) {
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        crtlock(fd, UNLOCK);
        close(fd);
        free(bytes);
        return;
    }
    memcpy(sha256_dst, HEXPREFIX, strlen(HEXPREFIX));
    if (strcmp(sha256_src, sha256_dst)) {
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        crtlock(fd, UNLOCK);
        close(fd);
        free(bytes);
        return;
    }
    if (ftruncate(fd, 0) == -1) {
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        free(bytes);
        crtlock(fd, UNLOCK);
        close(fd);
        return;
    }
    if (writechunk(fd, bytes, byteslen) != byteslen) {
        hdr = -1;
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
        free(bytes);
        crtlock(fd, UNLOCK);
        close(fd);
        return;   
    }
    hdr = 0;
    if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
        endcomm();
        return;
    }
    free(bytes);
    crtlock(fd, UNLOCK);
    close(fd);
}

void cmd_netlock(void)
{
    netlock_switch(NETLOCK_SWITCH);
}

void cmd_netunlock(void)
{
    char *pt = comm.buf + strlen(CMD_NETUNLOCK) + 1;
    int i = atoi(pt);
    if (i < 0 || i > NETLOCK_LEN) {
        cmd_fail(CMD_ENETLOCK);
        return;
    }
    if (!netlck[i].used || crtlock(netlck[i].fd, UNLOCK) == -1 ||
        pthread_cancel(netlck[i].th) == -1) {
        cmd_fail(CMD_ENETLOCK);
        return;
    }
    close(netlck[i].fd);
    netlck[i].used = 0;
    cmd_ok();
}

void cmd_netlocktry(void)
{
    netlock_switch(NETLOCKTRY_SWITCH);
}

static void netlock_switch(int s)
{
    char *pt;
    if (s == NETLOCK_SWITCH)
        pt = comm.buf + strlen(CMD_NETLOCK) + 1;
    else if (s == NETLOCKTRY_SWITCH)
        pt = comm.buf + strlen(CMD_NETLOCKTRY) + 1;
    char *pt2 = strchr(pt, CMD_SEPCH);
    if (!pt2) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *pt2++ = '\0';
    char path[PATH_MAX];
    if (jaildir(pt2, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    int i = 0;
    struct netlock_d *netpt = NULL;
    for (; i < NETLOCK_LEN; i++)
        if (!netlck[i].used) {
            netpt = &netlck[i];
            break;
        }
    if (!netpt) {
        cmd_fail(CMD_ENETLOCK);
        return;
    }
    netpt->fd = open(path, O_RDWR);
    if (netpt->fd == -1) {
        cmd_fail(CMD_ENETLOCK);
        return;
    }
    netpt->tmout = atoi(pt);
    int rc;
    if (s == NETLOCK_SWITCH)
        rc = crtlock(netpt->fd, BLK);
    else if (s == NETLOCKTRY_SWITCH)
        rc = crtlock(netpt->fd, NONBLK);
    if (rc == -1) {
        cmd_fail(CMD_ENETLOCK);
        return;
    }
    rc = pthread_create(&netpt->th, NULL, netlock_wdg, netpt);
    if (rc) {
        crtlock(netpt->fd, UNLOCK);
        cmd_fail(CMD_ENETLOCK);
        return;
    }
    pthread_detach(netpt->th);
    netpt->used = 1;
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    sprintf(comm.buf + sizeof CMD_OK, "%d", i);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

static void *netlock_wdg(void *prms)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    char act = comm.act;
    struct netlock_d *netlck = prms;
    int rest;
    while (1) {
        while (rest = sleep(netlck->tmout))
            sleep(rest);
        if (comm.act == act)
            exit(EXIT_SUCCESS);
        else
            act = comm.act;
    }
    return NULL;
}

void cmd_netmutacqtry(void)
{
    char *pt = comm.buf + strlen(CMD_NETMUTACQ_TRY) + 1;
    char *pt2 = strchr(pt, CMD_SEPCH);
    if (!pt2) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *pt2++ = '\0';
    char path[PATH_MAX];
    fsop = SECFS_RFILE | SECFS_WFILE;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    int fd = open(path, O_RDWR);
    if (fd == -1) {
        cmd_fail(CMD_ENETMUTEX);
        return;
    }
    if (crtlock(fd, BLK) == -1) {
        close(fd);
        cmd_fail(CMD_ENETMUTEX);
        return;
    }
    struct stat st = { 0 };
    if (stat(path, &st) == -1) {
        crtlock(fd, UNLOCK);
        close(fd);
        cmd_fail(CMD_ENETMUTEX);
        return;
    }
    if (!st.st_size) {
        if (writechunk(fd, pt2, strlen(pt2)) == -1) {
            crtlock(fd, UNLOCK);
            close(fd);
            cmd_fail(CMD_ENETMUTEX);
            return;
        }
        crtlock(fd, UNLOCK);
        close(fd);
        cmd_ok();
        return;
    }
    char tok[STDSTR];
    int rb;
    if ((rb = readchunk(fd, tok, sizeof tok)) == -1) {
        crtlock(fd, UNLOCK);
        close(fd);
        cmd_fail(CMD_ENETMUTEX);
        return;
    }
    tok[rb] = '\0';
    if (strcmp(tok, pt2)) {
        crtlock(fd, UNLOCK);
        close(fd);
        cmd_fail(CMD_ENETMUTEX);
        return;
    }
    crtlock(fd, UNLOCK);
    close(fd);
    cmd_ok();
}

void cmd_netmutrel(void)
{
    char *pt = comm.buf + strlen(CMD_NETMUTREL) + 1;
    char *pt2 = strchr(pt, CMD_SEPCH);
    if (!pt2) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *pt2++ = '\0';
    char path[PATH_MAX];
    fsop = SECFS_RFILE | SECFS_WFILE;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    int fd = open(path, O_RDWR);
    if (fd == -1) {
        cmd_fail(CMD_ENETMUTEX);
        return;
    }
    if (crtlock(fd, BLK) == -1) {
        close(fd);
        cmd_fail(CMD_ENETMUTEX);
        return;
    }
    struct stat st = { 0 };
    if (stat(path, &st) == -1) {
        crtlock(fd, UNLOCK);
        close(fd);
        cmd_fail(CMD_ENETMUTEX);
        return;
    }
    if (!st.st_size) {
        crtlock(fd, UNLOCK);
        close(fd);
        cmd_ok();
        return;
    }
    char tok[STDSTR];
    int rb;
    if ((rb = readchunk(fd, tok, sizeof tok)) == -1) {
        crtlock(fd, UNLOCK);
        close(fd);
        cmd_fail(CMD_ENETMUTEX);
        return;
    }
    tok[rb] = '\0';
    if (strcmp(tok, pt2)) {
        crtlock(fd, UNLOCK);
        close(fd);
        cmd_fail(CMD_ENETMUTEX);
        return;
    }
    ftruncate(fd, 0);
    crtlock(fd, UNLOCK);
    close(fd);
    cmd_ok();
}

void cmd_setfsid()
{
    char *pt = comm.buf + strlen(CMD_SETFSID) + 1;
    memset(fsid, 0, sizeof fsid);
    strcpy(fsid, pt);
    cmd_ok();
}

void cmd_setfsperm(void)
{
    char *pt = comm.buf + strlen(CMD_SETFSPERM) + 1;
    char tok[LINE_MAX];
    char path[PATH_MAX];
    char *pt2 = strchr(pt, CMD_SEPCH);
    if (!pt2) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *pt2++ = '\0';
    strcpy(tok, pt);
    pt = strchr(tok, FSPERM_TOK);
    if (!pt) {
        cmd_fail(CMD_ESETFSPERM);
        return;
    }
    fsop_ovrr = 1;
    if (jaildir(pt2, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    struct stat st = { 0 };
    stat(path, &st);
    if (!S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_ESETFSPERM);
        return;
    }
    strcat(path, "/");
    strcat(path, FSMETA);
    char swpfile[PATH_MAX];
    strcpy(swpfile, path);
    strcat(swpfile, FSMETASWP_EXT);
    char lckpath[PATH_MAX];
    strcpy(lckpath, path);
    strcat(lckpath, FSMETALCK_EXT);
    int lck = open(lckpath, O_RDWR | O_CREAT, DEFFILE_PERM);
    if (!lck) {
        cmd_fail(CMD_ESETFSPERM);
        return;
    }
    crtlock(lck, BLK);
    int fd = open(path, O_RDWR | O_CREAT, DEFFILE_PERM);
    if (fd == -1) {
        close(lck);
        cmd_fail(CMD_ESETFSPERM);
        return;
    }
    close(fd);
    if (cpfile(path, swpfile) == -1) {
        close(lck);
        cmd_fail(CMD_ESETFSPERM);
        return;
    }
    FILE *fs = fopen(swpfile, "a+");
    if (!fs) {
        unlink(swpfile);
        close(lck);
        cmd_fail(CMD_ESETFSPERM);
        return;
    }
    fseek(fs, 0, SEEK_END);
    int64_t sz = ftell(fs);
    fseek(fs, 0, SEEK_SET);
    if (!sz) {
        if (fputs(tok, fs) == EOF || fputs("\n", fs) == EOF) {
            fclose(fs);
            unlink(swpfile);
            close(lck);
            cmd_fail(CMD_ESETFSPERM);
            return;
        }
    } else {
        char line[LINE_MAX];
        int found = 0;
        while (fgets(line, sizeof line, fs)) {
            pt = strchr(line, FSPERM_TOK);
            if (pt)
                *pt = '\0';
            if (!strcmp(line, fsid)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            fclose(fs);
            unlink(swpfile);
            close(lck);
            cmd_fail(CMD_ESETFSPERM);
            return;
        } else {
            unsigned int perm = atoi(++pt);
            if (perm & SECFS_SETPERM) {
                if (fputs(tok, fs) == EOF || fputs("\n", fs) == EOF) {
                    fclose(fs);
                    unlink(swpfile);
                    close(lck);
                    cmd_fail(CMD_ESETFSPERM);
                    return;
                }
            } else {
                fclose(fs);
                unlink(swpfile);
                close(lck);
                cmd_fail(CMD_ESETFSPERM);
                return;
            }
        }
    }
    fclose(fs);
    if (rename(swpfile, path) == -1) {
        unlink(swpfile);
        close(lck);
        cmd_fail(CMD_ESETFSPERM);
        return;
    }
    char rdswp[PATH_MAX], rdfile[PATH_MAX];
    strcpy(rdswp, path);
    strcat(rdswp, FSMETASWP_EXT);
    if (cpfile(path, rdswp) == -1) {
        close(lck);
        cmd_fail(CMD_ESETFSPERM);
        return;
    }
    strcpy(rdfile, path);
    strcat(rdfile, FSMETARD_EXT);
    if (rename(rdswp, rdfile) == -1) {
        unlink(rdswp);
        close(lck);
        cmd_fail(CMD_ESETFSPERM);
        return;
    }
    close(lck);
    cmd_ok();
}

void cmd_remfsperm(void)
{
    char *pt = comm.buf + strlen(CMD_REMFSPERM) + 1;
    char tok[LINE_MAX];
    char path[PATH_MAX];
    char *pt2 = strchr(pt, CMD_SEPCH);
    if (!pt2) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *pt2++ = '\0';
    strcpy(tok, pt);
    fsop_ovrr = 1;
    if (jaildir(pt2, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    struct stat st = { 0 };
    stat(path, &st);
    if (!S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_EREMFSPERM);
        return;
    }
    strcat(path, "/");
    strcat(path, FSMETA);
    char swpfile[PATH_MAX];
    strcpy(swpfile, path);
    strcat(swpfile, FSMETASWP_EXT);
    char lckpath[PATH_MAX];
    strcpy(lckpath, path);
    strcat(lckpath, FSMETALCK_EXT);
    int lck = open(lckpath, O_RDWR | O_CREAT, DEFFILE_PERM);
    if (!lck) {
        cmd_fail(CMD_EREMFSPERM);
        return;
    }
    crtlock(lck, BLK);
    char fsmetard[PATH_MAX];
    strcpy(fsmetard, path);
    strcat(fsmetard, FSMETARD_EXT);
    unsigned int perm = getfsidperm(fsmetard, fsid);
    if (!(perm & SECFS_REMPERM)) {
        close(lck);
        cmd_fail(CMD_EREMFSPERM);
        return;
    }
    FILE *fperm = fopen(path, "r");
    FILE *fswap = fopen(swpfile, "w+");
    if (!fperm || !fswap) {
        fclose(fperm);
        fclose(fswap);
        close(lck);
        cmd_fail(CMD_EREMFSPERM);
        return;
    }
    char line[LINE_MAX];
    while (fgets(line, sizeof line, fperm)) {
        pt = strchr(line, FSPERM_TOK);
        if (pt) {
            *pt = '\0';
            if (!strcmp(line, tok))
                continue;
        }
        *pt = FSPERM_TOK;
        if (fputs(line, fswap) == EOF) {
            fclose(fperm);
            fclose(fswap);
            unlink(swpfile);
            close(lck);
            cmd_fail(CMD_EREMFSPERM);
            return;
        }
    }
    fclose(fperm);
    fclose(fswap);
    if (rename(swpfile, path) == -1) {
        unlink(swpfile);
        close(lck);
        cmd_fail(CMD_EREMFSPERM);
        return;
    }
    char rdfile[PATH_MAX];
    if (cpfile(path, swpfile) == -1) {
        close(lck);
        cmd_fail(CMD_EREMFSPERM);
        return;
    }
    strcpy(rdfile, path);
    strcat(rdfile, FSMETARD_EXT);
    if (rename(swpfile, rdfile) == -1) {
        unlink(swpfile);
        close(lck);
        cmd_fail(CMD_EREMFSPERM);
        return;
    }
    close(lck);
    cmd_ok();
}

void cmd_getfsperm(void)
{
    char *pt = comm.buf + strlen(CMD_GETFSPERM) + 1;
    char tok[LINE_MAX];
    char path[PATH_MAX];
    char *pt2 = strchr(pt, CMD_SEPCH);
    if (!pt2) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *pt2++ = '\0';
    strcpy(tok, pt);
    fsop_ovrr = 1;
    if (jaildir(pt2, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    struct stat st = { 0 };
    stat(path, &st);
    if (!S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_EGETFSPERM);
        return;
    }
    char perm[INTDIGITS];
    strcat(path, "/");
    strcat(path, FSMETA);
    strcat(path, FSMETARD_EXT);
    sprintf(perm, "%u", getfsidperm(path, tok));
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, perm);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_issecfs(void)
{
    char *pt = comm.buf + strlen(CMD_ISSECFS) + 1;
    char path[PATH_MAX];
    fsop_ovrr = 1;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    struct stat st = { 0 };
    stat(path, &st);
    if (!S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_ESRCNODIR);
        return;
    }
    strcat(path, "/");
    strcat(path, FSMETA);
    strcat(path, FSMETARD_EXT);
    if (access(path, F_OK)) {
        cmd_fail(CMD_EISSECFS);
        return;
    }
    cmd_ok();
}

void cmd_locksys(void)
{
    char *pt = comm.buf + strlen(CMD_LOCKSYS) + 1;
    char hash[LINE_MAX];
    strcpy(hash, tfproto.dbdir);
    strcat(hash, "/");
    strcat(hash, pt);
    if (access(tfproto.dbdir, F_OK)) {
        cmd_fail(CMD_EFILENOENT);
        return;
    }
    struct stat st = { 0 };
    stat(hash, &st);
    if (!S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_ENOTDIR);
        return;
    }
    strcpy(tfproto.dbdir, hash);
    tfproto.locksys = 0;
    cmd_ok();
}

void cmd_tasfs(void)
{
    char *pt = comm.buf + strlen(CMD_TASFS) + 1;
    char file[PATH_MAX] = "";
    fsop = SECFS_WFILE;
    if (jaildir(pt, file)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(file, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    int fd = open(file, O_RDWR | O_CREAT | O_EXCL, DEFFILE_PERM);
    if (fd == -1) {
        cmd_fail(CMD_ETASFS);
        return;
    }
    close(fd);
    cmd_ok();
}

void cmd_rmkdir(void)
{
    char *pt = comm.buf + strlen(CMD_RMKDIR) + 1;
    char path[PATH_MAX] = "";
    fsop = SECFS_MKDIR;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    pt = strtok(path, "/");
    char dir[PATH_MAX];
    strcpy(dir, "/");
    while (pt) {
        strcat(dir, "/");
        strcat(dir, pt);
        mkdir(dir, DEFDIR_PERM);
        pt = strtok(NULL, "/");
    }
    if (access(dir, F_OK)) {
        cmd_fail(CMD_ERMKDIR);
        return;
    }
    cmd_ok();
}

void cmd_gettz(void)
{
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, getenv("TZ"));
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_settz(void)
{
    char *pt = comm.buf + strlen(CMD_SETTZ) + 1;
    setenv("TZ", pt, 1);
    tzset();
    cmd_ok();
}

void cmd_localtime(void)
{
    char date[HTIMELEN] = "";
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    if (tm)
        strftime(date, HTIMELEN, "%Y-%m-%d %H:%M:%S %Z", tm);
    else { 
        cmd_fail(CMD_EBADDATE);
        return;
    }
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, date);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_dateftz(void)
{
    char *pt = comm.buf + strlen(CMD_DATEFTZ) + 1;
    char oldtz[LINE_MAX];
    strcpy(oldtz, getenv("TZ"));
    setenv("TZ", pt, 1);
    tzset();
    char date[HTIMELEN] = "";
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    if (tm)
        strftime(date, HTIMELEN, "%Y-%m-%d %H:%M:%S %Z", tm);
    else { 
        cmd_fail(CMD_EBADDATE);
        return;
    }
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, date);
    setenv("TZ", oldtz, 1);
    tzset();
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_lsrv2down(int mode)
{
    char *pt;
    if (mode == LSRITER_NONR)
        pt = comm.buf + strlen(CMD_LSV2DOWN) + 1;
    else if (mode == LSRITER_R)
        pt = comm.buf + strlen(CMD_LSRV2DOWN) + 1;
    char path[PATH_MAX];
    *path = '\0';
    if (mode == LSRITER_NONR)
        fsop = SECFS_LDIR;
    else if (mode == LSRITER_R)
        fsop = SECFS_LRDIR;
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(path, lcknam, 0)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    struct stat st = { 0 };
    if (stat(path, &st) == -1) {
        cmd_fail(CMD_ELSRV2);
        return;
    }
    if (!S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_ESRCNODIR);
        return;
    }
    if (path[strlen(path) - 1] != '/')
        strcat(path, "/");
    char root[PATH_MAX];
    normpath(path, root);
    cmd_ok();
    int32_t hdr;
    if (lsr_iter(root, mode, lsrdown_callback)) {
        hdr = LSV2DOWN_HDR_FAILED;
        if (!isbigendian())
            swapbo32(hdr);
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
    } else {
        hdr = LSV2DOWN_HDR_SUCCESS;
        if (!isbigendian())
            swapbo32(hdr);
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
            endcomm();
            return;
        }
    }
}

static void lsrdown_callback(const char *root, const char *filename, int isdir)
{
    char stdpath[PATH_MAX];
    normpath(filename, stdpath);
    if (strstr(stdpath, FSMETA))
        return;
    char *pt = stdpath + strlen(tfproto.dbdir); 
    if (isdir)
        strcat(stdpath, "/");
    if (!strstr(stdpath + strlen(root), SDEXT)) {
        char line[LINE_MAX];
        strcat(stdpath, "\n");
        strcpy(line, pt);
        int32_t hdr = strlen(line);
        if (!isbigendian())
            swapbo32(hdr);
        if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) 
            return;
        if (writebuf_ex(line, strlen(line)) == -1) 
            return;
    }
}

void cmd_runbash(void)
{
#define BASHBIN "bash"
    if (!tfproto.runbash) {
        cmd_fail(CMD_ERUNBASH);
        return;
    }
    char *pt = comm.buf + strlen(CMD_RUNBASH) + 1;
    if (access(pt, F_OK)) {
        cmd_fail(CMD_EFILENOENT);
        return;
    }
    pid_t pid = fork();
    if (!pid) {
        int fd = open("/dev/null", O_RDWR);
        if (fd == -1)
            exit(EXIT_FAILURE);
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        execlp(BASHBIN, BASHBIN, pt, NULL);
        /* Just in case execlp fails. */
        exit(EXIT_FAILURE);
    }
    cmd_ok();
}

void cmd_flycontext(void)
{
    char *pt = comm.buf + strlen(CMD_FLYCONTEXT) + 1;
    char path[PATH_MAX];
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    pt = strtok(path, "/");
    char dir[PATH_MAX];
    strcpy(dir, "/");
    while (pt) {
        strcat(dir, "/");
        strcat(dir, pt);
        mkdir(dir, DEFDIR_PERM);
        pt = strtok(NULL, "/");
    }
    strcat(dir, "/");
    if (access(dir, F_OK)) {
        cmd_fail(CMD_EFLYCONTEXT);
        return;
    }
    strcpy(tfproto.dbdir, dir);
    tfproto.flycontext = 0;
    cmd_ok();
}

void cmd_goaes(void)
{
    if (readbuf_ex(cipher_tx.key, sizeof cipher_tx.key) == -1)
        return;
    if (readbuf_ex(cipher_tx.iv, sizeof cipher_tx.iv) == -1)
        return;
    memcpy(cipher_rx.key, cipher_tx.key, sizeof cipher_tx.key);
    memcpy(cipher_rx.iv, cipher_tx.iv, sizeof cipher_tx.iv);
    if (!setblkon()) {
        cmd_ok();
        startblk();
    } else
        cmd_fail(CMD_EAES);
}

void cmd_tfpcrypto(void)
{
    cmd_ok();
    setblkoff();
}

void cmd_genuuid(void)
{
    char uuid[UUIDCHAR_LEN];
    uuidgen(uuid);
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, uuid);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}

void cmd_faitok(void)
{
    if (comm.faiuse) {
        cmd_fail(CMD_EFAITOK);
        return;
    }
    char *pt = comm.buf + strlen(CMD_FAITOK) + 1;
    uint64_t exp = atoll(pt);
    if (exp > tfproto.faitok_mq || exp <= 0)
        exp = tfproto.faitok_mq;
    exp = time(0) + exp * SPM;
    char expstr[LLDIGITS];
    sprintf(expstr, "%llu", (unsigned long long) exp);
    char uuid[UUIDCHAR_LEN];
    uuidgen(uuid);
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, expstr);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, uuid);
    strcat(comm.buf, CMD_SEPSTR);
    int keysz = random() % (MAX_KEYLEN - MIN_KEYLEN + 1) + MIN_KEYLEN;
    char *key = genkey(keysz);
    if (key == NULL) {
        cmd_fail(CMD_EFAITOK);
        return;
    }
    char *b64 = base64en(key, keysz);
    free(key);
    if (!b64) {
        cmd_fail(CMD_EFAITOK);
        return;
    }
    strcat(comm.buf, b64);
    if (savefai(uuid, exp, b64) == -1) {
        free(b64);
        cmd_fail(CMD_EFAITOK);
        return;
    }
    free(b64);
    comm.faiuse = 1;
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        endcomm();
}
