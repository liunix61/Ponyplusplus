#include <gtest/gtest.h>
#include "gtest_helpers.h"
#include "ponypp/typecheck.h"

TEST(TypeCheck, EmptyProgram) {
    TypeCheckResult* r = (TypeCheckResult*)calloc(1, sizeof(TypeCheckResult));
    ASSERT_NE(r, nullptr);
    EXPECT_NE(typecheck_program(nullptr, r), 0);
    EXPECT_EQ(r->ok, 0);
    EXPECT_EQ(r->error_count, 1);
    typecheck_free_result(r);
}

TEST(TypeCheck, ValidActor) {
    const char* src =
        "actor main {\n"
        "  var count: U64 = 0\n"
        "  be run() => {}\n"
        "}";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    TypeCheckResult* r = (TypeCheckResult*)calloc(1, sizeof(TypeCheckResult));
    EXPECT_EQ(typecheck_program(ast, r), 0);
    EXPECT_EQ(r->ok, 1);
    EXPECT_EQ(r->error_count, 0);
    typecheck_free_result(r);
    ast_node_free(ast);
}

TEST(TypeCheck, MultipleActors) {
    const char* src =
        "actor A { var x: U64 = 0 }\n"
        "actor B { var y: I64 = 42 }\n"
        "actor C { fun hello(): String => {} }";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    TypeCheckResult* r = (TypeCheckResult*)calloc(1, sizeof(TypeCheckResult));
    EXPECT_EQ(typecheck_program(ast, r), 0);
    typecheck_free_result(r);
    ast_node_free(ast);
}

TEST(TypeCheck, PrintCall) {
    const char* src =
        "actor main {\n"
        "  be run() => { print(\"hi\") }\n"
        "}";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    TypeCheckResult* r = (TypeCheckResult*)calloc(1, sizeof(TypeCheckResult));
    EXPECT_EQ(typecheck_program(ast, r), 0);
    typecheck_free_result(r);
    ast_node_free(ast);
}

TEST(TypeCheck, TypeCheckFree) {
    TypeCheckResult* r = (TypeCheckResult*)calloc(1, sizeof(TypeCheckResult));
    r->errors = (const char**)calloc(1, sizeof(char*));
    typecheck_free_result(r);
    EXPECT_TRUE(1);
}
