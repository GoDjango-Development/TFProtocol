/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef CRYPTO_H
#define CRYPTO_H

#include <openssl/ssl.h>
#include <inttypes.h>

/* Session key max length in bytes. Must be less than the RSA key length
    modulus minus used padding. In this case is 2048 / 8 - 42 = 214 bytes. */
#define KEYMAX 2048 / 8 - 42
/* Minimum session key lenght. */
#define KEYMIN 16
/* RSA key lenght modulus. */
#define RSA_KEYLEN 2048 / 8
/* Block cihper key length. */
#define BLK_KEYSZ 32
/* Block cipher initialization vector size. */
#define BLK_IVSZ 16
/* Block cihper length. */
#define BLK_SIZE 32

/* The posible states of encrypt system. */
enum cryptst { CRYPT_OFF, CRYPT_ON };

/* Message packing mode. */
enum pack { CRYPT_PACK, CRYPT_UNPACK, CRYPT_NOPACK };

/* Structure to store encryption key and function pointer to 
    encrypt/decrypt. It uses symmetric xor encryption. */
struct crypto {
    void (*encrypt)(struct crypto *cryp, char *data, int len);
    char *rndkey;
    int rndlen;
    char enkey[RSA_KEYLEN];
    enum cryptst st;
    int64_t seed;
    enum pack pack;
};

/* Structure to store encryption key, iv, and encrypt/decrypt
    buffer. It uses symmetric aes encryption. */
struct blkcipher {
    /* Encryption key. */
    unsigned char key[BLK_KEYSZ];
    /* Initialization vector. */
    unsigned char iv[BLK_IVSZ];
};

/* Define if aes block cipher should be used. */
extern int aestatus;

/* Initialize function pointers and random key. */
void initcrypto(struct crypto *cryp);
/* Encrypt random key with a rsa public key. 2048 bit and 
    RSA_PKCS1_OAEP_PADDING padding. */
/* This function it's not used in this version */
int enrankey(struct crypto *crypt, char *pubrsa);
/* Decrypt the session key sent by the client with server's private key. */
int derankey(struct crypto *crypt, char *privrsa);
/* Swap session key. */
void swapkey(struct crypto *crypt, char *newkey, int keylen);
/* Duplicate crypt structure. */
int dup_crypt(struct crypto *to, struct crypto *from);
/* Initialize blkcipher structure. */
void initcipher(struct blkcipher *cipher);
/* Encrypt using aes block cipher. Returns -1 on error. */
int blkencrypt(struct blkcipher *cipher, void *indata, int inlen, void *outdata, 
    int outlen);
/* Decrypt using aes block cipher. Returns -1 on error. */
int blkdecrypt(struct blkcipher *cipher, void *indata, int inlen, void *outdata,
    int outlen);

#endif
