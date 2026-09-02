#include "ponypp/stdlib.h"
#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* ==================== I/O Tests ==================== */

TEST(StdlibIO, FileWriteRead) {
    const char *path = "/tmp/ponypp_test_io.txt";
    PnyFile *f = pny_file_open(path, FILE_MODE_WRITE);
    ASSERT_NE(f, nullptr);
    int r = pny_file_write(f, "Hello Pony++", 12);
    ASSERT_EQ(r, 0);
    r = pny_file_close(f);
    ASSERT_EQ(r, 0);
    ASSERT_EQ(pny_file_size(path), 12);

    f = pny_file_open(path, FILE_MODE_READ);
    ASSERT_NE(f, nullptr);
    char *content = pny_file_read_all(f);
    ASSERT_NE(content, nullptr);
    ASSERT_STREQ(content, "Hello Pony++");
    free(content);
    pny_file_close(f);

    f = pny_file_open(path, FILE_MODE_APPEND);
    ASSERT_NE(f, nullptr);
    pny_file_write(f, " World", 6);
    pny_file_close(f);
    ASSERT_EQ(pny_file_size(path), 18);

    f = pny_file_open(path, FILE_MODE_READ);
    content = pny_file_read_all(f);
    ASSERT_NE(content, nullptr);
    ASSERT_STREQ(content, "Hello Pony++ World");
    free(content);
    pny_file_close(f);
    pny_file_delete(path);
}

TEST(StdlibIO, FileReadLine) {
    const char *path = "/tmp/ponypp_test_lines.txt";
    PnyFile *f = pny_file_open(path, FILE_MODE_WRITE);
    ASSERT_NE(f, nullptr);
    pny_file_printf(f, "line1\nline2\nline3\n");
    pny_file_close(f);

    f = pny_file_open(path, FILE_MODE_READ);
    ASSERT_NE(f, nullptr);
    char *l1 = pny_file_read_line(f);
    ASSERT_NE(l1, nullptr);
    ASSERT_STREQ(l1, "line1");
    free(l1);
    char *l2 = pny_file_read_line(f);
    ASSERT_STREQ(l2, "line2");
    free(l2);
    pny_file_close(f);
    pny_file_delete(path);
}

TEST(StdlibIO, FileNotFound) {
    PnyFile *f = pny_file_open("/tmp/ponypp_nonexist_xyz.txt", FILE_MODE_READ);
    ASSERT_EQ(f, nullptr);
    ASSERT_EQ(pny_file_size("/tmp/ponypp_nonexist_xyz.txt"), -1);
}

TEST(StdlibIO, FilePrintf) {
    const char *path = "/tmp/ponypp_test_printf.txt";
    PnyFile *f = pny_file_open(path, FILE_MODE_WRITE);
    ASSERT_NE(f, nullptr);
    int r = pny_file_printf(f, "count=%d active=%s", 42, "true");
    ASSERT_GT(r, 0);
    pny_file_close(f);
    ASSERT_EQ(pny_file_size(path), 20);
    pny_file_delete(path);
}

TEST(StdlibIO, PathJoin) {
    char *r = pny_path_join("/tmp", "sub");
    ASSERT_NE(r, nullptr);
    ASSERT_STREQ(r, "/tmp/sub");
    free(r);

    r = pny_path_join("/tmp/", "sub");
    ASSERT_STREQ(r, "/tmp/sub");
    free(r);

    r = pny_path_join("/tmp", "/sub");
    ASSERT_STREQ(r, "/tmp/sub");
    free(r);

    ASSERT_FALSE(pny_path_exists("/tmp/ponypp_nonexist_xyz123"));
}

TEST(StdlibIO, ConsoleIO) {
    pny_stdout_print("test ");
    pny_stdout_println("line");
    pny_stdout_print_int(42);
    pny_stdout_print_bool(true);
    pny_stdout_print_bool(false);
}

/* ==================== String Tests ==================== */

