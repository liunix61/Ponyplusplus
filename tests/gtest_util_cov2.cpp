#include <gtest/gtest.h>
#include <ponypp.h>
#include <ponypp/util.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

/* ==================== s_malloc/s_free ==================== */

TEST(UtilCov2, MallocZero) {
    char *p = s_malloc(0);
    /* 可能返回 NULL 或有效指针 */
    s_free(p);
}

TEST(UtilCov2, MallocLarge) {
    char *p = s_malloc(1024 * 1024);
    EXPECT_NE(p, nullptr);
    s_free(p);
}

TEST(UtilCov2, FreeNull) {
    s_free(nullptr);  /* 不崩溃 */
}

/* ==================== s_strdup/s_strndup ==================== */

TEST(UtilCov2, StrdupEmpty) {
    char *s = s_strdup("");
    EXPECT_NE(s, nullptr);
    EXPECT_STREQ(s, "");
    s_free(s);
}

TEST(UtilCov2, StrdupNull) {
    char *s = s_strdup(nullptr);
    EXPECT_EQ(s, nullptr);
}

TEST(UtilCov2, StrndupEmpty) {
    char *s = s_strndup("", 0);
    EXPECT_NE(s, nullptr);
    s_free(s);
}

TEST(UtilCov2, StrndupPartial) {
    char *s = s_strndup("hello world", 5);
    EXPECT_NE(s, nullptr);
    EXPECT_STREQ(s, "hello");
    s_free(s);
}

TEST(UtilCov2, StrndupLongerThanStr) {
    char *s = s_strndup("hi", 100);
    EXPECT_NE(s, nullptr);
    EXPECT_STREQ(s, "hi");
    s_free(s);
}

/* ==================== s_realloc ==================== */

TEST(UtilCov2, ReallocNull) {
    char *p = (char*)s_realloc(nullptr, 100);
    EXPECT_NE(p, nullptr);
    s_free(p);
}

TEST(UtilCov2, ReallocGrow) {
    char *p = s_malloc(10);
    strcpy(p, "hello");
    p = (char*)s_realloc(p, 100);
    EXPECT_NE(p, nullptr);
    EXPECT_STREQ(p, "hello");
    s_free(p);
}

TEST(UtilCov2, ReallocShrink) {
    char *p = s_malloc(100);
    strcpy(p, "hello");
    p = (char*)s_realloc(p, 10);
    EXPECT_NE(p, nullptr);
    EXPECT_STREQ(p, "hello");
    s_free(p);
}

/* ==================== s_strlen/s_strcmp ==================== */

TEST(UtilCov2, StrlenEmpty) {
    EXPECT_EQ(s_strlen(""), 0);
}

TEST(UtilCov2, StrlenNull) {
    EXPECT_EQ(s_strlen(nullptr), 0);
}

TEST(UtilCov2, StrcmpEqual) {
    EXPECT_EQ(s_strcmp("hello", "hello"), 0);
}

TEST(UtilCov2, StrcmpLess) {
    EXPECT_LT(s_strcmp("abc", "abd"), 0);
}

TEST(UtilCov2, StrcmpGreater) {
    EXPECT_GT(s_strcmp("abd", "abc"), 0);
}

TEST(UtilCov2, StrcmpNull) {
    EXPECT_EQ(s_strcmp(nullptr, nullptr), 0);
}

/* ==================== s_memcpy/s_memset ==================== */

TEST(UtilCov2, MemcpyBasic) {
    char src[] = "hello";
    char dst[10];
    s_memcpy(dst, src, 6);
    EXPECT_STREQ(dst, "hello");
}

TEST(UtilCov2, MemsetBasic) {
    char buf[10];
    s_memset(buf, 'A', 10);
    EXPECT_EQ(buf[0], 'A');
    EXPECT_EQ(buf[9], 'A');
}

/* ==================== s_strcpy/s_strcat ==================== */

TEST(UtilCov2, StrcpyBasic) {
    char dst[10];
    s_strcpy(dst, "hello");
    EXPECT_STREQ(dst, "hello");
}

