#include <gtest/gtest.h>
#include <ponypp/stdlib.h>
#include <cstring>
#include <cstdlib>
#include <cstdint>

/* 针对性覆盖 src/ponypp/stdlib.c 未覆盖行:
 * - 333-351: pny_str_join
 * - 804-808: pny_map_foreach
 * - 817-884: pny_proto_* (pb_grow/proto_new/free/len/data)
 * - 1345-1370: pny_mutex_* (new/free/lock/unlock/trylock + NULL)
 * - 1738-1765: pny_msgpack_write_int/uint 边界值
 */

/* ==================== String.join ==================== */

TEST(StdlibCov10, StrJoinBasic) {
    PnyString *a = pny_str_new("foo");
    PnyString *b = pny_str_new("bar");
    PnyString *sep = pny_str_new(",");
    const PnyString *arr[2] = { a, b };
    PnyString *r = pny_str_join(sep, arr, 2);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->len, 7); /* "foo,bar" */
    pny_str_free(r);
    pny_str_free(a);
    pny_str_free(b);
    pny_str_free(sep);
}

TEST(StdlibCov10, StrJoinEmpty) {
    EXPECT_EQ(pny_str_join(nullptr, nullptr, 0), nullptr);
}

TEST(StdlibCov10, StrJoinNoSep) {
    PnyString *a = pny_str_new("x");
    const PnyString *arr[1] = { a };
    PnyString *r = pny_str_join(nullptr, arr, 1);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->len, 1);
    pny_str_free(r);
    pny_str_free(a);
}

TEST(StdlibCov10, StrJoinNullEntry) {
    PnyString *sep = pny_str_new("-");
    const PnyString *arr[3] = { nullptr, nullptr, nullptr };
    PnyString *r = pny_str_join(sep, arr, 3);
    /* NULL 条目: total=sep 仅2个分隔符 */
    if (r) { EXPECT_EQ(r->len, 2); pny_str_free(r); }
    pny_str_free(sep);
}

/* ==================== Map.foreach ==================== */

static void count_cb(void *k, void *v, void *ctx) {
    (void)k; (void)v;
    (*(int *)ctx)++;
}

TEST(StdlibCov10, MapForeach) {
    PnyMap *m = pny_map_new(8, sizeof(int), sizeof(int));
    ASSERT_NE(m, nullptr);
    for (int i = 0; i < 5; i++) {
        int v = i * 10;
        pny_map_put(m, &i, &v);
    }
    int count = 0;
    pny_map_foreach(m, count_cb, &count);
    EXPECT_EQ(count, 5);
    pny_map_foreach(m, nullptr, nullptr);  /* NULL guard */
    pny_map_foreach(nullptr, count_cb, &count);  /* NULL guard */
    pny_map_free(m);
}

/* ==================== ProtoBuf ==================== */

TEST(StdlibCov10, ProtoLifecycle) {
    PnyProtoBuf *pb = pny_proto_new();
    ASSERT_NE(pb, nullptr);
    EXPECT_EQ(pny_proto_len(pb), 0);
    EXPECT_NE(pny_proto_data(pb), nullptr);

    /* 写大量 varint 触发 pb_grow */
    for (uint64_t i = 0; i < 200; i++) {
        EXPECT_GE(pny_proto_write_varint(pb, i), 0);
    }
    EXPECT_GT(pny_proto_len(pb), 0);
    pny_proto_free(pb);
}

TEST(StdlibCov10, ProtoNullGuards) {
    EXPECT_EQ(pny_proto_len(nullptr), 0u);
    EXPECT_EQ(pny_proto_data(nullptr), nullptr);
    pny_proto_free(nullptr);  /* no crash */
}

TEST(StdlibCov10, ProtoWriteFields) {
    PnyProtoBuf *pb = pny_proto_new();
    ASSERT_NE(pb, nullptr);
    EXPECT_GE(pny_proto_write_tag(pb, 1, 0), 0);
    EXPECT_GE(pny_proto_write_int32(pb, 2, -42), 0);
    EXPECT_GE(pny_proto_write_int64(pb, 3, -100000), 0);
    EXPECT_GE(pny_proto_write_uint32(pb, 4, 42), 0);
    EXPECT_GE(pny_proto_write_uint64(pb, 5, 1ULL << 40), 0);
    EXPECT_GE(pny_proto_write_bool(pb, 6, true), 0);
    pny_proto_free(pb);
}

/* ==================== Mutex ==================== */

