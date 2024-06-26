/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <init.h>
#include <string.h>
#include <sys/stat.h>
#include <core.h>
#include <util.h>

/* Protocol version text in the config file. */
#define PROTOVER "proto "
/* Hash text in the config file. */
#define HASH "hash "
/* Dbdir text in the config file. */
#define DBDIR "dbdir "
/* Port text in the config file. */
#define PORT "port "
/* Diskspace text in the config file. */
//#define DISKSPACE "diskspace "
/* Server RSA private key */
#define PRIVKEY "privkey {"
/* Private key end delimiter */
#define PRIVKEY_ENDTOK '}'
/* Default username token. */
#define DEFUSR "defusr "
/* Users database token. */
#define USERDB "userdb "
/* eXtended subsystem NTMEX token. */
#define XSNTMEX "xsntmex "
/* eXtended subsystem ACE token. */
#define XSACE "xsace "
/* Injail security flag. */
#define  INJAIL "injail"
/* TLB Token. */
#define TLB "tlb "
/* Server RSA public key */
#define PUBKEY "pubkey {"
/* Public key end delimiter */
#define PUBKEY_ENDTOK '}'
/* TRP Transparent Proxy DNS token. */
#define TRP_DNSTOK "trp_dns "
/* TRP Transparent Proxy IPV4 token. */
#define TRP_IPV4TOK "trp_ipv4 "
/* TRP Transparent Proxy IPV6 token. */
#define TRP_IPV6TOK "trp_ipv6 "
/* Initial token to declare a line as a comment. */
#define COMMENT '#'
/* DBDIR secure namespace token. */
#define DBDIR_SECTOK "_sec"
/* LOCKSYS flag. */
#define LOCKSYS "locksys"
/* Remote Procedure Call Proxy. */
#define RPCPROXY "rpcproxy "
/* Number of maximun child processes. */
#define NPROCMAX "nprocmax "
/* Allow RUNBASH command. */
#define RUNBASH "runbash"
/* FlyContext system. */
#define FLYCONTEXT "flycontext"
/* Default maximun expiration quanta for FAI token. */
#define FAIMAX_QDEF 360
/* FAI directory path. */
#define FAIPATH "faipath"
/* FAI token maximun expiration quanta. */
#define FAITOK_MQ "faitok_mq"

struct tfproto tfproto;
static char *buf;

/* Free used resources like descriptors and allocated dinamically memory */
static void freeres(FILE *fs, char *buf);
/* Set database directory normalized. */
static void stddbpath(void);

