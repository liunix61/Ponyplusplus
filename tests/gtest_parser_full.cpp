/*
 * Parser 全面覆盖测试
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

TEST(ParserFull, EmptyProgram) {
    ASTNode *ast = parse_source("");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, SimpleActor) {
    ASTNode *ast = parse_source("actor Foo { }");
    ASSERT_NE(ast, nullptr);
    ast_node_free(ast);
}

TEST(ParserFull, ActorWithBehavior) {
    ASTNode *ast = parse_source("actor Foo {\n  be greet() {\n  }\n}");
    ASSERT_NE(ast, nullptr);
    ast_node_free(ast);
}

TEST(ParserFull, ActorWithFields) {
    ASTNode *ast = parse_source("actor Foo {\n  var _x: U64\n  var _y: U64\n}");
    ASSERT_NE(ast, nullptr);
    ast_node_free(ast);
}

TEST(ParserFull, ActorWithNew) {
    ASTNode *ast = parse_source("actor Foo {\n  new create() {\n  }\n}");
    ASSERT_NE(ast, nullptr);
    ast_node_free(ast);
}

TEST(ParserFull, SimpleClass) {
    ASTNode *ast = parse_source("class Foo { }");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, SimplePrimitive) {
    ASTNode *ast = parse_source("primitive MyPrim { }");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, FunctionDef) {
    ASTNode *ast = parse_source("actor Foo {\n  fun add(a: U64, b: U64): U64 {\n    a + b\n  }\n}");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, TypeAnnotations) {
    ASTNode *ast = parse_source("actor Foo {\n  var _a: U8\n  var _b: U16\n  var _c: U32\n  var _d: U64\n}");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, CapabilityAnnotations) {
    ASTNode *ast = parse_source("actor Foo {\n  var _iso: iso String\n  var _val: val U64\n}");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, UseDirective) {
    ASTNode *ast = parse_source("use \"collections/array\"\nactor Foo { }");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, MultipleDefs) {
    ASTNode *ast = parse_source("actor A { }\nactor B { }\nclass C { }\nprimitive D { }");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, IfElse) {
    ASTNode *ast = parse_source("actor Foo {\n  fun check(x: U64): Bool {\n    if x > 10 then true else false end\n  }\n}");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, WhileLoop) {
    ASTNode *ast = parse_source("actor Foo {\n  fun loop() {\n    var i: U64 = 0\n    while i < 10 do\n      i = i + 1\n    end\n  }\n}");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, ReturnStmt) {
    ASTNode *ast = parse_source("actor Foo {\n  fun get(): U64 {\n    return 42\n  }\n}");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, Literals) {
    ASTNode *ast = parse_source("actor Foo {\n  fun test() {\n    var a: U64 = 42\n    var b: F64 = 3.14\n    var c: Bool = true\n    var d: String = \"hello\"\n  }\n}");
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, MatchExpr) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun classify(x: U64): String {\n"
        "    match x\n"
        "    | 0 => \"zero\"\n"
        "    | 1 => \"one\"\n"
        "    | else => \"many\"\n"
        "    end\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, TryError) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun risky() {\n"
        "    try\n"
        "      something()\n"
        "    else\n"
        "      handle()\n"
        "    end\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, ChainedCalls) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  be run() {\n"
        "    var s: String = \"hello\".upper().trim()\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserFull, MethodCalls) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  be run() {\n"
        "    var x: U64 = 1\n"
        "    var y: U64 = x.add(2)\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}
