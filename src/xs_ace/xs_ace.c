/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <xs_ace/xs_ace.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmd.h>
#include <util.h>
#include <string.h>
#include <tfproto.h>
#include <unistd.h>
#include <fcntl.h>
#include <init.h>
#include <sys/types.h>
#include <errno.h>
#include <tfproto.h>
#include <sys/stat.h>
#include <util.h>
#include <pthread.h>
#include <stdarg.h>
#include <signal.h>

#define KEYLEN 255
#define ARG_SEP " "
#define MINARGS 2
#define EMPTYSTR ""
#define BINACLEXT ".acl"
#define ANYARGCHR '*'

enum cltype { CLOSE_FD, CLOSE_FP };

struct rundat {
    int pdin[2];
    int pdout[2];
    FILE *fin;
    FILE *fout;
    pid_t pid;
};

char xs_errbuf[XS_ERRBFLEN];
char wdir[LINE_MAX];
static volatile int exitmod;
static int keyset;
static char *rxbuf;
static char *txbuf;
static int64_t lnbufsz;
static char **argls;
static int arglen;
static char *pbuftx;
static char *pbufrx;
static int64_t pbufsz;
static char binacl[PATH_MAX];

/* Write code to the client. */
static void writecode(int64_t code, const char *msg);
/* RUN_NL commmand read thread. */
static void *runnl_rdth(void *prms);
/* RUN_NL commmand write thread. */
static void *runnl_wrth(void *prms);
/* FILE pointer list close function. */
static void lstclose(enum cltype type, int argc, ...);
/* Free argument list resources. */
static void freeargs(void);
/* RUN_BUF commmand write thread. */
static void *runbuf_wrth(void *prms);
/* RUN_BUF commmand read thread. */
static void *runbuf_rdth(void *prms);
/* Send module exit status. */
static int sendes(int32_t es);
/* Check access control list file. */
static int chkacl(const char *binpath);
/* Check access control list file for xs_ace_run command. */
static int chkaclrun(const char *binpath);

void xs_ace(void)
{
    if (!pbufrx || !pbuftx) {
        pbuftx = malloc(PIPE_BUF);
        pbufrx = malloc(PIPE_BUF);
        if (!pbuftx || !pbufrx) {
            free(pbufrx);
            free(pbuftx);
            cmd_fail(XS_ACE_EBUF);
            return;
        }
        pbufsz = PIPE_BUF;
    }
    if (!rxbuf || !txbuf) {
        rxbuf = malloc(BUFSIZ);
        txbuf = malloc(BUFSIZ);
        if (!rxbuf || !txbuf) {
            free(rxbuf);
            free(txbuf);
            cmd_fail(XS_ACE_EBUF);
            return;
        }
        lnbufsz = BUFSIZ;
    }
    cmd_ok();
    exitmod = 0;
    while (!exitmod) {
         if (getcmd())
            return;
        comm.buf[COMMBUFLEN - 1] = 0;
        char cmd[CMD_NAMELEN];
        excmd(comm.buf, cmd);
        cmdtoupper(cmd);
        if (!strcmp(cmd, XS_ACE_INSKEY))
            xs_ace_inskey();
        else if (!strcmp(cmd, XS_ACE_EXIT))
            xs_ace_exit();
        else if (!strcmp(cmd, XS_ACE_RUN))
            xs_ace_run();
        else if (!strcmp(cmd, XS_ACE_RUNNL))
            xs_ace_runnl();
        else if (!strcmp(cmd, XS_ACE_SETARGS))
            xs_ace_setargs();
        else if (!strcmp(cmd, XS_ACE_SETWDIR))
            xs_ace_setwdir();
        else if (!strcmp(cmd, XS_ACE_RUNNL_SZ))
            xs_ace_run_bufsz(XS_BUFTYPE_LN);
        else if (!strcmp(cmd, XS_ACE_RUNBUF_SZ))
            xs_ace_run_bufsz(XS_BUFTYPE_BUF);
        else if (!strcmp(cmd, XS_ACE_RUNBUF))
            xs_ace_runbuf();
        else if (!strcmp(cmd, XS_ACE_SET_RUNBUF))
            xs_ace_set_runbuf();
        else if (!strcmp(cmd, XS_ACE_SET_RUNNL))
            xs_ace_set_runnl();
        else if (!strcmp(cmd, XS_ACE_GOBACK))
            xs_ace_goback();
        else if (!strcmp(cmd, XS_ACE_RUNBK))
            xs_ace_runbk();
        else
            cmd_unknown();
    }
}

