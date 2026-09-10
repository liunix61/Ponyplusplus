/*
 * Collections + Net/Utils 扩展测试
 * Set/Queue/Stack/Buffer + UDP/DNS/Date/Complex/Statistics
 */

#include <gtest/gtest.h>
#include <ponypp/stdlib.h>
#include <cstring>
#include <cmath>

/* ==================== Set ==================== */

TEST(Collections, SetBasics) {
    PnySet *s = pny_set_new();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(pny_set_size(s), 0u);
    EXPECT_TRUE(pny_set_add(s, "hello"));
    EXPECT_TRUE(pny_set_add(s, "world"));
    EXPECT_FALSE(pny_set_add(s, "hello"));  /* 重复 */
    EXPECT_EQ(pny_set_size(s), 2u);
    EXPECT_TRUE(pny_set_contains(s, "hello"));
    EXPECT_FALSE(pny_set_contains(s, "foo"));
    EXPECT_TRUE(pny_set_remove(s, "hello"));
    EXPECT_FALSE(pny_set_remove(s, "hello"));
    EXPECT_EQ(pny_set_size(s), 1u);
    pny_set_free(s);
}

TEST(Collections, SetClear) {
    PnySet *s = pny_set_new();
    pny_set_add(s, "a"); pny_set_add(s, "b");
    EXPECT_EQ(pny_set_size(s), 2u);
    pny_set_clear(s);
    EXPECT_EQ(pny_set_size(s), 0u);
    EXPECT_TRUE(pny_set_add(s, "a"));  /* 清空后可重新添加 */
    pny_set_free(s);
}

TEST(Collections, SetStress) {
    PnySet *s = pny_set_new();
    char key[32];
    for (int i = 0; i < 1000; i++) {
        snprintf(key, sizeof(key), "key-%d", i);
        EXPECT_TRUE(pny_set_add(s, key));
    }
    EXPECT_EQ(pny_set_size(s), 1000u);
    for (int i = 0; i < 1000; i++) {
        snprintf(key, sizeof(key), "key-%d", i);
        EXPECT_TRUE(pny_set_contains(s, key));
    }
    pny_set_free(s);
}

/* ==================== Queue ==================== */

TEST(Collections, QueueBasics) {
    PnyQueue *q = pny_queue_new(0);
    ASSERT_NE(q, nullptr);
    EXPECT_TRUE(pny_queue_is_empty(q));
    int a = 1, b = 2, c = 3;
    pny_queue_push(q, &a);
    pny_queue_push(q, &b);
    pny_queue_push(q, &c);
    EXPECT_EQ(pny_queue_size(q), 3u);
    EXPECT_EQ(pny_queue_peek(q), &a);  /* FIFO */
    EXPECT_EQ(pny_queue_pop(q), &a);
    EXPECT_EQ(pny_queue_pop(q), &b);
    EXPECT_EQ(pny_queue_pop(q), &c);
    EXPECT_TRUE(pny_queue_is_empty(q));
    EXPECT_EQ(pny_queue_pop(q), nullptr);
    pny_queue_free(q);
}

TEST(Collections, QueueGrow) {
    PnyQueue *q = pny_queue_new(4);
    int vals[100];
    for (int i = 0; i < 100; i++) {
        vals[i] = i;
        EXPECT_TRUE(pny_queue_push(q, &vals[i]));
    }
    EXPECT_EQ(pny_queue_size(q), 100u);
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(pny_queue_pop(q), &vals[i]);
    }
    pny_queue_free(q);
}

/* ==================== Stack ==================== */

TEST(Collections, StackBasics) {
    PnyStack *s = pny_stack_new(0);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(pny_stack_is_empty(s));
    int a = 1, b = 2, c = 3;
    pny_stack_push(s, &a);
    pny_stack_push(s, &b);
    pny_stack_push(s, &c);
    EXPECT_EQ(pny_stack_size(s), 3u);
    EXPECT_EQ(pny_stack_peek(s), &c);  /* LIFO */
    EXPECT_EQ(pny_stack_pop(s), &c);
    EXPECT_EQ(pny_stack_pop(s), &b);
    EXPECT_EQ(pny_stack_pop(s), &a);
    EXPECT_TRUE(pny_stack_is_empty(s));
    pny_stack_free(s);
}

/* ==================== Buffer ==================== */

TEST(Collections, BufferBasics) {
    PnyBuffer *b = pny_buffer_new(0);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(pny_buffer_len(b), 0u);
    pny_buffer_append_cstr(b, "Hello");
    pny_buffer_append_cstr(b, ", World");
    EXPECT_EQ(pny_buffer_len(b), 12u);
    EXPECT_EQ(memcmp(pny_buffer_data(b), "Hello, World", 12), 0);
    pny_buffer_clear(b);
    EXPECT_EQ(pny_buffer_len(b), 0u);
    pny_buffer_free(b);
}

