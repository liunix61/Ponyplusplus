#include "ponypp.h"
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/types.h"
#include "gtest_helpers.h"
#include <gtest/gtest.h>
#include <string.h>

TEST(TypeCheckExtra, TypeInt64) {
    Type *i = type_int64();
    ASSERT_TRUE(i != nullptr);
    type_free(i);
}

TEST(TypeCheckExtra, StringBool) {
    Type *s = type_string();
    ASSERT_TRUE(s != nullptr);
    ASSERT_EQ(s->kind, TYPE_STRING);
    ASSERT_TRUE(type_is_immutable(s));
    type_free(s);

    Type *b = type_bool();
    ASSERT_TRUE(b != nullptr);
    ASSERT_EQ(b->kind, TYPE_BOOL);
    type_free(b);
}

TEST(TypeCheckExtra, NoneAny) {
    Type *n = type_none();
    ASSERT_TRUE(n != nullptr);
    ASSERT_EQ(n->kind, TYPE_NONE);
    type_free(n);

    Type *a = type_any();
    ASSERT_TRUE(a != nullptr);
    ASSERT_EQ(a->kind, TYPE_ANY);
    type_free(a);
}

TEST(TypeCheckExtra, Generic) {
    Type *g = type_generic("T");
    ASSERT_TRUE(g != nullptr);
    ASSERT_STREQ(g->name, "T");
    type_free(g);
}

TEST(TypeCheckExtra, Tuple) {
    Type *i = type_int64();
    Type *s = type_string();
    Type *arr[] = {i, s};
    Type *t = type_tuple(arr, 2);
    ASSERT_TRUE(t != nullptr);
    ASSERT_EQ(t->tuple_count, 2);
    type_free(t); type_free(i); type_free(s);
}

TEST(TypeCheckExtra, Union) {
    Type *i = type_int64();
    Type *s = type_string();
    Type *arr2[] = {i, s};
    Type *u = type_union(arr2, 2);
    ASSERT_TRUE(u != nullptr);
    ASSERT_EQ(u->union_count, 2);
    type_free(u); type_free(i); type_free(s);
}

TEST(TypeCheckExtra, Array) {
    Type *i = type_int64();
    Type *a = type_array(i);
    ASSERT_TRUE(a != nullptr);
    ASSERT_EQ(a->kind, TYPE_ARRAY);
    ASSERT_EQ(a->array_element, i);
    type_free(a); type_free(i);
}

TEST(TypeCheckExtra, GenericWithArgs) {
    Type *base = type_generic("Box");
    Type *arg = type_int64();
    Type *arr3[] = {arg};
    Type *g = type_generic_with_args(base, arr3, 1);
    ASSERT_TRUE(g != nullptr);
    type_free(g); type_free(base); type_free(arg);
}

TEST(TypeCheckExtra, Sendable) {
    Type *i = type_int64();
    ASSERT_TRUE(type_is_sendable(i));
    Type *s = type_string();
    ASSERT_TRUE(type_is_sendable(s));
    type_free(i); type_free(s);
}

TEST(TypeCheckExtra, Context) {
    TypeContext ctx = type_context_new();
    ASSERT_TRUE(ctx.builtin_types != nullptr);
    type_context_free(&ctx);
}

TEST(TypeCheckExtra, Equals) {
    Type *a = type_int64();
    Type *b = type_int64();
    Type *s = type_string();
    ASSERT_TRUE(type_equals(a, b));
    ASSERT_FALSE(type_equals(a, s));
    type_free(a); type_free(b); type_free(s);
}

TEST(TypeCheckExtra, Release) {
    Type *t = type_int64();
    t->ref_count = 2;
    type_release(t);
    ASSERT_EQ(t->ref_count, 1);
    type_free(t);
}
