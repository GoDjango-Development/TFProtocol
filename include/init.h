/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef INIT_H
#define INIT_H

#include <limits.h>

/* Protocol version length */
#define PROTOLEN 32
/* Hash application identity lentgth. */
#define HASHLEN 256
/* Length of port number for network comm. */
#define PORTLEN 6
/* Length of server private key. */
#define PRIVLEN 2048
/* Length of server public key. */
#define PUBLEN 1024
/* Default username's length. */
#define DEFUSRLEN 256
/* Transparent Proxy none.*/
#define TRP_NONE 0
/* Transparent Proxy DNS type. */
#define TRP_DNS 1
/* Transparent Proxy IPv4 type. */
#define TRP_IPV4 2
/* Transparent Proxy IPv6 type. */
#define TRP_IPV6 3
/* Transparent Proxy length. */
#define TRPLEN 256

struct tfproto {
    /* Protocol version */
    char proto[PROTOLEN];
    /* Hash identity */
    char hash[HASHLEN];
    /* Directory for the database. */
    char dbdir[PATH_MAX];
    /* Number of port for comm. */
    char port[PORTLEN];
    /* Server RSA private key. */
    char priv[PRIVLEN];
    /* TFProtocol's default username. */
    char defusr[DEFUSRLEN];
    /* TFProtocol conf path. */
    char conf[PATH_MAX];
    /* TFProtocol path to users database. */
    char userdb[PATH_MAX];
    /* eXtended subsystem Native Module Execution (NTMEX) conf path. */
    char xsntmex[PATH_MAX];
    /* eXtended subsystem Arbitrary Code Execution (ACE) conf path. */
    char xsace[PATH_MAX];
    /* Injail falg. */
    int injail;
    /* Transfer Load Balancer file. */
    char tlb[PATH_MAX];
    /* Server RSA public key. */
    char pub[PUBLEN];
    /* Transparent Proxy type. */
    int trp_tp;
    /* Transparent Proxy. */
    char trp[TRPLEN];
} extern tfproto;

/* This function initialize the "struct tfproto" from file "*conf". */
int init(const char *conf);

#endif
