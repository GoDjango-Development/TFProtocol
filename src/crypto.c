/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <crypto.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <malloc.h>

#define MOD_VALUE 256

/* Encrypt/Decrypt function. */
static void encrypt(struct crypto *cryp, char *data, int len);

void initcrypto(struct crypto *cryp)
{
    cryp->encrypt = encrypt;
    cryp->rndkey = malloc(KEYMAX);
}

static void encrypt(struct crypto *cryp, char *data, int len)
{
    if (cryp->st == CRYPT_ON) {
        int c = 0, keyc = 0;
        for (; c < len; c++, keyc++) {
            if (keyc == cryp->rndlen)
                keyc = 0;
            if (cryp->pack == CRYPT_UNPACK) {
                *(data + c) = *(data + c) - (cryp->seed >> 56 & 0xFF);
                *(data + c) ^= cryp->rndkey[keyc];
            } else if (cryp->pack == CRYPT_PACK) {
                *(data + c) ^= cryp->rndkey[keyc];
                *(data + c) = (cryp->seed >> 56 & 0xFF) + *(data + c);
            }
            cryp->seed = cryp->seed * (cryp->seed >> 8 & 0xFFFFFFFF) + 
                (cryp->seed >> 40 & 0xFFFF);
            if (cryp->seed == 0)
                cryp->seed = *(int64_t *) cryp->rndkey;
            cryp->rndkey[keyc] = cryp->seed % MOD_VALUE;
        }
    }
}

/* This function it's not used in this version */
/* int enrankey(struct crypto *crypt, char *pubrsa) 
{    
    BIO *keybio = NULL;
    keybio = BIO_new_mem_buf(pubrsa, -1);
    RSA *rsa = NULL;
    rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
    int pad = RSA_PKCS1_OAEP_PADDING;
    int enbyt = RSA_public_encrypt(sizeof crypt->rndkey, crypt->rndkey, 
        crypt->enkey, rsa, pad);
    RSA_free(rsa);
    BIO_free(keybio);
    return enbyt;
} */

#if OPENSSL_VERSION_NUMBER < 0x10100000L

int derankey(struct crypto *crypt, char *privrsa)
{
    RSA *rsa = NULL;
    BIO *keybio = BIO_new_mem_buf(privrsa, -1);
    if (!keybio)
        return -1;
    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
    if (!rsa)
        return -1;
    int pad = RSA_PKCS1_OAEP_PADDING;
    int enbyt = RSA_private_decrypt(sizeof crypt->enkey, crypt->enkey, 
        crypt->rndkey, rsa, pad);
    if (enbyt == -1)
        return -1;
    RSA_free(rsa);
    BIO_free(keybio);
    crypt->rndlen = enbyt;
    if (enbyt >= KEYMIN)
        crypt->seed = *(int64_t *) crypt->rndkey;
    else
        return -1;
    return enbyt;
}

#elif OPENSSL_VERSION_NUMBER < 0x30000000L

int derankey(struct crypto *crypt, char *privrsa)
{
    EVP_PKEY* pevpkey = NULL;
    EVP_PKEY_CTX *ctx = NULL;
    RSA *rsa = NULL;
    size_t enbyt;
    BIO *keybio = BIO_new_mem_buf(privrsa, -1);
    if (!keybio)
        return -1;
    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
    if (!rsa)
        return -1;
    pevpkey = EVP_PKEY_new();
    if (!pevpkey)
        return -1;
    if (!EVP_PKEY_assign_RSA(pevpkey, rsa))
        return -1;
    ctx = EVP_PKEY_CTX_new(pevpkey, NULL);
    if (!ctx)
        return -1;
    if (EVP_PKEY_decrypt_init(ctx) <= 0)
        return -1;
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
        return -1;
    if (EVP_PKEY_decrypt(ctx, NULL, &enbyt, crypt->enkey, 
        sizeof crypt->enkey) <= 0)
        return -1;
    if (EVP_PKEY_decrypt(ctx, crypt->rndkey, &enbyt, crypt->enkey,
        sizeof crypt->enkey) <= 0)
        return -1;
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pevpkey);
    BIO_free(keybio);
    crypt->rndlen = enbyt;
    if (enbyt >= KEYMIN)
        crypt->seed = *(int64_t *) crypt->rndkey;
    else
        return -1;
    return enbyt;
}

#else

#include <openssl/decoder.h>