void xs_ace_inskey(void)
{
    if (keyset) {
        cmd_fail(XS_ACE_EKEYSET);
        return;        
    }
    char *pt = comm.buf + strlen(XS_ACE_INSKEY) + 1;
    char key[KEYLEN];
    strcpy(key, pt);
    FILE *keydb = fopen(tfproto.xsace, "r");
    if (!keydb) {
        cmd_fail(XS_ACE_EKEYREAD);
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
        cmd_fail(XS_ACE_EKEYNOTSET);
        return;
    }
    strcpy(binacl, tfproto.xsace);
    strcat(binacl, ".");
    strcat(binacl, key);
    strcat(binacl, BINACLEXT);
    cmd_ok();
}

void xs_ace_exit(void)
{
    exitmod = 1;
    keyset = 0;
    pbufsz = 0;
    lnbufsz = 0;
    free(rxbuf);
    free(txbuf);
    rxbuf = NULL;
    txbuf = NULL;
    free(pbufrx);
    free(pbuftx);
    pbufrx = NULL;
    pbuftx = NULL;
    freeargs();
    *wdir = '\0';
    binacl[0] = '\0';
    cmd_ok();
}

void xs_ace_run(void)
{
    if (!keyset) {
        cmd_fail(XS_ACE_EPERM);
        return;
    }
    char *pt = comm.buf + strlen(XS_ACE_RUN) + 1;
    char path[PATH_MAX], path2[PATH_MAX];
    path2[0] = '\0';
    char *ppt = strstr(pt, CMD_SEPSTR);
    if (ppt) {
        strcpy(path2, ppt + strlen(CMD_SEPSTR));
        *ppt = '\0';
    }
    strcpy(path, pt);
    if (!chkaclrun(path))  {
        cmd_fail(XS_ACE_EACL);
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        cmd_fail(XS_ACE_ERUN);
        return;
    }
    if (!pid) {
        lstclose(CLOSE_FD, 3, STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO);
        dup(comm.sock);
        dup(comm.sock);
        dup(comm.sock);
        char keylen[INTDIGITS];
        sprintf(keylen, "%d", cryp_org.rndlen * HEXDIG_LEN);
        char key[KEYMAX * HEXDIG_LEN + 1];
        bytetohex(cryp_org.rndkey, cryp_org.rndlen, key);
        if (strcmp(wdir, EMPTYSTR))
            chdir(wdir);
        if (!strlen(path2))
            execlp(path, path, key, keylen, NULL);
        else
            execlp(path, path, path2, key, keylen, NULL);
        cmd_fail(XS_ACE_ERUN);
        exit(EXIT_FAILURE);
    }
    int32_t es = sec_waitpid(pid);
    if (sendes(es) == -1) {
        exitmod = 1;
        endcomm();
        return;
    }
}

