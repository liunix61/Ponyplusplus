#include <gtest/gtest.h>
#include <ponypp/typecheck.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>

static bool tc_ok(const char *src) {
    Lexer *lx = lexer_new("t", src, strlen(src));
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("t", toks, tc);
    ASTNode *ast = parser_parse_program(p);
    bool ok = false;
    if (ast) {
        TypeCheckResult r;
        ok = (typecheck_program(ast, &r) == 0);
        ast_node_free(ast);
    }
    parser_free(p);
    free(toks);
    lexer_free(lx);
    return ok;
}

/* ==================== Builtin Functions ==================== */

TEST(TypecheckCov10, BuiltinPrintln) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => { println(\"hi\") }\n"
        "}\n"));
}

TEST(TypecheckCov10, BuiltinParseJson) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => { parse_json(\"{}\") }\n"
        "}\n"));
}

TEST(TypecheckCov10, BuiltinLogDebug) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => { log_debug(\"msg\") }\n"
        "}\n"));
}

TEST(TypecheckCov10, BuiltinLogInfo) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => { log_info(\"msg\") }\n"
        "}\n"));
}

TEST(TypecheckCov10, BuiltinLogWarn) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => { log_warn(\"msg\") }\n"
        "}\n"));
}

TEST(TypecheckCov10, BuiltinLogError) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => { log_error(\"msg\") }\n"
        "}\n"));
}

/* ==================== Print Multi-Arg ==================== */

TEST(TypecheckCov10, PrintTwoArgs) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => { print(\"a\", \"b\") }\n"
        "}\n"));
}

TEST(TypecheckCov10, PrintThreeArgs) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => { print(\"a\", \"b\", \"c\") }\n"
        "}\n"));
}

TEST(TypecheckCov10, PrintMixedTypes) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => { print(\"val:\", 42) }\n"
        "}\n"));
}

/* ==================== std Wildcard Import ==================== */




/* ==================== std.io Specific Types ==================== */



/* ==================== std.concurrent Types ==================== */




/* ==================== std.json Type ==================== */


/* ==================== std.time Type ==================== */


/* ==================== std.log Type ==================== */


/* ==================== Cross-Actor Field Access ==================== */

TEST(TypecheckCov10, CrossActorField) {
    EXPECT_TRUE(tc_ok(
        "actor Worker {\n"
        "  var _id: U32\n"
        "  new create() => { _id = 1 }\n"
        "}\n"
        "actor main {\n"
        "  new create() => {\n"
        "    let w = Worker.create()\n"
        "    w._id = 2\n"
        "  }\n"
        "}\n"));
}

/* ==================== Type Conversion ==================== */

TEST(TypecheckCov10, StringToU32) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"42\"\n"
        "    let n: U32 = s.u32()\n"
        "  }\n"
        "}\n"));
}

TEST(TypecheckCov10, U32ToString) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let n: U32 = 42\n"
        "    let s: String = n.string()\n"
        "  }\n"
        "}\n"));
}

/* ==================== Complex Expressions ==================== */

TEST(TypecheckCov10, NestedFunctionCall) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    print(parse_json(\"{}\"))\n"
        "  }\n"
        "}\n"));
}

TEST(TypecheckCov10, MultiplePrints) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    print(\"a\")\n"
        "    println(\"b\")\n"
        "    log_info(\"c\")\n"
        "  }\n"
        "}\n"));
}

/* ==================== Lambda Types ==================== */

TEST(TypecheckCov10, LambdaNoCapture) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let f = lambda() { print(\"hi\") }\n"
        "  }\n"
        "}\n"));
}

/* ==================== Match with Multiple Patterns ==================== */

TEST(TypecheckCov10, MatchMultiplePatterns) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: U32 = 1\n"
        "    match x\n"
        "    | 1 | 2 | 3 => print(\"small\")\n"
        "    | 4 | 5 => print(\"medium\")\n"
        "    else print(\"big\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

/* ==================== Try-Catch-Finally ==================== */

TEST(TypecheckCov10, TryCatchFinallyAllBranches) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    try\n"
        "      print(\"try\")\n"
        "    then\n"
        "      print(\"then\")\n"
        "    else\n"
        "      print(\"else\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}
