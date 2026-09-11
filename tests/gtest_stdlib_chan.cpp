#include <gtest/gtest.h>
#include <ponypp/stdlib.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

/* ==================== Channel ==================== */

TEST(StdlibChan, CreateFree) {
    PnyChan *c = pny_chan_new(4);
    ASSERT_NE(c, nullptr);
    EXPECT_FALSE(pny_chan_closed(c));
    pny_chan_free(c);
}

TEST(StdlibChan, SendRecv) {
    PnyChan *c = pny_chan_new(4);
    ASSERT_NE(c, nullptr);
    
    int a = 1, b = 2;
    EXPECT_EQ(pny_chan_send(c, &a), 0);
    EXPECT_EQ(pny_chan_send(c, &b), 0);
    EXPECT_EQ(pny_chan_len(c), 2);
    
    int *val = (int *)pny_chan_recv(c);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 1);
    
    val = (int *)pny_chan_recv(c);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 2);
    
    pny_chan_free(c);
}

TEST(StdlibChan, Close) {
    PnyChan *c = pny_chan_new(4);
    ASSERT_NE(c, nullptr);
    
    int a = 1;
    pny_chan_send(c, &a);
    
    EXPECT_FALSE(pny_chan_closed(c));
    pny_chan_close(c);
    EXPECT_TRUE(pny_chan_closed(c));
    
    /* 关闭后仍可接收已发送数据 */
    int *val = (int *)pny_chan_recv(c);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 1);
    
    /* 关闭后发送应失败 */
    EXPECT_NE(pny_chan_send(c, &a), 0);
    
    pny_chan_free(c);
}

TEST(StdlibChan, NullSafety) {
    pny_chan_free(nullptr);
    EXPECT_NE(pny_chan_send(nullptr, nullptr), 0);
    EXPECT_EQ(pny_chan_recv(nullptr), nullptr);
    EXPECT_TRUE(pny_chan_closed(nullptr));
    pny_chan_close(nullptr);
    EXPECT_EQ(pny_chan_len(nullptr), 0);
}

/* ==================== Stream ==================== */

TEST(StdlibStream, CreateWriteRead) {
    PnyStream *s = pny_stream_new();
    ASSERT_NE(s, nullptr);
    
    const char *data = "hello";
    EXPECT_EQ(pny_stream_write(s, data, strlen(data)), 5);
    EXPECT_EQ(pny_stream_available(s), 5);
    
    char buf[16] = {0};
    int n = pny_stream_read(s, buf, sizeof(buf));
    EXPECT_EQ(n, 5);
    EXPECT_EQ(memcmp(buf, "hello", 5), 0);
    
    EXPECT_FALSE(pny_stream_is_closed(s));
    pny_stream_close(s);
    EXPECT_TRUE(pny_stream_is_closed(s));
    
    pny_stream_free(s);
}

TEST(StdlibStream, NullSafety) {
    pny_stream_free(nullptr);
    EXPECT_NE(pny_stream_write(nullptr, "x", 1), 0);
    EXPECT_EQ(pny_stream_read(nullptr, nullptr, 0), -1);
    EXPECT_EQ(pny_stream_available(nullptr), 0);
    pny_stream_close(nullptr);
}

/* ==================== Mailbox ==================== */

TEST(StdlibMailbox, PutTake) {
    PnyMailbox *mb = pny_mailbox_new(10);
    ASSERT_NE(mb, nullptr);
    
    EXPECT_TRUE(pny_mailbox_is_empty(mb));
    
    int a = 1, b = 2;
    EXPECT_TRUE(pny_mailbox_put(mb, &a));
    EXPECT_TRUE(pny_mailbox_put(mb, &b));
    EXPECT_EQ(pny_mailbox_size(mb), 2);
    
    int *val = (int *)pny_mailbox_take(mb);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 1);
    
    val = (int *)pny_mailbox_take(mb);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 2);
    
    EXPECT_TRUE(pny_mailbox_is_empty(mb));
    
    pny_mailbox_free(mb);
}

TEST(StdlibMailbox, NullSafety) {
    pny_mailbox_free(nullptr);
    EXPECT_FALSE(pny_mailbox_put(nullptr, nullptr));
    EXPECT_EQ(pny_mailbox_take(nullptr), nullptr);
    EXPECT_EQ(pny_mailbox_size(nullptr), 0);
}

/* ==================== Group ==================== */

TEST(StdlibGroup, BasicOps) {
    PnyGroup *g = pny_group_new("test-group");
    ASSERT_NE(g, nullptr);
    
    EXPECT_STREQ(pny_group_name(g), "test-group");
    EXPECT_EQ(pny_group_size(g), 0);
    
    EXPECT_EQ(pny_group_add(g, 1), 0);
    EXPECT_EQ(pny_group_add(g, 2), 0);
    EXPECT_EQ(pny_group_size(g), 2);
    
    EXPECT_TRUE(pny_group_contains(g, 1));
    EXPECT_FALSE(pny_group_contains(g, 99));
    
    EXPECT_EQ(pny_group_remove(g, 1), 0);
    EXPECT_EQ(pny_group_size(g), 1);
    
    pny_group_free(g);
}

