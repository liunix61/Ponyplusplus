#include <gtest/gtest.h>
#include <cstdio>
#include <cstdint>
#include "gtest_helpers.h"
#include "ponypp/wasm.h"

TEST(Wasm, GeneratesFile) {
    const char* src = "actor Main { be hello() => {} }";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(wasm_write_program(ast, "/tmp/ponypp_test.wasm"), 0);
    FILE* f = fopen("/tmp/ponypp_test.wasm", "rb");
    ASSERT_NE(f, nullptr);
    unsigned char magic[8] = {0};
    fread(magic, 1, 8, f);
    fclose(f);
    EXPECT_EQ(magic[0], 0x00);
    EXPECT_EQ(magic[1], 0x61);
    EXPECT_EQ(magic[2], 0x73);
    EXPECT_EQ(magic[3], 0x6d);
    EXPECT_EQ(magic[4], 0x01);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_test.wasm");
}

TEST(Wasm, TargetName) {
    ASSERT_NE(wasm_target_name((TargetKind)0), nullptr);
    ASSERT_NE(wasm_target_name((TargetKind)4), nullptr);
}

TEST(Wasm, NullOutput) {
    EXPECT_NE(wasm_write_program(nullptr, nullptr), 0);
}
