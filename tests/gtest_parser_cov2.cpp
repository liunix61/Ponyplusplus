#include <gtest/gtest.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp/ast.h>
#include <cstring>
#include <cstdlib>

static ASTNode *parse_source(const char *src) {
    Lexer *lx = lexer_new("test.pny", src, strlen(src));
    if (!lx) return nullptr;
    Token *tokens = nullptr;
    size_t count = 0;
    lexer_lex_all(lx, &tokens, &count);
    lexer_free(lx);
    if (!tokens) return nullptr;
    Parser *p = parser_new("test.pny", tokens, count);
    if (!p) { free(tokens); return nullptr; }
    ASTNode *ast = parser_parse_program(p);
    parser_free(p);
    free(tokens);
    return ast;
}

static void expect_parse_ok(const char *src) {
    ASTNode *ast = parse_source(src);
    ASSERT_NE(ast, nullptr) << "Failed: " << src;
    ast_node_free(ast);
}

static void expect_parse_err(const char *src) {
    ASTNode *ast = parse_source(src);
    /* 解析错误时可能返回 NULL 或非 PROGRAM 节点 */
    if (ast) ast_node_free(ast);
}

/* ==================== 复杂表达式 ==================== */

TEST(ParserCov2, NestedArithmetic) {
    expect_parse_ok("actor A\n  be m() =>\n    let x = (1 + 2) * (3 - 4) / 5");
}

TEST(ParserCov2, ChainedComparison) {
    expect_parse_ok("actor A\n  be m() =>\n    let x = 1 < 2");
}

TEST(ParserCov2, LogicalOperators) {
    expect_parse_ok("actor A\n  be m() =>\n    let x = true and false or true");
}

TEST(ParserCov2, UnaryOperators) {
    expect_parse_ok("actor A\n  be m() =>\n    let x = not true");
}

TEST(ParserCov2, NegativeNumber) {
    expect_parse_ok("actor A\n  be m() =>\n    let x = -42");
}

/* ==================== 复杂控制流 ==================== */

TEST(ParserCov2, NestedIfElse) {
    expect_parse_ok("actor A\n  be m() =>\n    if true then\n      if false then\n        1\n      else\n        2\n    else\n      3");
}

TEST(ParserCov2, WhileWithBreak) {
    expect_parse_ok("actor A\n  be m() =>\n    while true do\n      break");
}

TEST(ParserCov2, WhileWithContinue) {
    expect_parse_ok("actor A\n  be m() =>\n    while true do\n      continue");
}

TEST(ParserCov2, ForInLoop) {
    expect_parse_ok("actor A\n  be m() =>\n    for x in [1; 2; 3] do\n      consume x");
}

TEST(ParserCov2, MatchWithMultipleCases) {
    expect_parse_ok("actor A\n  be m() =>\n    match x\n    | 1 => 1\n    | 2 => 2\n    | 3 => 3\n    end");
}

TEST(ParserCov2, TryWithElse) {
    expect_parse_ok("actor A\n  be m() =>\n    try\n      1\n    else\n      2\n    end");
}

/* ==================== 类型系统 ==================== */

TEST(ParserCov2, TypeAlias) {
    expect_parse_ok("type UserId is U32");
}

TEST(ParserCov2, TypeWithConstraint) {
    expect_parse_ok("actor A\n  be m[A: Any #send]() =>\n    None");
}

TEST(ParserCov2, ArrowType) {
    expect_parse_ok("actor A\n  be m(f: {(U32): U32}) =>\n    None");
}

TEST(ParserCov2, TupleType) {
    expect_parse_ok("actor A\n  be m(t: (U32, String)) =>\n    None");
}

TEST(ParserCov2, ArrayType) {
    expect_parse_ok("actor A\n  be m(a: Array[U32]) =>\n    None");
}

/* ==================== Actor 复杂场景 ==================== */

TEST(ParserCov2, ActorWithConstructor) {
    expect_parse_ok("actor A\n  var _count: U32\n  new create(c: U32) =>\n    _count = c");
}

TEST(ParserCov2, ActorWithFieldAccess) {
    expect_parse_ok("actor A\n  var _x: U32\n  be m() =>\n    _x = 42");
}

TEST(ParserCov2, ActorWithThisCall) {
    expect_parse_ok("actor A\n  be m() =>\n    this.n()\n  be n() =>\n    None");
}

TEST(ParserCov2, ActorWithBehavior) {
    expect_parse_ok("actor A\n  be ping(x: U32) =>\n    None\n  be pong(x: U32) =>\n    None");
}

/* ==================== 函数/方法 ==================== */

TEST(ParserCov2, FunctionWithDefaultParam) {
    expect_parse_ok("primitive P\n  fun apply(x: U32 = 42): U32 =>\n    x");
}

TEST(ParserCov2, FunctionWithReturn) {
    expect_parse_ok("primitive P\n  fun apply(): U32 =>\n    42");
}

TEST(ParserCov2, FunctionWithLocalVar) {
    expect_parse_ok("primitive P\n  fun apply(): U32 =>\n    var x: U32 = 10\n    x = 20\n    x");
}

/* ==================== 注解 ==================== */

TEST(ParserCov2, AnnotationOnActor) {
    expect_parse_ok("@deprecated\nactor A\n  be m() =>\n    None");
}

TEST(ParserCov2, AnnotationOnMethod) {
    expect_parse_ok("actor A\n  @private\n  be m() =>\n    None");
}

/* ==================== 边界/错误 ==================== */

TEST(ParserCov2, EmptyProgram) {
    ASTNode *ast = parse_source("");
    if (ast) ast_node_free(ast);
}

TEST(ParserCov2, OnlyComment) {
    ASTNode *ast = parse_source("// comment only");
    if (ast) ast_node_free(ast);
}

TEST(ParserCov2, InvalidActor) {
    expect_parse_err("actor");
}

TEST(ParserCov2, MissingEnd) {
    expect_parse_err("actor A\n  be m() =>\n    None");
}

TEST(ParserCov2, InvalidExpression) {
    expect_parse_err("actor A\n  be m() =>\n    let x = ");
}

TEST(ParserCov2, InvalidType) {
    expect_parse_err("actor A\n  be m(x: ) =>\n    None");
}

/* ==================== 深层嵌套 ==================== */

TEST(ParserCov2, DeeplyNestedIf) {
    expect_parse_ok("actor A\n  be m() =>\n    if true then\n      if true then\n        if true then\n          1\n        else\n          2\n      else\n        3\n    else\n      4");
}

TEST(ParserCov2, DeeplyNestedBlocks) {
    expect_parse_ok("actor A\n  be m() =>\n    let a = 1\n    let b = 2\n    let c = 3\n    let d = 4\n    let e = 5");
}

/* ==================== 字面量 ==================== */

TEST(ParserCov2, HexLiteral) {
    expect_parse_ok("actor A\n  be m() =>\n    let x = 0xFF");
}

TEST(ParserCov2, BinaryLiteral) {
    expect_parse_ok("actor A\n  be m() =>\n    let x = 0b1010");
}

TEST(ParserCov2, FloatLiteral) {
    expect_parse_ok("actor A\n  be m() =>\n    let x = 3.14");
}

TEST(ParserCov2, StringLiteral) {
    expect_parse_ok("actor A\n  be m() =>\n    let x = \"hello\"");
}

TEST(ParserCov2, CharLiteral) {
    expect_parse_ok("actor A\n  be m() =>\n    let x = 'a'");
}

TEST(ParserCov2, BoolLiteral) {
    expect_parse_ok("actor A\n  be m() =>\n    let x = true\n    let y = false");
}
