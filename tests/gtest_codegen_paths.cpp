/*
 * Codegen 路径覆盖测试
 */

#include <gtest/gtest.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp/codegen.h>
#include <ponypp/ast.h>
#include <ponypp.h>
#include <cstdio>
#include <cstring>

static bool compile_source(const char *src) {
    Lexer *lex = lexer_new("test.pny", src, strlen(src));
    if (!lex) return false;
    Token *tokens = nullptr;
    size_t count = 0;
    if (!lexer_lex_all(lex, &tokens, &count)) {
        lexer_free(lex);
        return false;
    }
    Parser *p = parser_new("test.pny", tokens, count);
    if (!p) { lexer_free(lex); return false; }
    ASTNode *ast = parser_parse_program(p);
    parser_free(p);
    lexer_free(lex);
    if (!ast) return false;
    
    FILE *out = tmpfile();
    if (!out) { ast_node_free(ast); return false; }
    Codegen *cg = codegen_new(out);
    if (!cg) { fclose(out); ast_node_free(ast); return false; }
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(out);
    ast_node_free(ast);
    return true;
}

TEST(CodegenPaths, EmptyActor) {
    EXPECT_TRUE(compile_source("actor Foo { }"));
}

TEST(CodegenPaths, ActorWithNew) {
    EXPECT_TRUE(compile_source("actor Foo {\n  new create() {\n  }\n}"));
}

TEST(CodegenPaths, ActorWithFields) {
    EXPECT_TRUE(compile_source("actor Foo {\n  var _x: U64\n  new create() {\n    _x = 0\n  }\n}"));
}

TEST(CodegenPaths, ActorWithBe) {
    EXPECT_TRUE(compile_source("actor Foo {\n  be greet() {\n  }\n}"));
}

TEST(CodegenPaths, ActorWithFun) {
    EXPECT_TRUE(compile_source("actor Foo {\n  fun add(a: U64, b: U64): U64 {\n    a + b\n  }\n}"));
}

TEST(CodegenPaths, ActorWithFunReturn) {
    EXPECT_TRUE(compile_source("actor Foo {\n  fun get(): U64 {\n    return 42\n  }\n}"));
}

TEST(CodegenPaths, IfStatement) {
    EXPECT_TRUE(compile_source("actor Foo {\n  fun check(x: U64): Bool {\n    if x > 10 then true else false end\n  }\n}"));
}

TEST(CodegenPaths, WhileLoop) {
    EXPECT_TRUE(compile_source("actor Foo {\n  fun loop() {\n    var i: U64 = 0\n    while i < 10 do\n      i = i + 1\n    end\n  }\n}"));
}

TEST(CodegenPaths, Arithmetic) {
    EXPECT_TRUE(compile_source("actor Foo {\n  fun calc(): U64 {\n    1 + 2 * 3 - 4 / 2\n  }\n}"));
}

TEST(CodegenPaths, Comparison) {
    EXPECT_TRUE(compile_source("actor Foo {\n  fun compare(a: U64, b: U64): Bool {\n    a > b\n  }\n}"));
}

TEST(CodegenPaths, LogicalOps) {
    EXPECT_TRUE(compile_source("actor Foo {\n  fun logic(a: Bool, b: Bool): Bool {\n    a and b\n  }\n}"));
}

TEST(CodegenPaths, UintTypes) {
    EXPECT_TRUE(compile_source("actor Foo {\n  var _a: U8\n  var _b: U16\n  var _c: U32\n  var _d: U64\n}"));
}

TEST(CodegenPaths, IntTypes) {
    EXPECT_TRUE(compile_source("actor Foo {\n  var _a: I8\n  var _b: I16\n  var _c: I32\n  var _d: I64\n}"));
}

TEST(CodegenPaths, FloatTypes) {
    EXPECT_TRUE(compile_source("actor Foo {\n  var _a: F32\n  var _b: F64\n}"));
}

TEST(CodegenPaths, StringLiteral) {
    EXPECT_TRUE(compile_source("actor Foo {\n  fun test(): String { \"hello\" }\n}"));
}

TEST(CodegenPaths, MultipleActors) {
    EXPECT_TRUE(compile_source("actor A {\n  be run() { }\n}\nactor B {\n  be run() { }\n}"));
}

TEST(CodegenPaths, BoolLiteral) {
    EXPECT_TRUE(compile_source("actor Foo {\n  fun test(): Bool { true }\n}"));
}

TEST(CodegenPaths, IntLiteral) {
    EXPECT_TRUE(compile_source("actor Foo {\n  fun test(): U64 { 42 }\n}"));
}

TEST(CodegenPaths, FloatLiteral) {
    EXPECT_TRUE(compile_source("actor Foo {\n  fun test(): F64 { 3.14 }\n}"));
}
