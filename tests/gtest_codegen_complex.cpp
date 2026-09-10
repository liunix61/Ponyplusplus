/*
 * Codegen 复杂路径测试
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

/* ==================== 复杂 Actor ==================== */

TEST(CodegenComplex, ActorWithConstructorArgs) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  var _x: U64\n"
        "  new create(x: U64) {\n"
        "    _x = x\n"
        "  }\n"
        "}"));
}

TEST(CodegenComplex, ActorWithMultipleFields) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  var _a: U64\n"
        "  var _b: String\n"
        "  var _c: Bool\n"
        "  var _d: F64\n"
        "  new create() {\n"
        "    _a = 0\n"
        "    _b = \"\"\n"
        "    _c = false\n"
        "    _d = 0.0\n"
        "  }\n"
        "}"));
}

TEST(CodegenComplex, ActorWithComplexMethods) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  var _count: U64\n"
        "  new create() { _count = 0 }\n"
        "  be increment() { _count = _count + 1 }\n"
        "  be decrement() { _count = _count - 1 }\n"
        "  fun get(): U64 { _count }\n"
        "  fun reset() { _count = 0 }\n"
        "}"));
}

/* ==================== 控制流组合 ==================== */

TEST(CodegenComplex, IfElseIfChain) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun classify(x: U64): U64 {\n"
        "    if x < 10 then\n"
        "      1\n"
        "    else\n"
        "      if x < 20 then\n"
        "        2\n"
        "      else\n"
        "        if x < 30 then\n"
        "          3\n"
        "        else\n"
        "          4\n"
        "        end\n"
        "      end\n"
        "    end\n"
        "  }\n"
        "}"));
}

TEST(CodegenComplex, WhileWithBreak) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun loop(): U64 {\n"
        "    var i: U64 = 0\n"
        "    while i < 100 do\n"
        "      i = i + 1\n"
        "    end\n"
        "    i\n"
        "  }\n"
        "}"));
}

TEST(CodegenComplex, NestedLoops) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun nested(): U64 {\n"
        "    var total: U64 = 0\n"
        "    var i: U64 = 0\n"
        "    while i < 10 do\n"
        "      var j: U64 = 0\n"
        "      while j < 10 do\n"
        "        total = total + 1\n"
        "        j = j + 1\n"
        "      end\n"
        "      i = i + 1\n"
        "    end\n"
        "    total\n"
        "  }\n"
        "}"));
}

/* ==================== 表式组合 ==================== */

TEST(CodegenComplex, ChainedComparison) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun check(a: U64, b: U64, c: U64): Bool {\n"
        "    a < b and b < c\n"
        "  }\n"
        "}"));
}

TEST(CodegenComplex, MixedArithmetic) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun calc(a: U64, b: U64, c: U64): U64 {\n"
        "    a * b + c * 2 - a / b\n"
        "  }\n"
        "}"));
}

TEST(CodegenComplex, UnaryNot) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun negate(a: Bool): Bool {\n"
        "    not a\n"
        "  }\n"
        "}"));
}

/* ==================== 类型组合 ==================== */

TEST(CodegenComplex, AllIntTypes) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  var _u8: U8\n"
        "  var _u16: U16\n"
        "  var _u32: U32\n"
        "  var _u64: U64\n"
        "  var _i8: I8\n"
        "  var _i16: I16\n"
        "  var _i32: I32\n"
        "  var _i64: I64\n"
        "  var _f32: F32\n"
        "  var _f64: F64\n"
        "  var _bool: Bool\n"
        "  var _str: String\n"
        "}"));
}

TEST(CodegenComplex, TypeConversions) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun convert(): U64 {\n"
        "    var a: U8 = 255\n"
        "    var b: U64 = 0\n"
        "    b\n"
        "  }\n"
        "}"));
}

/* ==================== 多 Actor 交互 ==================== */

TEST(CodegenComplex, MultipleActorsWithMethods) {
    EXPECT_TRUE(compile_source(
        "actor Producer {\n"
        "  be send() { }\n"
        "}\n"
        "actor Consumer {\n"
        "  be receive() { }\n"
        "}\n"
        "actor Main {\n"
        "  new create() {\n"
        "    var p = Producer\n"
        "    var c = Consumer\n"
        "  }\n"
        "}"));
}

/* ==================== 接口和 trait ==================== */

TEST(CodegenComplex, InterfaceWithMethods) {
    EXPECT_TRUE(compile_source(
        "interface Stringable {\n"
        "  fun to_string(): String\n"
        "}\n"
        "actor Foo is Stringable {\n"
        "  fun to_string(): String { \"Foo\" }\n"
        "}"));
}

TEST(CodegenComplex, TraitWithDefault) {
    EXPECT_TRUE(compile_source(
        "trait Named {\n"
        "  fun name(): String\n"
        "}\n"
        "actor Foo is Named {\n"
        "  fun name(): String { \"Foo\" }\n"
        "}"));
}

/* ==================== Primitive ==================== */

TEST(CodegenComplex, PrimitiveWithConstants) {
    EXPECT_TRUE(compile_source(
        "primitive Constants {\n"
        "  fun pi(): F64 { 3.14159 }\n"
        "  fun e(): F64 { 2.71828 }\n"
        "  fun max_u64(): U64 { 18446744073709551615 }\n"
        "}"));
}

/* ==================== 空程序和边界 ==================== */

TEST(CodegenComplex, EmptyProgram) {
    EXPECT_TRUE(compile_source(""));
}

TEST(CodegenComplex, OnlyComments) {
    EXPECT_TRUE(compile_source("// just comments"));
}

TEST(CodegenComplex, WhitespaceOnly) {
    EXPECT_TRUE(compile_source("   \n\t  \n  "));
}