TEST(UtilCov2, StrcatBasic) {
    char dst[20] = "hello";
    s_strcat(dst, " world");
    EXPECT_STREQ(dst, "hello world");
}

TEST(UtilCov2, StrlcpyBasic) {
    char dst[10];
    size_t n = s_strlcpy(dst, "hello", 10);
    EXPECT_EQ(n, 5);
    EXPECT_STREQ(dst, "hello");
}

TEST(UtilCov2, StrlcpyTruncated) {
    char dst[3];
    size_t n = s_strlcpy(dst, "hello", 3);
    EXPECT_EQ(n, 5);  /* 返回源串长度 */
    EXPECT_STREQ(dst, "he");
}

/* ==================== s_file_read/s_file_write ==================== */

TEST(UtilCov2, FileWriteRead) {
    const char *path = "/tmp/test_util_cov2.txt";
    const char *data = "hello world";
    int ret = s_file_write(path, data, strlen(data));
    EXPECT_EQ(ret, 0);
    char *read = s_file_read(path);
    EXPECT_NE(read, nullptr);
    EXPECT_STREQ(read, data);
    s_free(read);
    unlink(path);
}

TEST(UtilCov2, FileReadNonexistent) {
    char *data = s_file_read("/nonexistent/file.txt");
    EXPECT_EQ(data, nullptr);
}

TEST(UtilCov2, FileWriteNullPath) {
    EXPECT_NE(s_file_write(nullptr, "data", 4), 0);
}



/* ==================== s_resolve_output ==================== */

TEST(UtilCov2, ResolveOutputNull) {
    char *result = s_resolve_output(nullptr, ".wasm");
    s_free(result);
}

TEST(UtilCov2, ResolveOutputWithExt) {
    char *result = s_resolve_output("test", ".wasm");
    EXPECT_NE(result, nullptr);
    s_free(result);
}

TEST(UtilCov2, ResolveOutputNullExt) {
    char *result = s_resolve_output("test", nullptr);
    EXPECT_NE(result, nullptr);
    s_free(result);
}

/* ==================== token_type_name ==================== */

TEST(UtilCov2, TokenTypeNames) {
    EXPECT_NE(token_type_name(TK_EOF), nullptr);
    EXPECT_NE(token_type_name(TK_IDENT), nullptr);
    EXPECT_NE(token_type_name(TK_INT), nullptr);
}

/* ==================== capability_kind_name ==================== */

TEST(UtilCov2, CapabilityKindNames) {
    EXPECT_NE(capability_kind_name(CAP_ISO), nullptr);
    EXPECT_NE(capability_kind_name(CAP_TRN), nullptr);
    EXPECT_NE(capability_kind_name(CAP_REF), nullptr);
    EXPECT_NE(capability_kind_name(CAP_VAL), nullptr);
    EXPECT_NE(capability_kind_name(CAP_BOX), nullptr);
    EXPECT_NE(capability_kind_name(CAP_TAG), nullptr);
}

/* ==================== type_kind_name ==================== */

TEST(UtilCov2, TypeKindNames) {
    EXPECT_NE(type_kind_name(TYPE_UNKNOWN), nullptr);
    EXPECT_NE(type_kind_name(TYPE_INT64), nullptr);
    EXPECT_NE(type_kind_name(TYPE_INT32), nullptr);
    EXPECT_NE(type_kind_name(TYPE_UINT64), nullptr);
    EXPECT_NE(type_kind_name(TYPE_UINT32), nullptr);
    EXPECT_NE(type_kind_name(TYPE_UINT8), nullptr);
    EXPECT_NE(type_kind_name(TYPE_FLOAT64), nullptr);
    EXPECT_NE(type_kind_name(TYPE_FLOAT32), nullptr);
    EXPECT_NE(type_kind_name(TYPE_STRING), nullptr);
    EXPECT_NE(type_kind_name(TYPE_BOOL), nullptr);
    EXPECT_NE(type_kind_name(TYPE_NONE), nullptr);
    EXPECT_NE(type_kind_name(TYPE_ANY), nullptr);
}
