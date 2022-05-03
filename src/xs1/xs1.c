/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <xs1/xs1.h>
#include <xs1/xs1utils.h>
#include <stdio.h>
#include <cmd.h>
#include <err.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <util.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <init.h>

/* A common size for buffers. */
#define STDBUF 256
/* Enhanced readv2/writev2 commnads error messages buffer size. */
#define ERRSTR_BUF 126
/* Whence types for seek. */
#define WH_END "END"
#define WH_CUR "CUR"
#define WH_SET "SET"
/* Lock types for lock. */
#define LCK_LOCK "LOCK"
#define LCK_LOCKW "LOCKW"
#define LCK_UNLOCK "UNLOCK"
/* Maximum number of bytes that read can return. */
#define READ_MAX (COMMBUFLEN - sizeof(CMD_OK))
/* Deprecated. */
/* Maximum number of bytes that can be written to a file at a time. */
#define WRITE_MAX (COMMBUFLEN - (sizeof XS1_WRITE + (sizeof(int32_t) / 2 * \
    3 + sizeof(int32_t)) + 1 + COMMBUFDIG + 1))
#undef WRITE_MAX

/* Maximum number of bytes that can be written to a file at a time. */
static int WRITE_MAX;

/* XS1 Commands. */

/* Open file descriptor. */
#define XS1_OPEN "XS1_OPEN"
/* Close file descriptor. */
#define XS1_CLOSE "XS1_CLOSE"
/* Truncates an opened file descriptor. */
#define XS1_TRUNC "XS1_TRUNC"
/* Sets the file descriptor position in the file. */
#define XS1_SEEK "XS1_SEEK"
/* Locks the associated file of the speciefied file descriptor. */
#define XS1_LOCK "XS1_LOCK"
/* Read bytes from the file descriptor. */
#define XS1_READ "XS1_READ"
/* Write bytes to file descriptor. */
#define XS1_WRITE "XS1_WRITE"
/* Read bytes from the file descriptor. It uses variable length buffer */
#define XS1_READv2 "XS1_READV2"
/* Write bytes to the file descriptor. It uses variable length buffer */
#define XS1_WRITEv2 "XS1_WRITEV2"

/* XS1 Command error status. */

/* No left file descriptors hole available. */
#define XS1_ENODES "XS1 1 : No file descriptor hole available."
/* Failed to open a new file descriptor. */
#define XS1_EOPEN "XS1 2 : Failed to open requested file."
/* Invalid file descriptor to use in close operation. */
#define XS1_EINVFD "XS1 3 : Invalid file descriptor."
/* Invalid value specified for truncation. */
#define XS1_ETRUNC "XS1 4 : Invalid value for truncation."
/* Error ocurred while the operation was in process. */
#define XS1_ETRUNCOP "XS1 5 : Error while truncating the file."
/* Invalid vlaue specified for seek. */
#define XS1_ESEEK "XS1 6 : Invalid value for seek."
/* Invalid specified whence constant. */
#define XS1_EWHENCE "XS1 7 : Invalid whence constant."
/* Error ocurred while seeking in the file. */
#define XS1_ESEEKOP "XS1 8 : Error while seeking in the file."
/* Speciefied lock type unknown. */
#define XS1_ELOCKU "XS1 9 : Lock type unknown."
/* Error ocurred while locking the file. */
#define XS1_ELOCKOP "XS1 10 : Error while locking the file."
/* In a non-blocking mode the lock was unable to be acquired. */
#define XS1_ELOCKTRY "XS1 11 : Unable to acquire the lock."
/* Invalid speciefied length for read. */
#define XS1_EREADLEN "XS1 12 : Invalid length value for read."
/* Error while reading the file. */
#define XS1_EREADOP "XS1 13 : Error while reading the file."
/* Error while using the network interface. */
#define XS1_ENET "XS1 14 : Network operation error."
/* Invalid speciefied size for write. */
#define XS1_EWRITELEN "XS1 15 : Invalid size value for write."
/* Error while writing the file. */
#define XS1_EWRITEOP "XS1 16 : Error while writing the file."
/* Error allocating memory. */
#define XS1_EBUFALLOC "XS1 17 : Error allocating buffer."

