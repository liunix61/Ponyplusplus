/*
 * Pony++ Crypto - OpenSSL Backend
 *
 * 适用于: Linux (RPi5/RK3588), macOS, 桌面
 * 编译条件: PONY_CRYPTO_OPENSSL (由CMake自动检测)
 */

#ifdef PONY_CRYPTO_OPENSSL

#include "ponypp/crypto.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <string.h>
#include <stdlib.h>

/* ==================== SHA-256 ==================== */

int pny_sha256(const void *data, size_t len, uint8_t out[PNY_SHA256_DIGEST_LEN]) {
    if (!out) return PNY_CRYPTO_BAD_ARG;
    unsigned int out_len = 0;
    if (!EVP_Digest(data, len, out, &out_len, EVP_sha256(), NULL))
        return PNY_CRYPTO_ERR;
    return PNY_CRYPTO_OK;
}

struct PnySHA256Ctx {
    EVP_MD_CTX *md_ctx;
};

PnySHA256Ctx *pny_sha256_new(void) {
    PnySHA256Ctx *ctx = (PnySHA256Ctx *)calloc(1, sizeof(PnySHA256Ctx));
    if (!ctx) return NULL;
    ctx->md_ctx = EVP_MD_CTX_new();
    if (!ctx->md_ctx) { free(ctx); return NULL; }
    if (!EVP_DigestInit_ex(ctx->md_ctx, EVP_sha256(), NULL)) {
        EVP_MD_CTX_free(ctx->md_ctx); free(ctx); return NULL;
    }
    return ctx;
}

void pny_sha256_free(PnySHA256Ctx *ctx) {
    if (!ctx) return;
    if (ctx->md_ctx) EVP_MD_CTX_free(ctx->md_ctx);
    free(ctx);
}

int pny_sha256_update(PnySHA256Ctx *ctx, const void *data, size_t len) {
    if (!ctx || !ctx->md_ctx) return PNY_CRYPTO_BAD_ARG;
    if (!EVP_DigestUpdate(ctx->md_ctx, data, len)) return PNY_CRYPTO_ERR;
    return PNY_CRYPTO_OK;
}

int pny_sha256_final(PnySHA256Ctx *ctx, uint8_t out[PNY_SHA256_DIGEST_LEN]) {
    if (!ctx || !ctx->md_ctx || !out) return PNY_CRYPTO_BAD_ARG;
    unsigned int out_len = 0;
    if (!EVP_DigestFinal_ex(ctx->md_ctx, out, &out_len)) return PNY_CRYPTO_ERR;
    return PNY_CRYPTO_OK;
}

/* ==================== SHA-512 ==================== */

int pny_sha512(const void *data, size_t len, uint8_t out[PNY_SHA512_DIGEST_LEN]) {
    if (!out) return PNY_CRYPTO_BAD_ARG;
    unsigned int out_len = 0;
    if (!EVP_Digest(data, len, out, &out_len, EVP_sha512(), NULL))
        return PNY_CRYPTO_ERR;
    return PNY_CRYPTO_OK;
}

/* ==================== HMAC-SHA256 ==================== */

int pny_hmac_sha256(const void *key, size_t key_len,
                    const void *data, size_t data_len,
                    uint8_t out[PNY_SHA256_DIGEST_LEN]) {
    if (!key || !out) return PNY_CRYPTO_BAD_ARG;
    unsigned int out_len = 0;
    if (!HMAC(EVP_sha256(), key, (int)key_len, data, data_len, out, &out_len))
        return PNY_CRYPTO_ERR;
    return PNY_CRYPTO_OK;
}

/* ==================== AES-CBC ==================== */

static const EVP_CIPHER *aes_cbc_cipher(size_t key_len) {
    switch (key_len) {
        case 16: return EVP_aes_128_cbc();
        case 24: return EVP_aes_192_cbc();
        case 32: return EVP_aes_256_cbc();
        default: return NULL;
    }
}

int pny_aes_cbc_encrypt(const uint8_t *key, size_t key_len,
                        uint8_t iv[PNY_AES_BLOCK_LEN],
                        const uint8_t *in, size_t in_len,
                        uint8_t *out, size_t *out_len) {
    const EVP_CIPHER *cipher = aes_cbc_cipher(key_len);
    if (!cipher || !key || !iv || !in || !out || !out_len) return PNY_CRYPTO_BAD_ARG;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return PNY_CRYPTO_ERR;

    int len = 0, total = 0;
    int ret = PNY_CRYPTO_OK;

    if (!EVP_EncryptInit_ex(ctx, cipher, NULL, key, iv)) { ret = PNY_CRYPTO_ERR; goto done; }
    if (!EVP_EncryptUpdate(ctx, out, &len, in, (int)in_len)) { ret = PNY_CRYPTO_ERR; goto done; }
    total = len;
    if (!EVP_EncryptFinal_ex(ctx, out + total, &len)) { ret = PNY_CRYPTO_ERR; goto done; }
    total += len;
    *out_len = (size_t)total;

done:
    EVP_CIPHER_CTX_free(ctx);
    return ret;
}

