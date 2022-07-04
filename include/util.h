/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <mempool.h>
#include <time.h>

/* Buffer for unix time in timestamp format. */
#define UXTIMELEN 22
/* Buffer for time in human readble format. */
#define HTIMELEN 32
/* Number of decimal digits for long long type, including the sign and the
    traling 0. */
#define LLDIGITS sizeof(long long) / 2 * 3 + sizeof(long long) + 2
/* Number of decimal digits for int type, including the sign and the traling 0. 
    */
#define INTDIGITS sizeof(int) / 2 * 3 + sizeof(int) + 2
/* Swap byte order of 32 bit integer: Big-to-Little or Little-to-Big endian. */
#define swapbo32(value) value = (value >> 24 & 0xFF) | (value >> 16 & 0xFF) << \
    8 | (value >> 8 & 0xFF) << 16 | (value & 0xFF) << 24;
/* Swap byte order of 64 bit integer: Big-to-Little or Little-to-Big endian. */
#define swapbo64(value) value = (value >> 56 & 0xFF) | (value >> 48 & 0xFF) << \
    8 | (value >> 40 & 0xFF) << 16 | (value >> 32 & 0xFF) << 24 | (value >> 24 \
    & 0xFF) << 32 | (value >> 16 & 0xFF) << 40 | (value >> 8 & 0xFF) << 48 | \
    (value & 0xFF) << 56
/* MS -millisecond- multiplication factor. */
#define MILLISEC 1000
/* Nanosecond multiplication factor. */
#define NANOSEC 1000000000
/* Number of needed hexadecimal digits to store one byte. */
#define HEXDIG_LEN 2
/* hexadecimal string prefix. */
#define HEXPREFIX "0x"
/* Multiplication factor for disk block-size. */
#define BLKSZFAC 16
/* SHA256 formated hex string padded with 0. */ 
#define SHA256STR_LEN 65
/* Max ipv4 length. */
#define IPV4LEN 16
/* Max ipv6 length. */
#define IPV6LEN 30
/* Marco for IPv4. */
#define IPTYPE_V4 0
/* Marco for IPv6. */
#define IPTYPE_V6 1
/* DNS resolution timeout. */
#define DNSTMOUT 5
/* LSR_ITER non-recursive mode. */
#define LSRITER_NONR 0
/* LSR_ITER recursive mode. */
#define LSRITER_R 1
/* Short length string. */
#define SHORTSTR 255

/* FApi operation codes. */
enum fapiopc { FAPI_READ, FAPI_WRITE, FAPI_OVERWRITE};

/* Create lock options. */
enum crtlock { BLK, NONBLK, UNLOCK };

/* Temp file pointer type to obscure strcture. */
typedef struct tmp *Tmp;
/* File Api pointer type to obscure structure */
typedef struct fapi *Fapi;
/* lsr_iter callback prototype. */
typedef void (*lsr_iter_callback)(const char *filename, int isdir);

/* Jail actions in tf daemon working directory. Return -1 for failed 
    status and 0 for success. */
int jaildir(const char *src, char *dst);
/* Make temporary file at daemon working directory. */
Tmp mktmp(const char *cat);
/* Delete and release resources binded to temp file. */
void freetmp(Tmp tmp);
/* Write line to temp file. \n not included. */
void writetmp(Tmp tmp, const char *str);
/* Return pointer to internal strcture path location. Dont modify this
    pointer. */
const char *tmppath(Tmp tmp);
/* Check path exist and is accessible. It returns 0 for OK. */
int chkpath(const char *path);
/* Initialize File Api. */
Fapi fapinit(const char *path, void *buf, enum fapiopc code);
/* Free File Api data structure. */
void fapifree(Fapi api);
/* Read data chunk and return actually readed bytes o 0 for EOF. */
int fapiread(Fapi api, int offset, int len);
/* Return pointer to internal string with last error of File Api. */
const char *fapierr(void);
/* Write data chunk and return actually written bytes. */
int fapiwrite(Fapi api, int offset, int len);
/* Get partition free space in bytes at specified path. */
unsigned long long freespace(const char *path);
/* List directory and store result in dl memory pool. dl parameter must be
    initialized before call listdir. */
void listdir(const char *path, struct mempool *dl, int recur);
/* Copy source directory recursively at destination. */
int cpr(const char *src, const char *dst);
/* Remove directory recursively. Any directory that matches -excl- will be 
    excluded from deleting. */
int rmr(const char *path, const char *excl);
/* Try to adcquire the lock. Return -1 if already locked. */
int trylck(const char *path, const char *file, int rmtrail);
/* Normalize path removing any extra slash. */
void normpath(const char *path, char *rst);
/* Get current time in unix format. */
void gettm(time_t *t, char *time);
/* Get current time in human readble format: yyyy-mm-dd HH:MM:SS UTC. Return 
    0 to success and -1 for failed status . */
int gettmf(time_t *t, char *time);
/* Convert the command to upper case. */
void cmdtoupper(char *cmd);
/* Convert date in human readble format: yyyy-mm-dd HH:MM:SS UTC timestamp 
    unix string. */
void strtotm(char *date);
/* Copy string from from to to as much as sz bytes. */
void strcpy_sec(char *to, const char *from, int sz);
/* Store in an internal array the value of v as string. This is not a 
    thread-safe function. */
char *itostr(long long v);
/* Check whether a memory model is Big-endian. */
int isbigendian(void);
/* Lock a file, and creates it if not exist. */
int crtlock(int fd, enum crtlock lck);
/* Write chunk of data to file (fd). */
int64_t writechunk(int fd, char *buf, int64_t len);
/* Read chunk of data from file (fd). */
int64_t readchunk(int fd, char *buf, int64_t len);
/* Returns non-zero if a secure directory is involved in the path. */
int issecdir(const char *path);
/* Converts an array of -sz- bytes from 'bytes' to hex and store it in 'hex'." 
    The size of buffer 'hex' should be at least 'sz' * HEXDIG_LEN + 1. */
void bytetohex(const unsigned char *bytes, int64_t sz, char *hex);
/* Generates random data. */
void randent(char *ran, int len);
/* Read data from descriptor and discard it. */
void rdnull(int fd, int64_t sz);
/* Make sha256 to a file specified by path. */
int sha256sum(const char *path, char *sha256_str);
/* DNS ip4/6 resolver. */
int resolvhn(const char *host, char *ip, int v6, int timeout);
/* Remove traling dir from path. */
void rmtrdir(char *str);
/* List directory recursively or not and return it in a callback.*/
int lsr_iter(const char *path, int rec, lsr_iter_callback callback);
/* Copy file from source to destination. */
int cpfile(const char *src, const char *dst);
/* Securely wait for process exit status. */
int sec_waitpid(int pid);

#endif
