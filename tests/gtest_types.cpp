#include <gtest/gtest.h>
#include "ponypp/types.h"

TEST(Types, BuiltinTypes) {
    Type* t_int = type_int64();
    ASSERT_NE(t_int, nullptr);
    EXPECT_EQ(t_int->kind, TYPE_INT64);
    type_free(t_int);

    Type* t_str = type_string();
    ASSERT_NE(t_str, nullptr);
    type_free(t_str);

    Type* t_bool = type_bool();
    ASSERT_NE(t_bool, nullptr);
    type_free(t_bool);

    Type* t_none = type_none();
    ASSERT_NE(t_none, nullptr);
    type_free(t_none);

    Type* t_any = type_any();
    ASSERT_NE(t_any, nullptr);
    type_free(t_any);
}

TEST(Types, TypeEquals) {
    Type* a = type_int64();
    Type* b = type_int64();
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_TRUE(type_equals(a, b));
    type_free(a);
    type_free(b);
}

TEST(Types, TypeKindName) {
    ASSERT_NE(type_kind_name(TYPE_INT64), nullptr);
    ASSERT_NE(type_kind_name(TYPE_STRING), nullptr);
    ASSERT_NE(type_kind_name(TYPE_BOOL), nullptr);
}

TEST(Types, Generic) {
    Type* g = type_generic("T");
    ASSERT_NE(g, nullptr);
    type_free(g);
}

TEST(Types, Context) {
    TypeContext ctx = type_context_new();
    EXPECT_NE(ctx.unknown, nullptr);
    type_context_free(&ctx);
}

TEST(Types, Tuple) {
    Type* int_t = type_int64();
    Type* str_t = type_string();
    Type* types[2] = {int_t, str_t};
    Type* tuple = type_tuple(types, 2);
    ASSERT_NE(tuple, nullptr);
    EXPECT_EQ(tuple->tuple_count, 2);
    type_free(tuple);
    type_free(int_t);
    type_free(str_t);
}

TEST(Types, Array) {
    Type* elem = type_int64();
    Type* arr = type_array(elem);
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(arr->kind, TYPE_ARRAY);
    type_free(arr);
    type_free(elem);
}
