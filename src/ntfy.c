/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <ntfy.h>
#include <tfproto.h>
#include <string.h> 
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <util.h>
#include <stdlib.h>
#include <cmd.h>
#include <util.h>
#include <time.h>
#include <dirent.h>
#include <sys/socket.h>
#include <float.h>

/* 25 ms of default timeout to look for directory changes. */
#define NTFY_TIMEOUT 25000000
/* Notification symbol length. */
#define NTFY_SYMLEN 8
/* Max length of digits for the timeout value. */
#define TMOUTLEN DBL_DIG + 3

/* Structures of notification data */
struct data {
    char path[PATH_MAX];
    char sym[NTFY_SYMLEN];
};

struct ntfy {
    struct data *data;
    int count;
    int index;
} static ntfy;

/* Loop condition variable. */
static int loop = 1;
/* timeout for the sleep interval. */
static double timeout;
/* The timeout structure for the nanosleep call. */
static struct timespec tm;

/* Set-up timespec timeout structure. */
static void setupntfy(void);
/* Check directory activity. */
static void chkact(void);
/* Check symbol integrity for notification: 0 = ok, -1 = error. */
static int chksym(const char *sym);
/* Find notification file in directory. */
static int findntfy(const char *path, const char *sym, char *flnam);
/* Send notification and wait for client response to continue. Return -1 for
    read/write error or CMD_END command; -2 for delete actual notification file
    and continue; and 0 for continue without delete the notification file. */
static int sendntfy(void);

void cmd_startntfy(void)
{
    if (!ntfy.data) {
        cmd_fail(CMD_ESTARTNTFY);
        return;
    }
    char *pt = comm.buf + strlen(CMD_STARTNTFY) + 1;
    char tmout[TMOUTLEN] = "";
    strcpy_sec(tmout, pt, TMOUTLEN);
    timeout = strtod(tmout, NULL);
    setupntfy();
    while (loop) {
        chkact();
        nanosleep(&tm, NULL);
        if (recv(comm.sock ,NULL, 1, MSG_PEEK | MSG_DONTWAIT) == 0)
            loop = 0;
    }
    /* End of communication after loop of notification terminates. */
    endcomm();
}

void cmd_addntfy(void)
{
    char *pt = comm.buf + strlen(CMD_ADDNTFY) + 1;
    char sym[NTFY_SYMLEN] = "";
    int c = 0;
    for (; c < NTFY_SYMLEN - 1 && *pt != CMD_SEPCH; c++)
        sym[c] = *pt++;
    sym[c] = '\0';
    if (chksym(sym)) {
        cmd_fail(CMD_EADDNTFY);
        return;
    }
    pt++;
    char path[PATH_MAX] = "";
    if (jaildir(pt, path)) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (chkpath(path)) {
        cmd_fail(CMD_EADDNTFY);
        return;
    }
    strcat(path, "/");
    struct data *data = ntfy.data;
    ntfy.data = realloc(ntfy.data, sizeof(struct data) * (ntfy.count + 1));
    if (!ntfy.data) {
        ntfy.data = data;
        cmd_fail(CMD_EADDNTFY);
        return;
    }
    ntfy.count++;
    strcpy((ntfy.data + ntfy.count - 1)->path, path);
    strcpy((ntfy.data + ntfy.count - 1)->sym, sym);
    cmd_ok();
}

static void setupntfy(void)
{
   if (timeout != 0 ) {
        tm.tv_sec = timeout;
        tm.tv_nsec = abs((int) ((timeout - tm.tv_sec) * NANOSEC));
    } else {
        tm.tv_sec = 0;
        tm.tv_nsec = NTFY_TIMEOUT;
    } 
}

static void chkact(void)
{
    int i = 0;
    char flnam[NAME_MAX] = "";
    char num[12] = "";
    for (; i < ntfy.count; i++) {
        if (findntfy((ntfy.data + i)->path, (ntfy.data + i)->sym, flnam)) {
            sprintf(num, "%d", i);
            strcpy(comm.buf, num);
            strcat(comm.buf, CMD_SEPSTR);
            strcat(comm.buf, flnam);
            int nr = 0;
            if ((nr = sendntfy()) == -1) {
                loop = 0;
                break;
            } else if (nr == -2) {
                char path[PATH_MAX] = "";
                strcpy(path, (ntfy.data + i)->path);
                strcat(path, flnam);
                unlink(path);
            }
        }
    }
}

static int chksym(const char *sym)
{
    const char forbsyms[][NTFY_SYMLEN] = { ".", "..", "/" };
    int i = 0;
    for (; i < sizeof forbsyms / NTFY_SYMLEN; i++)
        if (!strcmp(sym, forbsyms[i]))
            return -1;
    if (strstr(sym, forbsyms[2]))
        return -1;
    return 0;
}

static int findntfy(const char *path, const char *sym, char *flnam)
{
    struct dirent *den;
    DIR *dir = opendir(path);
    while ((den = readdir(dir)) != NULL)
        if (den->d_type != DT_DIR && den->d_type != DT_LNK && 
            strcmp(den->d_name, ".") && strcmp(den->d_name, "..")) {
            char *pt = strstr(den->d_name, sym);
            if (pt && pt == den->d_name) {
                strcpy_sec(flnam, den->d_name, NAME_MAX);
                return 1;
            }
        }
    closedir(dir);
    return 0;
}

static int sendntfy(void)
{
    if (writebuf(comm.buf, strlen(comm.buf)) == -1)
        return -1;
    if (getcmd())
        return -1;
    cmdtoupper(comm.buf);
    if (!strcmp(comm.buf, CMD_END))
        return -1;
    if (!strcmp(comm.buf, CMD_DEL))
        return -2;
    else
        return 0;
}