int pny_aes_cbc_decrypt(const uint8_t *key, size_t key_len,
                        uint8_t iv[PNY_AES_BLOCK_LEN],
                        const uint8_t *in, size_t in_len,
                        uint8_t *out, size_t *out_len) {
    const EVP_CIPHER *cipher = aes_cbc_cipher(key_len);
    if (!cipher || !key || !iv || !in || !out || !out_len) return PNY_CRYPTO_BAD_ARG;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return PNY_CRYPTO_ERR;

    int len = 0, total = 0;
    int ret = PNY_CRYPTO_OK;

    if (!EVP_DecryptInit_ex(ctx, cipher, NULL, key, iv)) { ret = PNY_CRYPTO_ERR; goto done; }
    if (!EVP_DecryptUpdate(ctx, out, &len, in, (int)in_len)) { ret = PNY_CRYPTO_ERR; goto done; }
    total = len;
    if (!EVP_DecryptFinal_ex(ctx, out + total, &len)) { ret = PNY_CRYPTO_ERR; goto done; }
    total += len;
    *out_len = (size_t)total;

done:
    EVP_CIPHER_CTX_free(ctx);
    return ret;
}

/* ==================== AES-GCM ==================== */

static const EVP_CIPHER *aes_gcm_cipher(size_t key_len) {
    switch (key_len) {
        case 16: return EVP_aes_128_gcm();
        case 24: return EVP_aes_192_gcm();
        case 32: return EVP_aes_256_gcm();
        default: return NULL;
    }
}

int pny_aes_gcm_encrypt(const uint8_t *key, size_t key_len,
                        const uint8_t *iv, size_t iv_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t *in, size_t in_len,
                        uint8_t *out, size_t *out_len,
                        uint8_t tag[16]) {
    const EVP_CIPHER *cipher = aes_gcm_cipher(key_len);
    if (!cipher || !key || !iv || !in || !out || !out_len || !tag) return PNY_CRYPTO_BAD_ARG;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return PNY_CRYPTO_ERR;

    int len = 0, total = 0;
    int ret = PNY_CRYPTO_OK;

    if (!EVP_EncryptInit_ex(ctx, cipher, NULL, NULL, NULL)) { ret = PNY_CRYPTO_ERR; goto done; }
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)iv_len, NULL)) { ret = PNY_CRYPTO_ERR; goto done; }
    if (!EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) { ret = PNY_CRYPTO_ERR; goto done; }
    if (aad && aad_len > 0) {
        if (!EVP_EncryptUpdate(ctx, NULL, &len, aad, (int)aad_len)) { ret = PNY_CRYPTO_ERR; goto done; }
    }
    if (!EVP_EncryptUpdate(ctx, out, &len, in, (int)in_len)) { ret = PNY_CRYPTO_ERR; goto done; }
    total = len;
    if (!EVP_EncryptFinal_ex(ctx, out + total, &len)) { ret = PNY_CRYPTO_ERR; goto done; }
    total += len;
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag)) { ret = PNY_CRYPTO_ERR; goto done; }
    *out_len = (size_t)total;

done:
    EVP_CIPHER_CTX_free(ctx);
    return ret;
}

int pny_aes_gcm_decrypt(const uint8_t *key, size_t key_len,
                        const uint8_t *iv, size_t iv_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t *in, size_t in_len,
                        const uint8_t tag[16],
                        uint8_t *out, size_t *out_len) {
    const EVP_CIPHER *cipher = aes_gcm_cipher(key_len);
    if (!cipher || !key || !iv || !in || !out || !out_len || !tag) return PNY_CRYPTO_BAD_ARG;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return PNY_CRYPTO_ERR;

    int len = 0, total = 0;
    int ret = PNY_CRYPTO_OK;

    if (!EVP_DecryptInit_ex(ctx, cipher, NULL, NULL, NULL)) { ret = PNY_CRYPTO_ERR; goto done; }
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)iv_len, NULL)) { ret = PNY_CRYPTO_ERR; goto done; }
    if (!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) { ret = PNY_CRYPTO_ERR; goto done; }
    if (aad && aad_len > 0) {
        if (!EVP_DecryptUpdate(ctx, NULL, &len, aad, (int)aad_len)) { ret = PNY_CRYPTO_ERR; goto done; }
    }
    if (!EVP_DecryptUpdate(ctx, out, &len, in, (int)in_len)) { ret = PNY_CRYPTO_ERR; goto done; }
    total = len;
    /* 设置tag再验证 */
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, (void *)tag)) { ret = PNY_CRYPTO_ERR; goto done; }
    if (!EVP_DecryptFinal_ex(ctx, out + total, &len)) { ret = PNY_CRYPTO_ERR; goto done; }
    total += len;
    *out_len = (size_t)total;

done:
    EVP_CIPHER_CTX_free(ctx);
    return ret;
}

/* ==================== 随机数 ==================== */

int pny_random_bytes(uint8_t *buf, size_t len) {
    if (!buf || len == 0) return PNY_CRYPTO_BAD_ARG;
    if (RAND_bytes(buf, (int)len) != 1) return PNY_CRYPTO_ERR;
    return PNY_CRYPTO_OK;
}

/* ==================== 工具 ==================== */

size_t pny_pkcs7_pad_len(size_t data_len, size_t block_len) {
    if (block_len == 0) return 0;
    return block_len - (data_len % block_len);
}

#endif /* PONY_CRYPTO_OPENSSL */
