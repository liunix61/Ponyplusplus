/*
 * Pony++ Crypto - mbedTLS Backend
 *
 * 适用于: MCU (ESP32/STM32), 平台SDK自带mbedTLS
 * 编译条件: PONY_CRYPTO_MBEDTLS (由CMake或MCU构建系统定义)
 */

#ifdef PONY_CRYPTO_MBEDTLS

#include "ponypp/crypto.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/md.h"
#include "mbedtls/aes.h"
#include "mbedtls/gcm.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include <string.h>
#include <stdlib.h>

/* ==================== SHA-256 ==================== */

int pny_sha256(const void *data, size_t len, uint8_t out[PNY_SHA256_DIGEST_LEN]) {
    if (!out) return PNY_CRYPTO_BAD_ARG;
    /* mbedtls 3.x: mbedtls_sha256() 一次性接口 */
    if (mbedtls_sha256(data, len, out, 0) != 0) return PNY_CRYPTO_ERR;
    return PNY_CRYPTO_OK;
}

struct PnySHA256Ctx {
    mbedtls_sha256_context sha;
};

PnySHA256Ctx *pny_sha256_new(void) {
    PnySHA256Ctx *ctx = (PnySHA256Ctx *)calloc(1, sizeof(PnySHA256Ctx));
    if (!ctx) return NULL;
    mbedtls_sha256_init(&ctx->sha);
    if (mbedtls_sha256_starts(&ctx->sha, 0) != 0) {
        mbedtls_sha256_free(&ctx->sha);
        free(ctx);
        return NULL;
    }
    return ctx;
}

void pny_sha256_free(PnySHA256Ctx *ctx) {
    if (!ctx) return;
    mbedtls_sha256_free(&ctx->sha);
    free(ctx);
}

int pny_sha256_update(PnySHA256Ctx *ctx, const void *data, size_t len) {
    if (!ctx) return PNY_CRYPTO_BAD_ARG;
    if (mbedtls_sha256_update(&ctx->sha, data, len) != 0) return PNY_CRYPTO_ERR;
    return PNY_CRYPTO_OK;
}

int pny_sha256_final(PnySHA256Ctx *ctx, uint8_t out[PNY_SHA256_DIGEST_LEN]) {
    if (!ctx || !out) return PNY_CRYPTO_BAD_ARG;
    if (mbedtls_sha256_finish(&ctx->sha, out) != 0) return PNY_CRYPTO_ERR;
    return PNY_CRYPTO_OK;
}

/* ==================== SHA-512 ==================== */

int pny_sha512(const void *data, size_t len, uint8_t out[PNY_SHA512_DIGEST_LEN]) {
    if (!out) return PNY_CRYPTO_BAD_ARG;
    if (mbedtls_sha512(data, len, out, 0) != 0) return PNY_CRYPTO_ERR;
    return PNY_CRYPTO_OK;
}

/* ==================== HMAC-SHA256 ==================== */

int pny_hmac_sha256(const void *key, size_t key_len,
                    const void *data, size_t data_len,
                    uint8_t out[PNY_SHA256_DIGEST_LEN]) {
    if (!key || !out) return PNY_CRYPTO_BAD_ARG;
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!info) return PNY_CRYPTO_ERR;
    if (mbedtls_md_hmac(info, key, key_len, data, data_len, out) != 0)
        return PNY_CRYPTO_ERR;
    return PNY_CRYPTO_OK;
}

/* ==================== AES-CBC ==================== */

int pny_aes_cbc_encrypt(const uint8_t *key, size_t key_len,
                        uint8_t iv[PNY_AES_BLOCK_LEN],
                        const uint8_t *in, size_t in_len,
                        uint8_t *out, size_t *out_len) {
    if (!key || !iv || !in || !out || !out_len) return PNY_CRYPTO_BAD_ARG;
    if (in_len % PNY_AES_BLOCK_LEN != 0) return PNY_CRYPTO_BAD_ARG;

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    int ret = PNY_CRYPTO_OK;

    if (mbedtls_aes_setkey_enc(&aes, key, (unsigned int)(key_len * 8)) != 0) {
        ret = PNY_CRYPTO_ERR; goto done;
    }
    if (mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, in_len, iv, in, out) != 0) {
        ret = PNY_CRYPTO_ERR; goto done;
    }
    *out_len = in_len;

