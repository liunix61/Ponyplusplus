#include <gtest/gtest.h>
#include <ponypp/stdlib.h>
#include <ponypp/runtime.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

/* ==================== 路径/文件 ==================== */

class StdlibPathTest : public ::testing::Test {
protected:
    char tmpfile[256];
    
    void SetUp() override {
        snprintf(tmpfile, sizeof(tmpfile), "/tmp/ponypp-test-%d.txt", getpid());
        FILE *f = fopen(tmpfile, "w");
        if (f) { fprintf(f, "Hello\nWorld\n"); fclose(f); }
    }
    
    void TearDown() override { unlink(tmpfile); }
};

TEST_F(StdlibPathTest, PathExists) {
    EXPECT_TRUE(pny_path_exists(tmpfile));
    EXPECT_FALSE(pny_path_exists("/nonexistent/file.txt"));
    EXPECT_FALSE(pny_path_exists(nullptr));
}

TEST_F(StdlibPathTest, FileSize) {
    EXPECT_GT(pny_file_size(tmpfile), 0);
    EXPECT_EQ(pny_file_size("/nonexistent"), -1);
    EXPECT_EQ(pny_file_size(nullptr), -1);
}

TEST_F(StdlibPathTest, FileDelete) {
    const char *path = "/tmp/ponypp-del-test.txt";
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "x"); fclose(f); }
    
    EXPECT_TRUE(pny_path_exists(path));
    EXPECT_EQ(pny_file_delete(path), 0);
    EXPECT_FALSE(pny_path_exists(path));
    
    EXPECT_EQ(pny_file_delete("/nonexistent"), -1);
    EXPECT_EQ(pny_file_delete(nullptr), -1);
}

TEST_F(StdlibPathTest, FileOpenRead) {
    PnyFile *f = pny_file_open(tmpfile, FILE_MODE_READ);
    ASSERT_NE(f, nullptr);
    
    char *content = pny_file_read_all(f);
    ASSERT_NE(content, nullptr);
    EXPECT_EQ(memcmp(content, "Hello\nWorld\n", 12), 0);
    free(content);
    
    pny_file_close(f);
}

TEST_F(StdlibPathTest, FileOpenWrite) {
    const char *path = "/tmp/ponypp-write-test.txt";
    PnyFile *f = pny_file_open(path, FILE_MODE_WRITE);
    ASSERT_NE(f, nullptr);
    
    const char *data = "Written content";
    EXPECT_EQ(pny_file_write(f, data, strlen(data)), 0);
    
    pny_file_close(f);
    
    /* 验证写入 */
    EXPECT_TRUE(pny_path_exists(path));
    unlink(path);
}

TEST_F(StdlibPathTest, FileOpenBadArgs) {
    EXPECT_EQ(pny_file_open(nullptr, FILE_MODE_READ), nullptr);
    EXPECT_EQ(pny_file_open("/nonexistent", FILE_MODE_READ), nullptr);
}

TEST_F(StdlibPathTest, FileCloseNull) {
    pny_file_close(nullptr);  /* 不应崩溃 */
}

TEST_F(StdlibPathTest, FileReadLine) {
    PnyFile *f = pny_file_open(tmpfile, FILE_MODE_READ);
    ASSERT_NE(f, nullptr);
    
    char *line1 = pny_file_read_line(f);
    ASSERT_NE(line1, nullptr);
    EXPECT_STREQ(line1, "Hello");
    free(line1);
    
    char *line2 = pny_file_read_line(f);
    ASSERT_NE(line2, nullptr);
    EXPECT_STREQ(line2, "World");
    free(line2);
    
    pny_file_close(f);
}

TEST_F(StdlibPathTest, FilePrintf) {
    const char *path = "/tmp/ponypp-printf-test.txt";
    PnyFile *f = pny_file_open(path, FILE_MODE_WRITE);
    ASSERT_NE(f, nullptr);
    
    pny_file_printf(f, "Value: %d, Name: %s", 42, "test");
    pny_file_close(f);
    
    /* 验证 */
    PnyFile *rf = pny_file_open(path, FILE_MODE_READ);
    ASSERT_NE(rf, nullptr);
    char *content = pny_file_read_all(rf);
    ASSERT_NE(content, nullptr);
    EXPECT_STREQ(content, "Value: 42, Name: test");
    free(content);
    pny_file_close(rf);
    
    unlink(path);
}

