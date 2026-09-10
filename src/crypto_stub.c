/*
 * Pony++ Crypto - Stub Backend
 *
 * 适用于: 无加密库的最小MCU目标
 * 所有加密操作返回 PNY_CRYPTO_NO_LIB
 * 编译条件: 无 PONY_CRYPTO_OPENSSL 且无 PONY_CRYPTO_MBEDTLS
 */

#if !defined(PONY_CRYPTO_OPENSSL) && !defined(PONY_CRYPTO_MBEDTLS)

#include "ponypp/crypto.h"
#include <string.h>
#include <stdlib.h>

int pny_sha256(const void *data, size_t len, uint8_t out[PNY_SHA256_DIGEST_LEN]) {
    (void)data; (void)len; (void)out;
    return PNY_CRYPTO_NO_LIB;
}

struct PnySHA256Ctx { int dummy; };

PnySHA256Ctx *pny_sha256_new(void) { return NULL; }
void pny_sha256_free(PnySHA256Ctx *ctx) { (void)ctx; }
int pny_sha256_update(PnySHA256Ctx *ctx, const void *data, size_t len) {
    (void)ctx; (void)data; (void)len;
    return PNY_CRYPTO_NO_LIB;
}
int pny_sha256_final(PnySHA256Ctx *ctx, uint8_t out[PNY_SHA256_DIGEST_LEN]) {
    (void)ctx; (void)out;
    return PNY_CRYPTO_NO_LIB;
}

int pny_sha512(const void *data, size_t len, uint8_t out[PNY_SHA512_DIGEST_LEN]) {
    (void)data; (void)len; (void)out;
    return PNY_CRYPTO_NO_LIB;
}

int pny_hmac_sha256(const void *key, size_t key_len,
                    const void *data, size_t data_len,
                    uint8_t out[PNY_SHA256_DIGEST_LEN]) {
    (void)key; (void)key_len; (void)data; (void)data_len; (void)out;
    return PNY_CRYPTO_NO_LIB;
}

int pny_aes_cbc_encrypt(const uint8_t *key, size_t key_len,
                        uint8_t iv[PNY_AES_BLOCK_LEN],
                        const uint8_t *in, size_t in_len,
                        uint8_t *out, size_t *out_len) {
    (void)key; (void)key_len; (void)iv; (void)in; (void)in_len; (void)out; (void)out_len;
    return PNY_CRYPTO_NO_LIB;
}

int pny_aes_cbc_decrypt(const uint8_t *key, size_t key_len,
                        uint8_t iv[PNY_AES_BLOCK_LEN],
                        const uint8_t *in, size_t in_len,
                        uint8_t *out, size_t *out_len) {
    (void)key; (void)key_len; (void)iv; (void)in; (void)in_len; (void)out; (void)out_len;
    return PNY_CRYPTO_NO_LIB;
}

int pny_aes_gcm_encrypt(const uint8_t *key, size_t key_len,
                        const uint8_t *iv, size_t iv_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t *in, size_t in_len,
                        uint8_t *out, size_t *out_len,
                        uint8_t tag[16]) {
    (void)key; (void)key_len; (void)iv; (void)iv_len; (void)aad; (void)aad_len;
    (void)in; (void)in_len; (void)out; (void)out_len; (void)tag;
    return PNY_CRYPTO_NO_LIB;
}

int pny_aes_gcm_decrypt(const uint8_t *key, size_t key_len,
                        const uint8_t *iv, size_t iv_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t *in, size_t in_len,
                        const uint8_t tag[16],
                        uint8_t *out, size_t *out_len) {
    (void)key; (void)key_len; (void)iv; (void)iv_len; (void)aad; (void)aad_len;
    (void)in; (void)in_len; (void)tag; (void)out; (void)out_len;
    return PNY_CRYPTO_NO_LIB;
}

int pny_random_bytes(uint8_t *buf, size_t len) {
    (void)buf; (void)len;
    return PNY_CRYPTO_NO_LIB;
}

size_t pny_pkcs7_pad_len(size_t data_len, size_t block_len) {
    if (block_len == 0) return 0;
    return block_len - (data_len % block_len);
}

#endif /* stub */
