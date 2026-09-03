#include <gtest/gtest.h>
#include <cstdio>
#include "gtest_helpers.h"
#include "ponypp/codegen.h"

TEST(Codegen, GeneratesC) {
    const char* src = "actor main { be run() => { print(\"hi\") } }";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_gen.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    ASSERT_NE(cg, nullptr);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);

    FILE* rf = fopen("/tmp/ponypp_gen.c", "r");
    ASSERT_NE(rf, nullptr);
    char buf[8192] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
    fclose(rf);
    EXPECT_GT(n, 0);
    EXPECT_NE(std::strstr(buf, "typedef struct"), nullptr);
    EXPECT_NE(std::strstr(buf, "int main"), nullptr);
    EXPECT_NE(std::strstr(buf, "PnyRuntime"), nullptr);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen.c");
}

TEST(Codegen, EmptyProgram) {
    ASTNode* ast = parse_to_ast("");
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_empty_gen.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    ASSERT_NE(cg, nullptr);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_empty_gen.c");
}

TEST(Codegen, ActorWithFields) {
    const char* src =
        "actor Counter {\n"
        "  var count: U64 = 0\n"
        "  be run() => {}\n"
        "}";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_fields.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    ASSERT_NE(cg, nullptr);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_fields.c");
}