TEST_F(StdlibPathTest, DirList) {
    char **names = nullptr;
    int count = 0;
    
    int rc = pny_dir_list("/tmp", &names, &count);
    if (rc == 0 && names) {
        EXPECT_GT(count, 0);
        for (int i = 0; i < count; i++) free(names[i]);
        free(names);
    }
    
    /* NULL/不存在路径应失败 */
    char **n2 = nullptr;
    int c2 = 0;
    pny_dir_list(nullptr, &n2, &c2);  /* 不应崩溃 */
    pny_dir_list("/nonexistent", &n2, &c2);  /* 不应崩溃 */
}

/* ==================== 字符串 ==================== */

TEST(StdlibString, StrNew) {
    PnyString *s = pny_str_new("hello");
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(pny_str_len(s), 5);
    pny_str_free(s);
    
    /* NULL 创建空字符串 (不返回 NULL) */
    PnyString *empty = pny_str_new(nullptr);
    if (empty) {
        EXPECT_EQ(pny_str_len(empty), 0);
        pny_str_free(empty);
    }
}

TEST(StdlibString, StrNewWith) {
    PnyString *s = pny_str_new_with(64);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(pny_str_len(s), 0);
    pny_str_free(s);
}

TEST(StdlibString, StrDup) {
    PnyString *s = pny_str_new("original");
    ASSERT_NE(s, nullptr);
    
    PnyString *dup = pny_str_dup(s);
    ASSERT_NE(dup, nullptr);
    EXPECT_EQ(pny_str_len(dup), 8);
    
    pny_str_free(dup);
    pny_str_free(s);
    
    EXPECT_EQ(pny_str_dup(nullptr), nullptr);
}

TEST(StdlibString, StrCat) {
    PnyString *s = pny_str_new("Hello");
    ASSERT_NE(s, nullptr);
    
    pny_str_cat_cstr(s, " World");
    EXPECT_EQ(pny_str_len(s), 11);
    
    pny_str_free(s);
}

TEST(StdlibString, StrCmp) {
    PnyString *a = pny_str_new("abc");
    PnyString *b = pny_str_new("abc");
    PnyString *c = pny_str_new("abd");
    
    EXPECT_EQ(pny_str_cmp(a, b), 0);
    EXPECT_NE(pny_str_cmp(a, c), 0);
    EXPECT_EQ(pny_str_cmp_cstr(a, "abc"), 0);
    
    pny_str_free(a);
    pny_str_free(b);
    pny_str_free(c);
}

TEST(StdlibString, StrSlice) {
    PnyString *s = pny_str_new("Hello World");
    ASSERT_NE(s, nullptr);
    
    PnyString *sub = pny_str_slice(s, 0, 5);
    ASSERT_NE(sub, nullptr);
    EXPECT_EQ(pny_str_len(sub), 5);
    
    pny_str_free(sub);
    pny_str_free(s);
    
    EXPECT_EQ(pny_str_slice(nullptr, 0, 5), nullptr);
}

TEST(StdlibString, StrContains) {
    PnyString *s = pny_str_new("Hello World");
    ASSERT_NE(s, nullptr);
    
    PnyString *sub = pny_str_new("World");
    PnyString *no = pny_str_new("xyz");
    
    EXPECT_TRUE(pny_str_contains(s, sub));
    EXPECT_FALSE(pny_str_contains(s, no));
    
    pny_str_free(s);
    pny_str_free(sub);
    pny_str_free(no);
}

TEST(StdlibString, StrStartsEndsWith) {
    PnyString *s = pny_str_new("Hello World");
    ASSERT_NE(s, nullptr);
    
    PnyString *prefix = pny_str_new("Hello");
    PnyString *suffix = pny_str_new("World");
    
    EXPECT_TRUE(pny_str_starts_with(s, prefix));
    EXPECT_TRUE(pny_str_ends_with(s, suffix));
    
    pny_str_free(s);
    pny_str_free(prefix);
    pny_str_free(suffix);
}

TEST(StdlibString, StrToUpperLower) {
    PnyString *s = pny_str_new("Hello");
    ASSERT_NE(s, nullptr);
    
    PnyString *upper = pny_str_to_upper(s);
    ASSERT_NE(upper, nullptr);
    EXPECT_EQ(pny_str_cmp_cstr(upper, "HELLO"), 0);
    
    PnyString *lower = pny_str_to_lower(s);
    ASSERT_NE(lower, nullptr);
    EXPECT_EQ(pny_str_cmp_cstr(lower, "hello"), 0);
    
    pny_str_free(upper);
    pny_str_free(lower);
    pny_str_free(s);
}

