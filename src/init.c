/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <init.h>
#include <string.h>
#include <sys/stat.h>

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

struct tfproto tfproto;
static char *buf;

/* Free used resources like descriptors and allocated dinamically memory */
static void freeres(FILE *fs, char *buf);
/* Set database directory normalized. */
static void stddbpath(void);

int init(const char *conf)
{
    char ln[LINE_MAX];
    strcpy(tfproto.conf, conf);
    FILE *fs = fopen(conf, "r");
    if (fs == NULL)
        return -1;
    struct stat st;
    stat(conf, &st);
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
        tfproto.proto[i++] = *pt++;
    if (!strlen(tfproto.proto)) {
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
        tfproto.hash[i++] = *pt++;
    if (!strlen(tfproto.hash)) {
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
        tfproto.dbdir[i++] = *pt++;
    if (!strlen(tfproto.dbdir)) {
        freeres(fs, buf);
        return -1;
    }
    tfproto.dbdir[strlen(tfproto.dbdir)] = '/';
    pt = strstr(buf, PORT);
    if (!pt) {
        freeres(fs, buf);
        return -1;
    }
    pt += strlen(PORT);
    i = 0;
    while (*pt != '\n' && *pt != '\0' && i < PORTLEN - 1)
        tfproto.port[i++] = *pt++;
    if (!strlen(tfproto.port)) {
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
        tfproto.priv[i++] = *pt++;
    pt = strstr(buf, DEFUSR);
    if (pt) {
        pt += strlen(DEFUSR);
        i = 0;
        while (*pt != '\n' && *pt != '\0' && i < DEFUSRLEN - 1)
            tfproto.defusr[i++] = *pt++;
    }
    pt = strstr(buf, USERDB);
    if (pt) {
        pt += strlen(USERDB);
        i = 0;
        while (*pt != '\n' && *pt != '\0' && i < PATH_MAX - 1)
            tfproto.userdb[i++] = *pt++;
    }
    pt = strstr(buf, XSNTMEX);
    if (pt) {
        pt += strlen(XSNTMEX);
        i = 0;
        while (*pt != '\n' && *pt != '\0' && i < PATH_MAX - 1)
            tfproto.xsntmex[i++] = *pt++;
    }
    pt = strstr(buf, XSACE);
    if (pt) {
        pt += strlen(XSACE);
        i = 0;
        while (*pt != '\n' && *pt != '\0' && i < PATH_MAX - 1)
            tfproto.xsace[i++] = *pt++;
    }
    pt = strstr(buf, INJAIL);
    if (pt)
        tfproto.injail = 1;
    pt = strstr(buf, TLB);
    i = 0;
    if (pt) {
        pt += strlen(TLB);
        while (*pt != '\n' && *pt != '\0' && i < PATH_MAX - 1)
            tfproto.tlb[i++] = *pt++;
    }
    pt = strstr(buf, PUBKEY);
    if (pt) {
        i = 0;
        pt += strlen(PUBKEY);
        while (*pt != '\0' && *pt != PUBKEY_ENDTOK)
            tfproto.pub[i++] = *pt++;
    }
    i = 0;
    if (pt = strstr(buf, TRP_DNSTOK)) {
        tfproto.trp_tp = TRP_DNS;
        pt += strlen(TRP_DNSTOK);
    } else if (pt = strstr(buf, TRP_IPV4TOK)) {
        tfproto.trp_tp = TRP_IPV4;
        pt += strlen(TRP_IPV4TOK);
    } else if (pt = strstr(buf, TRP_IPV6TOK)) {
        tfproto.trp_tp = TRP_IPV6;
        pt += strlen(TRP_IPV6TOK);
    } else
        tfproto.trp_tp = TRP_NONE;
    if (pt) 
        while (*pt != '\n' && *pt != '\0' && i < TRPLEN - 1)
            tfproto.trp[i++] = *pt++;
    freeres(fs, buf);
    stddbpath();
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
