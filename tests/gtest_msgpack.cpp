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
/* ==================== Protobuf ==================== */

TEST(Protobuf, WriteReadVarint) {
    uint8_t buf[16];
    /* 写入varint */
    PnyProtoBuf *pb = pny_proto_new();
    ASSERT_NE(pb, nullptr);
    pny_proto_write_varint(pb, 300);
    EXPECT_EQ(pny_proto_len(pb), 2u);  /* 300需要2字节 */

    /* 读取 */
    uint64_t val;
    int n = pny_proto_read_varint(pny_proto_data(pb), pny_proto_len(pb), &val);
    EXPECT_EQ(n, 2);
    EXPECT_EQ(val, 300u);
    pny_proto_free(pb);
}

TEST(Protobuf, WriteReadInt32) {
    PnyProtoBuf *pb = pny_proto_new();
    pny_proto_write_int32(pb, 1, 42);
    pny_proto_write_int32(pb, 2, -1);

    /* 读取字段1 */
    PnyProtoField f;
    int n = pny_proto_read_field(pny_proto_data(pb), pny_proto_len(pb), &f);
    EXPECT_GT(n, 0);
    EXPECT_EQ(f.field_num, 1u);
    EXPECT_EQ(f.wire_type, PROTO_WIRE_VARINT);
    EXPECT_EQ(f.varint_val, 42u);

    /* 读取字段2 */
    n = pny_proto_read_field(pny_proto_data(pb) + n, pny_proto_len(pb) - n, &f);
    EXPECT_GT(n, 0);
    EXPECT_EQ(f.field_num, 2u);
    pny_proto_free(pb);
}

TEST(Protobuf, WriteReadString) {
    PnyProtoBuf *pb = pny_proto_new();
    const char *msg = "Hello, Protobuf!";
    pny_proto_write_string(pb, 3, msg);

    PnyProtoField f;
    int n = pny_proto_read_field(pny_proto_data(pb), pny_proto_len(pb), &f);
    EXPECT_GT(n, 0);
    EXPECT_EQ(f.field_num, 3u);
    EXPECT_EQ(f.wire_type, PROTO_WIRE_LEN_DELIM);
    EXPECT_EQ(f.bytes_val.len, strlen(msg));
    EXPECT_EQ(memcmp(f.bytes_val.ptr, msg, f.bytes_val.len), 0);
    pny_proto_free(pb);
}

TEST(Protobuf, WriteReadBool) {
    PnyProtoBuf *pb = pny_proto_new();
    pny_proto_write_bool(pb, 1, true);
    pny_proto_write_bool(pb, 2, false);

    PnyProtoField f;
    int n = pny_proto_read_field(pny_proto_data(pb), pny_proto_len(pb), &f);
    EXPECT_EQ(f.varint_val, 1u);
    n = pny_proto_read_field(pny_proto_data(pb) + n, pny_proto_len(pb) - n, &f);
    EXPECT_EQ(f.varint_val, 0u);
    pny_proto_free(pb);
}

TEST(Protobuf, WriteReadDouble) {
    PnyProtoBuf *pb = pny_proto_new();
    double val = 3.14159265358979;
    pny_proto_write_double(pb, 5, val);

    PnyProtoField f;
    int n = pny_proto_read_field(pny_proto_data(pb), pny_proto_len(pb), &f);
    EXPECT_GT(n, 0);
    EXPECT_EQ(f.field_num, 5u);
    EXPECT_EQ(f.wire_type, PROTO_WIRE_FIXED64);
    EXPECT_DOUBLE_EQ(f.double_val, val);
    pny_proto_free(pb);
}

TEST(Protobuf, WriteReadBytes) {
    PnyProtoBuf *pb = pny_proto_new();
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    pny_proto_write_bytes(pb, 7, data, 4);

    PnyProtoField f;
    int n = pny_proto_read_field(pny_proto_data(pb), pny_proto_len(pb), &f);
    EXPECT_GT(n, 0);
    EXPECT_EQ(f.field_num, 7u);
    EXPECT_EQ(f.bytes_val.len, 4u);
    EXPECT_EQ(memcmp(f.bytes_val.ptr, data, 4), 0);
    pny_proto_free(pb);
}

TEST(Protobuf, MultipleFields) {
    PnyProtoBuf *pb = pny_proto_new();
    pny_proto_write_uint32(pb, 1, 100);
    pny_proto_write_string(pb, 2, "test");
    pny_proto_write_bool(pb, 3, true);

    /* 顺序读取3个字段 */
    PnyProtoField f;
    size_t offset = 0;
    int n = pny_proto_read_field(pny_proto_data(pb) + offset, pny_proto_len(pb) - offset, &f);
    EXPECT_EQ(f.field_num, 1u);
    EXPECT_EQ(f.varint_val, 100u);
    offset += n;

    n = pny_proto_read_field(pny_proto_data(pb) + offset, pny_proto_len(pb) - offset, &f);
    EXPECT_EQ(f.field_num, 2u);
    EXPECT_EQ(f.bytes_val.len, 4u);
    offset += n;

    n = pny_proto_read_field(pny_proto_data(pb) + offset, pny_proto_len(pb) - offset, &f);
    EXPECT_EQ(f.field_num, 3u);
    EXPECT_EQ(f.varint_val, 1u);
    pny_proto_free(pb);
}

TEST(Protobuf, EmbeddedMessage) {
    /* 构造嵌套消息: {1: {1: 42, 2: "hi"}} */
    PnyProtoBuf *inner = pny_proto_new();
    pny_proto_write_int32(inner, 1, 42);
    pny_proto_write_string(inner, 2, "hi");

    PnyProtoBuf *outer = pny_proto_new();
    pny_proto_write_message(outer, 10, pny_proto_data(inner), pny_proto_len(inner));

    PnyProtoField f;
    int n = pny_proto_read_field(pny_proto_data(outer), pny_proto_len(outer), &f);
    EXPECT_GT(n, 0);
    EXPECT_EQ(f.field_num, 10u);
    EXPECT_EQ(f.wire_type, PROTO_WIRE_LEN_DELIM);
    EXPECT_EQ(f.bytes_val.len, pny_proto_len(inner));

    /* 解析内层消息 */
    PnyProtoField inner_f;
    int m = pny_proto_read_field(f.bytes_val.ptr, f.bytes_val.len, &inner_f);
    EXPECT_EQ(inner_f.field_num, 1u);
    EXPECT_EQ(inner_f.varint_val, 42u);

    pny_proto_free(inner);
    pny_proto_free(outer);
}
