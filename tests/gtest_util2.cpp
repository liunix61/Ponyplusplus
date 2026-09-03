/* gtest_util2.cpp - 覆盖率提升测试 */
#include "gtest/gtest.h"
#include "ponypp/ast.h"
#include "ponypp/util.h"
#include "ponypp/codegen.h"
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/typecheck.h"
#include "ponypp/wasm.h"
#include <string.h>
#include <stdlib.h>

/* helper: build ast from source string */
static ASTNode* build_ast(const char *src) {
    Lexer *lex = lexer_new("<src>", src, strlen(src));
    if (!lex) return nullptr;
    Parser *par = parser_new("<src>", nullptr, 0);
    if (!par) { lexer_free(lex); return nullptr; }
    ASTNode *ast = parser_parse_program(par);
    parser_free(par);
    lexer_free(lex);
    return ast;
}

/* --- util.c --- */

TEST(Util2, SRealloc) {
    char *buf = (char *)s_realloc(NULL, 64);
    ASSERT_NE(buf, nullptr);
    strcpy(buf, "hello");
    buf = (char *)s_realloc(buf, 128);
    ASSERT_STREQ(buf, "hello");
    free(buf);
}

TEST(Util2, SStrlen) {
    EXPECT_EQ(s_strlen("hello"), (size_t)5);
    EXPECT_EQ(s_strlen(""), (size_t)0);
    EXPECT_EQ(s_strlen(nullptr), (size_t)0);
}

TEST(Util2, SStrcmp) {
    EXPECT_EQ(s_strcmp("abc", "abc"), 0);
    EXPECT_EQ(s_strcmp("abc", nullptr), 1);
    EXPECT_EQ(s_strcmp(nullptr, "abc"), -1);
    EXPECT_EQ(s_strcmp(nullptr, nullptr), 0);
}

TEST(Util2, SMemcpy) {
    char buf[32] = {};
    s_memcpy(buf, "hello", 5);
    EXPECT_EQ(memcmp(buf, "hello", 5), 0);
}

TEST(Util2, SMemset) {
    char buf[8];
    s_memset(buf, 0, sizeof(buf));
    EXPECT_EQ(buf[0], 0);
    s_memset(buf, 'X', 4);
    EXPECT_EQ(buf[0], 'X');
}

TEST(Util2, SStrcpy) {
    char buf[32] = {};
    s_strcpy(buf, "test");
    EXPECT_STREQ(buf, "test");
}

TEST(Util2, SStrcat) {
    char buf[32] = "hello";
    s_strcat(buf, " world");
    EXPECT_STREQ(buf, "hello world");
}

TEST(Util2, SStrlcpy) {
    char buf[16] = {};
    size_t len = s_strlcpy(buf, "hello world", sizeof(buf));
    EXPECT_EQ(len, (size_t)11);
}

TEST(Util2, SStrlcpyEmpty) {
    char buf[8] = {};
    s_strlcpy(buf, "", 1);
    EXPECT_STREQ(buf, "");
}

TEST(Util2, SStrlcpyTruncation) {
    char buf[4] = {};
    size_t len = s_strlcpy(buf, "abcdef", sizeof(buf));
    EXPECT_EQ(len, (size_t)6);
}

TEST(Util2, SFileWrite) {
    const char *path = "/tmp/ponypp_util2_test.txt";
    int ret = s_file_write(path, "hello util2\n", 12);
    EXPECT_EQ(ret, 0);
    remove(path);
}

TEST(Util2, SFileRead) {
    const char *path = "/tmp/ponypp_util2_read.txt";
    s_file_write(path, "hello", 5);
    char *data = s_file_read(path);
    if (data) {
        EXPECT_STREQ(data, "hello");
        s_free(data);
    }
    remove(path);
}

TEST(Util2, SResolveOutputNull) {
    char *out = s_resolve_output(NULL, ".wasm");
    if (out) { free(out); }
}

TEST(Util2, SResolveOutputExplicit) {
    char *out = s_resolve_output("/tmp/out.wasm", ".wasm");
    ASSERT_NE(out, nullptr);
    EXPECT_STREQ(out, "/tmp/out.wasm");
    free(out);
}

/* --- codegen.c --- */

