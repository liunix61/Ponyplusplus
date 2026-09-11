/*
 * Pony++ Crypto Module Tests
 *
 * SHA-256/512 已知向量验证 + AES-CBC/GCM 往返 + HMAC + 随机数
 */

#include <gtest/gtest.h>
#include <ponypp/crypto.h>
#include <cstring>
#include <vector>

/* ==================== SHA-256 ==================== */

TEST(Crypto, Sha256Empty) {
    /* SHA-256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 */
    uint8_t out[PNY_SHA256_DIGEST_LEN];
    ASSERT_EQ(pny_sha256("", 0, out), PNY_CRYPTO_OK);
    const uint8_t expected[32] = {
        0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,
        0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55
    };
    EXPECT_EQ(memcmp(out, expected, 32), 0);
}

TEST(Crypto, Sha256Abc) {
    /* SHA-256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad */
    uint8_t out[PNY_SHA256_DIGEST_LEN];
    ASSERT_EQ(pny_sha256("abc", 3, out), PNY_CRYPTO_OK);
    const uint8_t expected[32] = {
        0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
        0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad
    };
    EXPECT_EQ(memcmp(out, expected, 32), 0);
}

TEST(Crypto, Sha256Streaming) {
    /* 增量: SHA256("a"+"b"+"c") == SHA256("abc") */
    PnySHA256Ctx *ctx = pny_sha256_new();
    ASSERT_NE(ctx, nullptr);
    pny_sha256_update(ctx, "a", 1);
    pny_sha256_update(ctx, "b", 1);
    pny_sha256_update(ctx, "c", 1);
    uint8_t out[PNY_SHA256_DIGEST_LEN];
    ASSERT_EQ(pny_sha256_final(ctx, out), PNY_CRYPTO_OK);
    const uint8_t expected[32] = {
        0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
        0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad
    };
    EXPECT_EQ(memcmp(out, expected, 32), 0);
    pny_sha256_free(ctx);
}

TEST(Crypto, Sha512Abc) {
    /* SHA-512("abc") 前16字节: ddaf35a193617aba... */
    uint8_t out[PNY_SHA512_DIGEST_LEN];
    ASSERT_EQ(pny_sha512("abc", 3, out), PNY_CRYPTO_OK);
    const uint8_t prefix[16] = {
        0xdd,0xaf,0x35,0xa1,0x93,0x61,0x7a,0xba,0xcc,0x41,0x73,0x49,0xae,0x20,0x41,0x31
    };
    EXPECT_EQ(memcmp(out, prefix, 16), 0);
}

/* ==================== HMAC-SHA256 ==================== */

TEST(Crypto, HmacSha256Rfc4231) {
    /* RFC 4231 Test Case 1: key=0x0b*20, data="Hi There" */
    uint8_t key[20]; memset(key, 0x0b, 20);
    uint8_t out[PNY_SHA256_DIGEST_LEN];
    ASSERT_EQ(pny_hmac_sha256(key, 20, "Hi There", 8, out), PNY_CRYPTO_OK);
    const uint8_t expected[32] = {
        0xb0,0x34,0x4c,0x61,0xd8,0xdb,0x38,0x53,0x5c,0xa8,0xaf,0xce,0xaf,0x0b,0xf1,0x2b,
        0x88,0x1d,0xc2,0x00,0xc9,0x83,0x3d,0xa7,0x26,0xe9,0x37,0x6c,0x2e,0x32,0xcf,0xf7
    };
    EXPECT_EQ(memcmp(out, expected, 32), 0);
}

/* ==================== AES-CBC ==================== */

TEST(Crypto, AesCbcRoundtrip128) {
    uint8_t key[16]; memset(key, 0x42, 16);
    uint8_t iv_enc[16]; memset(iv_enc, 0x11, 16);
    uint8_t iv_dec[16]; memset(iv_dec, 0x11, 16);

    /* 32字节明文 (PKCS7对齐) */
    const char *plain = "Hello Pony++ Crypto Module!!";  /* 28字节 */
    size_t pad = pny_pkcs7_pad_len(28, 16);  /* 4 */
    EXPECT_EQ(pad, 4u);
    size_t padded_len = 28 + pad;  /* 32 */

    uint8_t padded[32];
    memcpy(padded, plain, 28);
    memset(padded + 28, (int)pad, pad);

    uint8_t cipher[64], recovered[64];
    size_t cipher_len = 0, rec_len = 0;

    ASSERT_EQ(pny_aes_cbc_encrypt(key, 16, iv_enc, padded, padded_len, cipher, &cipher_len), PNY_CRYPTO_OK);
    /* OpenSSL EVP自动padding: 32字节对齐输入 → 追加一整块padding → 48字节 */
    EXPECT_EQ(cipher_len, 48u);

    ASSERT_EQ(pny_aes_cbc_decrypt(key, 16, iv_dec, cipher, cipher_len, recovered, &rec_len), PNY_CRYPTO_OK);
    EXPECT_EQ(rec_len, 32u);  /* 解密后去掉padding恢复32字节 */
    EXPECT_EQ(memcmp(recovered, padded, 32), 0);
}

