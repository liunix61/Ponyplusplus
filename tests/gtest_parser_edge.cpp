/*
 * Parser 边缘语法测试
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

/* ==================== 嵌套结构 ==================== */

TEST(ParserEdge, NestedIfElse) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun test(x: U64): U64 {\n"
        "    if x > 10 then\n"
        "      1\n"
        "    else\n"
        "      if x > 5 then\n"
        "        2\n"
        "      else\n"
        "        3\n"
        "      end\n"
        "    end\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserEdge, NestedWhile) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun test() {\n"
        "    var i: U64 = 0\n"
        "    while i < 10 do\n"
        "      var j: U64 = 0\n"
        "      while j < 10 do\n"
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

TEST(ParserEdge, ComplexArithmetic) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun test(): U64 {\n"
        "    (1 + 2) * 3 - 4 / 2 % 3\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserEdge, UnaryOperators) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun test(x: I64): I64 {\n"
        "    -x\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserEdge, NotOperator) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun test(a: Bool): Bool {\n"
        "    not a\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 多参数 ==================== */

TEST(ParserEdge, ManyParams) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun test(a: U64, b: U64, c: U64, d: U64, e: U64): U64 {\n"
        "    a + b + c + d + e\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserEdge, NoParams) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun test(): U64 { 42 }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 字段访问 ==================== */

TEST(ParserEdge, ThisFieldAccess) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  var _x: U64\n"
        "  fun get(): U64 { this._x }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserEdge, ThisFieldAssign) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  var _x: U64\n"
        "  fun set(v: U64) { this._x = v }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 多行为 ==================== */

TEST(ParserEdge, MultipleBehaviors) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  be start() { }\n"
        "  be stop() { }\n"
        "  be restart() { }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserEdge, MultipleFunctions) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun a(): U64 { 1 }\n"
        "  fun b(): U64 { 2 }\n"
        "  fun c(): U64 { 3 }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 复杂类型 ==================== */

TEST(ParserEdge, ArrayType) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  var _arr: Array[U64]\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserEdge, MapType) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  var _map: Map[String, U64]\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 混合定义 ==================== */

TEST(ParserEdge, MixedDefs) {
    ASTNode *ast = parse_source(
        "use \"collections/array\"\n"
        "actor A {\n"
        "  be run() { }\n"
        "}\n"
        "class B {\n"
        "  var _x: U64\n"
        "}\n"
        "primitive C {\n"
        "  fun value(): U64 { 42 }\n"
        "}\n"
        "interface D {\n"
        "  fun name(): String\n"
        "}\n"
        "trait E {\n"
        "  fun id(): U64\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 空块 ==================== */

TEST(ParserEdge, EmptyMethodBody) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  be empty() { }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

TEST(ParserEdge, EmptyNewBody) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  new create() { }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 注释 ==================== */

TEST(ParserEdge, WithComments) {
    ASTNode *ast = parse_source(
        "// Actor definition\n"
        "actor Foo {\n"
        "  // Behavior\n"
        "  be run() {\n"
        "    // Do nothing\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 发送/调用 ==================== */

TEST(ParserEdge, AsyncSend) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  be run() {\n"
        "    var other: Foo = Foo\n"
        "    other!greet()\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 复杂 match ==================== */

TEST(ParserEdge, MatchMultipleArms) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun classify(x: U64): String {\n"
        "    match x\n"
        "    | 0 => \"zero\"\n"
        "    | 1 => \"one\"\n"
        "    | 2 => \"two\"\n"
        "    | 3 => \"three\"\n"
        "    | else => \"many\"\n"
        "    end\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}

/* ==================== 复杂 try ==================== */

TEST(ParserEdge, TryWithThen) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  fun test() {\n"
        "    try\n"
        "      risky()\n"
        "    else\n"
        "      handle()\n"
        "    then\n"
        "      cleanup()\n"
        "    end\n"
        "  }\n"
        "}"
    );
    if (ast) ast_node_free(ast);
}
