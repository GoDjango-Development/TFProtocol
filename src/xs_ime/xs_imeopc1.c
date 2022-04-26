/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <xs_ime/xs_imeopc1.h>
#include <xs_ime/xs_ime.h>
#include <string.h>
#include <util.h>
#include <tfproto.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

/* Message separator from users. */
#define MSGSEPCH ' '
/* User separator. */
#define USRSEP ":"
/* Folder's name for messages. */
#define MSGFOLDER "messages"
/* Folder's name for blocked users. */
#define BLACKLIST "blacklist"
/* Temporary file extension. */
#define TMPEXT ".tmp"
/* Lock file name to implement unique user. */
#define LCKFILE "loged-in"
/* Dot separator. */
#define DOTCH '.'
/* Nanosecond factor. */
#define NANOSEC 1000000000
/* Temporary message directory */
#define MSGTMPDIR "msgtmp"

/* Copy message to destination user's folder. */
static void cpymsg(char *usr, const char *msg);
/* Make message directory inside user's folder if isn't exist. */
static void mkmsgdir(void);
/* Make temp path for message writing. if the second parameter is no null then \
    is set to the timestamp used in temporary name. */
static void mktmpath(char *tmp, char *tmpstr);
/* Test if user is already loged-in.*/
static int islogin(void);
/* Concatenate porcess id. */
static void catpid(char *buf);
/* Check user in the blacklist. */
static int chkblk(const char *usr);
/* Selector function for directory search. */
static int msgselect(const struct dirent *ent);
/* Compare and swap timestamp of message. */
static int ctmstmp(const char *msg);
/* Delete message if autodel flag is on. */
static void delmsg(const char *msg);
/* Send to client the message. */
static int writemsg(const char *msg);

/* Lock file path. */
static char lckfile[PATH_MAX];
/* Function to compare timestamps. */
static int timecmp(const struct dirent **a, const struct dirent **b);
/* Login lock file. */
static int lckfd;

void opc_timer(void)
{
    if (readbuf_ex(inbuf, sizeof(uint64_t)) == -1)
        return;
    uint64_t *ival = (uint64_t *) inbuf;
    if (!isbigendian())
            swapbo64(*ival);
    static struct timespec tmout;
    struct timespec *bck = tm;
    tmout.tv_sec = *ival / NANOSEC;
    tmout.tv_nsec = *ival % NANOSEC;
    tm = &tmout;
    *bck = tmout;
}

void opc_orgunit(void)
{
    int sz = getbufsz(&rdhdr);
    if (readbuf_ex(inbuf, sz) != -1) {
        if (chkrdy()) {
            sendopc(OPC_UNIT, OPC_FAILED);
            return;
        }
        if ((ready & SETUP) != SETUP) {
            sendopc(OPC_UNIT, OPC_FAILED);
            return;
        }
        if (sz > PATH_MAX - 1)
            sz = PATH_MAX - 1;
        memcpy(orgunit, inbuf, sz);
        orgunit[sz] = '\0';
        char tmp[PATH_MAX];
        jaildir(orgunit, tmp);
        strcpy(orgunit, tmp);
        if (chkpath(orgunit) || sz >= PATH_MAX) {
            ready &= ~ORUN;
            sendopc(OPC_UNIT, OPC_FAILED);
        } else {
            ready |= ORUN;
            sendopc(OPC_UNIT, OPC_SUCCESS);
        }
    }
}