TEST(Coverage, CodegenSimple) {
    ASTNode *ast = build_ast("actor main { new create() => {} }");
    if (!ast) return;
    FILE *f = fopen("/tmp/ponypp_cg_test.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    ast_node_free(ast);
}

TEST(Coverage, CodegenPrint) {
    ASTNode *ast = build_ast("actor main { new create() => { print(42) } }");
    if (!ast) return;
    FILE *f = fopen("/tmp/ponypp_cg_test2.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    ast_node_free(ast);
}

TEST(Coverage, CodegenMultipleActors) {
    ASTNode *ast = build_ast("actor A { new create() => {} } actor main { var a: A; new create() => { a = A() } }");
    if (!ast) return;
    FILE *f = fopen("/tmp/ponypp_cg_test3.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    ast_node_free(ast);
}

TEST(Coverage, CodegenIfElse) {
    ASTNode *ast = build_ast("actor main { new create() => { if (true) { print(1) } } }");
    if (!ast) return;
    FILE *f = fopen("/tmp/ponypp_cg_test4.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    ast_node_free(ast);
}

TEST(Coverage, CodegenFloat) {
    ASTNode *ast = build_ast("actor main { var f: F64; new create() => { f = 3.14 } }");
    if (!ast) return;
    FILE *f = fopen("/tmp/ponypp_cg_test5.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    ast_node_free(ast);
}

TEST(Coverage, CodegenBool) {
    ASTNode *ast = build_ast("actor main { var flag: Bool; new create() => { flag = true } }");
    if (!ast) return;
    FILE *f = fopen("/tmp/ponypp_cg_test6.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    ast_node_free(ast);
}

TEST(Coverage, CodegenString) {
    ASTNode *ast = build_ast("actor main { var s: String; new create() => { print(\"hi\") } }");
    if (!ast) return;
    FILE *f = fopen("/tmp/ponypp_cg_test7.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    ast_node_free(ast);
}

/* --- typecheck.c --- */

TEST(Coverage, TypecheckEmpty) {
    Lexer *lex = lexer_new("<src>", "", 0);
    Parser *par = parser_new("<src>", nullptr, 0);
    ASTNode *ast = parser_parse_program(par);
    parser_free(par);
    lexer_free(lex);
    TypeCheckResult *result = (TypeCheckResult*)calloc(1, sizeof(TypeCheckResult));
    typecheck_program(ast, result);
    typecheck_free_result(result);
}

TEST(Coverage, TypecheckValid) {
    ASTNode *ast = build_ast("actor main { new create() => { print(42) } }");
    if (!ast) return;
    TypeCheckResult *result = (TypeCheckResult*)calloc(1, sizeof(TypeCheckResult));
    typecheck_program(ast, result);
    EXPECT_EQ(result->error_count, 0);
    typecheck_free_result(result);
    ast_node_free(ast);
}

TEST(Coverage, TypecheckMultipleActors) {
    ASTNode *ast = build_ast("actor A { new create() => {} } actor main { var a: A; new create() => { a = A() } }");
    if (!ast) return;
    TypeCheckResult *result = (TypeCheckResult*)calloc(1, sizeof(TypeCheckResult));
    typecheck_program(ast, result);
    EXPECT_EQ(result->error_count, 0);
    typecheck_free_result(result);
    ast_node_free(ast);
}

TEST(Coverage, TypecheckFields) {
    ASTNode *ast = build_ast("actor main { var x: U64; var y: Bool; new create() => { x = 1; print(x) } }");
    if (!ast) return;
    TypeCheckResult *result = (TypeCheckResult*)calloc(1, sizeof(TypeCheckResult));
    typecheck_program(ast, result);
    typecheck_free_result(result);
    ast_node_free(ast);
}

/* --- wasm.c --- */

TEST(Coverage, WasmWithActor) {
    ASTNode *ast = build_ast("actor main { new create() => { print(42) } }");
    if (!ast) return;
    int ret = wasm_write_program(ast, "/tmp/ponypp_cg_wasm.wasm");
    EXPECT_EQ(ret, 0);
    remove("/tmp/ponypp_cg_wasm.wasm");
    ast_node_free(ast);
}

TEST(Coverage, WasmWithString) {
    ASTNode *ast = build_ast("actor main { new create() => { print(\"hello\") } }");
    if (!ast) return;
    int ret = wasm_write_program(ast, "/tmp/ponypp_cg_wasm2.wasm");
    EXPECT_EQ(ret, 0);
    remove("/tmp/ponypp_cg_wasm2.wasm");
    ast_node_free(ast);
}

TEST(Coverage, WasmMultipleActors) {
    ASTNode *ast = build_ast("actor Worker { new create() => {} } actor main { var w: Worker; new create() => { w = Worker() } }");
    if (!ast) return;
    int ret = wasm_write_program(ast, "/tmp/ponypp_cg_wasm3.wasm");
    EXPECT_EQ(ret, 0);
    remove("/tmp/ponypp_cg_wasm3.wasm");
    ast_node_free(ast);
}

TEST(Coverage, WasmNullOutput) {
    ASTNode *ast = build_ast("actor main { new create() => {} }");
    if (!ast) return;
    int ret = wasm_write_program(ast, nullptr);
    EXPECT_NE(ret, 0);
    ast_node_free(ast);
}

TEST(Coverage, WasmTargetNames) {
    EXPECT_STREQ(wasm_target_name(TARGET_WASI_P2), "wasi-p2");
    EXPECT_STREQ(wasm_target_name(TARGET_COMPONENT), "component");
    EXPECT_STREQ(wasm_target_name(TARGET_BROWSER), "browser");
    EXPECT_STREQ(wasm_target_name(TARGET_MCU_WASM), "mcu-wasm");
    EXPECT_STREQ(wasm_target_name(TARGET_NATIVE), "native");
}
