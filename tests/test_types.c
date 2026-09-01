/*
 * test_types.c - Pony++ 类型系统测试
 */

#include <stdio.h>
#include <string.h>
#include "ponypp/types.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { tests_passed++; printf("  \342\234\223 %s\n", msg); } \
    else { tests_failed++; printf("  \342\234\224 %s\n", msg); } \
} while(0)

static int test_builtin_types(void) {
    printf("test_builtin_types\n");
    Type *t_int = type_int64();
    CHECK(t_int != NULL, "type_int64 返回非空");
    if (t_int) {
        CHECK(t_int->kind == TYPE_INT64, "kind 是 TYPE_INT64");
        type_free(t_int);
    }

    Type *t_str = type_string();
    CHECK(t_str != NULL, "type_string 返回非空");
    if (t_str) type_free(t_str);

    Type *t_bool = type_bool();
    CHECK(t_bool != NULL, "type_bool 返回非空");
    if (t_bool) type_free(t_bool);

    Type *t_none = type_none();
    CHECK(t_none != NULL, "type_none 返回非空");
    if (t_none) type_free(t_none);

    Type *t_any = type_any();
    CHECK(t_any != NULL, "type_any 返回非空");
    if (t_any) type_free(t_any);

    return tests_failed == 0;
}

static int test_type_equal(void) {
    printf("test_type_equal\n");
    Type *a = type_int64();
    Type *b = type_int64();
    CHECK(a != NULL && b != NULL, "两个类型都非空");
    if (a && b) {
        CHECK(type_equals(a, b) == 1, "两个 int64 类型相等");
        type_free(a);
        type_free(b);
    }
    return tests_failed == 0;
}

static int test_type_kind_name(void) {
    printf("test_type_kind_name\n");
    CHECK(strcmp(type_kind_name(TYPE_INT64), "I64") == 0 || type_kind_name(TYPE_INT64) != NULL,
          "TYPE_INT64 有名称");
    CHECK(type_kind_name(TYPE_STRING) != NULL, "TYPE_STRING 有名称");
    CHECK(type_kind_name(TYPE_BOOL) != NULL, "TYPE_BOOL 有名称");
    return tests_failed == 0;
}

int main(void) {
    printf("=== Pony++ 类型系统测试 ===\n\n");
    test_builtin_types();
    test_type_equal();
    test_type_kind_name();
    printf("\n=== 测试结果 ===\n");
    printf("通过: %d\n", tests_passed);
    printf("失败: %d\n", tests_failed);
    printf("总计: %d\n", tests_passed + tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