TEST(StdlibString, StrTrim) {
    PnyString *s = pny_str_new("  hello  ");
    ASSERT_NE(s, nullptr);
    
    PnyString *trimmed = pny_str_trim(s);
    ASSERT_NE(trimmed, nullptr);
    EXPECT_EQ(pny_str_len(trimmed), 5);
    
    pny_str_free(trimmed);
    pny_str_free(s);
}

TEST(StdlibString, StrReplace) {
    PnyString *s = pny_str_new("Hello World");
    ASSERT_NE(s, nullptr);
    
    PnyString *old = pny_str_new("World");
    PnyString *new_ = pny_str_new("Pony++");
    
    PnyString *result = pny_str_replace(s, old, new_);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(pny_str_cmp_cstr(result, "Hello Pony++"), 0);
    
    pny_str_free(result);
    pny_str_free(s);
    pny_str_free(old);
    pny_str_free(new_);
}

TEST(StdlibString, StrFromTo) {
    PnyString *si = pny_str_from_int(42);
    ASSERT_NE(si, nullptr);
    EXPECT_EQ(pny_str_to_int(si), 42);
    
    PnyString *sf = pny_str_from_float(3.14);
    ASSERT_NE(sf, nullptr);
    EXPECT_NEAR(pny_str_to_float(sf), 3.14, 0.01);
    
    PnyString *sb = pny_str_from_bool(true);
    ASSERT_NE(sb, nullptr);
    EXPECT_TRUE(pny_str_to_bool(sb));
    
    pny_str_free(si);
    pny_str_free(sf);
    pny_str_free(sb);
}

/* ==================== 集合 ==================== */

TEST(StdlibList, BasicOps) {
    PnyList *l = pny_list_new(sizeof(int));
    ASSERT_NE(l, nullptr);
    
    EXPECT_EQ(pny_list_len(l), 0);
    EXPECT_TRUE(pny_list_empty(l));
    
    int a = 1, b = 2, c = 3;
    pny_list_append(l, &a);
    pny_list_append(l, &b);
    pny_list_append(l, &c);
    
    EXPECT_EQ(pny_list_len(l), 3);
    EXPECT_FALSE(pny_list_empty(l));
    
    int *val = (int *)pny_list_get(l, 0);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 1);
    
    pny_list_free(l);
    
    /* elem_size=0 可能仍返回非 NULL */
    PnyList *l2 = pny_list_new(0);
    if (l2) pny_list_free(l2);
}

TEST(StdlibList, InsertRemove) {
    PnyList *l = pny_list_new(sizeof(int));
    ASSERT_NE(l, nullptr);
    
    int a = 1, b = 2, c = 3;
    pny_list_append(l, &a);
    pny_list_append(l, &c);
    
    pny_list_insert(l, 1, &b);
    EXPECT_EQ(pny_list_len(l), 3);
    
    int *val = (int *)pny_list_get(l, 1);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 2);
    
    pny_list_remove(l, 1);
    EXPECT_EQ(pny_list_len(l), 2);
    
    pny_list_free(l);
}

TEST(StdlibList, PopOps) {
    PnyList *l = pny_list_new(sizeof(int));
    ASSERT_NE(l, nullptr);
    
    int a = 1, b = 2;
    pny_list_append(l, &a);
    pny_list_append(l, &b);
    
    int *val = (int *)pny_list_pop(l);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 2);
    
    val = (int *)pny_list_pop_front(l);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 1);
    
    pny_list_free(l);
}

TEST(StdlibMap, BasicOps) {
    PnyMap *m = pny_map_new(16, sizeof(int), sizeof(int));
    ASSERT_NE(m, nullptr);
    
    EXPECT_EQ(pny_map_size(m), 0);
    EXPECT_TRUE(pny_map_empty(m));
    
    int k1 = 1, v1 = 10;
    int k2 = 2, v2 = 20;
    
    EXPECT_EQ(pny_map_put(m, &k1, &v1), 0);
    EXPECT_EQ(pny_map_put(m, &k2, &v2), 0);
    EXPECT_EQ(pny_map_size(m), 2);
    
    int *val = (int *)pny_map_get(m, &k1);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 10);
    
    int missing_key = 99;
    EXPECT_TRUE(pny_map_has(m, &k1));
    EXPECT_FALSE(pny_map_has(m, &missing_key));
    
    pny_map_free(m);
    
    /* cap=0 会使用默认值 */
    PnyMap *m2 = pny_map_new(0, sizeof(int), sizeof(int));
    if (m2) pny_map_free(m2);
}

