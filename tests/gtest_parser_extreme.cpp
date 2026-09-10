/*
 * Parser 极限测试
 */

#include <gtest/gtest.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp/ast.h>
#include <ponypp.h>
#include <cstring>

static ASTNode *parse_source(const char *src) {
    Lexer *lex = lexer_new("test.pny", src, strlen(src));
    if (!lex) return nullptr;
    Token *tokens = nullptr;
    size_t count = 0;
    if (!lexer_lex_all(lex, &tokens, &count)) {
        lexer_free(lex);
        return nullptr;
    }
    Parser *p = parser_new("test.pny", tokens, count);
    if (!p) { lexer_free(lex); return nullptr; }
    ASTNode *ast = parser_parse_program(p);
    parser_free(p);
    lexer_free(lex);
    return ast;
}

/* ==================== 深层嵌套 ==================== */

TEST(ParserExtreme, DeeplyNestedIf) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun deep(x: U64): U64 {\n"
        "    if x > 100 then\n"
        "      if x > 200 then\n"
        "        if x > 300 then\n"
        "          if x > 400 then\n"
        "            4\n"
        "          else\n"
        "            3\n"
        "          end\n"
        "        else\n"
        "          2\n"
        "        end\n"
        "      else\n"
        "        1\n"
        "      end\n"
        "    else\n"
        "      0\n"
        "    end\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserExtreme, DeeplyNestedWhile) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun deep() {\n"
        "    var i: U64 = 0\n"
        "    while i < 10 do\n"
        "      var j: U64 = 0\n"
        "      while j < 10 do\n"
        "        var k: U64 = 0\n"
        "        while k < 10 do\n"
        "          var l: U64 = 0\n"
        "          while l < 10 do\n"
        "            l = l + 1\n"
        "          end\n"
        "          k = k + 1\n"
        "        end\n"
        "        j = j + 1\n"
        "      end\n"
        "      i = i + 1\n"
        "    end\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 复杂表达式 ==================== */

TEST(ParserExtreme, LongArithmeticChain) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun calc(): U64 {\n"
        "    1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserExtreme, MixedPrecedence) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun calc(): U64 {\n"
        "    1 + 2 * 3 - 4 / 2 + 5 * 6 - 7 / 1\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserExtreme, ParenthesizedExpr) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun calc(): U64 {\n"
        "    (((1 + 2)))\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 多参数方法 ==================== */

TEST(ParserExtreme, ManyParams) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun many(a: U64, b: U64, c: U64, d: U64, e: U64, f: U64, g: U64, h: U64): U64 {\n"
        "    a + b + c + d + e + f + g + h\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 多字段 ==================== */

TEST(ParserExtreme, ManyFields) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  var _a: U8\n  var _b: U16\n  var _c: U32\n  var _d: U64\n"
        "  var _e: I8\n  var _f: I16\n  var _g: I32\n  var _h: I64\n"
        "  var _i: F32\n  var _j: F64\n  var _k: Bool\n  var _l: String\n"
        "  var _m: Array[U64]\n  var _n: Map[String, U64]\n  var _o: Set[String]\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 多方法 ==================== */

TEST(ParserExtreme, ManyMethods) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun f1(): U64 { 1 }\n"
        "  fun f2(): U64 { 2 }\n"
        "  fun f3(): U64 { 3 }\n"
        "  fun f4(): U64 { 4 }\n"
        "  fun f5(): U64 { 5 }\n"
        "  be b1() { }\n"
        "  be b2() { }\n"
        "  be b3() { }\n"
        "  be b4() { }\n"
        "  be b5() { }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 多 Actor ==================== */

TEST(ParserExtreme, ManyActors) {
    ASTNode *ast = parse_source(
        "actor A { be run() { } }\n"
        "actor B { be run() { } }\n"
        "actor C { be run() { } }\n"
        "actor D { be run() { } }\n"
        "actor E { be run() { } }\n"
        "actor F { be run() { } }\n"
        "actor G { be run() { } }\n"
        "actor H { be run() { } }"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 混合定义 ==================== */

TEST(ParserExtreme, AllDefTypes) {
    ASTNode *ast = parse_source(
        "use \"collections/array\"\n"
        "actor A { be run() { } }\n"
        "class B { var _x: U64 }\n"
        "primitive C { fun value(): U64 { 42 } }\n"
        "interface D { fun name(): String }\n"
        "trait E { fun id(): U64 }"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 复杂类型注解 ==================== */

TEST(ParserExtreme, NestedGenericTypes) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  var _a: Map[String, Array[U64]]\n"
        "  var _b: Array[Map[String, U64]]\n"
        "  var _c: Map[String, Map[String, U64]]\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 能力组合 ==================== */

TEST(ParserExtreme, AllCapabilities) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  var _iso: iso String\n"
        "  var _trn: trn String\n"
        "  var _ref: ref Foo\n"
        "  var _val: val U64\n"
        "  var _box: box Foo\n"
        "  var _tag: tag Foo\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 复杂控制流 ==================== */

TEST(ParserExtreme, IfWithComplexCondition) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun check(a: U64, b: U64, c: U64, d: U64): Bool {\n"
        "    if a > 10 and b < 20 or c == 30 and d != 40 then\n"
        "      true\n"
        "    else\n"
        "      false\n"
        "    end\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 复杂赋值 ==================== */

TEST(ParserExtreme, ChainedAssignments) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun test() {\n"
        "    var a: U64 = 1\n"
        "    var b: U64 = a\n"
        "    var c: U64 = b\n"
        "    var d: U64 = c\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 字符串操作 ==================== */

TEST(ParserExtreme, StringOperations) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun test(): String {\n"
        "    var a: String = \"hello\"\n"
        "    var b: String = \"world\"\n"
        "    a + b\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 边界值 ==================== */

TEST(ParserExtreme, BoundaryIntegers) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun test() {\n"
        "    var a: U8 = 0\n"
        "    var b: U8 = 255\n"
        "    var c: I8 = -128\n"
        "    var d: I8 = 127\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserExtreme, BoundaryFloats) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun test() {\n"
        "    var a: F32 = 0.0\n"
        "    var b: F64 = 3.14159\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 空结构 ==================== */

TEST(ParserExtreme, MinimalActor) {
    ASTNode *ast = parse_source("actor A{}");
    if (ast) ast_node_free(ast);
}

TEST(ParserExtreme, EmptyInterface) {
    ASTNode *ast = parse_source("interface I{}");
    if (ast) ast_node_free(ast);
}

TEST(ParserExtreme, EmptyTrait) {
    ASTNode *ast = parse_source("trait T{}");
    if (ast) ast_node_free(ast);
}

TEST(ParserExtreme, EmptyPrimitive) {
    ASTNode *ast = parse_source("primitive P{}");
    if (ast) ast_node_free(ast);
}

TEST(ParserExtreme, EmptyClass) {
    ASTNode *ast = parse_source("class C{}");
    if (ast) ast_node_free(ast);
}
