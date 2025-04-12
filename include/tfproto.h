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
/* Secure FileSystem metadata file. */
#define FSMETA ".securefs_metadata"
/* Secure FileSystem metadata file extension for reading. */
#define FSMETARD_EXT ".rd"
/* Secure FileSystem metadata swap file extension. */
#define FSMETASWP_EXT ".swp"
/* Secure FileSystem permision token separator. */
#define FSPERM_TOK ':'
/* Secure FileSystem metadata lock file extension. */
#define FSMETALCK_EXT ".lck"
/* Secure FileSystem -others- identity. */
#define FSOTHERUSR ""

/* Secure FileSystem permission bits. */
enum SECFS { 
    SECFS_SETPERM = 1, SECFS_REMPERM = 2, SECFS_RFILE = 4, SECFS_WFILE = 8,
    SECFS_LDIR = 16, SECFS_RMDIR = 32, SECFS_MKDIR = 64, SECFS_DFILE = 128,
    SECFS_STAT = 256, SECFS_FDUPD = 512, SECFS_UXPERM = 1024, 
    SECFS_LRDIR = 2048
};

/* Shared memory structure. */
struct shm_obj {
    volatile uint32_t oobcount;
};

/* Secure FileSystem current operation. */
extern unsigned int volatile fsop;
extern volatile int fsop_ovrr;

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
    int faiuse;
} extern comm; 

/* Cryptography structures containing crypt and decrypt functions. */
extern struct crypto cryp_rx;
extern struct crypto cryp_tx;
extern struct crypto cryp_org;
/* Cryptography structures for aes cipher block crypt and decrypt. */
extern struct blkcipher cipher_rx;
extern struct blkcipher cipher_tx;
/* Secure FileSystem identity token. */
extern char fsid[LINE_MAX];
/* Define if block cipher should be used. */
extern int blkstatus;

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
/* Secure FileSystem security operations. */
int secfs_proc(const char *src);
/* Get identity permission. */
unsigned int getfsidperm(const char *path, const char *id);
/* Enable Block Cipher layer. */
int setblkon(void);
/* Disable Block Cipher layer. */
void setblkoff(void);
/* Actually starts AES cipher. */
void startblk(void);
/* Set client identity impersonation avoidance state */
void setcid(int st);

#endif