TEST(StdlibString, BasicOps) {
    PnyString *s = pny_str_new("Hello");
    ASSERT_EQ(pny_str_len(s), 5);
    ASSERT_FALSE(pny_str_empty(s));
    ASSERT_EQ(pny_str_cmp_cstr(s, "Hello"), 0);
    ASSERT_LT(pny_str_cmp_cstr(s, "World"), 0);
    ASSERT_GT(pny_str_cmp_cstr(s, "A"), 0);
    pny_str_free(s);
}

TEST(StdlibString, EmptyAndNull) {
    PnyString *e = pny_str_new("");
    ASSERT_TRUE(pny_str_empty(e));
    ASSERT_EQ(pny_str_len(e), 0);
    ASSERT_EQ(pny_str_cmp(e, e), 0);
    pny_str_free(e);

    ASSERT_EQ(pny_str_len(nullptr), 0);
    ASSERT_TRUE(pny_str_empty(nullptr));
}

TEST(StdlibString, Cat) {
    PnyString *s = pny_str_new("Hello");
    pny_str_cat_cstr(s, ", ");
    PnyString *w = pny_str_new("World");
    pny_str_cat(s, w);
    ASSERT_EQ(pny_str_len(s), 12);
    ASSERT_EQ(pny_str_cmp_cstr(s, "Hello, World"), 0);
    pny_str_free(s);
    pny_str_free(w);
}

TEST(StdlibString, Slice) {
    PnyString *s = pny_str_new("Hello World");
    PnyString *sub = pny_str_slice(s, 6, 11);
    ASSERT_NE(sub, nullptr);
    ASSERT_EQ(pny_str_len(sub), 5);
    ASSERT_EQ(pny_str_cmp_cstr(sub, "World"), 0);
    pny_str_free(sub);

    PnyString *sub2 = pny_str_slice(s, 0, 5);
    ASSERT_EQ(pny_str_cmp_cstr(sub2, "Hello"), 0);
    pny_str_free(sub2);
    pny_str_free(s);
}

TEST(StdlibString, Contains) {
    PnyString *s = pny_str_new("Hello World");
    PnyString *sub = pny_str_new("World");
    ASSERT_TRUE(pny_str_contains(s, sub));
    PnyString *nx = pny_str_new("xyz");
    ASSERT_FALSE(pny_str_contains(s, nx));
    pny_str_free(s); pny_str_free(sub); pny_str_free(nx);
}

TEST(StdlibString, StartsWithEndsWith) {
    PnyString *s = pny_str_new("Hello World");
    PnyString *pre = pny_str_new("Hello");
    PnyString *suf = pny_str_new("World");
    ASSERT_TRUE(pny_str_starts_with(s, pre));
    ASSERT_TRUE(pny_str_ends_with(s, suf));
    PnyString *nx = pny_str_new("xyz");
    ASSERT_FALSE(pny_str_starts_with(s, nx));
    ASSERT_FALSE(pny_str_ends_with(s, nx));
    pny_str_free(s); pny_str_free(pre); pny_str_free(suf); pny_str_free(nx);
}

TEST(StdlibString, Replace) {
    PnyString *s = pny_str_new("Hello World World");
    PnyString *old_ = pny_str_new("World");
    PnyString *new_ = pny_str_new("Pony");
    PnyString *r = pny_str_replace(s, old_, new_);
    ASSERT_NE(r, nullptr);
    ASSERT_EQ(pny_str_cmp_cstr(r, "Hello Pony Pony"), 0);
    pny_str_free(r); pny_str_free(s); pny_str_free(old_); pny_str_free(new_);
}

TEST(StdlibString, UpperLowerTrim) {
    PnyString *s = pny_str_new("Hello World");
    PnyString *up = pny_str_to_upper(s);
    ASSERT_EQ(pny_str_cmp_cstr(up, "HELLO WORLD"), 0);
    pny_str_free(up);

    PnyString *lo = pny_str_to_lower(s);
    ASSERT_EQ(pny_str_cmp_cstr(lo, "hello world"), 0);
    pny_str_free(lo);
    pny_str_free(s);

    PnyString *t = pny_str_new("  hello  ");
    PnyString *tm = pny_str_trim(t);
    ASSERT_EQ(pny_str_cmp_cstr(tm, "hello"), 0);
    pny_str_free(tm); pny_str_free(t);
}

