/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <xs_ntmex/xs_ntmex.h>
#include <stdio.h>
#include <cmd.h>
#include <util.h>
#include <string.h>
#include <tfproto.h>
#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>
#include <init.h>
#include <dlfcn.h>
#include <sys/utsname.h>

#define KEYLEN 255
#define ENTRYSYM "xs_ntmex"
#define SYSNFOLEN 2048
#define ACLEXT ".acl"

typedef void (*Ntmexentry)(int64_t (*r_io)(char *buf, int64_t len),
    int64_t (*w_io)(char *buf, int64_t len), 
    int64_t (*rex_io)(char *buf, int64_t len),
    int64_t (*wex_io)(char *buf, int64_t len), 
    int (*jd)(const char *src,char *dst));

static int exitmod;
static Ntmexentry entry;
static int keyset;
static void *sohdl;
static char acl[PATH_MAX];

static int chkacl(const char *path);

void xs_ntmex(void)
{
    cmd_ok();
    exitmod = 0;
    while (!exitmod) {
         if (getcmd())
            return;
        comm.buf[COMMBUFLEN - 1] = 0;
        char cmd[CMD_NAMELEN];
        excmd(comm.buf, cmd);
        cmdtoupper(cmd);
        if (!strcmp(cmd, XS_NTMEX_INSKEY))
            xs_ntmex_inskey();
        else if (!strcmp(cmd, XS_NTMEX_GOBACK))
            xs_ntmex_goback();
        else if (!strcmp(cmd, XS_NTMEX_EXIT))
            xs_ntmex_exit();
        else if (!strcmp(cmd, XS_NTMEX_LOAD))
            xs_ntmex_load();
        else if (!strcmp(cmd, XS_NTMEX_RUN))
            xs_ntmex_run();
        else if (!strcmp(cmd, XS_NTMEX_SYSNFO))
            xs_ntmex_sysnfo();
        else
            cmd_unknown();
    }
}

void xs_ntmex_inskey(void)
{
    if (keyset) {
        cmd_fail(XS_NTMEX_EKEYSET);
        return;        
    }
    char *pt = comm.buf + strlen(XS_NTMEX_INSKEY) + 1;
    char key[KEYLEN];
    strcpy(key, pt);
    FILE *keydb = fopen(tfproto.xsntmex, "r");
    if (!keydb) {
        cmd_fail(XS_NTMEX_EKEYREAD);
        return;
    }
    char buf[KEYLEN];
    while (fgets(buf, sizeof buf, keydb)) {
        pt = strchr(buf, '\n');
        if (pt) {
            *pt = '\0';
            if (!strcmp(buf, key)) {
                keyset = 1;
                break;
            }
        }
    }
    fclose(keydb);
    if (!keyset) {
        cmd_fail(XS_NTMEX_EKEYNOTSET);
        return;
    }
    strcpy(acl, tfproto.xsntmex);
    strcat(acl, ".");
    strcat(acl, key);
    strcat(acl, ACLEXT);
    cmd_ok();
}

void xs_ntmex_goback(void)
{
    exitmod = 1;
    cmd_ok();
}

void xs_ntmex_exit(void)
{
    exitmod = 1;
    keyset = 0;
    entry = NULL;
    acl[0] = '\0';
    if (sohdl)
        dlclose(sohdl);
    sohdl = NULL;
    cmd_ok();
}

void xs_ntmex_load(void)
{
    if (!keyset) {
        cmd_fail(XS_NTMEX_EPERM);
        return;
    }    
    if (sohdl) {
        dlclose(sohdl);
        sohdl = NULL;
        entry = NULL;
    }
    char *pt = comm.buf + strlen(XS_NTMEX_LOAD) + 1;
    char path[PATH_MAX];
    strcpy(path, pt);
    if (access(path, F_OK) == -1) {
        cmd_fail(CMD_EACCESS);
        return;
    }
    if (!chkacl(path))  {
        cmd_fail(XS_NTMEX_EACL);
        return;
    }
    sohdl = dlopen(path, RTLD_LAZY);
    if (!sohdl) {
        cmd_fail(XS_NTMEX_ELOAD);
        return;
    }
    entry = dlsym(sohdl, ENTRYSYM); 
    if (!entry) {
        dlclose(sohdl);
        sohdl = NULL;
        cmd_fail(XS_NTMEX_ELOAD);
        return;
    }
    cmd_ok();
}

void xs_ntmex_run(void)
{
    if (!keyset) {
        cmd_fail(XS_NTMEX_EPERM);
        return;
    }
    if (!entry) {
        cmd_fail(XS_NTMEX_ERUN);
        return;
    }
    cmd_ok();
    entry(readbuf, writebuf, readbuf_ex, writebuf_ex, jaildir);
}

void xs_ntmex_sysnfo(void)
{
    if (!keyset) {
        cmd_fail(XS_NTMEX_EPERM);
        return;
    }
    struct utsname nfo = { 0 };
    if (uname(&nfo) == -1) {
        cmd_fail(XS_NTMEX_ENFO);
        return;
    }
    strcpy(comm.buf, CMD_OK);
    strcat(comm.buf, " ");
    strcat(comm.buf, nfo.sysname);
    strcat(comm.buf, " | ");
    strcat(comm.buf, nfo.release);
    strcat(comm.buf, " | ");
    strcat(comm.buf, nfo.version);
    strcat(comm.buf, " | ");
    strcat(comm.buf, nfo.machine);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1) {
        exitmod = 1;
        endcomm();
        return;
    }
}

static int chkacl(const char *path)
{
    if (access(acl, F_OK))
        return 1;
    FILE *acldb = fopen(acl, "r");
    if (!acldb)
        return 0;
    char *pt;
    char buf[LINE_MAX];
    while (fgets(buf, sizeof buf, acldb)) {
        pt = strchr(buf, '\n');
        if (pt) 
            *pt = '\0';
        if (!strcmp(buf, path)) {
            fclose(acldb);
            return 1;
        }
    }
    fclose(acldb);
    return 0;
}