int init(const char **argv, struct tfproto *tfproto)
{
    char ln[LINE_MAX];
    strcpy(tfproto->bin, *argv);
    strcpy(tfproto->conf, *(argv + 1));
    tfproto->argv = argv;
    FILE *fs = fopen(tfproto->conf, "r");
    if (fs == NULL)
        return -1;
    struct stat st;
    stat(tfproto->conf, &st);
    buf = malloc(st.st_size + 1);
    *buf = '\0';
    memset(buf, 0, st.st_size + 1);
    if (!buf) {
        freeres(fs, NULL);
        return -1;
    }
    while (fgets(ln, sizeof ln, fs))
        if (ln[0] != COMMENT)
            strcat(buf, ln);
    char *pt = strstr(buf, PROTOVER);
    if (!pt) {
        freeres(fs, buf);
        return -1;
    }
    pt += strlen(PROTOVER);
    int i = 0;
    while (*pt != '\n' && *pt != '\0' && i < PROTOLEN - 1)
        tfproto->proto[i++] = *pt++;
    if (!strlen(tfproto->proto)) {
        freeres(fs, buf);
        return -1;
    }
    pt = strstr(buf, HASH);
    if (!pt) {
        freeres(fs, buf);
        return -1;
    }
    pt += strlen(HASH);
    i = 0;
    while (*pt != '\n' && *pt != '\0' && i < HASHLEN - 1)
        tfproto->hash[i++] = *pt++;
    if (!strlen(tfproto->hash)) {
        freeres(fs, buf);
        return -1;
    }
    pt = strstr(buf, DBDIR);
    if (!pt) {
        freeres(fs, buf);
        return -1;
    }
    pt += strlen(DBDIR);
    i = 0;
    while (*pt != '\n' && *pt != '\0' && i < PATH_MAX - 1)
        tfproto->dbdir[i++] = *pt++;
    if (!strlen(tfproto->dbdir)) {
        freeres(fs, buf);
        return -1;
    }
    tfproto->dbdir[strlen(tfproto->dbdir)] = '/';
    pt = strstr(buf, PORT);
    if (!pt) {
        freeres(fs, buf);
        return -1;
    }
    pt += strlen(PORT);
    i = 0;
    while (*pt != '\n' && *pt != '\0' && i < PORTLEN - 1)
        tfproto->port[i++] = *pt++;
    if (!strlen(tfproto->port)) {
        freeres(fs, buf);
        return -1;
    }
    pt = strstr(buf, PRIVKEY);
    if (!pt) {
        freeres(fs, buf);
        return -1;
    }
    pt += strlen(PRIVKEY);
    i = 0;
    while (*pt != '\0' && *pt != PRIVKEY_ENDTOK)
        tfproto->priv[i++] = *pt++;
    pt = strstr(buf, DEFUSR);
    if (pt) {
        pt += strlen(DEFUSR);
        i = 0;
        while (*pt != '\n' && *pt != '\0' && i < DEFUSRLEN - 1)
            tfproto->defusr[i++] = *pt++;
    }
    pt = strstr(buf, USERDB);
    if (pt) {
        pt += strlen(USERDB);
        i = 0;
        while (*pt != '\n' && *pt != '\0' && i < PATH_MAX - 1)
            tfproto->userdb[i++] = *pt++;
    }
    pt = strstr(buf, RPCPROXY);
    if (pt) {
        pt += strlen(RPCPROXY);
        i = 0;
        while (*pt != '\n' && *pt != '\0' && i < PATH_MAX - 1)
            tfproto->rpcproxy[i++] = *pt++;
    }
    pt = strstr(buf, FAIPATH);
    if (pt) {
        pt += strlen(FAIPATH) + 1;
        i = 0;
        while (*pt != '\n' && *pt != '\0' && i < PATH_MAX - 1)
            tfproto->faipath[i++] = *pt++;
        strcat(tfproto->faipath, "/");
        
    } else 
        return -1;
    pt = strstr(buf, FAITOK_MQ);
    if (pt) {
        pt += strlen(FAITOK_MQ) + 1;
        i = 0;
        char faiq[LLDIGITS];
        while (*pt != '\n' && *pt != '\0' && i < PATH_MAX - 1)
            faiq[i++] = *pt++;
        faiq[i] = '\0';
        tfproto->faitok_mq = atoll(faiq);
    } else 
        tfproto->faitok_mq = FAIMAX_QDEF;
    pt = strstr(buf, XSNTMEX);
    if (pt) {
        pt += strlen(XSNTMEX);
        i = 0;
        while (*pt != '\n' && *pt != '\0' && i < PATH_MAX - 1)
            tfproto->xsntmex[i++] = *pt++;
    }
    pt = strstr(buf, XSACE);
    if (pt) {
        pt += strlen(XSACE);
        i = 0;
        while (*pt != '\n' && *pt != '\0' && i < PATH_MAX - 1)
            tfproto->xsace[i++] = *pt++;
    }
    pt = strstr(buf, INJAIL);
    if (pt)
        tfproto->injail = 1;
    pt = strstr(buf, RUNBASH);
    if (pt)
        tfproto->runbash = 1;
    pt = strstr(buf, FLYCONTEXT);
    if (pt)
        tfproto->flycontext = 1;
    pt = strstr(buf, LOCKSYS);
    if (pt)
        tfproto->locksys = 1;
    pt = strstr(buf, TLB);
    i = 0;
    if (pt) {
        pt += strlen(TLB);
        while (*pt != '\n' && *pt != '\0' && i < PATH_MAX - 1)
            tfproto->tlb[i++] = *pt++;
    }
    pt = strstr(buf, NPROCMAX);
    if (pt) {
        i = 0;
        pt += strlen(NPROCMAX);
        while (*pt != '\n' && *pt != '\0' && i < LINE_MAX - 1)
            ln[i++] = *pt++;
        ln[i] = '\0';
        tfproto->nprocmax = atoll(ln);
    }
    pt = strstr(buf, PUBKEY);
    if (pt) {
        i = 0;
        pt += strlen(PUBKEY);
        while (*pt != '\0' && *pt != PUBKEY_ENDTOK)
            tfproto->pub[i++] = *pt++;
    }
    i = 0;
    if (pt = strstr(buf, TRP_DNSTOK)) {
        tfproto->trp_tp = TRP_DNS;
        pt += strlen(TRP_DNSTOK);
    } else if (pt = strstr(buf, TRP_IPV4TOK)) {
        tfproto->trp_tp = TRP_IPV4;
        pt += strlen(TRP_IPV4TOK);
    } else if (pt = strstr(buf, TRP_IPV6TOK)) {
        tfproto->trp_tp = TRP_IPV6;
        pt += strlen(TRP_IPV6TOK);
    } else
        tfproto->trp_tp = TRP_NONE;
    if (pt) 
        while (*pt != '\n' && *pt != '\0' && i < TRPLEN - 1)
            tfproto->trp[i++] = *pt++;
    freeres(fs, buf);
    stddbpath();
    setmaxchilds(tfproto->nprocmax);
    return 0;
}

static void freeres(FILE *fs, char *buf)
{
    if (buf)
        free(buf);
    if (fs)
        fclose(fs);
}

static void stddbpath(void)
{
    char path[PATH_MAX];
    realpath(tfproto.dbdir, path);
    strcpy(tfproto.dbdir, path);
    strcat(tfproto.dbdir, "/");
}
