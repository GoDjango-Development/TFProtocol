/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef XS_IME_H
#define XS_IME_H

#include <stdint.h>
#include <pthread.h>
#include <signal.h>
#include <limits.h>
#include <xs_ime/xs_imerr.h>

/* Main buffer length for store the flow. */
#define BUFLEN 16 * 1024
/* Max user name length. */
#define USRNAME_LEN 128
/* Success operation code status. */
#define OPC_SUCCESS 0
/* Failed operation code status. */
#define OPC_FAILED -1
/* Operation code for exit IME module. */
#define OPC_EXIT 0x0

/* Flags of steps for communication. */
enum steps { ORUN = 1, USR = 2, SETUP = 4 };

/* Flags of settings. */
enum stflgs { UNUSR = 1, AUTODEL = 2 };

/* Packet header, without padding. */
#pragma pack(push, 1)
struct xsime_hdr {
    char opcode;
    int32_t sz;
};
#pragma pack(pop)

/* Struct for opcode sender mechanism. */
struct opcsnd {
    struct xsime_hdr hdr;
    int send;
    pthread_mutex_t mut;
    struct timespec tm;
} extern opcsnd;

/* Nanosleep structure variable. */
extern struct timespec *tm;
/* Read header of XSIME packet. */
extern struct xsime_hdr rdhdr;
/* Write header of XSIME packet. */
extern struct xsime_hdr wrhdr;
/* Input buffer. */
extern char inbuf[BUFLEN];
/* Output buffer. */
extern char outbuf[BUFLEN];
/* Flag to signal when the protocol its ready to send and receive messages. */
extern volatile sig_atomic_t ready;
/* Path for the organizing unit. */
extern char orgunit[PATH_MAX];
/* Path for the user directory. */
extern char usrpath[PATH_MAX];
/* Path for the user's messages directory. */
extern char usrmsg[PATH_MAX];
/* Path for the user's blacklist directory. */
extern char usrblk[PATH_MAX];
/* User name. */
extern char usrnam[USRNAME_LEN];
/* Unique user instance. */
extern char unusr;
/* Auto delete messages after sending. */
extern char autodel;
/* Starting timestamp of resquested messages. */
extern unsigned long long tmstmp;

/* Start the XS_IME protocol dynamics. */
void xsime_start();
/* Get in current endianess xsime_hdr sz member. */
int32_t getbufsz(struct xsime_hdr *hdr);
/* Set xsime_hdr sz member in Big-endian. */
void setbufsz(int32_t sz, struct xsime_hdr *hdr);
/* Send Operation code to the client. */
void sendopc(int opc, int sz);
/* Check whether the communication it's ready. */
int chkrdy(void);

#endif
