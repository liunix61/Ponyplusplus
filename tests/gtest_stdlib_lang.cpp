#include <gtest/gtest.h>
#include <ponypp/stdlib.h>
#include <cstring>
#include <cstdlib>

/* ==================== 内存操作 ==================== */

TEST(StdlibMem, AllocFree) {
    void *p = pny_mem_alloc(64);
    ASSERT_NE(p, nullptr);
    pny_mem_free(p);
}

TEST(StdlibMem, Calloc) {
    int *p = (int *)pny_mem_calloc(10, sizeof(int));
    ASSERT_NE(p, nullptr);
    for (int i = 0; i < 10; i++) EXPECT_EQ(p[i], 0);
    pny_mem_free(p);
}

TEST(StdlibMem, Realloc) {
    char *p = (char *)pny_mem_alloc(16);
    ASSERT_NE(p, nullptr);
    memset(p, 'A', 16);
    
    p = (char *)pny_mem_realloc(p, 64);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p[0], 'A');
    pny_mem_free(p);
}

TEST(StdlibMem, CopyMoveSetCmp) {
    char src[16] = "hello";
    char dest[16] = {0};
    
    pny_mem_copy(dest, src, 6);
    EXPECT_STREQ(dest, "hello");
    
    char buf[16] = "abcdefgh";
    pny_mem_move(buf + 2, buf, 6);
    EXPECT_EQ(buf[2], 'a');
    
    char zero[8];
    pny_mem_set(zero, 0, 8);
    EXPECT_EQ(zero[0], 0);
    
    EXPECT_EQ(pny_mem_cmp("abc", "abc", 3), 0);
    EXPECT_LT(pny_mem_cmp("abc", "abd", 3), 0);
    EXPECT_GT(pny_mem_cmp("abd", "abc", 3), 0);
}

/* ==================== 字符处理 ==================== */

TEST(StdlibChar, Classification) {
    EXPECT_TRUE(pny_char_is_alpha('A'));
    EXPECT_TRUE(pny_char_is_alpha('z'));
    EXPECT_FALSE(pny_char_is_alpha('1'));
    EXPECT_FALSE(pny_char_is_alpha(' '));
    
    EXPECT_TRUE(pny_char_is_digit('0'));
    EXPECT_TRUE(pny_char_is_digit('9'));
    EXPECT_FALSE(pny_char_is_digit('a'));
    
    EXPECT_TRUE(pny_char_is_alnum('a'));
    EXPECT_TRUE(pny_char_is_alnum('5'));
    EXPECT_FALSE(pny_char_is_alnum('!'));
    
    EXPECT_TRUE(pny_char_is_upper('A'));
    EXPECT_FALSE(pny_char_is_upper('a'));
    
    EXPECT_TRUE(pny_char_is_lower('a'));
    EXPECT_FALSE(pny_char_is_lower('A'));
    
    EXPECT_TRUE(pny_char_is_space(' '));
    EXPECT_TRUE(pny_char_is_space('\t'));
    EXPECT_TRUE(pny_char_is_space('\n'));
    EXPECT_FALSE(pny_char_is_space('x'));
}

TEST(StdlibChar, Conversion) {
    EXPECT_EQ(pny_char_to_upper('a'), 'A');
    EXPECT_EQ(pny_char_to_upper('z'), 'Z');
    EXPECT_EQ(pny_char_to_upper('A'), 'A');
    
    EXPECT_EQ(pny_char_to_lower('A'), 'a');
    EXPECT_EQ(pny_char_to_lower('Z'), 'z');
    EXPECT_EQ(pny_char_to_lower('a'), 'a');
}

/* ==================== 数值转换 ==================== */

TEST(StdlibParse, ParseInt) {
    EXPECT_EQ(pny_parse_int("42"), 42);
    EXPECT_EQ(pny_parse_int("-42"), -42);
    EXPECT_EQ(pny_parse_int("0"), 0);
    EXPECT_EQ(pny_parse_int("999999999"), 999999999);
    EXPECT_EQ(pny_parse_int(nullptr), 0);
    EXPECT_EQ(pny_parse_int("abc"), 0);
}

TEST(StdlibParse, ParseFloat) {
    EXPECT_NEAR(pny_parse_float("3.14"), 3.14, 0.001);
    EXPECT_NEAR(pny_parse_float("-2.5"), -2.5, 0.001);
    EXPECT_NEAR(pny_parse_float("0.0"), 0.0, 0.001);
    EXPECT_NEAR(pny_parse_float(nullptr), 0.0, 0.001);
}