void xs_ace_runnl(void)
{
    if (!keyset) {
        writecode(XS_ACE_ERROR, XS_ACE_EPERM);
        return;
    }
    char *pt = comm.buf + strlen(XS_ACE_RUNNL) + 1;
    char path[PATH_MAX];
    strcpy(path, pt);
    if (!chkacl(path))  {
        writecode(XS_ACE_ERROR, XS_ACE_EACL);
        return;
    }
    int rs; 
    int pdin[2];
    int pdout[2];
    rs = pipe(pdin);
    rs += pipe(pdout);
    if (rs) {
        writecode(XS_ACE_ERROR, XS_ACE_ERUN);
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        lstclose(CLOSE_FD, 4, pdin[0], pdin[1], pdout[0], pdout[1]);
        writecode(XS_ACE_ERROR, XS_ACE_ERUN);
        return;
    }
    if (!pid) {
        lstclose(CLOSE_FD, 3, STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO);
        lstclose(CLOSE_FD, 2, pdin[1], pdout[0]);
        dup(pdin[0]);
        dup(pdout[1]);
        dup(pdout[1]);
        lstclose(CLOSE_FD, 2, pdin[0], pdout[1]);
        sigset_t mask;
        sigemptyset(&mask);
        pthread_sigmask(SIG_SETMASK, &mask, NULL);
        if (strcmp(wdir, EMPTYSTR))
            chdir(wdir);
        if (!argls) {
            argls = malloc(sizeof(char *) * MINARGS);
            *argls = malloc(strlen(path) + 1);
            strcpy(*argls, path);
            *(argls + 1) = NULL;
            execvp(path, argls);
        } else
            execvp(path, argls);
        exit(EXIT_FAILURE);
    }
    lstclose(CLOSE_FD, 2, pdout[1], pdin[0]);
    FILE *fin = fdopen(pdin[1], "w");
    FILE *fout = fdopen(pdout[0], "r");
    if (!fin || !fout) {
        lstclose(CLOSE_FD, 2, pdin[1], pdout[0]);
        writecode(XS_ACE_ERROR, XS_ACE_ERUN);
        return;   
    }
    rs = setvbuf(fin, NULL, _IOLBF, BUFSIZ);
    rs += setvbuf(fout, NULL, _IOLBF, BUFSIZ);
    if (rs) {
        lstclose(CLOSE_FD, 4, pdin[0], pdin[1], pdout[0], pdout[1]);
        lstclose(CLOSE_FP, 2, fin, fout);
        writecode(XS_ACE_ERROR, XS_ACE_ERUN);
        return;
    }
    struct rundat rdat;
    rdat.pdin[0] = pdin[0];
    rdat.pdin[1] = pdin[1];
    rdat.pdout[0] = pdout[0];
    rdat.pdout[1] = pdout[1];
    rdat.fin = fin;
    rdat.fout = fout;
    rdat.pid = pid;
    pthread_t wrth;
    rs = pthread_create(&wrth, NULL, runnl_wrth, &rdat);
    if (rs) {
        lstclose(CLOSE_FD, 4, pdin[0], pdin[1], pdout[0], pdout[1]);
        lstclose(CLOSE_FP, 2, fin, fout);
        writecode(XS_ACE_ERROR, XS_ACE_ERUN);
        return;
    }
    pthread_t rdth;
    rs = pthread_create(&rdth, NULL, runnl_rdth, &rdat);
    if (rs) {
        lstclose(CLOSE_FD, 4, pdin[0], pdin[1], pdout[0], pdout[1]);
        lstclose(CLOSE_FP, 2, fin, fout);
        pthread_cancel(wrth);
        writecode(XS_ACE_ERROR, XS_ACE_ERUN);
        return;
    }
    writecode(XS_ACE_OK, NULL);
    pthread_join(rdth, NULL);
    pthread_join(wrth, NULL);
    if (exitmod)
        return;
    int32_t es = sec_waitpid(pid);
    lstclose(CLOSE_FD, 4, pdin[0], pdin[1], pdout[0], pdout[1]);
    lstclose(CLOSE_FP, 2, fin, fout);
    if (sendes(es) == -1) {
        exitmod = 1;
        endcomm();
        return;
    }
}