TEST(Crypto, AesCbcRoundtrip256) {
    uint8_t key[32]; memset(key, 0x99, 32);
    uint8_t iv_enc[16]; memset(iv_enc, 0x55, 16);
    uint8_t iv_dec[16]; memset(iv_dec, 0x55, 16);

    uint8_t data[16]; memset(data, 0xAB, 16);
    uint8_t cipher[32], recovered[32];
    size_t cipher_len = 0, rec_len = 0;

    ASSERT_EQ(pny_aes_cbc_encrypt(key, 32, iv_enc, data, 16, cipher, &cipher_len), PNY_CRYPTO_OK);
    ASSERT_EQ(pny_aes_cbc_decrypt(key, 32, iv_dec, cipher, cipher_len, recovered, &rec_len), PNY_CRYPTO_OK);
    EXPECT_EQ(memcmp(recovered, data, 16), 0);
}

/* ==================== AES-GCM ==================== */

TEST(Crypto, AesGcmRoundtrip) {
    uint8_t key[32]; memset(key, 0x77, 32);
    uint8_t iv[12]; memset(iv, 0x33, 12);
    uint8_t aad[8] = {1,2,3,4,5,6,7,8};

    const char *msg = "Secret message for GCM";
    size_t msg_len = strlen(msg);

    uint8_t cipher[64], recovered[64], tag[16], tag2[16];
    size_t cipher_len = 0, rec_len = 0;

    ASSERT_EQ(pny_aes_gcm_encrypt(key, 32, iv, 12, aad, 8,
                (const uint8_t *)msg, msg_len, cipher, &cipher_len, tag), PNY_CRYPTO_OK);
    EXPECT_EQ(cipher_len, msg_len);

    ASSERT_EQ(pny_aes_gcm_decrypt(key, 32, iv, 12, aad, 8,
                cipher, cipher_len, tag, recovered, &rec_len), PNY_CRYPTO_OK);
    EXPECT_EQ(rec_len, msg_len);
    EXPECT_EQ(memcmp(recovered, msg, msg_len), 0);
}

TEST(Crypto, AesGcmTamperDetected) {
    /* 篡改密文 -> 解密失败 (认证) */
    uint8_t key[32]; memset(key, 0x77, 32);
    uint8_t iv[12]; memset(iv, 0x33, 12);

    const char *msg = "AuthenticData!!";
    uint8_t cipher[32], tag[16], recovered[32];
    size_t cipher_len = 0, rec_len = 0;

    ASSERT_EQ(pny_aes_gcm_encrypt(key, 32, iv, 12, NULL, 0,
                (const uint8_t *)msg, 15, cipher, &cipher_len, tag), PNY_CRYPTO_OK);

    /* 篡改第0字节 */
    cipher[0] ^= 0x01;

    EXPECT_EQ(pny_aes_gcm_decrypt(key, 32, iv, 12, NULL, 0,
                cipher, cipher_len, tag, recovered, &rec_len), PNY_CRYPTO_ERR);
}

/* ==================== 随机数 ==================== */

TEST(Crypto, RandomBytes) {
    uint8_t buf1[32], buf2[32];
    ASSERT_EQ(pny_random_bytes(buf1, 32), PNY_CRYPTO_OK);
    ASSERT_EQ(pny_random_bytes(buf2, 32), PNY_CRYPTO_OK);
    /* 两次随机不应相同 (概率极低) */
    EXPECT_NE(memcmp(buf1, buf2, 32), 0);
}

TEST(Crypto, RandomBytesBadArgs) {
    uint8_t buf[16];
    EXPECT_EQ(pny_random_bytes(nullptr, 16), PNY_CRYPTO_BAD_ARG);
    EXPECT_EQ(pny_random_bytes(buf, 0), PNY_CRYPTO_BAD_ARG);
}

/* ==================== NULL 安全 ==================== */

TEST(Crypto, NullSafety) {
    uint8_t out[64];
    EXPECT_EQ(pny_sha256(nullptr, 0, out), PNY_CRYPTO_OK);  /* NULL+0len 可接受 */
    EXPECT_EQ(pny_sha256("x", 1, nullptr), PNY_CRYPTO_BAD_ARG);
    EXPECT_EQ(pny_hmac_sha256(nullptr, 0, "x", 1, out), PNY_CRYPTO_BAD_ARG);
    EXPECT_EQ(pny_aes_cbc_encrypt(nullptr, 16, out, out, 16, out, nullptr), PNY_CRYPTO_BAD_ARG);
}

/* ==================== Ed25519 签名 ==================== */

TEST(Crypto, Ed25519Keygen) {
    uint8_t pub[32], priv[32];
    ASSERT_EQ(pny_ed25519_keygen(pub, priv), PNY_CRYPTO_OK);
    /* 公钥和私钥不应相同 */
    EXPECT_NE(memcmp(pub, priv, 32), 0);
}

