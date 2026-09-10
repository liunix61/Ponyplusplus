/*
 * MessagePack + Stream + Mailbox + Group 测试
 */

#include <gtest/gtest.h>
#include <ponypp/stdlib.h>
#include <cstring>

/* ==================== MessagePack ==================== */

TEST(Msgpack, WriteReadInt) {
    uint8_t buf[16];
    /* 正整数 (fixint) */
    int n = pny_msgpack_write_uint(buf, sizeof(buf), 42);
    EXPECT_EQ(n, 1);
    PnyMsgpackValue v;
    int r = pny_msgpack_read(buf, n, &v);
    EXPECT_EQ(r, 1);
    EXPECT_EQ(v.type, PNY_MSGPACK_UINT);
    EXPECT_EQ(v.uint_val, 42u);

    /* 负整数 (negative fixint) */
    n = pny_msgpack_write_int(buf, sizeof(buf), -5);
    EXPECT_EQ(n, 1);
    r = pny_msgpack_read(buf, n, &v);
    EXPECT_EQ(v.type, PNY_MSGPACK_INT);
    EXPECT_EQ(v.int_val, -5);

    /* int32 */
    n = pny_msgpack_write_int(buf, sizeof(buf), -100000);
    EXPECT_EQ(n, 5);
    r = pny_msgpack_read(buf, n, &v);
    EXPECT_EQ(v.int_val, -100000);
}

TEST(Msgpack, WriteReadStr) {
    uint8_t buf[256];
    const char *msg = "Hello, MessagePack!";
    int n = pny_msgpack_write_str(buf, sizeof(buf), msg);
    ASSERT_GT(n, 0);
    PnyMsgpackValue v;
    int r = pny_msgpack_read(buf, n, &v);
    EXPECT_EQ(r, n);
    EXPECT_EQ(v.type, PNY_MSGPACK_STR);
    EXPECT_EQ(v.str_bin.len, strlen(msg));
    EXPECT_EQ(memcmp(v.str_bin.ptr, msg, v.str_bin.len), 0);
}

TEST(Msgpack, WriteReadBool) {
    uint8_t buf[4];
    int n = pny_msgpack_write_bool(buf, sizeof(buf), true);
    PnyMsgpackValue v;
    pny_msgpack_read(buf, n, &v);
    EXPECT_TRUE(v.bool_val);

    n = pny_msgpack_write_bool(buf, sizeof(buf), false);
    pny_msgpack_read(buf, n, &v);
    EXPECT_FALSE(v.bool_val);
}

TEST(Msgpack, WriteReadDouble) {
    uint8_t buf[16];
    double val = 3.14159265358979;
    int n = pny_msgpack_write_double(buf, sizeof(buf), val);
    EXPECT_EQ(n, 9);
    PnyMsgpackValue v;
    pny_msgpack_read(buf, n, &v);
    EXPECT_EQ(v.type, PNY_MSGPACK_DOUBLE);
    EXPECT_DOUBLE_EQ(v.double_val, val);
}

TEST(Msgpack, WriteReadNil) {
    uint8_t buf[4];
    int n = pny_msgpack_write_nil(buf, sizeof(buf));
    EXPECT_EQ(n, 1);
    PnyMsgpackValue v;
    pny_msgpack_read(buf, n, &v);
    EXPECT_EQ(v.type, PNY_MSGPACK_NIL);
}

TEST(Msgpack, ArrayMapHeader) {
    uint8_t buf[8];
    /* fixarray */
    int n = pny_msgpack_write_array_header(buf, sizeof(buf), 5);
    EXPECT_EQ(n, 1);
    PnyMsgpackValue v;
    pny_msgpack_read(buf, n, &v);
    EXPECT_EQ(v.type, PNY_MSGPACK_ARRAY);
    EXPECT_EQ(v.count, 5u);

    /* fixmap */
    n = pny_msgpack_write_map_header(buf, sizeof(buf), 3);
    pny_msgpack_read(buf, n, &v);
    EXPECT_EQ(v.type, PNY_MSGPACK_MAP);
    EXPECT_EQ(v.count, 3u);
}

TEST(Msgpack, BinData) {
    uint8_t buf[256];
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    int n = pny_msgpack_write_bin(buf, sizeof(buf), data, 4);
    ASSERT_GT(n, 0);
    PnyMsgpackValue v;
    pny_msgpack_read(buf, n, &v);
    EXPECT_EQ(v.type, PNY_MSGPACK_BIN);
    EXPECT_EQ(v.str_bin.len, 4u);
    EXPECT_EQ(memcmp(v.str_bin.ptr, data, 4), 0);
}