TEST(StdlibString, Format) {
    PnyString *s = pny_str_format("count=%d, pi=%.2f, ok=%s", 42, 3.14, "true");
    ASSERT_NE(s, nullptr);
    ASSERT_EQ(pny_str_cmp_cstr(s, "count=42, pi=3.14, ok=true"), 0);
    pny_str_free(s);
}

TEST(StdlibString, FromIntFloatBool) {
    PnyString *si = pny_str_from_int(42);
    ASSERT_EQ(pny_str_cmp_cstr(si, "42"), 0);
    pny_str_free(si);

    PnyString *sf = pny_str_from_float(3.14);
    ASSERT_TRUE(pny_str_starts_with(sf, pny_str_new("3.14")));
    pny_str_free(sf);

    PnyString *sb = pny_str_from_bool(true);
    ASSERT_EQ(pny_str_cmp_cstr(sb, "true"), 0);
    pny_str_free(sb);
}

TEST(StdlibString, ToIntFloatBool) {
    PnyString *si = pny_str_new("12345");
    ASSERT_EQ(pny_str_to_int(si), 12345);
    pny_str_free(si);

    PnyString *sf = pny_str_new("3.14");
    double v = pny_str_to_float(sf);
    ASSERT_TRUE(v > 3.1 && v < 3.2);
    pny_str_free(sf);

    ASSERT_TRUE(pny_str_to_bool(pny_str_new("true")));
    ASSERT_TRUE(pny_str_to_bool(pny_str_new("1")));
    ASSERT_FALSE(pny_str_to_bool(pny_str_new("false")));
    ASSERT_FALSE(pny_str_to_bool(pny_str_new("0")));
    ASSERT_FALSE(pny_str_to_bool(nullptr));
}

TEST(StdlibString, Dup) {
    PnyString *s = pny_str_new("dup test");
    PnyString *d = pny_str_dup(s);
    ASSERT_NE(d, s);
    ASSERT_EQ(pny_str_cmp(s, d), 0);
    pny_str_free(d);
    pny_str_free(s);
}

TEST(StdlibString, NewWithCap) {
    PnyString *s = pny_str_new_with(64);
    ASSERT_EQ(pny_str_len(s), 0);
    pny_str_cat_cstr(s, "growing");
    ASSERT_EQ(pny_str_len(s), 7);
    pny_str_free(s);
}

/* ==================== List Tests ==================== */

TEST(StdlibList, Basic) {
    PnyList *l = pny_list_new(sizeof(int));
    ASSERT_EQ(pny_list_len(l), 0);
    ASSERT_TRUE(pny_list_empty(l));

    int v1 = 10, v2 = 20, v3 = 30;
    pny_list_append(l, &v1);
    pny_list_append(l, &v2);
    pny_list_append(l, &v3);
    ASSERT_EQ(pny_list_len(l), 3);
    ASSERT_EQ(*(int*)pny_list_get(l, 1), 20);

    pny_list_prepend(l, &v1);
    ASSERT_EQ(pny_list_len(l), 4);
    ASSERT_EQ(*(int*)pny_list_get(l, 0), 10);
    pny_list_free(l);
}

TEST(StdlibList, PopSetRemove) {
    int v1 = 10, v2 = 20, v3 = 30;
    PnyList *l = pny_list_new(sizeof(int));
    pny_list_append(l, &v1);
    pny_list_append(l, &v2);
    pny_list_append(l, &v3);

    int *last = (int*)pny_list_pop(l);
    ASSERT_EQ(*last, 30);
    ASSERT_EQ(pny_list_len(l), 2);

    int nv = 99;
    pny_list_set(l, 0, &nv);
    ASSERT_EQ(*(int*)pny_list_get(l, 0), 99);

    pny_list_remove(l, 1);
    ASSERT_EQ(pny_list_len(l), 1);
    pny_list_free(l);
}

