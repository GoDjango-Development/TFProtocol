/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef TFPROTO_H
#define TFPROTO_H

#include <netinet/in.h>
#include <stdint.h>
#include <crypto.h>

/* Communication buffer lenght for commands */
#define COMMBUFLEN 512 * 1024
/* Deprecated. */
/* Number of decimal digits of the size of communication buffer. */
#define COMMBUFDIG 3
#undef COMMBUFDIG
/* Keepalive unique key lenght. */
#define SRVID_LEN 40
/* Daemonize flag. */
#define MKDAEMON 1

/* Shared memory structure. */
struct shm_obj {
    volatile uint32_t oobcount;
};

/* Contains needed members for communicating potentially used from severals
    modules in the program */
struct comm {
    int sock;
    int sock_prox;
    struct sockaddr_in6 addr;
    struct sockaddr addr_prox;
    int addr_proxtp;
    socklen_t addrsz;
    char buf[COMMBUFLEN]; 
    char *rxbuf_prox;
    char *txbuf_prox;
    char srvid[SRVID_LEN];
    int shmfd;
    struct shm_obj *shmobj;
    int buflen;
    volatile char act;
} extern comm; 

/* Cryptography structures containing crypt and decrypt functions. */
extern struct crypto cryp_rx;
extern struct crypto cryp_tx;
extern struct crypto cryp_org;

/* Initialize internal data and run mainloop function */
void begincomm(int sock, struct sockaddr_in6 *rmaddr, socklen_t *rmaddrsz);
/* End communication and do clean-up terminating the process */
void endcomm(void);
/* Frist sends a header indicating the message's length, then sends the actual 
    data from the buffer. This function return -1 for error. */
int64_t writebuf(char *buf, int64_t len);
/* First reads a header to get the message's length, then reads the data to the
    buffer. This function return -1 for error. */
int64_t readbuf(char *buf, int64_t len);
/* Do some clean-up before terminate process */
void cleanup(void);
/* This function is equivalent to readbuf, except it does not checks for a
    received header for message boundaires. */
int64_t readbuf_ex(char *buf, int64_t len);
/* This function is equivalent to writebuf, except it does not sends a header
    for message boundaires. */
int64_t writebuf_ex(char *buf, int64_t len);
/* This function is equivalent to readbuf_ex but indicating the descriptor. */
int64_t readbuf_exfd(int fd, char *buf, int64_t len, int enc);
/* This function is equivalent to writebuf but indicating the descriptor. */
int64_t writebuf_exfd(int fd, char *buf, int64_t len, int enc);


#endif