/* ==================== Stream ==================== */

TEST(Stream, WriteRead) {
    PnyStream *st = pny_stream_new();
    ASSERT_NE(st, nullptr);
    EXPECT_EQ(pny_stream_available(st), 0u);
    pny_stream_write(st, "Hello", 5);
    pny_stream_write(st, " World", 6);
    EXPECT_EQ(pny_stream_available(st), 11u);
    char buf[32] = {0};
    int n = pny_stream_read(st, buf, sizeof(buf));
    EXPECT_EQ(n, 11);
    EXPECT_STREQ(buf, "Hello World");
    EXPECT_EQ(pny_stream_available(st), 0u);
    pny_stream_free(st);
}

TEST(Stream, CloseEOF) {
    PnyStream *st = pny_stream_new();
    pny_stream_write(st, "data", 4);
    pny_stream_close(st);
    EXPECT_TRUE(pny_stream_is_closed(st));
    char buf[16];
    int n = pny_stream_read(st, buf, sizeof(buf));
    EXPECT_EQ(n, 4);  /* 读出剩余数据 */
    n = pny_stream_read(st, buf, sizeof(buf));
    EXPECT_EQ(n, -2);  /* EOF */
    pny_stream_free(st);
}

/* ==================== Mailbox ==================== */

TEST(Mailbox, PutTake) {
    PnyMailbox *mb = pny_mailbox_new(0);
    ASSERT_NE(mb, nullptr);
    EXPECT_TRUE(pny_mailbox_is_empty(mb));
    int a = 1, b = 2;
    pny_mailbox_put(mb, &a);
    pny_mailbox_put(mb, &b);
    EXPECT_EQ(pny_mailbox_size(mb), 2u);
    EXPECT_EQ(pny_mailbox_take(mb), &a);  /* FIFO */
    EXPECT_EQ(pny_mailbox_take(mb), &b);
    EXPECT_TRUE(pny_mailbox_is_empty(mb));
    pny_mailbox_free(mb);
}

TEST(Mailbox, MaxSize) {
    PnyMailbox *mb = pny_mailbox_new(2);
    int a = 1, b = 2, c = 3;
    EXPECT_TRUE(pny_mailbox_put(mb, &a));
    EXPECT_TRUE(pny_mailbox_put(mb, &b));
    EXPECT_FALSE(pny_mailbox_put(mb, &c));  /* 满 */
    pny_mailbox_free(mb);
}

/* ==================== Group ==================== */

TEST(Group, Basics) {
    PnyGroup *g = pny_group_new("workers");
    ASSERT_NE(g, nullptr);
    EXPECT_STREQ(pny_group_name(g), "workers");
    EXPECT_EQ(pny_group_size(g), 0u);
    EXPECT_EQ(pny_group_add(g, 1), 0);
    EXPECT_EQ(pny_group_add(g, 2), 0);
    EXPECT_EQ(pny_group_add(g, 1), -2);  /* 重复 */
    EXPECT_EQ(pny_group_size(g), 2u);
    EXPECT_TRUE(pny_group_contains(g, 1));
    EXPECT_FALSE(pny_group_contains(g, 3));
    EXPECT_EQ(pny_group_remove(g, 1), 0);
    EXPECT_EQ(pny_group_remove(g, 1), -2);  /* 已移除 */
    EXPECT_EQ(pny_group_size(g), 1u);
    pny_group_free(g);
}

TEST(Group, Members) {
    PnyGroup *g = pny_group_new("team");
    for (int i = 0; i < 10; i++) pny_group_add(g, i * 10);
    int ids[10];
    int n = pny_group_members(g, ids, 10);
    EXPECT_EQ(n, 10);
    EXPECT_EQ(ids[0], 0);
    EXPECT_EQ(ids[9], 90);
    pny_group_free(g);
}

/* ==================== Error ==================== */

TEST(Error, Strings) {
    EXPECT_STREQ(pny_error_str(PNY_ERR_NONE), "no error");
    EXPECT_STREQ(pny_error_str(PNY_ERR_EOF), "end of file");
    EXPECT_STREQ(pny_error_str(PNY_ERR_TIMEOUT), "timeout");
    EXPECT_NE(pny_error_str(PNY_ERR_UNKNOWN), nullptr);
}
