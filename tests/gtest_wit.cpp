#include <gtest/gtest.h>
#include <cstdio>
#include "gtest_helpers.h"
#include "ponypp/wit.h"

TEST(Wit, GeneratesFile) {
    const char* src = "actor Counter { var count: U64 = 0\n  be increment() => {} }";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(wit_write_program(ast, "/tmp/ponypp_test.wit"), 0);
    FILE* f = fopen("/tmp/ponypp_test.wit", "r");
    ASSERT_NE(f, nullptr);
    char buf[2048] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    EXPECT_GT(n, 0);
    EXPECT_NE(std::strstr(buf, "package ponypp"), nullptr);
    EXPECT_NE(std::strstr(buf, "interface"), nullptr);
    EXPECT_NE(std::strstr(buf, "world"), nullptr);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_test.wit");
}

TEST(Wit, EmptyProgram) {
    ASTNode* ast = parse_to_ast("");
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(wit_write_program(ast, "/tmp/ponypp_empty.wit"), 0);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_empty.wit");
}