TEST(Collections, BufferBinary) {
    PnyBuffer *b = pny_buffer_new(0);
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    pny_buffer_append(b, data, 4);
    pny_buffer_append_byte(b, 0x42);
    EXPECT_EQ(pny_buffer_len(b), 5u);
    const uint8_t *p = pny_buffer_data(b);
    EXPECT_EQ(p[0], 0xDE);
    EXPECT_EQ(p[4], 0x42);
    pny_buffer_free(b);
}

/* ==================== Complex ==================== */

TEST(Utils, ComplexArithmetic) {
    PnyComplex a = pny_complex_new(3, 4);
    PnyComplex b = pny_complex_new(1, 2);
    PnyComplex sum = pny_complex_add(a, b);
    EXPECT_DOUBLE_EQ(sum.re, 4);
    EXPECT_DOUBLE_EQ(sum.im, 6);
    PnyComplex prod = pny_complex_mul(a, b);
    EXPECT_DOUBLE_EQ(prod.re, -5);  /* 3*1-4*2 */
    EXPECT_DOUBLE_EQ(prod.im, 10);  /* 3*2+4*1 */
    EXPECT_DOUBLE_EQ(pny_complex_abs(a), 5.0);  /* |3+4i|=5 */
}

/* ==================== Statistics ==================== */

TEST(Utils, Statistics) {
    double data[] = {4, 8, 6, 5, 3, 7};
    EXPECT_DOUBLE_EQ(pny_stats_mean(data, 6), 5.5);
    EXPECT_DOUBLE_EQ(pny_stats_min(data, 6), 3);
    EXPECT_DOUBLE_EQ(pny_stats_max(data, 6), 8);
    double median = pny_stats_median(data, 6);  /* 排序后: 3,4,5,6,7,8 -> (5+6)/2 */
    EXPECT_DOUBLE_EQ(median, 5.5);
    EXPECT_GT(pny_stats_stddev(data, 6), 0);
}

/* ==================== Date ==================== */

TEST(Utils, DateBasics) {
    PnyDateTime dt;
    ASSERT_EQ(pny_date_now(&dt), 0);
    EXPECT_GT(dt.year, 2020);
    EXPECT_GE(dt.month, 1);
    EXPECT_LE(dt.month, 12);
    /* timestamp往返 */
    int64_t ts = pny_date_to_timestamp(&dt);
    PnyDateTime dt2;
    pny_date_from_timestamp(ts, &dt2);
    EXPECT_EQ(dt.year, dt2.year);
    EXPECT_EQ(dt.month, dt2.month);
    EXPECT_EQ(dt.day, dt2.day);
}

TEST(Utils, DateFormat) {
    PnyDateTime dt = {2026, 9, 10, 14, 30, 0, 0};
    char buf[64];
    const char *result = pny_date_format(&dt, "%Y-%m-%d %H:%M:%S", buf, sizeof(buf));
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(buf, "2026-09-10 14:30:00");
}

/* ==================== DNS ==================== */

TEST(Utils, DnsLocalhost) {
    PnyDnsResult result;
    int n = pny_dns_resolve("localhost", &result);
    EXPECT_GE(n, 1);
    if (n > 0) {
        EXPECT_STREQ(result.addrs[0], "127.0.0.1");
    }
}

/* ==================== UDP ==================== */

TEST(Net, UdpLoopback) {
    /* 发送到自己 */
    PnyUdpSocket *rx = pny_udp_open("127.0.0.1", 39876);
    ASSERT_NE(rx, nullptr);
    pny_udp_set_timeout(rx, 100);

    PnyUdpSocket *tx = pny_udp_open("127.0.0.1", 39877);
    ASSERT_NE(tx, nullptr);

    const char *msg = "UDP test";
    int sent = pny_udp_sendto(tx, msg, strlen(msg), "127.0.0.1", 39876);
    EXPECT_EQ(sent, (int)strlen(msg));

    char buf[256] = {0};
    char src[64] = {0};
    int src_port = 0;
    int n = pny_udp_recvfrom(rx, buf, sizeof(buf), src, sizeof(src), &src_port);
    EXPECT_EQ(n, (int)strlen(msg));
    EXPECT_STREQ(buf, "UDP test");
    EXPECT_STREQ(src, "127.0.0.1");
    EXPECT_EQ(src_port, 39877);

    pny_udp_close(tx);
    pny_udp_close(rx);
}