TEST(StdlibList, RemoveVal) {
    int v1 = 10, v2 = 20, v3 = 30;
    PnyList *l = pny_list_new(sizeof(int));
    pny_list_append(l, &v1);
    pny_list_append(l, &v2);
    pny_list_append(l, &v3);
    int target = 20;
    ASSERT_EQ(pny_list_remove_val(l, &target, [](const void *a, const void *b) {
        return *(const int*)a - *(const int*)b;
    }), 0);
    ASSERT_EQ(pny_list_len(l), 2);
    pny_list_free(l);
}

TEST(StdlibList, InsertAndSlice) {
    int v1 = 10, v2 = 20, v3 = 30, v4 = 15;
    PnyList *l = pny_list_new(sizeof(int));
    pny_list_append(l, &v1);
    pny_list_append(l, &v2);
    pny_list_append(l, &v3);
    pny_list_insert(l, 1, &v4);
    ASSERT_EQ(pny_list_len(l), 4);
    ASSERT_EQ(*(int*)pny_list_get(l, 1), 15);

    PnyList *sl = pny_list_slice(l, 1, 3);
    ASSERT_EQ(pny_list_len(sl), 2);
    ASSERT_EQ(*(int*)pny_list_get(sl, 0), 15);
    pny_list_free(sl);
    pny_list_free(l);
}

TEST(StdlibList, SortAndReverse) {
    int v1 = 30, v2 = 10, v3 = 20;
    PnyList *l = pny_list_new(sizeof(int));
    pny_list_append(l, &v1);
    pny_list_append(l, &v2);
    pny_list_append(l, &v3);
    pny_list_sort(l, [](const void *a, const void *b) {
        return *(const int*)a - *(const int*)b;
    });
    ASSERT_EQ(*(int*)pny_list_get(l, 0), 10);
    ASSERT_EQ(*(int*)pny_list_get(l, 1), 20);
    ASSERT_EQ(*(int*)pny_list_get(l, 2), 30);

    pny_list_reverse(l);
    ASSERT_EQ(*(int*)pny_list_get(l, 0), 30);
    pny_list_free(l);
}

TEST(StdlibList, IndexOf) {
    int v1 = 10, v2 = 20, v3 = 30;
    PnyList *l = pny_list_new(sizeof(int));
    pny_list_append(l, &v1);
    pny_list_append(l, &v2);
    pny_list_append(l, &v3);
    ASSERT_EQ(pny_list_index_of(l, &v2, [](const void *a, const void *b) {
        return *(const int*)a - *(const int*)b;
    }), 1);
    ASSERT_EQ(pny_list_index_of(l, &v1, nullptr), -1);
    pny_list_free(l);
}

TEST(StdlibList, OpaquePointer) {
    PnyList *l = pny_list_new(0);
    const char *a = "hello";
    const char *b = "world";
    pny_list_append(l, (void*)a);
    pny_list_append(l, (void*)b);
    const char *got = (const char*)pny_list_get(l, 1);
    ASSERT_STREQ(got, b);
    pny_list_free(l);
}

/* ==================== Map Tests ==================== */

TEST(StdlibMap, Basic) {
    PnyMap *m = pny_map_new(16, 0, 0);
    ASSERT_TRUE(pny_map_empty(m));
    ASSERT_EQ(pny_map_size(m), 0);

    int *key1 = (int*)malloc(sizeof(int)); *key1 = 1;
    int *val1 = (int*)malloc(sizeof(int)); *val1 = 100;
    int *key2 = (int*)malloc(sizeof(int)); *key2 = 2;
    int *val2 = (int*)malloc(sizeof(int)); *val2 = 200;
    ASSERT_EQ(pny_map_put(m, key1, val1), 0);
    ASSERT_EQ(pny_map_put(m, key2, val2), 0);
    ASSERT_EQ(pny_map_size(m), 2);

    void *v = pny_map_get(m, key1);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(*(int*)v, 100);
    v = pny_map_get(m, key2);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(*(int*)v, 200);

    int *key3 = (int*)malloc(sizeof(int)); *key3 = 3;
    ASSERT_FALSE(pny_map_has(m, key3));
    free(key3);
    pny_map_free(m);
}