int derankey(struct crypto *crypt, char *privrsa)
{
    EVP_PKEY* pevpkey = NULL;
    EVP_PKEY_CTX *ctx = NULL;
    size_t enbyt;
    BIO *keybio = BIO_new_mem_buf(privrsa, -1);
    if (!keybio)
        return -1;
    pevpkey = EVP_PKEY_new();
    if (!pevpkey)
        return -1;
    OSSL_DECODER_CTX *ossl_ctx= OSSL_DECODER_CTX_new_for_pkey(&pevpkey,
	NULL, NULL, NULL, 0, NULL, NULL);
    if (!ossl_ctx)
        return -1;
    if (OSSL_DECODER_from_bio(ossl_ctx, keybio) != 1)
        return -1;
    ctx = EVP_PKEY_CTX_new(pevpkey, NULL);
    if (!ctx)
        return -1;
    if (EVP_PKEY_decrypt_init(ctx) <= 0)
        return -1;
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
        return -1;
    if (EVP_PKEY_decrypt(ctx, NULL, &enbyt, crypt->enkey, 
        sizeof crypt->enkey) <= 0)
        return -1;
    if (EVP_PKEY_decrypt(ctx, crypt->rndkey, &enbyt, crypt->enkey,
        sizeof crypt->enkey) <= 0)
        return -1;
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pevpkey);
    BIO_free(keybio);
    crypt->rndlen = enbyt;
    if (enbyt >= KEYMIN)
        crypt->seed = *(int64_t *) crypt->rndkey;
    else
        return -1;
    return enbyt;
}

#endif

void swapkey(struct crypto *crypt, char *newkey, int keylen)
{
    free(crypt->rndkey);
    crypt->rndkey = newkey;
    crypt->rndlen = keylen;
}

int dup_crypt(struct crypto *to, struct crypto *from)
{
    memcpy(to, from, sizeof(struct crypto));
    to->rndkey = malloc(KEYMAX);
    if (!to->rndkey)
        return -1;
    memcpy(to->rndkey, from->rndkey, from->rndlen);
    return 0;
}

int blkinit_en(struct blkcipher *cipher)
{
    memset(cipher, 0, sizeof(struct blkcipher));
    if (!(cipher->ctx = EVP_CIPHER_CTX_new()))
        return -1;
    if (EVP_EncryptInit_ex(cipher->ctx, EVP_aes_256_cbc(), NULL, cipher->key,
        cipher->iv) != 1)
        return -1;
    return 0;
}

int blkinit_de(struct blkcipher *cipher)
{
    memset(cipher, 0, sizeof(struct blkcipher));
    if (!(cipher->ctx = EVP_CIPHER_CTX_new()))
        return -1;
    if (EVP_DecryptInit_ex(cipher->ctx, EVP_aes_256_cbc(), NULL, cipher->key,
        cipher->iv) != 1)
        return -1;
    return 0;
}

void blkfin(struct blkcipher *cipher)
{
    EVP_CIPHER_CTX_free(cipher->ctx);
}

int blkencrypt(struct blkcipher *cipher, void *cidata, void *pldata, int pllen)
{
    int cilen = 0;
    if (EVP_EncryptInit_ex(cipher->ctx, NULL, NULL, NULL, NULL) != 1)
        return -1;
    if (EVP_EncryptUpdate(cipher->ctx, cidata, &cilen, pldata, pllen) != 1)
        return -1;
    return cilen;
}

int blkdecrypt(struct blkcipher *cipher, void *pldata, void *cidata, int cilen)
{
    int pllen = 0;
    if (EVP_DecryptInit_ex(cipher->ctx, NULL, NULL, NULL, NULL) != 1)
        return -1;
    if (EVP_DecryptUpdate(cipher->ctx, pldata, &pllen, cidata, cilen) != 1)
        return -1;
    return pllen;
}

int blkend_de(struct blkcipher *cipher, void *pldata, int pllen)
{
    int exlen = 0;
    if (EVP_DecryptFinal_ex(cipher->ctx, pldata + pllen, &exlen) != 1)
        return -1;
    return pllen + exlen;
}

int blkend_en(struct blkcipher *cipher, void *cidata, int cilen)
{
    int exlen = 0;
    if (EVP_EncryptFinal_ex(cipher->ctx, cidata + cilen, &exlen) != 1)
        return -1;
    return cilen + exlen;
}

char *base64en(void *in, int len)
{
    const int explen = (len + 2) / 3 * 4;
    char *out = malloc(explen + 1);
    if (!out)
        return NULL;
    const int outlen = EVP_EncodeBlock(out, in, len);
    if (explen != outlen) {
        free(out);
        return NULL;
    }
    return out;
}

char *genkey(int len)
{
    int i = 0;
    char *key = malloc(len);
    if (!key)
        return NULL;
    srand(time(0));
    for (; i < len; i++)
        *(key + i) = random();
    return key;
}

void *base64dec(char *in, int len)
{
    const int explen = len * 3 / 4;
    char *out = malloc(explen);
    if (!out)
        return NULL;
    const int outlen = EVP_DecodeBlock(out, in, len);
    if (explen != outlen) {
        free(out);
        return NULL;
    }
    return out;
}