static void writecode(int64_t code, const char *msg)
{
    int64_t err = code;
    if (code == XS_ACE_ERROR) {
        if (!isbigendian())
            swapbo64(err);
        strcpy(xs_errbuf, msg);
        if (writebuf_ex((char *) &err, sizeof err) == 1) {
            exitmod = 1;
            endcomm();
            return;
        }
        if (writebuf_ex(xs_errbuf, sizeof xs_errbuf) == 1) {
            exitmod = 1;
            endcomm();
            return;
        }
    } else if (code == XS_ACE_OK || code == XS_ACE_RUNEND) {
        if (!isbigendian())
            swapbo64(err);
        if (writebuf_ex((char *) &err, sizeof err) == 1) {
            exitmod = 1;
            endcomm();
            return;
        }
    }
}

static void *runnl_rdth(void *prms)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    int64_t hdr;
    struct rundat *rdat = prms;
    while (1) {
        if (readbuf_ex((char *) &hdr, sizeof hdr) == -1) {
            exitmod = 1;
            endcomm();
            kill(rdat->pid, SIGKILL);
            sec_waitpid(rdat->pid);
            return NULL;
        }
        if (!isbigendian())
            swapbo64(hdr);
        if (hdr == XS_ACE_OK) 
            break;
        else if (hdr == XS_ACE_SIGKILL) {
            kill(rdat->pid, SIGKILL);
            continue;
        } else if (hdr == XS_ACE_SIGTERM) {
            kill(rdat->pid, SIGTERM);
            continue;
        } else if (hdr == XS_ACE_SIGUSR1) {
            kill(rdat->pid, SIGUSR1);
            continue;
        } else if (hdr == XS_ACE_SIGUSR2) {
            kill(rdat->pid, SIGUSR2);
            continue;
        } else if (hdr == XS_ACE_SIGHUP) {
            kill(rdat->pid, SIGHUP);
            continue;
        }
        memset(rxbuf, 0, lnbufsz);
        if (readbuf_ex(rxbuf, hdr) == -1) {
            exitmod = 1;
            endcomm();
            kill(rdat->pid, SIGKILL);
            sec_waitpid(rdat->pid);
            return NULL;
        }
        fprintf(rdat->fin, "%s\n", rxbuf);
    }
    return NULL;
}

static void *runnl_wrth(void *prms)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    int64_t hdr;
    struct rundat *rdat = prms;
    while (1) {
        memset(txbuf, 0, lnbufsz);
        if (fgets(txbuf, lnbufsz, rdat->fout) != NULL) {
            int64_t len = strlen(txbuf);
            hdr = len;
            if (!isbigendian())
                swapbo64(hdr);
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                exitmod = 1;
                endcomm();
                return NULL;
            }
            if (writebuf_ex(txbuf, len) == -1) {
                exitmod = 1;
                endcomm();
                return NULL;
            }
        } else 
            break;
    }
    writecode(XS_ACE_RUNEND, NULL);
    return NULL;
}

static void lstclose(enum cltype type, int argc, ...)
{    
    va_list list;
    va_start(list, argc);
    int i = 0;
    for (; i < argc; i++)
        if (type == CLOSE_FD)
            close(va_arg(list, int));
        else if (type == CLOSE_FP)
            fclose(va_arg(list, FILE *));
    va_end(list);
}

void xs_ace_setargs(void)
{
    if (!keyset) {
        cmd_fail(XS_ACE_EPERM);
        return;        
    }
    char *pt = comm.buf + strlen(XS_ACE_SETARGS);
    freeargs();
    do
        if (*pt == ' ')
            arglen++;
    while (*pt++);
    if (!arglen) {
        cmd_ok();
        return;
    }
    argls = malloc(sizeof(char *) * (arglen + 1)); 
    if (!argls) {
        cmd_fail(XS_ACE_EARGS);
        return;
    }
    *(argls + arglen) = NULL;
    char *strpt = strtok(comm.buf + strlen(XS_ACE_SETARGS) + 1, ARG_SEP);
    if (!strpt) {
        cmd_fail(XS_ACE_EARGS);
        return;
    }
    int c = 0;
    for (; c < arglen; c++) {
        *(argls + c) = malloc(strlen(strpt) + 1);
        strcpy(*(argls + c), strpt);
        strpt = strtok(NULL, ARG_SEP);
    }
    cmd_ok();
}