TEST(Crypto, Ed25519KeygenBadArgs) {
    uint8_t pub[32], priv[32];
    EXPECT_EQ(pny_ed25519_keygen(nullptr, priv), PNY_CRYPTO_BAD_ARG);
    EXPECT_EQ(pny_ed25519_keygen(pub, nullptr), PNY_CRYPTO_BAD_ARG);
}

TEST(Crypto, Ed25519SignVerify) {
    uint8_t pub[32], priv[32], sig[64];
    const char *msg = "Hello Pony++ Ed25519!";
    
    ASSERT_EQ(pny_ed25519_keygen(pub, priv), PNY_CRYPTO_OK);
    ASSERT_EQ(pny_ed25519_sign(priv, msg, strlen(msg), sig), PNY_CRYPTO_OK);
    EXPECT_EQ(pny_ed25519_verify(pub, msg, strlen(msg), sig), PNY_CRYPTO_OK);
}

TEST(Crypto, Ed25519SignVerifyBadArgs) {
    uint8_t pub[32], priv[32], sig[64];
    const char *msg = "test";
    
    EXPECT_EQ(pny_ed25519_sign(nullptr, msg, 4, sig), PNY_CRYPTO_BAD_ARG);
    EXPECT_EQ(pny_ed25519_sign(priv, nullptr, 4, sig), PNY_CRYPTO_BAD_ARG);
    EXPECT_EQ(pny_ed25519_sign(priv, msg, 4, nullptr), PNY_CRYPTO_BAD_ARG);
    EXPECT_EQ(pny_ed25519_verify(nullptr, msg, 4, sig), PNY_CRYPTO_BAD_ARG);
    EXPECT_EQ(pny_ed25519_verify(pub, nullptr, 4, sig), PNY_CRYPTO_BAD_ARG);
    EXPECT_EQ(pny_ed25519_verify(pub, msg, 4, nullptr), PNY_CRYPTO_BAD_ARG);
}

TEST(Crypto, Ed25519VerifyWrongMessage) {
    uint8_t pub[32], priv[32], sig[64];
    const char *msg = "original message";
    
    ASSERT_EQ(pny_ed25519_keygen(pub, priv), PNY_CRYPTO_OK);
    ASSERT_EQ(pny_ed25519_sign(priv, msg, strlen(msg), sig), PNY_CRYPTO_OK);
    
    /* 用错误消息验证应失败 */
    EXPECT_EQ(pny_ed25519_verify(pub, "wrong", 5, sig), PNY_CRYPTO_VERIFY_FAIL);
}

TEST(Crypto, Ed25519VerifyWrongKey) {
    uint8_t pub1[32], priv1[32], pub2[32], priv2[32], sig[64];
    const char *msg = "test message";
    
    ASSERT_EQ(pny_ed25519_keygen(pub1, priv1), PNY_CRYPTO_OK);
    ASSERT_EQ(pny_ed25519_keygen(pub2, priv2), PNY_CRYPTO_OK);
    ASSERT_EQ(pny_ed25519_sign(priv1, msg, strlen(msg), sig), PNY_CRYPTO_OK);
    
    /* 用错误公钥验证应失败 */
    EXPECT_EQ(pny_ed25519_verify(pub2, msg, strlen(msg), sig), PNY_CRYPTO_VERIFY_FAIL);
}

TEST(Crypto, Ed25519SignVerifyEmptyMessage) {
    uint8_t pub[32], priv[32], sig[64];
    
    ASSERT_EQ(pny_ed25519_keygen(pub, priv), PNY_CRYPTO_OK);
    ASSERT_EQ(pny_ed25519_sign(priv, "", 0, sig), PNY_CRYPTO_OK);
    EXPECT_EQ(pny_ed25519_verify(pub, "", 0, sig), PNY_CRYPTO_OK);
}

/* ==================== 包签名 ==================== */

TEST(Crypto, PkgSignVerify) {
    uint8_t pub[32], priv[32], sig[64];
    const char *pkg = "package data content";
    
    ASSERT_EQ(pny_ed25519_keygen(pub, priv), PNY_CRYPTO_OK);
    ASSERT_EQ(pny_pkg_sign(priv, pkg, strlen(pkg), sig), PNY_CRYPTO_OK);
    EXPECT_EQ(pny_pkg_verify(pub, pkg, strlen(pkg), sig), PNY_CRYPTO_OK);
}

TEST(Crypto, PkgVerifyCorrupted) {
    uint8_t pub[32], priv[32], sig[64];
    const char *pkg = "package data";
    
    ASSERT_EQ(pny_ed25519_keygen(pub, priv), PNY_CRYPTO_OK);
    ASSERT_EQ(pny_pkg_sign(priv, pkg, strlen(pkg), sig), PNY_CRYPTO_OK);
    
    /* 篡改签名应失败 */
    sig[0] ^= 0xFF;
    EXPECT_EQ(pny_pkg_verify(pub, pkg, strlen(pkg), sig), PNY_CRYPTO_VERIFY_FAIL);
}
