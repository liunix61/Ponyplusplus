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

/* ==================== Chained method calls (lines 711-722) ==================== */

TEST(ParserCov17, ChainedMethodCallsWithArgs) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hello\"\n"
        "    s.substr(1, 3).to_upper()\n"
        "  }\n"
        "}\n"));
}

TEST(ParserCov17, ChainedFieldAccess) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hello\"\n"
        "    s.size()\n"
        "  }\n"
        "}\n"));
}

/* ==================== Method name error (lines 900-901) ==================== */

TEST(ParserCov17, MethodNameError) {
    ASTNode *ast = parse_src(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hello\"\n"
        "    s.\n"
        "  }\n"
        "}\n");
    ast_node_free(ast);
}

/* ==================== Expression function body (lines 933-982) ==================== */

TEST(ParserCov17, ExpressionFunctionBody) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  fun add(a: U32, b: U32): U32 => a + b\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(ParserCov17, BlockFunctionBody) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  fun add(a: U32, b: U32): U32 =>\n"
        "    let result: U32 = a + b\n"
        "    return result\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== Nested actors ==================== */

TEST(ParserCov17, MultipleActors) {
    EXPECT_TRUE(parse_ok(
        "actor Worker {\n"
        "  new create() => { }\n"
        "}\n"
        "actor Manager {\n"
        "  new create() => { }\n"
        "}\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== Actor with behavior ==================== */

TEST(ParserCov17, ActorWithBehavior) {
    EXPECT_TRUE(parse_ok(
        "actor Worker {\n"
        "  new create() => { }\n"
        "  be process(msg: String) =>\n"
        "    print(msg)\n"
        "}\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== Complex match patterns ==================== */


/* ==================== Try with error binding ==================== */

TEST(ParserCov17, TryWithErrorBinding) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    try\n"
        "      print(\"try\")\n"
        "    else\n"
        "      let e: String = \"error\"\n"
        "      print(e)\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

/* ==================== Array literals ==================== */

TEST(ParserCov17, ArrayLiteral) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let arr: Array[U32] = [1, 2, 3]\n"
        "  }\n"
        "}\n"));
}

/* ==================== Map literals ==================== */

TEST(ParserCov17, MapLiteral) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let m: Map[String, U32] = {\"a\": 1, \"b\": 2}\n"
        "  }\n"
        "}\n"));
}