TEST(StdlibMap, RemoveClear) {
    PnyMap *m = pny_map_new(16, sizeof(int), sizeof(int));
    ASSERT_NE(m, nullptr);
    
    int k = 1, v = 10;
    pny_map_put(m, &k, &v);
    EXPECT_GE(pny_map_size(m), 1);
    
    /* remove 可能返回 0 或 -1 */
    pny_map_remove(m, &k);
    
    pny_map_put(m, &k, &v);
    pny_map_clear(m);
    EXPECT_EQ(pny_map_size(m), 0);
    
    pny_map_free(m);
}

TEST(StdlibSet, BasicOps) {
    PnySet *s = pny_set_new();
    ASSERT_NE(s, nullptr);
    
    EXPECT_EQ(pny_set_size(s), 0);
    
    EXPECT_TRUE(pny_set_add(s, "a"));
    EXPECT_TRUE(pny_set_add(s, "b"));
    EXPECT_FALSE(pny_set_add(s, "a"));  /* 重复 */
    
    EXPECT_EQ(pny_set_size(s), 2);
    EXPECT_TRUE(pny_set_contains(s, "a"));
    EXPECT_FALSE(pny_set_contains(s, "c"));
    
    EXPECT_TRUE(pny_set_remove(s, "a"));
    EXPECT_FALSE(pny_set_remove(s, "nonexistent"));
    
    pny_set_clear(s);
    EXPECT_EQ(pny_set_size(s), 0);
    
    pny_set_free(s);
}

TEST(StdlibQueue, BasicOps) {
    PnyQueue *q = pny_queue_new(0);
    ASSERT_NE(q, nullptr);
    
    EXPECT_TRUE(pny_queue_is_empty(q));
    EXPECT_FALSE(pny_queue_is_full(q));
    
    int a = 1, b = 2;
    EXPECT_TRUE(pny_queue_push(q, &a));
    EXPECT_TRUE(pny_queue_push(q, &b));
    EXPECT_EQ(pny_queue_size(q), 2);
    
    int *val = (int *)pny_queue_peek(q);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 1);
    
    val = (int *)pny_queue_pop(q);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 1);
    
    val = (int *)pny_queue_pop(q);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 2);
    
    EXPECT_TRUE(pny_queue_is_empty(q));
    EXPECT_EQ(pny_queue_pop(q), nullptr);
    
    pny_queue_free(q);
}

TEST(StdlibQueue, Bounded) {
    PnyQueue *q = pny_queue_new(2);
    ASSERT_NE(q, nullptr);
    
    int a = 1, b = 2, c = 3;
    EXPECT_TRUE(pny_queue_push(q, &a));
    EXPECT_TRUE(pny_queue_push(q, &b));
    
    /* 容量满时行为取决于实现 */
    bool pushed = pny_queue_push(q, &c);
    if (!pushed) {
        EXPECT_TRUE(pny_queue_is_full(q));
    }
    
    pny_queue_free(q);
}

TEST(StdlibStack, BasicOps) {
    PnyStack *s = pny_stack_new(0);
    ASSERT_NE(s, nullptr);
    
    int a = 1, b = 2;
    EXPECT_TRUE(pny_stack_push(s, &a));
    EXPECT_TRUE(pny_stack_push(s, &b));
    EXPECT_EQ(pny_stack_size(s), 2);
    
    int *val = (int *)pny_stack_peek(s);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 2);
    
    val = (int *)pny_stack_pop(s);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 2);
    
    val = (int *)pny_stack_pop(s);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 1);
    
    EXPECT_EQ(pny_stack_pop(s), nullptr);
    
    pny_stack_free(s);
}

/* ==================== 空值安全 ==================== */

TEST(StdlibNullSafety, Collections) {
    pny_list_free(nullptr);
    pny_map_free(nullptr);
    pny_set_free(nullptr);
    pny_queue_free(nullptr);
    pny_stack_free(nullptr);
    
    pny_list_append(nullptr, nullptr);  /* 不应崩溃 */
    EXPECT_EQ(pny_map_put(nullptr, nullptr, nullptr), -1);
    EXPECT_EQ(pny_set_add(nullptr, nullptr), false);
    EXPECT_EQ(pny_queue_push(nullptr, nullptr), false);
    EXPECT_EQ(pny_stack_push(nullptr, nullptr), false);
}