/* Packet header, without padding. */

#pragma pack(push, 1)
typedef struct v2hdr {
    int32_t fdidx;
    uint64_t bytes;
} V2hdr;
#pragma pack(pop)

static struct ds ds;
static char errstr[ERRSTR_BUF];

/* Open file descriptor. */
static void xs1_cmdopen(void);
/* Close file descriptor. */
static void xs1_cmdclose(void);
/* Truncates an opened file descriptor. */
static void xs1_cmdtrunc(void);
/* Validates descriptor index. Returns -1 for invalid file descriptor index,
    otherwise returns the file descriptor index. */
static int xs1_chkidx(const char *pt);
/* Sets the file descriptor position in the file. */
static void xs1_cmdseek(void);
/* Locks the associated file of the file descriptor. */
static void xs1_cmdlock(void);
/* Reads n bytes up to READ_MAX from file descriptor. */
static void xs1_cmdread(void); 
/* Writes n bytes up to WRITE_MAX to file descriptor. */
static void xs1_cmdwrite(void);
/* Reads n bytes from file descriptor. It uses variable length buffer. */
static void xs1_cmdreadv2(void);
/* Write n bytes to file descriptor. It uses variable length buffer. */
static void xs1_cmdwritev2(void);
/* Validates descriptor index. Returns -1 for invalid file descriptor index,
    otherwise returns the file descriptor index. */
static int xs1_chkidx_int64(int64_t idx);

void xs1_parse(const char *cmd)
{
    char num[INTDIGITS];
    sprintf(num, "%d", COMMBUFLEN);
    WRITE_MAX = (COMMBUFLEN - (sizeof XS1_WRITE + (sizeof(int32_t) / 2 * \
        3 + sizeof(int32_t)) + 1 + strlen(num) + 1));
    if (!strcmp(cmd, XS1_OPEN))
        xs1_cmdopen();
    else if (!strcmp(cmd, XS1_CLOSE))
        xs1_cmdclose();
    else if (!strcmp(cmd, XS1_TRUNC))
        xs1_cmdtrunc();
    else if (!strcmp(cmd, XS1_SEEK))
        xs1_cmdseek();
    else if (!strcmp(cmd, XS1_LOCK))
        xs1_cmdlock();
    else if (!strcmp(cmd, XS1_READ))
        xs1_cmdread();
    else if (!strcmp(cmd, XS1_WRITE))
        xs1_cmdwrite();
    else if (!strcmp(cmd, XS1_READv2))
        xs1_cmdreadv2();
    else if (!strcmp(cmd, XS1_WRITEv2))
        xs1_cmdwritev2();
    else 
        cmd_unknown();
}

static void xs1_cmdopen(void)
{
    char *pt = comm.buf + strlen(XS1_OPEN) + 1;
    char file[PATH_MAX] = "";
    fsop = SECFS_RFILE | SECFS_WFILE;
    if (jaildir(pt, file)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!trylck(file, lcknam, 1)) {
        cmd_fail(CMD_ELOCKED);
        return;
    }
    if (access(file, F_OK)) {
        cmd_fail(CMD_ESRCFILE);
        return;
    }
    struct stat st = { 0 };
    stat(file, &st);
    if (S_ISDIR(st.st_mode)) {
        cmd_fail(CMD_EISDIR);
        return;
    }
    int32_t *fd = xs1_getfd(&ds);
    if (!fd) {
        cmd_fail(XS1_ENODES);
        return;
    }
    *fd = open(file, O_RDWR);
    if (*fd == -1) {
        cmd_fail(XS1_EOPEN);
        return;
    }
    int32_t i = xs1_getidx(fd, &ds);
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, itostr(i));
    if (writebuf(comm.buf, strlen(comm.buf)) == -1) {
        close(*fd);
        cmd_fail(XS1_EOPEN);
        return;
    }
}

