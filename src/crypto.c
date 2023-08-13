/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <crypto.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

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
int enrankey(struct crypto *crypt, char *pubrsa) 
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
}

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

#else

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
    if ( !pevpkey)
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