static void freeargs(void)
{
    if (argls != NULL) {
        int c = 0;
        for (; c < arglen; c++)
            free(*(argls + c));
        free(argls);
        argls = NULL;
        arglen = 0;
    }
}

void xs_ace_run_bufsz(int buftp)
{
    if (!keyset) {
        cmd_fail(XS_ACE_EPERM);
        return;        
    }
    if (buftp == XS_BUFTYPE_LN)
        sprintf(comm.buf, "%s %lld", CMD_OK, (long long int) lnbufsz);
    else if (buftp == XS_BUFTYPE_BUF)
        sprintf(comm.buf, "%s %lld", CMD_OK, (long long int) pbufsz);
    if (writebuf(comm.buf, strlen(comm.buf)) == -1) {
        exitmod = 1;
        endcomm();
    }
}

void xs_ace_runbuf(void)
{
    if (!keyset) {
        writecode(XS_ACE_ERROR, XS_ACE_EPERM);
        return;
    }
    char *pt = comm.buf + strlen(XS_ACE_RUNBUF) + 1;
    char path[PATH_MAX];
    strcpy(path, pt);
    if (!chkacl(path))  {
        writecode(XS_ACE_ERROR, XS_ACE_EACL);
        return;
    }
    int rs; 
    int pdin[2];
    int pdout[2];
    rs = pipe(pdin);
    rs += pipe(pdout);
    if (rs) {
        writecode(XS_ACE_ERROR, XS_ACE_ERUN);
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        lstclose(CLOSE_FD, 4, pdin[0], pdin[1], pdout[0], pdout[1]);
        writecode(XS_ACE_ERROR, XS_ACE_ERUN);
        return;
    }
    if (!pid) {
        lstclose(CLOSE_FD, 3, STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO);
        lstclose(CLOSE_FD, 2, pdin[1], pdout[0]);
        dup(pdin[0]);
        dup(pdout[1]);
        dup(pdout[1]);
        lstclose(CLOSE_FD, 2, pdin[0], pdout[1]);
        sigset_t mask;
        sigemptyset(&mask);
        pthread_sigmask(SIG_SETMASK, &mask, NULL);
        if (strcmp(wdir, EMPTYSTR))
            chdir(wdir);
        if (!argls) {
            argls = malloc(sizeof(char *) * MINARGS);
            *argls = malloc(strlen(path) + 1);
            strcpy(*argls, path);
            *(argls + 1) = NULL;
            execvp(path, argls);
            
        } else
            execvp(path, argls);
        exit(EXIT_FAILURE);
    }
    lstclose(CLOSE_FD, 2, pdout[1], pdin[0]);
    struct rundat rdat;
    rdat.pdin[0] = pdin[0];
    rdat.pdin[1] = pdin[1];
    rdat.pdout[0] = pdout[0];
    rdat.pdout[1] = pdout[1];
    rdat.pid = pid;
    pthread_t wrth;
    rs = pthread_create(&wrth, NULL, runbuf_wrth, &rdat);
    if (rs) {
        lstclose(CLOSE_FD, 4, pdin[0], pdin[1], pdout[0], pdout[1]);
        writecode(XS_ACE_ERROR, XS_ACE_ERUN);
        return;
    }
    pthread_t rdth;
    rs = pthread_create(&rdth, NULL, runbuf_rdth, &rdat);
    if (rs) {
        lstclose(CLOSE_FD, 4, pdin[0], pdin[1], pdout[0], pdout[1]);
        pthread_cancel(wrth);
        writecode(XS_ACE_ERROR, XS_ACE_ERUN);
        return;
    }
    writecode(XS_ACE_OK, NULL);
    pthread_join(rdth, NULL);
    pthread_join(wrth, NULL);
    if (exitmod)
        return;
    int32_t es = sec_waitpid(pid);
    lstclose(CLOSE_FD, 4, pdin[0], pdin[1], pdout[0], pdout[1]);
    if (sendes(es) == -1) {
        exitmod = 1;
        endcomm();
        return;
    }
}