void opc_usr(void)
{
    int sz = getbufsz(&rdhdr);
    if (readbuf_ex(inbuf, sz) != -1) {
        if (chkrdy()) {
            sendopc(OPC_USR, OPC_FAILED);
            return;
        }
        if ((ready & SETUP) != SETUP) {
            sendopc(OPC_USR, OPC_FAILED);
            return;
        }
        char tmp[PATH_MAX];
        if (sz > PATH_MAX - 1)
            sz = PATH_MAX - 1;
        memcpy(tmp, inbuf, sz);
        tmp[sz] = '\0';
        strcpy(usrnam, tmp);
        strcpy(usrpath, orgunit);
        strcat(usrpath, "/");
        strcat(usrpath, tmp);
        if (unusr && islogin()) {
            sendopc(OPC_USR, OPC_FAILED);
            return;
        }
        if (chkpath(usrpath) || sz >= USRNAME_LEN) {
            ready &= ~USR;
            sendopc(OPC_USR, OPC_FAILED);
        } else {
            ready |= USR;
            mkmsgdir();
            sendopc(OPC_USR, OPC_SUCCESS);
        }
    }
}

void opc_sndmsg(void)
{
    int sz = getbufsz(&rdhdr);
    if (sz + sizeof rdhdr >= BUFLEN)
        return;
    if (readbuf_ex(inbuf, sz) != -1) {
        if (!chkrdy()) {
            sendopc(OPC_SNDMSG, OPC_FAILED);
            return;
        }
        char *msgstart = strchr(inbuf, MSGSEPCH);
        if (msgstart) {
            *msgstart = '\0';
            int usrsz = strlen(inbuf) + 1;
            if (sz - usrsz <= 0)
                return;
            msgstart++;
            char msgtmp[PATH_MAX];
            char stmpstr[USRNAME_LEN + UXTIMELEN + 2]; 
            mktmpath(msgtmp, stmpstr);
            strcat(stmpstr, USRSEP);
            strcat(stmpstr, usrnam);
            strcat(stmpstr, USRSEP);
            int fd = open(msgtmp, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | 
                S_IRGRP | S_IROTH);
            int wb = 0;
            do
                wb = write(fd, stmpstr, strlen(stmpstr));
            while (wb == -1 && errno == EINTR);
            do
                wb = write(fd, msgstart, sz - usrsz);
            while (wb == -1 && errno == EINTR);
            close(fd);
            char *usrpt = strtok(inbuf, USRSEP);
            while (usrpt) {
                cpymsg(usrpt, msgtmp);
                usrpt = strtok(NULL, USRSEP);
            }
            unlink(msgtmp);
        }
    }
}

static void cpymsg(char *usr, const char *msg)
{
    char path[PATH_MAX];
    strcpy(path, orgunit);
    strcat(path, "/");
    strcat(path, usr);
    strcat(path, "/");
    strcat(path, MSGFOLDER);
    strcat(path, "/");
    if (chkblk(path))
        return;
    char file[PATH_MAX];
    char *bg = strstr(msg, MSGTMPDIR) + strlen(MSGTMPDIR) + 1;
    char *end = strstr(msg, TMPEXT);
    int c = 0;
    while (bg < end)
        file[c++] = *bg++;
    file[c] = '\0';
    strcat(path, file);
    link(msg, path);
}

static void mkmsgdir(void)
{
    char msgdir[PATH_MAX];
    strcpy(msgdir, usrpath);
    strcat(msgdir, "/");
    strcat(msgdir, MSGFOLDER);
    strcpy(usrmsg, msgdir);
    mkdir(msgdir, S_IRWXU);
    strcat(msgdir, "/");
    strcat(msgdir, BLACKLIST);
    strcpy(usrblk, msgdir);
    mkdir(msgdir, S_IRWXU);
    strcpy(msgdir, usrpath);
    strcat(msgdir, "/");
    strcat(msgdir, MSGFOLDER);
    strcat(msgdir, "/");
    strcat(msgdir, MSGTMPDIR);
    mkdir(msgdir, S_IRWXU);
}

