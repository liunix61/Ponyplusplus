#include <gtest/gtest.h>
#include <cstdio>
#include "gtest_helpers.h"
#include "ponypp/wit.h"

TEST(Wit, GeneratesFile) {
    const char* src = "actor Counter { var count: U64 = 0\n  be increment() => {} }";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(wit_write_program(ast, "/tmp/ponypp_test.wit", TARGET_WASI_P2), 0);
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
    EXPECT_EQ(wit_write_program(ast, "/tmp/ponypp_empty.wit", TARGET_WASI_P2), 0);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_empty.wit");
}

TEST(Wit, GenericTypeParams) {
    const char* src = "actor Queue[T, U] { var head: T\n  be push(x: T) => {}\n  fun pop(): U => 0 }\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(wit_write_program(ast, "/tmp/ponypp_generic.wit", TARGET_WASI_P2), 0);
    FILE* f = fopen("/tmp/ponypp_generic.wit", "r");
    ASSERT_NE(f, nullptr);
    char buf[2048] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    EXPECT_NE(std::strstr(buf, "interface Queue<T, U>"), nullptr);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_generic.wit");
}

TEST(Wit, MCUWorld) {
    const char* src = "actor Blinker { var led: U32\n  be toggle() => {} }\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(wit_write_program(ast, "/tmp/ponypp_mcu.wit", TARGET_MCU_WASM), 0);
    FILE* f = fopen("/tmp/ponypp_mcu.wit", "r");
    ASSERT_NE(f, nullptr);
    char buf[2048] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    EXPECT_NE(std::strstr(buf, "ponypp:mcu"), nullptr);
    EXPECT_NE(std::strstr(buf, "gpio"), nullptr);
    EXPECT_NE(std::strstr(buf, "uart"), nullptr);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_mcu.wit");
}

TEST(Wit, BrowserWorld) {
    const char* src = "actor App { var state: String\n  be init() => {} }\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(wit_write_program(ast, "/tmp/ponypp_browser.wit", TARGET_BROWSER), 0);
    FILE* f = fopen("/tmp/ponypp_browser.wit", "r");
    ASSERT_NE(f, nullptr);
    char buf[2048] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    EXPECT_NE(std::strstr(buf, "ponypp:browser"), nullptr);
    EXPECT_NE(std::strstr(buf, "js/glue"), nullptr);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_browser.wit");
}

TEST(Wit, Wasip3World) {
    const char* src = "actor Server { be start() => {} }\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(wit_write_program(ast, "/tmp/ponypp_p3.wit", TARGET_WASI_P3), 0);
    FILE* f = fopen("/tmp/ponypp_p3.wit", "r");
    ASSERT_NE(f, nullptr);
    char buf[2048] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    EXPECT_NE(std::strstr(buf, "ponypp:wasi-p3"), nullptr);
    EXPECT_NE(std::strstr(buf, "wasi:cli/terminal"), nullptr);
    EXPECT_NE(std::strstr(buf, "wasi:clocks"), nullptr);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_p3.wit");
}

TEST(Wit, GenericWithBrowserTarget) {
    const char* src = "actor Store[K, V] { var size: U64 = 0\n  be put(key: K, val: V) => {}\n  fun get(key: K): V => 0 }\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(wit_write_program(ast, "/tmp/ponypp_gen_browser.wit", TARGET_BROWSER), 0);
    FILE* f = fopen("/tmp/ponypp_gen_browser.wit", "r");
    ASSERT_NE(f, nullptr);
    char buf[2048] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    EXPECT_NE(std::strstr(buf, "interface Store<K, V>"), nullptr);
    EXPECT_NE(std::strstr(buf, "ponypp:browser"), nullptr);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_browser.wit");
}