TEST(StdlibGroup, Members) {
    PnyGroup *g = pny_group_new("test");
    ASSERT_NE(g, nullptr);
    
    pny_group_add(g, 10);
    pny_group_add(g, 20);
    pny_group_add(g, 30);
    
    int ids[8];
    int count = pny_group_members(g, ids, 8);
    EXPECT_EQ(count, 3);
    
    pny_group_free(g);
}

TEST(StdlibGroup, NullSafety) {
    pny_group_free(nullptr);
    EXPECT_EQ(pny_group_add(nullptr, 1), -1);
    EXPECT_EQ(pny_group_remove(nullptr, 1), -1);
    EXPECT_EQ(pny_group_size(nullptr), 0);
    EXPECT_FALSE(pny_group_contains(nullptr, 1));
    EXPECT_EQ(pny_group_name(nullptr), nullptr);
}

/* ==================== Promise/Future ==================== */

TEST(StdlibPromise, CreateFulfill) {
    PnyPromise *p = pny_promise_new();
    ASSERT_NE(p, nullptr);
    
    EXPECT_FALSE(pny_promise_is_done(p));
    
    int val = 42;
    EXPECT_EQ(pny_promise_fulfill(p, &val, sizeof(val)), 0);
    EXPECT_TRUE(pny_promise_is_done(p));
    
    size_t out_size = 0;
    void *result = pny_promise_value(p, &out_size);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(out_size, sizeof(val));
    EXPECT_EQ(*(int *)result, 42);
    
    /* 重复 fulfill 应失败 */
    EXPECT_NE(pny_promise_fulfill(p, &val, sizeof(val)), 0);
    
    pny_promise_free(p);
}

TEST(StdlibPromise, Future) {
    PnyPromise *p = pny_promise_new();
    ASSERT_NE(p, nullptr);
    
    int val = 99;
    pny_promise_fulfill(p, &val, sizeof(val));
    
    PnyFuture *f = pny_future_from_promise(p);
    if (f) {
        EXPECT_TRUE(pny_future_is_done(f));
        
        size_t out_size = 0;
        void *result = pny_future_value(f, &out_size);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*(int *)result, 99);
        
        /* future 是借用引用, 不 free */
    }
    
    pny_promise_free(p);
}

TEST(StdlibPromise, NullSafety) {
    pny_promise_free(nullptr);
    EXPECT_FALSE(pny_promise_is_done(nullptr));
    EXPECT_EQ(pny_promise_value(nullptr, nullptr), nullptr);
    EXPECT_NE(pny_promise_fulfill(nullptr, nullptr, 0), 0);
}

/* ==================== Buffer ==================== */

TEST(StdlibBuffer, CreateAppend) {
    PnyBuffer *buf = pny_buffer_new(16);
    ASSERT_NE(buf, nullptr);
    
    EXPECT_EQ(pny_buffer_len(buf), 0);
    
    EXPECT_TRUE(pny_buffer_append(buf, "hello", 5));
    EXPECT_EQ(pny_buffer_len(buf), 5);
    
    EXPECT_TRUE(pny_buffer_append_byte(buf, '!'));
    EXPECT_EQ(pny_buffer_len(buf), 6);
    
    EXPECT_TRUE(pny_buffer_append_cstr(buf, " world"));
    EXPECT_EQ(pny_buffer_len(buf), 12);
    
    const uint8_t *data = pny_buffer_data(buf);
    ASSERT_NE(data, nullptr);
    EXPECT_EQ(memcmp(data, "hello! world", 12), 0);
    
    pny_buffer_clear(buf);
    EXPECT_EQ(pny_buffer_len(buf), 0);
    
    pny_buffer_free(buf);
}

TEST(StdlibBuffer, Reserve) {
    PnyBuffer *buf = pny_buffer_new(4);
    ASSERT_NE(buf, nullptr);
    
    EXPECT_TRUE(pny_buffer_reserve(buf, 1024));
    
    /* 写入大量数据 */
    char data[512];
    memset(data, 'A', sizeof(data));
    EXPECT_TRUE(pny_buffer_append(buf, data, sizeof(data)));
    EXPECT_EQ(pny_buffer_len(buf), 512);
    
    pny_buffer_free(buf);
}

TEST(StdlibBuffer, NullSafety) {
    pny_buffer_free(nullptr);
    EXPECT_EQ(pny_buffer_len(nullptr), 0);
    EXPECT_FALSE(pny_buffer_append(nullptr, "x", 1));
    EXPECT_EQ(pny_buffer_data(nullptr), nullptr);
    pny_buffer_clear(nullptr);
}

/* ==================== 复数 ==================== */

TEST(StdlibComplex, BasicOps) {
    PnyComplex c = pny_complex_new(3.0, 4.0);
    EXPECT_NEAR(pny_complex_abs(c), 5.0, 0.01);
    
    PnyComplex sum = pny_complex_add(c, c);
    PnyComplex diff = pny_complex_sub(c, c);
    PnyComplex prod = pny_complex_mul(c, c);
    
    (void)sum;
    (void)diff;
    (void)prod;
}