done:
    mbedtls_aes_free(&aes);
    return ret;
}

int pny_aes_cbc_decrypt(const uint8_t *key, size_t key_len,
                        uint8_t iv[PNY_AES_BLOCK_LEN],
                        const uint8_t *in, size_t in_len,
                        uint8_t *out, size_t *out_len) {
    if (!key || !iv || !in || !out || !out_len) return PNY_CRYPTO_BAD_ARG;
    if (in_len % PNY_AES_BLOCK_LEN != 0) return PNY_CRYPTO_BAD_ARG;

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    int ret = PNY_CRYPTO_OK;

    if (mbedtls_aes_setkey_dec(&aes, key, (unsigned int)(key_len * 8)) != 0) {
        ret = PNY_CRYPTO_ERR; goto done;
    }
    if (mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, in_len, iv, in, out) != 0) {
        ret = PNY_CRYPTO_ERR; goto done;
    }
    *out_len = in_len;

done:
    mbedtls_aes_free(&aes);
    return ret;
}

/* ==================== AES-GCM ==================== */

int pny_aes_gcm_encrypt(const uint8_t *key, size_t key_len,
                        const uint8_t *iv, size_t iv_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t *in, size_t in_len,
                        uint8_t *out, size_t *out_len,
                        uint8_t tag[16]) {
    if (!key || !iv || !in || !out || !out_len || !tag) return PNY_CRYPTO_BAD_ARG;

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    int ret = PNY_CRYPTO_OK;

    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, (unsigned int)(key_len * 8)) != 0) {
        ret = PNY_CRYPTO_ERR; goto done;
    }
    if (mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, in_len,
                                  iv, iv_len, aad, aad_len, in, out, 16, tag) != 0) {
        ret = PNY_CRYPTO_ERR; goto done;
    }
    *out_len = in_len;

done:
    mbedtls_gcm_free(&gcm);
    return ret;
}

int pny_aes_gcm_decrypt(const uint8_t *key, size_t key_len,
                        const uint8_t *iv, size_t iv_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t *in, size_t in_len,
                        const uint8_t tag[16],
                        uint8_t *out, size_t *out_len) {
    if (!key || !iv || !in || !out || !out_len || !tag) return PNY_CRYPTO_BAD_ARG;

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    int ret = PNY_CRYPTO_OK;

    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, (unsigned int)(key_len * 8)) != 0) {
        ret = PNY_CRYPTO_ERR; goto done;
    }
    if (mbedtls_gcm_auth_decrypt(&gcm, in_len, iv, iv_len, aad, aad_len,
                                 tag, 16, in, out) != 0) {
        ret = PNY_CRYPTO_ERR; goto done;
    }
    *out_len = in_len;

done:
    mbedtls_gcm_free(&gcm);
    return ret;
}

/* ==================== 随机数 ==================== */

int pny_random_bytes(uint8_t *buf, size_t len) {
    if (!buf || len == 0) return PNY_CRYPTO_BAD_ARG;

    /* MCU: 用平台熵源 + CTR-DRBG */
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    int ret = PNY_CRYPTO_OK;
    if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                              (const unsigned char *)"ponypp", 6) != 0) {
        ret = PNY_CRYPTO_ERR; goto done;
    }
    if (mbedtls_ctr_drbg_random(&ctr_drbg, buf, len) != 0) {
        ret = PNY_CRYPTO_ERR; goto done;
    }

done:
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return ret;
}

/* ==================== 工具 ==================== */

size_t pny_pkcs7_pad_len(size_t data_len, size_t block_len) {
    if (block_len == 0) return 0;
    return block_len - (data_len % block_len);
}

#endif /* PONY_CRYPTO_MBEDTLS */