TEST(StdlibCov10, MutexLifecycle) {
    PnyMutex *m = pny_mutex_new();
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(pny_mutex_lock(m), 0);
    EXPECT_EQ(pny_mutex_unlock(m), 0);
    EXPECT_EQ(pny_mutex_trylock(m), 0);
    EXPECT_EQ(pny_mutex_unlock(m), 0);
    pny_mutex_free(m);
}

TEST(StdlibCov10, MutexNullGuards) {
    EXPECT_EQ(pny_mutex_lock(nullptr), -1);
    EXPECT_EQ(pny_mutex_unlock(nullptr), -1);
    EXPECT_EQ(pny_mutex_trylock(nullptr), -1);
    pny_mutex_free(nullptr);  /* no crash */
}

/* ==================== msgpack 边界值 ==================== */

TEST(StdlibCov10, MsgpackUintBoundaries) {
    uint8_t buf[16];
    /* 0xff -> 0xcc 2字节 */
    EXPECT_EQ(pny_msgpack_write_uint(buf, sizeof(buf), 0xff), 2);
    /* 0xffff -> 0xcd 3字节 */
    EXPECT_EQ(pny_msgpack_write_uint(buf, sizeof(buf), 0xffff), 3);
    /* 0xffffffff -> 0xce 5字节 */
    EXPECT_EQ(pny_msgpack_write_uint(buf, sizeof(buf), 0xffffffffULL), 5);
    /* > 32bit -> 0xcf 9字节 */
    EXPECT_EQ(pny_msgpack_write_uint(buf, sizeof(buf), 0x100000000ULL), 9);
    /* 小值 0x7f -> 1字节正 fixint */
    EXPECT_EQ(pny_msgpack_write_uint(buf, sizeof(buf), 0x7f), 1);
    /* NULL buf */
    EXPECT_EQ(pny_msgpack_write_uint(nullptr, 0, 1), -1);
}

TEST(StdlibCov10, MsgpackIntBoundaries) {
    uint8_t buf[16];
    /* 负 fixint -1 */
    EXPECT_EQ(pny_msgpack_write_int(buf, sizeof(buf), -1), 1);
    /* -32 负 fixint 边界 */
    EXPECT_EQ(pny_msgpack_write_int(buf, sizeof(buf), -32), 1);
    /* -33 -> 0xd0 2字节 */
    EXPECT_EQ(pny_msgpack_write_int(buf, sizeof(buf), -33), 2);
    /* -128 -> 0xd0 2字节 */
    EXPECT_EQ(pny_msgpack_write_int(buf, sizeof(buf), -128), 2);
    /* -129 -> 0xd1 3字节 */
    EXPECT_EQ(pny_msgpack_write_int(buf, sizeof(buf), -129), 3);
    /* -32768 -> 0xd1 3字节 */
    EXPECT_EQ(pny_msgpack_write_int(buf, sizeof(buf), -32768), 3);
    /* -32769 -> 0xd2 5字节 */
    EXPECT_EQ(pny_msgpack_write_int(buf, sizeof(buf), -32769), 5);
    /* -2147483648 -> 0xd2 5字节 */
    EXPECT_EQ(pny_msgpack_write_int(buf, sizeof(buf), -2147483648LL), 5);
    /* 更小 -> 0xd3 9字节 */
    EXPECT_EQ(pny_msgpack_write_int(buf, sizeof(buf), -2147483649LL), 9);
    /* 正数走 uint 路径 */
    EXPECT_EQ(pny_msgpack_write_int(buf, sizeof(buf), 100), 1);
    /* NULL buf */
    EXPECT_EQ(pny_msgpack_write_int(nullptr, 0, 1), -1);
}

TEST(StdlibCov10, MsgpackBufTooSmall) {
    uint8_t buf[2] = {0};
    /* 0x1000 需要3字节但只有2 */
    EXPECT_EQ(pny_msgpack_write_uint(buf, 2, 0x1000), -1);
    /* 0x10000 需要5字节 */
    EXPECT_EQ(pny_msgpack_write_uint(buf, 2, 0x10000), -1);
    /* 2^32 需要9字节 */
    EXPECT_EQ(pny_msgpack_write_uint(buf, 2, 0x100000000ULL), -1);
    /* 负数 5字节需要, 只给2 */
    EXPECT_EQ(pny_msgpack_write_int(buf, 2, -100000), -1);
    /* 负数 9字节需要, 只给4 */
    uint8_t buf4[4] = {0};
    EXPECT_EQ(pny_msgpack_write_int(buf4, 4, -5000000000LL), -1);
}
