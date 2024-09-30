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
/* Block cihper key length. */
#define BLK_SIZE 16
/* Max and Min session key lengths for FAITOK interface. */
#define FAIMAX_KEYLEN KEYMAX
#define FAIMIN_KEYLEN 128

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

/* Structure to store block encryption key and iv. 
    It uses symmetric encryption. */
struct blkcipher {
    /* Encryption key. */
    unsigned char key[BLK_KEYSZ];
    /* Initialization vector. */
    unsigned char iv[BLK_IVSZ];
    /* Encrypted/Decrypted data. */
    unsigned char data[BLK_SIZE];
    /* Encrypted/Decrypted data tmp. */
    unsigned char tmpbuf[BLK_SIZE];
    /* Cipher Context */
    EVP_CIPHER_CTX *ctx;
};

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
int blkinit_en(struct blkcipher *cipher);
int blkinit_de(struct blkcipher *cipher);
/* Finalize Block Cipher structure. */
void blkfin(struct blkcipher *cipher);
/* Block Cipher encryption function. */
int blkencrypt(struct blkcipher *cipher, void *cidata, void *pldata, int pllen);
int blkend_en(struct blkcipher *cipher, void *cidata, int cilen);
/* Block Cipher decryption function. */
int blkdecrypt(struct blkcipher *cipher, void *pldata, void *cidata, int cilen);
int blkend_de(struct blkcipher *cipher, void *pldata, int pllen);
/* Generate random key */
char *genkey(int len);

#endif