static void *runbuf_rdth(void *prms)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    int64_t hdr;
    struct rundat *rdat = prms;
    while (1) {
        if (readbuf_ex((char *) &hdr, sizeof hdr) == -1) {
            exitmod = 1;
            endcomm();
            kill(rdat->pid, SIGKILL);
            sec_waitpid(rdat->pid);
            return NULL;
        }
        if (!isbigendian())
            swapbo64(hdr);
        if (hdr == XS_ACE_OK) 
            break;
        else if (hdr == XS_ACE_SIGKILL) {
            kill(rdat->pid, SIGKILL);
            continue;
        } else if (hdr == XS_ACE_SIGTERM) {
            kill(rdat->pid, SIGTERM);
            continue;
        } else if (hdr == XS_ACE_SIGUSR1) {
            kill(rdat->pid, SIGUSR1);
            continue;
        } else if (hdr == XS_ACE_SIGUSR2) {
            kill(rdat->pid, SIGUSR2);
            continue;
        } else if (hdr == XS_ACE_SIGHUP) {
            kill(rdat->pid, SIGHUP);
            continue;
        }
        if (readbuf_ex(pbufrx, hdr) == -1) {
            exitmod = 1;
            endcomm();
            kill(rdat->pid, SIGKILL);
            sec_waitpid(rdat->pid);
            return NULL;
        }
        int64_t wb = 0;
        int64_t written = 0;
        do {
            wb = writechunk(rdat->pdin[1], pbufrx + written, hdr - written);
            written += wb;
        } while (wb != -1 && hdr - written > 0);
    }
    return NULL;
}

static void *runbuf_wrth(void *prms)
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    int64_t hdr, buflen;
    struct rundat *rdat = prms;
    while (1) {
        if ((buflen = readchunk(rdat->pdout[0], pbuftx, sizeof pbuftx)) > 0) {
            hdr = buflen;
            if (!isbigendian())
                swapbo64(hdr);
            if (writebuf_ex((char *) &hdr, sizeof hdr) == -1) {
                exitmod = 1;
                endcomm();
                return NULL;
            }
            if (writebuf_ex(pbuftx, buflen) == -1) {
                exitmod = 1;
                endcomm();
                return NULL;
            }
        } else
            break;
    }
    writecode(XS_ACE_RUNEND, NULL);
    return NULL;
}

void xs_ace_set_runbuf(void)
{
    if (!keyset) {
        cmd_fail(XS_ACE_EKEYNOTSET);
        return;
    }
    char *pt = comm.buf + strlen(XS_ACE_SET_RUNBUF) + 1;
    int64_t newsz;
    newsz = atoll(pt);
    if (newsz < sizeof(int64_t)) {
        cmd_fail(XS_ACE_EBUF);
        return;
    }
    pbufsz = newsz;
    free(pbufrx);
    free(pbuftx);
    pbufrx = malloc(newsz);
    pbuftx = malloc(newsz);
    if (!pbufrx || !pbuftx) {
        free(pbufrx);
        free(pbuftx);
        cmd_fail(XS_ACE_EBUF);
        return;
    }
    cmd_ok();
}

void xs_ace_set_runnl(void)
{
    if (!keyset) {
        cmd_fail(XS_ACE_EKEYNOTSET);
        return;
    }
    char *pt = comm.buf + strlen(XS_ACE_SET_RUNNL) + 1;
    int64_t newsz;
    newsz = atoll(pt);
    if (newsz < sizeof(int64_t)) {
        cmd_fail(XS_ACE_EBUF);
        return;
    }
    lnbufsz = newsz;
    free(rxbuf);
    free(txbuf);
    rxbuf = malloc(newsz);
    txbuf = malloc(newsz);
    if (!rxbuf || !txbuf) {
        free(rxbuf);
        free(txbuf);
        cmd_fail(XS_ACE_EBUF);
        return;
    }
    cmd_ok();
}