static void xs1_cmdclose(void)
{
    char *pt = comm.buf + strlen(XS1_CLOSE) + 1;
    int32_t idx;
    if ((idx = xs1_chkidx(pt)) == -1) {
        cmd_fail(XS1_EINVFD);
        return;
    }
    close(*(ds.fds + idx));
    *(ds.fds + idx) = -1;
    cmd_ok();
}

static void xs1_cmdtrunc(void)
{
    char *pt = comm.buf + strlen(XS1_TRUNC) + 1;
    int32_t idx;
    if ((idx = xs1_chkidx(pt)) == -1) {
        cmd_fail(XS1_EINVFD);
        return;
    }
    pt = strstr(pt, CMD_SEPSTR);
    if (!pt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    pt++;
    char num[LLDIGITS] = "";
    strcpy_sec(num, pt, LLDIGITS);
    if (!strcmp(num, "")) {
        cmd_fail(XS1_ETRUNC);
        return;
    }
    off_t len = atoll(num);
    if (len < 0) {
        cmd_fail(XS1_ETRUNC);
        return;
    }
    struct stat st = { 0 };
    fstat(*(ds.fds + idx), &st);
    if (ftruncate(*(ds.fds + idx), len) == -1) {
        cmd_fail(XS1_ETRUNCOP);
        return;
    }
    cmd_ok();
}

static int xs1_chkidx(const char *pt)
{
    char num[LLDIGITS] = "";
    strcpy_sec(num, pt, LLDIGITS);
    if (!strcmp(num, ""))
        return -1;
    int32_t idx = atoll(num);
    if (idx < 0 || idx >= ds.count || !(ds.fds + idx) || 
        *(ds.fds + idx) == -1)
        return -1;
    return idx;
}

static void xs1_cmdseek(void)
{
    char *pt = comm.buf + strlen(XS1_SEEK) + 1;
    int32_t idx;
    if ((idx = xs1_chkidx(pt)) == -1) {
        cmd_fail(XS1_EINVFD);
        return;
    }
    pt = strstr(pt, CMD_SEPSTR);
    if (!pt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    pt++;
    char num[LLDIGITS] = "";
    strcpy_sec(num, pt, LLDIGITS);
    if (!strcmp(num, "")) {
        cmd_fail(XS1_ESEEK);
        return;
    }
    off_t offset = atoll(num);
    pt = strstr(pt, CMD_SEPSTR);
    if (!pt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    pt++;
    strcpy_sec(num, pt, LLDIGITS);
    cmdtoupper(num);
    int wh;
    if (!strcmp(num, WH_SET))
        wh = SEEK_SET;
    else if (!strcmp(num, WH_CUR))
        wh = SEEK_CUR;
    else if (!strcmp(num, WH_END))
        wh = SEEK_END;
    else {
        cmd_fail(XS1_EWHENCE);
        return;
    }
    if ((offset = lseek(*(ds.fds + idx), offset, wh)) == -1) {
        cmd_fail(XS1_ESEEKOP);
        return;
    }
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, CMD_SEPSTR);
    strcat(comm.buf, itostr(offset));
    writebuf(comm.buf, strlen(comm.buf));
}

static void xs1_cmdlock(void)
{
    char *pt = comm.buf + strlen(XS1_LOCK) + 1;
    int32_t idx;
    if ((idx = xs1_chkidx(pt)) == -1) {
        cmd_fail(XS1_EINVFD);
        return;
    }
    pt = strstr(pt, CMD_SEPSTR);
    if (!pt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    pt++;
    char lckt[STDBUF] = "";
    strcpy_sec(lckt, pt, LLDIGITS);
    cmdtoupper(lckt);
    struct flock lock;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    int lprm;
    if (!strcmp(lckt, LCK_LOCK)) {
        lprm = F_SETLK;
        lock.l_type = F_WRLCK;    
    } else if (!strcmp(lckt, LCK_LOCKW)) {
        lprm = F_SETLKW;
        lock.l_type = F_WRLCK;    
    } else if (!strcmp(lckt, LCK_UNLOCK)) {
        lprm = F_SETLK;
        lock.l_type = F_UNLCK;    
    } else {
        cmd_fail(XS1_ELOCKU);
        return;
    }
    int lr = fcntl(*(ds.fds + idx), lprm, &lock);
    if (lprm == F_SETLKW && lr == -1) {
        cmd_fail(XS1_ELOCKOP);
        return;
    } else if (lprm == F_SETLK && lr == -1) {
        cmd_fail(XS1_ELOCKTRY);
        return;
    }
    cmd_ok();
}

static void xs1_cmdread(void)
{
    char *pt = comm.buf + strlen(XS1_READ) + 1;
    int32_t idx;
    if ((idx = xs1_chkidx(pt)) == -1) {
        cmd_fail(XS1_EINVFD);
        return;
    }
    pt = strstr(pt, CMD_SEPSTR);
    if (!pt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    pt++;
    char num[LLDIGITS] = "";
    strcpy_sec(num, pt, LLDIGITS);
    if (!strcmp(num, "")) {
        cmd_fail(XS1_EREADLEN);
        return;
    }
    size_t sz = atoll(num);
    if (sz > READ_MAX || sz < 0) {
        cmd_fail(XS1_EREADLEN);
        return;
    }
    off_t offset = lseek(*(ds.fds + idx), 0, SEEK_CUR);
    int rb;
    do 
        rb = read(*(ds.fds + idx), comm.buf + strlen(CMD_OK) + 1, sz);
    while (rb == -1 && errno == EINTR);
    if (rb == -1) {
        lseek(*(ds.fds + idx), offset, SEEK_SET);
        cmd_fail(XS1_EREADOP);
        return;
    }
    strcpy(comm.buf, CMD_OK);
    *(comm.buf + strlen(CMD_OK)) = CMD_SEPCH;
    if (writebuf(comm.buf, rb + strlen(CMD_OK) + 1) == -1) {
        lseek(*(ds.fds + idx), offset, SEEK_SET);
        cmd_fail(XS1_ENET);
        return;
    }
}

static void xs1_cmdwrite(void)
{
    char *pt = comm.buf + strlen(XS1_WRITE) + 1;
    int32_t idx;
    if ((idx = xs1_chkidx(pt)) == -1) {
        cmd_fail(XS1_EINVFD);
        return;
    }
    pt = strstr(pt, CMD_SEPSTR);
    if (!pt) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    pt++;
    char *pte = strstr(pt, CMD_SEPSTR);
    if (!pte) {
        cmd_fail(CMD_EPARAM);
        return;
    }
    *pte = '\0';
    char num[LLDIGITS] = "";
    strcpy_sec(num, pt, LLDIGITS);
    if (!strcmp(num, "")) {
        cmd_fail(XS1_EWRITELEN);
        return;
    }
    pte++;
    size_t sz = atoll(num);
    if (sz > WRITE_MAX || sz < 0) {
        cmd_fail(XS1_EWRITELEN);
        return;
    }
    off_t offset = lseek(*(ds.fds + idx), 0, SEEK_CUR);
    struct stat st = { 0 };
    fstat(*(ds.fds + idx), &st);
    int wb;
    do 
        wb = write(*(ds.fds + idx), pte, sz);
    while (wb == -1 && errno == EINTR);
    if (wb == -1 && errno == ENOSPC) {
        cmd_fail(CMD_EQUOTA);
        return;
    }
    if (wb == -1 || wb < sz) {
        lseek(*(ds.fds + idx), offset, SEEK_SET);
        cmd_fail(XS1_EWRITEOP);
        return;
    }
    cmd_ok();
}

static void xs1_cmdreadv2(void)
{
    V2hdr hdr;
    if (readbuf_ex((char *) &hdr, sizeof hdr) == -1) {
        endcomm();
        return;
    }
    if (!isbigendian())
        swapbo32(hdr.fdidx);
    if (!isbigendian())
        swapbo64(hdr.bytes);
    int64_t sz;
    if (xs1_chkidx_int64(hdr.fdidx) == -1) {
        sz = -1;
        strcpy(errstr, XS1_EINVFD);
        if (writebuf_ex((char *) &sz, sizeof sz) == -1) {
            endcomm();
            return;
        }
        if (writebuf_ex(errstr, sizeof errstr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    void *buf = malloc(hdr.bytes);
    if (!buf) {
        strcpy(errstr, XS1_EBUFALLOC);
        sz = -1;
        if (writebuf_ex((char *) &sz, sizeof sz) == -1) {
            endcomm();
            return;
        }
        if (writebuf_ex(errstr, sizeof errstr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    sz = readchunk(*(ds.fds + hdr.fdidx), buf, hdr.bytes);
    if (sz == -1) {
        free(buf);
        strcpy(errstr, XS1_EREADOP);
        if (writebuf_ex((char *) &sz, sizeof sz) == -1) {
            endcomm();
            return;
        }
        if (writebuf_ex(errstr, sizeof errstr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    int64_t szcp = sz;
    if (!isbigendian())
        swapbo64(sz);
    if (writebuf_ex((char *) &sz, sizeof sz) == -1) {
        endcomm();
        return;
    }
    if (!szcp) {
        free(buf);
        return;
    }
    if (writebuf_ex(buf, szcp) == -1) {
        endcomm();
        return;
    }
    free(buf);
}

static int xs1_chkidx_int64(int64_t idx)
{
     if (idx < 0 || idx >= ds.count || !(ds.fds + idx) || *(ds.fds + idx) == -1)
        return -1;
    return idx;
}

static void xs1_cmdwritev2(void)
{
    V2hdr hdr;
    if (readbuf_ex((char *) &hdr, sizeof hdr) == -1) {
        endcomm();
        return;
    }
    if (!isbigendian())
        swapbo32(hdr.fdidx);
    if (!isbigendian())
        swapbo64(hdr.bytes);
    int64_t sz;
    void *buf = malloc(hdr.bytes);
    if (!buf) {
        rdnull(comm.sock, hdr.bytes);
        strcpy(errstr, XS1_EBUFALLOC);
        sz = -1;
        if (writebuf_ex((char *) &sz, sizeof sz) == -1) {
            endcomm();
            return;
        }
        if (writebuf_ex(errstr, sizeof errstr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    if (readbuf_ex(buf, hdr.bytes) == -1) {
        endcomm();
        return;
    }
    if (xs1_chkidx_int64(hdr.fdidx) == -1) {
        sz = -1;
        strcpy(errstr, XS1_EINVFD);
        if (writebuf_ex((char *) &sz, sizeof sz) == -1) {
            endcomm();
            return;
        }
        if (writebuf_ex(errstr, sizeof errstr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    sz = writechunk(*(ds.fds + hdr.fdidx), buf, hdr.bytes);
    if (sz == -1) {
        free(buf);
        strcpy(errstr, XS1_EWRITEOP);
        if (writebuf_ex((char *) &sz, sizeof sz) == -1) {
            endcomm();
            return;
        }
        if (writebuf_ex(errstr, sizeof errstr) == -1) {
            endcomm();
            return;
        }
        return;
    }
    if (!isbigendian())
        swapbo64(sz);
    if (writebuf_ex((char *) &sz, sizeof sz) == -1) {
        endcomm();
        return;
    }
    free(buf);
}
