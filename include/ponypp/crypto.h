/*
 * Pony++ Crypto Module
 *
 * 双后端: OpenSSL (Linux/桌面) + mbedTLS (MCU: STM32/ESP32)
 * 通过 CMake 自动检测, 通过 PONY_CRYPTO_OPENSSL / PONY_CRYPTO_MBEDTLS 宏切换
 *
 * 设计原则:
 * - MCU 零依赖回退: 无加密库时返回错误, 不链接任何加密代码
 * - 统一 API: 应用代码不感知后端差异
 * - 最小 API 面: 哈希/对称加密/HMAC/随机数, 覆盖嵌入式安全需求
 */

#ifndef PONYPP_CRYPTO_H
#define PONYPP_CRYPTO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 错误码 */
#define PNY_CRYPTO_OK        0
#define PNY_CRYPTO_ERR      -1   /* 通用错误 */
#define PNY_CRYPTO_NO_LIB   -2   /* 无加密库可用 */
#define PNY_CRYPTO_BAD_ARG  -3   /* 参数错误 */
#define PNY_CRYPTO_BUF_SMALL -4  /* 输出缓冲区不足 */
#define PNY_CRYPTO_VERIFY_FAIL -5 /* 签名验证失败 */

/* SHA-256: 32字节摘要 */
#define PNY_SHA256_DIGEST_LEN 32
#define PNY_SHA256_BLOCK_LEN  64

/* SHA-512: 64字节摘要 */
#define PNY_SHA512_DIGEST_LEN 64

/* AES: 16字节块, 16/24/32字节密钥 */
#define PNY_AES_BLOCK_LEN 16

/* ==================== 哈希 ==================== */

/* 一次性 SHA-256 */
int pny_sha256(const void *data, size_t len, uint8_t out[PNY_SHA256_DIGEST_LEN]);

/* 增量 SHA-256 (流式) */
typedef struct PnySHA256Ctx PnySHA256Ctx;
PnySHA256Ctx *pny_sha256_new(void);
void pny_sha256_free(PnySHA256Ctx *ctx);
int pny_sha256_update(PnySHA256Ctx *ctx, const void *data, size_t len);
int pny_sha256_final(PnySHA256Ctx *ctx, uint8_t out[PNY_SHA256_DIGEST_LEN]);

/* 一次性 SHA-512 */
int pny_sha512(const void *data, size_t len, uint8_t out[PNY_SHA512_DIGEST_LEN]);

/* ==================== HMAC ==================== */

/* HMAC-SHA256 */
int pny_hmac_sha256(const void *key, size_t key_len,
                    const void *data, size_t data_len,
                    uint8_t out[PNY_SHA256_DIGEST_LEN]);

/* ==================== AES 对称加密 ==================== */

/* AES-CBC: 加密 (iv 16字节, 会被修改) */
int pny_aes_cbc_encrypt(const uint8_t *key, size_t key_len,
                        uint8_t iv[PNY_AES_BLOCK_LEN],
                        const uint8_t *in, size_t in_len,
                        uint8_t *out, size_t *out_len);

/* AES-CBC: 解密 */
int pny_aes_cbc_decrypt(const uint8_t *key, size_t key_len,
                        uint8_t iv[PNY_AES_BLOCK_LEN],
                        const uint8_t *in, size_t in_len,
                        uint8_t *out, size_t *out_len);

/* AES-GCM: 认证加密 (AEAD) */
int pny_aes_gcm_encrypt(const uint8_t *key, size_t key_len,
                        const uint8_t *iv, size_t iv_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t *in, size_t in_len,
                        uint8_t *out, size_t *out_len,
                        uint8_t tag[16]);

int pny_aes_gcm_decrypt(const uint8_t *key, size_t key_len,
                        const uint8_t *iv, size_t iv_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t *in, size_t in_len,
                        const uint8_t tag[16],
                        uint8_t *out, size_t *out_len);

/* ==================== 随机数 ==================== */

/* 密码学安全随机数 */
int pny_random_bytes(uint8_t *buf, size_t len);

/* ==================== 工具 ==================== */


/* ==================== Ed25519 签名 ==================== */

#define PNY_ED25519_KEY_LEN    32
#define PNY_ED25519_SIG_LEN    64

/* 生成 Ed25519 密钥对 */
int pny_ed25519_keygen(uint8_t pub[PNY_ED25519_KEY_LEN], 
                        uint8_t priv[PNY_ED25519_KEY_LEN]);

/* Ed25519 签名 */
int pny_ed25519_sign(const uint8_t priv[PNY_ED25519_KEY_LEN],
                      const void *msg, size_t msg_len,
                      uint8_t sig[PNY_ED25519_SIG_LEN]);

/* Ed25519 验证 */
int pny_ed25519_verify(const uint8_t pub[PNY_ED25519_KEY_LEN],
                        const void *msg, size_t msg_len,
                        const uint8_t sig[PNY_ED25519_SIG_LEN]);

/* 包签名: 签名包内容 */
int pny_pkg_sign(const uint8_t priv[PNY_ED25519_KEY_LEN],
                  const void *pkg_data, size_t pkg_len,
                  uint8_t sig[PNY_ED25519_SIG_LEN]);

/* 包验证: 验证包签名 */
int pny_pkg_verify(const uint8_t pub[PNY_ED25519_KEY_LEN],
                    const void *pkg_data, size_t pkg_len,
                    const uint8_t sig[PNY_ED25519_SIG_LEN]);

/* PKCS#7 填充 (AES-CBC 需要) */
size_t pny_pkcs7_pad_len(size_t data_len, size_t block_len);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_CRYPTO_H */
