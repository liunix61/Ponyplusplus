#include <gtest/gtest.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp/ast.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>

static ASTNode *parse_src(const char *src) {
    Lexer *lx = lexer_new("t", src, strlen(src));
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("t", toks, tc);
    ASTNode *ast = parser_parse_program(p);
    parser_free(p);
    free(toks);
    lexer_free(lx);
    return ast;
}

static bool parse_ok(const char *src) {
    ASTNode *ast = parse_src(src);
    bool ok = (ast != nullptr);
    ast_node_free(ast);
    return ok;
}

/* ==================== Capability Types ==================== */

TEST(ParserCov4, CapIso) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: iso String = \"hello\"\n"
        "  }\n"
        "}\n"));
}

TEST(ParserCov4, CapTrn) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: trn String = \"hello\"\n"
        "  }\n"
        "}\n"));
}

TEST(ParserCov4, CapRef) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: ref String = \"hello\"\n"
        "  }\n"
        "}\n"));
}

TEST(ParserCov4, CapVal) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: val String = \"hello\"\n"
        "  }\n"
        "}\n"));
}

TEST(ParserCov4, CapBox) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: box String = \"hello\"\n"
        "  }\n"
        "}\n"));
}

TEST(ParserCov4, CapTag) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: tag String = \"hello\"\n"
        "  }\n"
        "}\n"));
}

/* ==================== Type Keywords ==================== */

TEST(ParserCov4, TypeKeywordType) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: type U32 = 1\n"
        "  }\n"
        "}\n"));
}

/* ==================== Consume Errors ==================== */

TEST(ParserCov4, MissingBrace) {
    ASTNode *ast = parse_src("actor main {\n  new create() => { print(\"hi\")\n");
    /* Should not crash */
    ast_node_free(ast);
}

TEST(ParserCov4, MissingParen) {
    ASTNode *ast = parse_src("actor main {\n  new create( => { }\n}");
    ast_node_free(ast);
}

TEST(ParserCov4, MissingArrow) {
    ASTNode *ast = parse_src("actor main {\n  new create() { }\n}");
    ast_node_free(ast);
}

/* ==================== Complex Types ==================== */

TEST(ParserCov4, ArrayType) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: Array[String] = Array[String].create()\n"
        "  }\n"
        "}\n"));
}

TEST(ParserCov4, MapType) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: Map[String, U32] = Map[String, U32].create()\n"
        "  }\n"
        "}\n"));
}

/* ==================== Match Patterns ==================== */


/* ==================== Try-Catch-Finally ==================== */

TEST(ParserCov4, TryCatchOnly) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    try\n"
        "      print(\"try\")\n"
        "    else\n"
        "      print(\"else\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

TEST(ParserCov4, TryFinallyOnly) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    try\n"
        "      print(\"try\")\n"
        "    then\n"
        "      print(\"then\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

/* ==================== Nested Structures ==================== */


TEST(ParserCov4, NestedIfInWhile) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    var i: U32 = 0\n"
        "    while i < 10 do\n"
        "      if i > 5 then print(\"big\") end\n"
        "      i = i + 1\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

/* ==================== Import Variants ==================== */

TEST(ParserCov4, ImportStd) {
    EXPECT_TRUE(parse_ok("use \"std\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(ParserCov4, ImportStdIo) {
    EXPECT_TRUE(parse_ok("use \"std.io\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(ParserCov4, ImportStdConcurrent) {
    EXPECT_TRUE(parse_ok("use \"std.concurrent\"\nactor main {\n  new create() => { }\n}\n"));
}

/* ==================== Actor with Multiple Constructors ==================== */

TEST(ParserCov4, ActorWithCreateAndApply) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => { }\n"
        "  fun apply(): U32 => 42\n"
        "}\n"));
}

/* ==================== Error Recovery ==================== */

TEST(ParserCov4, EmptyActor) {
    EXPECT_TRUE(parse_ok("actor main {\n}\n"));
}

TEST(ParserCov4, ActorWithOnlyNew) {
    EXPECT_TRUE(parse_ok("actor main {\n  new create() => { }\n}\n"));
}