TEST(StdlibMap, UpdateAndRemove) {
    PnyMap *m = pny_map_new(16, 0, 0);
    int *k1 = (int*)malloc(sizeof(int)); *k1 = 1;
    int *v1 = (int*)malloc(sizeof(int)); *v1 = 10;
    int *v1b = (int*)malloc(sizeof(int)); *v1b = 99;
    pny_map_put(m, k1, v1);
    pny_map_put(m, k1, v1b);
    ASSERT_EQ(*(int*)pny_map_get(m, k1), 99);
    ASSERT_EQ(pny_map_size(m), 1);

    ASSERT_EQ(pny_map_remove(m, k1), 0);
    ASSERT_EQ(pny_map_size(m), 0);
    ASSERT_EQ(pny_map_get(m, k1), nullptr);
    pny_map_free(m);
}

TEST(StdlibMap, Clear) {
    PnyMap *m = pny_map_new(8, 0, 0);
    for (int i = 0; i < 5; i++) {
        int *k = (int*)malloc(sizeof(int)); *k = i;
        int *v = (int*)malloc(sizeof(int)); *v = i;
        pny_map_put(m, k, v);
    }
    ASSERT_EQ(pny_map_size(m), 5);
    pny_map_clear(m);
    ASSERT_TRUE(pny_map_empty(m));
    ASSERT_EQ(pny_map_size(m), 0);
    pny_map_free(m);
}

/* ==================== Atomic Tests ==================== */

TEST(StdlibAtomic, StoreLoad) {
    PnyAtomicInt64 a;
    pny_atomic_store(&a, 42);
    ASSERT_EQ(pny_atomic_load(&a), 42);
}

TEST(StdlibAtomic, Add) {
    PnyAtomicInt64 a;
    pny_atomic_store(&a, 10);
    ASSERT_EQ(pny_atomic_add(&a, 5), 15);
    ASSERT_EQ(pny_atomic_add(&a, 3), 18);
}

TEST(StdlibAtomic, CAS) {
    PnyAtomicInt64 a;
    pny_atomic_store(&a, 10);
    ASSERT_EQ(pny_atomic_cas(&a, 10, 20), 10);
    ASSERT_EQ(pny_atomic_load(&a), 20);
    ASSERT_EQ(pny_atomic_cas(&a, 10, 30), 20); /* expected mismatch */
    ASSERT_EQ(pny_atomic_load(&a), 20);
}

/* ==================== Test Framework Tests ==================== */

static void test_pass_case(void) { }

TEST(StdlibTest, Suite) {
    PnyTestSuite *s = pny_test_suite_new("TestSuite");
    ASSERT_NE(s, nullptr);
    pny_test_suite_add(s, test_pass_case);
    pny_test_suite_add(s, test_pass_case);
    ASSERT_EQ(s->count, 2);
    pny_test_run_suite(s, false);
    ASSERT_EQ(s->report.total, 2);
    ASSERT_EQ(s->report.passed, 2);
    ASSERT_EQ(s->report.failed, 0);
    pny_test_suite_free(s);
}

TEST(StdlibTest, AssertHelpers) {
    /* These should produce no output on success */
    pny_assert_true(true, (AssertCtx){__LINE__, __FILE__, "true"});
    pny_assert_eq_int(1, 1, (AssertCtx){__LINE__, __FILE__, "1==1"});
    pny_assert_eq_str("a", "a", (AssertCtx){__LINE__, __FILE__, "str"});
    pny_assert_null(nullptr, (AssertCtx){__LINE__, __FILE__, "null"});
    pny_assert_not_null((void*)1, (AssertCtx){__LINE__, __FILE__, "nonnull"});
}

TEST(StdlibTest, AssertFailOutput) {
    pny_assert_true(false, (AssertCtx){__LINE__, __FILE__, "expected_fail"});
    pny_assert_eq_int(1, 2, (AssertCtx){__LINE__, __FILE__, "1!=2"});
    pny_assert_not_null(nullptr, (AssertCtx){__LINE__, __FILE__, "nonnull"});
}