static void mktmpath(char *tmp, char *tmpstr)
{
    static char date[UXTIMELEN] = "";
    time_t t = time(NULL);
    gettm(&t, date);
    if (tmpstr)
        strcpy(tmpstr, date);
    static long long id = 0;
    static char idstr[LLDIGITS];
    sprintf(idstr, "%llu", id++);
    strcpy(tmp, usrmsg);
    strcat(tmp, "/");
    strcat(tmp, MSGTMPDIR);
    strcat(tmp, "/");
    strcat(tmp, date);
    strcat(tmp, "-");
    strcat(tmp, idstr);
    strcat(tmp, ".");
    strcat(tmp, usrnam);
    strcat(tmp, ".");
    catpid(tmp);
    strcat(tmp, TMPEXT);
}

void opc_setup(void)
{
    int sz = getbufsz(&rdhdr);
    if ((sz & UNUSR) == UNUSR)
        unusr = 1;
    else 
        unusr = 0;
    if ((sz & AUTODEL) == AUTODEL)
        autodel = 1;
    else
        autodel = 0;
    if (readbuf_ex(inbuf, sizeof tmstmp) != -1) {
        uint64_t *ts = (uint64_t *) inbuf;
        if (!isbigendian())
            swapbo64(*ts);
        tmstmp = *ts;
        ready |= SETUP;
    }
}

static int islogin(void)
{
    strcpy(lckfile, usrpath);
    strcat(lckfile, "/");
    strcat(lckfile, LCKFILE);
    lckfd = open(lckfile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (lckfd == -1 || crtlock(lckfd, NONBLK) == -1)
        return 1;
    else
        return 0;
}

void opc_cleanup(void)
{
    crtlock(lckfd, UNLOCK);
    close(lckfd);
}

static void catpid(char *buf)
{
    char strpid[LLDIGITS];
    pid_t pid = getpid();
    sprintf(strpid, "%d", pid);
    strcat(buf, strpid);
}

static int chkblk(const char *usr)
{
    char path[PATH_MAX];
    strcpy(path, usr);
    strcat(path, BLACKLIST);
    strcat(path, "/");
    strcat(path, usrnam);
    if (!access(path, F_OK))
        return 1;
    else
        return 0;
}

void opc_sndmsg2(void)
{
    int sd;
    struct dirent **ent;
    struct dirent *dir;
    if ((sd = scandir(usrmsg, &ent, msgselect, timecmp)) != -1) {
        int c = 0;
        for (; c < sd; c++) {
            dir = *(ent + c);
            if (ctmstmp(dir->d_name) && writemsg(dir->d_name) && autodel)
                delmsg(dir->d_name);
        }
    }
}

static int msgselect(const struct dirent *ent)
{
    if (ent->d_type == DT_REG)
        return 1;
    else
        return 0;
}

static int ctmstmp(const char *msg)
{
    unsigned long long tmp = strtoull(msg, NULL, 10);
    if (tmp > tmstmp) {
        tmstmp = tmp;
        return 1;
    }
    return 0;
}

static void delmsg(const char *msg)
{
    char path[PATH_MAX];
    strcpy(path, usrmsg);
    strcat(path, "/");
    strcat(path, msg);
    unlink(path);
}

static int writemsg(const char *msg)
{
    char path[PATH_MAX];
    strcpy(path, usrmsg);
    strcat(path, "/");
    strcat(path, msg);
    wrhdr.opcode = OPC_SNDMSG;
    int fd;
    if ((fd = open(path, O_RDWR)) != -1) {
        int rb;
        do
            rb = read(fd, outbuf, BUFLEN);
        while (rb == -1 && errno == EINTR);
        close(fd);
        setbufsz(rb, &wrhdr);
        if (writebuf_ex((char *) &wrhdr, sizeof wrhdr) == -1)
            return 0;
        if (writebuf_ex(outbuf, rb) == -1)
            return 0;
    }
    return 1;
}

static int timecmp(const struct dirent **a, const struct dirent **b)
{
    unsigned long long tma, tmb;
    tma = strtoull((*a)->d_name, NULL, 10);
    tmb = strtoull((*b)->d_name, NULL, 10);
    if (tma > tmb)
        return 1;
    else if (tma < tmb)
        return -1;
    else 
        return 0;
}
