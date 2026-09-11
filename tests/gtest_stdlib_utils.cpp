#include <gtest/gtest.h>
#include <ponypp/stdlib.h>
#include <cstring>
#include <cstdlib>

/* ==================== UUID ==================== */

TEST(StdlibUUID, V4Generate) {
    char uuid[37];
    ASSERT_EQ(pny_uuid_v4(uuid, sizeof(uuid)), 0);
    
    /* UUID v4 格式: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx */
    EXPECT_EQ(strlen(uuid), 36);
    EXPECT_EQ(uuid[8], '-');
    EXPECT_EQ(uuid[13], '-');
    EXPECT_EQ(uuid[18], '-');
    EXPECT_EQ(uuid[23], '-');
    EXPECT_EQ(uuid[14], '4');  /* version 4 */
}

TEST(StdlibUUID, V4Unique) {
    char uuid1[37], uuid2[37];
    ASSERT_EQ(pny_uuid_v4(uuid1, sizeof(uuid1)), 0);
    ASSERT_EQ(pny_uuid_v4(uuid2, sizeof(uuid2)), 0);
    /* 两次生成不应相同 (概率极低) */
    EXPECT_STRNE(uuid1, uuid2);
}

TEST(StdlibUUID, V4BadArgs) {
    char uuid[37];
    EXPECT_EQ(pny_uuid_v4(nullptr, 37), -1);
    EXPECT_EQ(pny_uuid_v4(uuid, 10), -1);  /* 缓冲区太小 */
}

TEST(StdlibUUID, ShortGenerate) {
    char uuid[17];
    ASSERT_EQ(pny_uuid_short(uuid, sizeof(uuid)), 0);
    EXPECT_EQ(strlen(uuid), 16);
}

TEST(StdlibUUID, ShortBadArgs) {
    char uuid[17];
    EXPECT_EQ(pny_uuid_short(nullptr, 17), -1);
    EXPECT_EQ(pny_uuid_short(uuid, 5), -1);
}

/* ==================== Base64 ==================== */

TEST(StdlibBase64, EncodeDecode) {
    const char *data = "Hello, Pony++!";
    char encoded[64];
    char decoded[64];
    
    int enc_len = pny_base64_encode(data, strlen(data), encoded, sizeof(encoded));
    ASSERT_GT(enc_len, 0);
    
    int dec_len = pny_base64_decode(encoded, enc_len, decoded, sizeof(decoded));
    ASSERT_EQ(dec_len, (int)strlen(data));
    EXPECT_EQ(memcmp(decoded, data, dec_len), 0);
}

TEST(StdlibBase64, EncodeEmpty) {
    char encoded[16];
    int len = pny_base64_encode("", 0, encoded, sizeof(encoded));
    EXPECT_EQ(len, 0);
}

TEST(StdlibBase64, EncodeBadArgs) {
    char buf[64];
    EXPECT_EQ(pny_base64_encode(nullptr, 10, buf, sizeof(buf)), -1);
    EXPECT_EQ(pny_base64_encode("data", 4, nullptr, 64), -1);
    EXPECT_EQ(pny_base64_decode(nullptr, 10, buf, sizeof(buf)), -1);
    EXPECT_EQ(pny_base64_decode("ZGF0YQ==", 8, nullptr, 64), -1);
}

TEST(StdlibBase64, EncodeKnownValue) {
    /* "Man" -> "TWFu" */
    char encoded[16];
    int len = pny_base64_encode("Man", 3, encoded, sizeof(encoded));
    ASSERT_EQ(len, 4);
    EXPECT_EQ(memcmp(encoded, "TWFu", 4), 0);
}

/* ==================== Hex ==================== */

TEST(StdlibHex, EncodeDecode) {
    const uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    char encoded[32];
    uint8_t decoded[16];
    
    int enc_len = pny_hex_encode(data, 4, encoded, sizeof(encoded));
    ASSERT_EQ(enc_len, 8);
    EXPECT_EQ(memcmp(encoded, "deadbeef", 8), 0);
    
    int dec_len = pny_hex_decode(encoded, enc_len, decoded, sizeof(decoded));
    ASSERT_EQ(dec_len, 4);
    EXPECT_EQ(memcmp(decoded, data, 4), 0);
}

TEST(StdlibHex, EncodeEmpty) {
    char encoded[16];
    uint8_t data[1] = {0};
    int len = pny_hex_encode(data, 0, encoded, sizeof(encoded));
    EXPECT_EQ(len, 0);
}

TEST(StdlibHex, EncodeBadArgs) {
    char buf[32];
    uint8_t data[] = {0xAB};
    EXPECT_EQ(pny_hex_encode(data, 1, nullptr, 32), -1);
    EXPECT_EQ(pny_hex_decode(nullptr, 2, buf, sizeof(buf)), -1);
    EXPECT_EQ(pny_hex_decode("ab", 2, nullptr, 16), -1);
}

TEST(StdlibHex, DecodeInvalidHex) {
    uint8_t buf[16];
    /* 非 hex 字符 */
    int len = pny_hex_decode("zz", 2, buf, sizeof(buf));
    EXPECT_LE(len, 0);
}

/* ==================== 综合 ==================== */

TEST(StdlibMisc, Base64RoundTrip) {
    /* 测试各种长度 */
    for (int len = 0; len <= 32; len++) {
        char data[32];
        for (int i = 0; i < len; i++) data[i] = (char)('A' + (i % 26));
        
        char encoded[64];
        int enc_len = pny_base64_encode(data, len, encoded, sizeof(encoded));
        ASSERT_GE(enc_len, 0);
        
        char decoded[64];
        int dec_len = pny_base64_decode(encoded, enc_len, decoded, sizeof(decoded));
        ASSERT_EQ(dec_len, len);
        if (len > 0) {
            EXPECT_EQ(memcmp(decoded, data, len), 0);
        }
    }
}

TEST(StdlibMisc, HexRoundTrip) {
    /* 测试各种长度 */
    for (int len = 0; len <= 16; len++) {
        uint8_t data[16];
        for (int i = 0; i < len; i++) data[i] = (uint8_t)(i * 17);
        
        char encoded[64];
        int enc_len = pny_hex_encode(data, len, encoded, sizeof(encoded));
        ASSERT_EQ(enc_len, len * 2);
        
        uint8_t decoded[16];
        int dec_len = pny_hex_decode(encoded, enc_len, decoded, sizeof(decoded));
        ASSERT_EQ(dec_len, len);
        if (len > 0) {
            EXPECT_EQ(memcmp(decoded, data, len), 0);
        }
    }
}
