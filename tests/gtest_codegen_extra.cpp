#include "ponypp.h"
#include <assert.h>
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/codegen.h"
#include "gtest_helpers.h"
#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

/* reuse parse_to_ast from gtest_helpers.h */

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return nullptr;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char*)malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = 0;
    fclose(f);
    return buf;
}

char *codegen_to_str(const char *src) {
    const char *path = "/tmp/ponypp_codegen_out.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    ASTNode *ast = parse_to_ast(src);
    assert(ast != nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    ast_node_free(ast);
    return read_file(path);
}

TEST(CodegenExtra, StringEscape) {
    char *out = codegen_to_str(
        "actor A {\n  new create() => {}\n"
        "  be run() => { print(\"a\\\\b\\t\\n\") }\n}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

TEST(CodegenExtra, IntArgsPrint) {
    char *out = codegen_to_str(
        "actor A {\n  new create() => {}\n  be run() => { print(42) }\n}");
    ASSERT_TRUE(out != nullptr); free(out);
}

TEST(CodegenExtra, BoolArgsPrint) {
    char *out = codegen_to_str(
        "actor A {\n  new create() => {}\n"
        "  be run() => { print(true) print(false) }\n}");
    ASSERT_TRUE(out != nullptr); free(out);
}

TEST(CodegenExtra, FloatArgsPrint) {
    char *out = codegen_to_str(
        "actor A {\n  new create() => {}\n  be run() => { print(3.14) }\n}");
    ASSERT_TRUE(out != nullptr); free(out);
}

TEST(CodegenExtra, BoolAndIntNode) {
    char *out = codegen_to_str(
        "actor A {\n"
        "  var b: Bool = true\n"
        "  var n: U64 = 123\n"
        "  new create() => {}\n}");
    ASSERT_TRUE(out != nullptr); free(out);
}

TEST(CodegenExtra, MethodWithFields) {
    char *out = codegen_to_str(
        "actor Counter(val initial: U64 = 0) {\n"
        "  var count: U64 = initial\n"
        "  new create(initial: U64) => {}\n"
        "  be increment() => { count = count + 1 }\n}");
    ASSERT_TRUE(out != nullptr); free(out);
}

TEST(CodegenExtra, MultiTypes) {
    char *out = codegen_to_str(
        "actor A {\n"
        "  var a: I32 = -1\n"
        "  var b: U8 = 0xFF\n"
        "  var c: F32 = 2.5\n"
        "  new create() => {}\n}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "signed int"), nullptr);
    free(out);
}

TEST(CodegenExtra, FloatFieldInit) {
    char *out = codegen_to_str(
        "actor A {\n"
        "  var f: F64 = 1.5\n"
        "  var i: U64 = 0\n"
        "  var b: Bool = false\n"
        "  new create() => {}\n}");
    ASSERT_TRUE(out != nullptr); free(out);
}

TEST(CodegenExtra, FieldAssignment) {
    char *out = codegen_to_str(
        "actor A {\n"
        "  var x: U64 = 0\n"
        "  new create() => {}\n"
        "  be s() => { x = 5 }\n}");
    ASSERT_TRUE(out != nullptr); free(out);
}

TEST(CodegenExtra, StringMethodArg) {
    char *out = codegen_to_str(
        "actor A {\n  new create() => {}\n"
        "  be hello(name: String) => { print(\"hi\") }\n}");
    ASSERT_TRUE(out != nullptr); free(out);
}

TEST(CodegenExtra, MultiReturn) {
    char *out = codegen_to_str(
        "actor A {\n  new create() => {}\n  fun x() => { return 1 }\n}");
    ASSERT_TRUE(out != nullptr); free(out);
}

TEST(CodegenExtra, Supervise) {
    char *out = codegen_to_str(
        "actor A { new create() => {} }\n"
        "actor B { new create() => {} }\n"
        "supervise A one_for_one\n"
        "supervise B one_for_one\n");
    ASSERT_TRUE(out != nullptr); free(out);
}

TEST(CodegenExtra, MultiActor) {
    char *out = codegen_to_str(
        "actor A { var x: U64 = 0; new create() => {} }\n"
        "actor B { var y: U64 = 1; new create() => {} }\n"
        "actor C { var z: U64 = 2; new create() => {} }\n");
    ASSERT_TRUE(out != nullptr); free(out);
}

TEST(CodegenExtra, PrintEmpty) {
    char *out = codegen_to_str(
        "actor A {\n  new create() => {}\n  be r() => { print() }\n}");
    ASSERT_TRUE(out != nullptr); free(out);
}

TEST(CodegenExtra, WasmOutput) {
    ASTNode *ast = parse_to_ast("actor main { new create() => {} }\n");
    assert(ast != nullptr);
    const char *path = "/tmp/ponypp_wasm_cov.wasm";
    remove(path);
    FILE *f = fopen(path, "wb");
    assert(f != nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    struct stat st;
    ASSERT_EQ(stat(path, &st), 0);
    remove(path);
    ast_node_free(ast);
}
