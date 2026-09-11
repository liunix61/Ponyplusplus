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

/* ==================== tc_is_builtin_func ==================== */

TEST(TypecheckCov11, BuiltinFuncNull) {
    /* print(nullptr) should not crash */
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => { print(\"\") }\n"
        "}\n"));
}

/* ==================== tc_module_types ==================== */

TEST(TypecheckCov11, ModuleTypesNull) {
    /* use "" should not crash */
    EXPECT_TRUE(tc_ok(
        "use \"\"\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(TypecheckCov11, ModuleTypesStd) {
    EXPECT_TRUE(tc_ok(
        "use \"std\"\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(TypecheckCov11, ModuleTypesStdIo) {
    EXPECT_TRUE(tc_ok(
        "use \"std.io\"\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(TypecheckCov11, ModuleTypesStdConcurrent) {
    EXPECT_TRUE(tc_ok(
        "use \"std.concurrent\"\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(TypecheckCov11, ModuleTypesStdJson) {
    EXPECT_TRUE(tc_ok(
        "use \"std.json\"\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(TypecheckCov11, ModuleTypesStdTime) {
    EXPECT_TRUE(tc_ok(
        "use \"std.time\"\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(TypecheckCov11, ModuleTypesStdLog) {
    EXPECT_TRUE(tc_ok(
        "use \"std.log\"\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(TypecheckCov11, ModuleTypesStdNet) {
    EXPECT_TRUE(tc_ok(
        "use \"std.net\"\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(TypecheckCov11, ModuleTypesStdCrypto) {
    EXPECT_TRUE(tc_ok(
        "use \"std.crypto\"\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(TypecheckCov11, ModuleTypesStdCollections) {
    EXPECT_TRUE(tc_ok(
        "use \"std.collections\"\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(TypecheckCov11, ModuleTypesStdString) {
    EXPECT_TRUE(tc_ok(
        "use \"std.string\"\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== tc_is_std_type ==================== */


















/* ==================== Print Multi-Arg Branch ==================== */

TEST(TypecheckCov11, PrintMultiArgBranch) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => { print(\"a\", \"b\", \"c\", \"d\") }\n"
        "}\n"));
}

/* ==================== NODE_CALL with print data ==================== */

TEST(TypecheckCov11, CallPrintWithArgs) {
    EXPECT_TRUE(tc_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    print(\"hello\")\n"
        "    print(42)\n"
        "    print(true)\n"
        "  }\n"
        "}\n"));
}