void xs_ace_goback(void)
{
    exitmod = 1;
    cmd_ok();
}

static int sendes(int32_t es)
{
    if (!isbigendian())
        swapbo32(es);
    if (writebuf_ex((char *) &es, sizeof es) == -1)
        return -1;
    return 0;
}

void xs_ace_setwdir(void)
{
    if (!keyset) {
        cmd_fail(XS_ACE_EKEYNOTSET);
        return;
    }
    char tmpwd[PATH_MAX];
    getcwd(tmpwd, sizeof tmpwd);
    char *pt = comm.buf + strlen(XS_ACE_SETWDIR) + 1;
    if (chdir(pt) == -1) {
        chdir(tmpwd);
        cmd_fail(XS_ACE_EWDIR);
        return;
    }
    strcpy(wdir, pt);
    chdir(tmpwd);
    cmd_ok();
}

static int chkacl(const char *binpath)
{
    if (access(binacl, F_OK))
        return 1;
    FILE *acldb = fopen(binacl, "r");
    if (!acldb)
        return 0;
    char buf[LINE_MAX];
    char *pt;
    char args[LINE_MAX];
    if (argls) {
        strcpy(args, binpath);
        strcat(args, " ");
        char **ap = argls;
        while (*ap != NULL) {
            strcat(args, *ap++);
            strcat(args, " ");
        }
        pt = strchr(args, '\0');
        if (pt)
            *(pt - 1) = '\0';
    }
    while (fgets(buf, sizeof buf, acldb)) {
        pt = strchr(buf, '\n');
        if (pt) 
            *pt = '\0';
        if (buf[0] == ANYARGCHR) {
            pt = strchr(buf, ' ');
            if (pt)
                *pt = '\0';
            if (!strcmp(buf + 1, binpath)) {
                fclose(acldb);
                return 1;
            }
        } else if (!strcmp(buf, args)) {
            fclose(acldb);
            return 1;
        }
    }
    fclose(acldb);
    return 0;
}

static int chkaclrun(const char *binpath)
{
    if (access(binacl, F_OK))
        return 1;
    FILE *acldb = fopen(binacl, "r");
    if (!acldb)
        return 0;
    char *pt;
    char buf[LINE_MAX];
    while (fgets(buf, sizeof buf, acldb)) {
        pt = strchr(buf, '\n');
        if (pt) 
            *pt = '\0';
        if (!strcmp(buf, binpath)) {
            fclose(acldb);
            return 1;
        }
    }
    fclose(acldb);
    return 0;
}

void xs_ace_runbk(void)
{
    if (!keyset) {
        cmd_fail(XS_ACE_EPERM);
        return;
    }
    char *pt = comm.buf + strlen(XS_ACE_RUNBK) + 1;
    char path[PATH_MAX];
    strcpy(path, pt);
    if (!chkacl(path))  {
        cmd_fail(XS_ACE_EACL);
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        cmd_fail(XS_ACE_ERUN);
        return;
    }
    if (!pid) {
        pid = fork();
        if (!pid) {
            sigset_t mask;
            sigemptyset(&mask);
            pthread_sigmask(SIG_SETMASK, &mask, NULL);
            if (strcmp(wdir, EMPTYSTR))
                chdir(wdir);
            if (!argls) {
                argls = malloc(sizeof(char *) * MINARGS);
                *argls = malloc(strlen(path) + 1);
                strcpy(*argls, path);
                *(argls + 1) = NULL;
                execvp(path, argls);
            } else
                execvp(path, argls);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    sec_waitpid(pid);
    cmd_ok();
}