TEST(StdlibParse, IntToString) {
    char *s = pny_int_to_string(42);
    ASSERT_NE(s, nullptr);
    EXPECT_STREQ(s, "42");
    free(s);
    
    s = pny_int_to_string(-100);
    ASSERT_NE(s, nullptr);
    EXPECT_STREQ(s, "-100");
    free(s);
    
    s = pny_int_to_string(0);
    ASSERT_NE(s, nullptr);
    EXPECT_STREQ(s, "0");
    free(s);
}

TEST(StdlibParse, FloatToString) {
    char *s = pny_float_to_string(3.14);
    ASSERT_NE(s, nullptr);
    EXPECT_NEAR(atof(s), 3.14, 0.01);
    free(s);
}

/* ==================== 环境/进程 ==================== */

TEST(StdlibEnv, GetSet) {
    EXPECT_EQ(pny_env_set("PONYPP_TEST_VAR", "hello"), 0);
    
    const char *val = pny_env_get("PONYPP_TEST_VAR");
    ASSERT_NE(val, nullptr);
    EXPECT_STREQ(val, "hello");
    
    EXPECT_EQ(pny_env_get("PONYPP_NONEXISTENT_VAR"), nullptr);
}

TEST(StdlibEnv, GetPid) {
    int32_t pid = pny_getpid();
    EXPECT_GT(pid, 0);
}

TEST(StdlibEnv, GetCwd) {
    const char *cwd = pny_getcwd();
    ASSERT_NE(cwd, nullptr);
    EXPECT_GT(strlen(cwd), 0);
}

TEST(StdlibEnv, SleepMs) {
    int64_t start = pny_time_now_ms();
    pny_sleep_ms(10);
    int64_t elapsed = pny_time_now_ms() - start;
    EXPECT_GE(elapsed, 8); /* 允许少量误差 */
}

/* ==================== 断言 ==================== */

TEST(StdlibAssert, PassCondition) {
    /* 不应崩溃 */
    pny_assert(true, "this should pass");
    pny_assert(1 == 1, "equality check");
}

/* ==================== 排序 ==================== */

TEST(StdlibSort, SortInt) {
    int64_t arr[] = {5, 3, 1, 4, 2};
    pny_sort_int(arr, 5);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
    EXPECT_EQ(arr[3], 4);
    EXPECT_EQ(arr[4], 5);
}

TEST(StdlibSort, SortFloat) {
    double arr[] = {3.14, 1.41, 2.72, 0.58};
    pny_sort_float(arr, 4);
    EXPECT_NEAR(arr[0], 0.58, 0.001);
    EXPECT_NEAR(arr[1], 1.41, 0.001);
    EXPECT_NEAR(arr[2], 2.72, 0.001);
    EXPECT_NEAR(arr[3], 3.14, 0.001);
}

TEST(StdlibSort, SortStr) {
    const char *arr[] = {"banana", "apple", "cherry"};
    pny_sort_str(arr, 3);
    EXPECT_STREQ(arr[0], "apple");
    EXPECT_STREQ(arr[1], "banana");
    EXPECT_STREQ(arr[2], "cherry");
}

TEST(StdlibSort, SortEdgeCases) {
    int64_t single[] = {42};
    pny_sort_int(single, 1);
    EXPECT_EQ(single[0], 42);
    
    pny_sort_int(nullptr, 0);
    
    int64_t sorted[] = {1, 2, 3};
    pny_sort_int(sorted, 3);
    EXPECT_EQ(sorted[0], 1);
}

TEST(StdlibSort, BinarySearch) {
    int64_t arr[] = {1, 3, 5, 7, 9, 11, 13};
    
    EXPECT_EQ(pny_binary_search_int(arr, 7, 7), 3);
    EXPECT_EQ(pny_binary_search_int(arr, 7, 1), 0);
    EXPECT_EQ(pny_binary_search_int(arr, 7, 13), 6);
    EXPECT_EQ(pny_binary_search_int(arr, 7, 4), -1);
    EXPECT_EQ(pny_binary_search_int(arr, 7, 0), -1);
    EXPECT_EQ(pny_binary_search_int(arr, 7, 100), -1);
    EXPECT_EQ(pny_binary_search_int(nullptr, 0, 42), -1);
}

