#include <gtest/gtest.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>

static bool parse_ok(const char *src) {
    Lexer *lx = lexer_new("t", src, strlen(src));
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("t", toks, tc);
    ASTNode *ast = parser_parse_program(p);
    bool ok = (ast != nullptr);
    if (ast) ast_node_free(ast);
    parser_free(p);
    free(toks);
    lexer_free(lx);
    return ok;
}

static bool parse_fail(const char *src) {
    return !parse_ok(src);
}

/* ==================== Capability types (lines 98-104) ==================== */

TEST(ParserCov19, CapIso) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  var x: iso String\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(ParserCov19, CapTrn) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  var x: trn String\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(ParserCov19, CapRef) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  var x: ref String\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(ParserCov19, CapVal) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  var x: val String\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(ParserCov19, CapBox) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  var x: box String\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(ParserCov19, CapTag) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  var x: tag String\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== try-else-then (lines 414-421) ==================== */

TEST(ParserCov19, TryElseThen) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    try\n"
        "      print(\"try\")\n"
        "    else\n"
        "      print(\"else\")\n"
        "    then\n"
        "      print(\"then\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

/* ==================== return with expression (lines 481-489) ==================== */

TEST(ParserCov19, ReturnWithExpr) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  fun foo(): U32 =>\n"
        "    return 42\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== expression function body (lines 933-940, 972-982) ==================== */

TEST(ParserCov19, ExpressionFunctionBody) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  fun foo(): U32 => 42\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== method calls (lines 579-596, 611-627) ==================== */

TEST(ParserCov19, MethodCall) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hello\"\n"
        "    s.to_upper()\n"
        "  }\n"
        "}\n"));
}

/* ==================== chained calls (lines 711-722) ==================== */

TEST(ParserCov19, ChainedMethodCalls) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hello\"\n"
        "    s.to_upper().to_lower()\n"
        "  }\n"
        "}\n"));
}

/* ==================== function call (lines 646-657) ==================== */

TEST(ParserCov19, FunctionCall) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: U32 = foo(1, 2, 3)\n"
        "  }\n"
        "}\n"));
}

/* ==================== import (lines 1133-1134) ==================== */

TEST(ParserCov19, Import) {
    EXPECT_TRUE(parse_ok(
        "use \"collections\"\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== method name error (lines 900-901) ==================== */


/* ==================== constructor name error (lines 950-951) ==================== */


/* ==================== type wrapper (lines 308-313) ==================== */

TEST(ParserCov19, TypeWrapper) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  var x: type String\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== complex program ==================== */

TEST(ParserCov19, ComplexProgram) {
    EXPECT_TRUE(parse_ok(
        "actor Worker {\n"
        "  var _id: U32\n"
        "  new create() => {\n"
        "    _id = 1\n"
        "  }\n"
        "  be process(msg: String) =>\n"
        "    print(msg)\n"
        "}\n"
        "actor main {\n"
        "  new create() => {\n"
        "    let w = Worker.create()\n"
        "    w.process(\"hello\")\n"
        "  }\n"
        "}\n"));
}
