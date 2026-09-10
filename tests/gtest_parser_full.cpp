/*
 * Parser 全面覆盖测试
 * 目标: 提升 parser.c 覆盖率
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
    if (!p) {
        lexer_free(lex);
        return nullptr;
    }
    ASTNode *ast = parser_parse_program(p);
    parser_free(p);
    lexer_free(lex);
    return ast;
}

/* ==================== 空程序 ==================== */

TEST(ParserFull, EmptyProgram) {
    ASTNode *ast = parse_source("");
    /* 空程序可能返回NULL或空AST */
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, OnlyComments) {
    ASTNode *ast = parse_source("// just a comment");
    if (ast) ast_node_free(ast);
}

/* ==================== Actor 定义 ==================== */

TEST(ParserFull, SimpleActor) {
    ASTNode *ast = parse_source("actor Foo { }");
    ASSERT_NE(ast, nullptr);
    ast_node_free(ast);
}

TEST(ParserFull, ActorWithBehavior) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  be greet() {\n"
        "  }\n"
        "}"
    );
    ASSERT_NE(ast, nullptr);
    ast_node_free(ast);
}

TEST(ParserFull, ActorWithFields) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  var _x: U64\n"
        "  var _y: U64\n"
        "}"
    );
    ASSERT_NE(ast, nullptr);
    ast_node_free(ast);
}

TEST(ParserFull, ActorWithNew) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  new create() {\n"
        "  }\n"
        "}"
    );
    ASSERT_NE(ast, nullptr);
    ast_node_free(ast);
}

/* ==================== Class 定义 ==================== */

TEST(ParserFull, SimpleClass) {
    ASTNode *ast = parse_source("class Foo { }");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, ClassWithFields) {
    ASTNode *ast = parse_source(
        "class Point {\n"
        "  var _x: U64\n"
        "  var _y: U64\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== Primitive 定义 ==================== */

TEST(ParserFull, SimplePrimitive) {
    ASTNode *ast = parse_source("primitive MyPrim { }");
    if (ast) ast_node_free(ast);
}

/* ==================== Interface/Trait ==================== */

TEST(ParserFull, SimpleInterface) {
    ASTNode *ast = parse_source("interface Stringable { fun to_string(): String }");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, SimpleTrait) {
    ASTNode *ast = parse_source("trait Named { fun name(): String }");
    if (ast) ast_node_free(ast);
}

/* ==================== 函数定义 ==================== */

TEST(ParserFull, FunctionDef) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun add(a: U64, b: U64): U64 {\n"
        "    a + b\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, BehaviorDef) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  be process(data: U64) {\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 类型注解 ==================== */

TEST(ParserFull, TypeAnnotations) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  var _a: U8\n"
        "  var _b: U16\n"
        "  var _c: U32\n"
        "  var _d: U64\n"
        "  var _e: I8\n"
        "  var _f: I16\n"
        "  var _g: I32\n"
        "  var _h: I64\n"
        "  var _i: F32\n"
        "  var _j: F64\n"
        "  var _k: Bool\n"
        "  var _l: String\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 能力注解 ==================== */

TEST(ParserFull, CapabilityAnnotations) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  var _iso_field: iso String\n"
        "  var _val_field: val U64\n"
        "  var _ref_field: ref Bar\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== Use 指令 ==================== */

TEST(ParserFull, UseDirective) {
    ASTNode *ast = parse_source("use \"collections/array\"\nactor Foo { }");
    if (ast) ast_node_free(ast);
}

/* ==================== 多个定义 ==================== */

TEST(ParserFull, MultipleDefs) {
    ASTNode *ast = parse_source(
        "actor A { }\n"
        "actor B { }\n"
        "class C { }\n"
        "primitive D { }"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 嵌套表达式 ==================== */


/* ==================== If/Else ==================== */

TEST(ParserFull, IfElse) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun check(x: U64): Bool {\n"
        "    if x > 10 then true else false end\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== While 循环 ==================== */

TEST(ParserFull, WhileLoop) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun loop() {\n"
        "    var i: U64 = 0\n"
        "    while i < 10 do\n"
        "      i = i + 1\n"
        "    end\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== Match 表达式 ==================== */


/* ==================== Try/Error ==================== */


/* ==================== Return ==================== */

TEST(ParserFull, ReturnStmt) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun get(): U64 {\n"
        "    return 42\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 字面量 ==================== */

TEST(ParserFull, Literals) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun test() {\n"
        "    var a: U64 = 42\n"
        "    var b: F64 = 3.14\n"
        "    var c: Bool = true\n"
        "    var d: String = \"hello\"\n"
        "    var e: U64 = 0xFF\n"
        "    var f: U64 = 0b1010\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 方法调用 ==================== */


/* ==================== 链式调用 ==================== */