/* ==================== 随机数 ==================== */

TEST(StdlibRandom, SeedAndRange) {
    pny_random_seed(42);
    
    for (int i = 0; i < 100; i++) {
        double f = pny_random_float();
        EXPECT_GE(f, 0.0);
        EXPECT_LE(f, 1.0);
    }
    
    for (int i = 0; i < 100; i++) {
        int64_t r = pny_random_range(10, 20);
        EXPECT_GE(r, 10);
        EXPECT_LT(r, 20);
    }
    
    /* 相同种子 → 相同序列 */
    pny_random_seed(42);
    double first = pny_random_float();
    pny_random_seed(42);
    double second = pny_random_float();
    EXPECT_DOUBLE_EQ(first, second);
}

TEST(StdlibRandom, EdgeCases) {
    /* min >= max → 返回 min */
    EXPECT_EQ(pny_random_range(5, 5), 5);
    EXPECT_EQ(pny_random_range(10, 5), 10);
}

/* ==================== 时间 ==================== */

TEST(StdlibTime, NowMs) {
    int64_t t1 = pny_time_now_ms();
    EXPECT_GT(t1, 0);
    
    pny_sleep_ms(5);
    
    int64_t t2 = pny_time_now_ms();
    EXPECT_GE(t2, t1);
}

TEST(StdlibTime, NowUs) {
    int64_t t = pny_time_now_us();
    EXPECT_GT(t, 0);
}

/* ==================== 字符串扩展 ==================== */

TEST(StdlibStrExt, Find) {
    PnyString *s = pny_str_new("hello world");
    ASSERT_NE(s, nullptr);
    
    EXPECT_EQ(pny_str_find(s, "hello"), 0);
    EXPECT_EQ(pny_str_find(s, "world"), 6);
    EXPECT_EQ(pny_str_find(s, "o"), 4);
    EXPECT_EQ(pny_str_find(s, "xyz"), -1);
    
    pny_str_free(s);
}

TEST(StdlibStrExt, Repeat) {
    PnyString *s = pny_str_new("ab");
    ASSERT_NE(s, nullptr);
    
    PnyString *r = pny_str_repeat(s, 3);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->len, 6);
    EXPECT_EQ(memcmp(r->data, "ababab", 6), 0);
    
    pny_str_free(r);
    pny_str_free(s);
}

TEST(StdlibStrExt, PadLeft) {
    PnyString *s = pny_str_new("hi");
    ASSERT_NE(s, nullptr);
    
    PnyString *p = pny_str_pad_left(s, 5, '0');
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->len, 5);
    EXPECT_EQ(memcmp(p->data, "000hi", 5), 0);
    
    /* 已经够长 → 不填充 */
    PnyString *p2 = pny_str_pad_left(s, 1, '0');
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(p2->len, 2);
    
    pny_str_free(p2);
    pny_str_free(p);
    pny_str_free(s);
}

TEST(StdlibStrExt, Reverse) {
    PnyString *s = pny_str_new("hello");
    ASSERT_NE(s, nullptr);
    
    PnyString *r = pny_str_reverse(s);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->len, 5);
    EXPECT_EQ(memcmp(r->data, "olleh", 5), 0);
    
    pny_str_free(r);
    pny_str_free(s);
}

TEST(StdlibStrExt, CharAt) {
    PnyString *s = pny_str_new("hello");
    ASSERT_NE(s, nullptr);
    
    EXPECT_EQ(pny_str_char_at(s, 0), 'h');
    EXPECT_EQ(pny_str_char_at(s, 4), 'o');
    EXPECT_EQ(pny_str_char_at(s, 5), '\0'); /* out of bounds */
    EXPECT_EQ(pny_str_char_at(nullptr, 0), '\0');
    
    pny_str_free(s);
}

TEST(StdlibStrExt, NullSafety) {
    EXPECT_EQ(pny_str_find(nullptr, "x"), -1);
    EXPECT_EQ(pny_str_repeat(nullptr, 3), nullptr);
    EXPECT_EQ(pny_str_pad_left(nullptr, 5, '0'), nullptr);
    EXPECT_EQ(pny_str_reverse(nullptr), nullptr);
    EXPECT_EQ(pny_str_char_at(nullptr, 0), '\0');
}
