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

/* ==================== tc_is_builtin_func: time/math ==================== */

TEST(TypecheckCov13, BuiltinTimeNow) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { time_now() }\n}\n"));
}

TEST(TypecheckCov13, BuiltinTimeElapsed) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { time_elapsed() }\n}\n"));
}

TEST(TypecheckCov13, BuiltinMathPi) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { math_pi() }\n}\n"));
}

TEST(TypecheckCov13, BuiltinMathSqrt) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { math_sqrt(4.0) }\n}\n"));
}

TEST(TypecheckCov13, BuiltinMathSin) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { math_sin(1.0) }\n}\n"));
}

TEST(TypecheckCov13, BuiltinMathCos) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { math_cos(1.0) }\n}\n"));
}

/* ==================== tc_module_types: all std modules ==================== */

TEST(TypecheckCov13, ModuleStdConcurrent) {
    EXPECT_TRUE(tc_ok("use \"std.concurrent\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov13, ModuleStdIo) {
    EXPECT_TRUE(tc_ok("use \"std.io\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov13, ModuleStdJson) {
    EXPECT_TRUE(tc_ok("use \"std.json\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov13, ModuleStdTime) {
    EXPECT_TRUE(tc_ok("use \"std.time\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov13, ModuleStdLog) {
    EXPECT_TRUE(tc_ok("use \"std.log\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov13, ModuleStdNet) {
    EXPECT_TRUE(tc_ok("use \"std.net\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov13, ModuleStdCrypto) {
    EXPECT_TRUE(tc_ok("use \"std.crypto\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov13, ModuleStdCollections) {
    EXPECT_TRUE(tc_ok("use \"std.collections\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov13, ModuleStdString) {
    EXPECT_TRUE(tc_ok("use \"std.string\"\nactor main {\n  new create() => { }\n}\n"));
}

/* ==================== NODE_CALL with print data ==================== */

TEST(TypecheckCov13, PrintWithOneArg) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { print(\"hello\") }\n}\n"));
}

TEST(TypecheckCov13, PrintWithTwoArgs) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { print(\"a\", \"b\") }\n}\n"));
}

/* ==================== tc_is_int_type: all int types ==================== */

TEST(TypecheckCov13, IntTypeU8) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: U8 = 1\n  }\n}\n"));
}

TEST(TypecheckCov13, IntTypeU16) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: U16 = 1\n  }\n}\n"));
}

TEST(TypecheckCov13, IntTypeU32) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: U32 = 1\n  }\n}\n"));
}

TEST(TypecheckCov13, IntTypeU64) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: U64 = 1\n  }\n}\n"));
}

TEST(TypecheckCov13, IntTypeI8) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: I8 = 1\n  }\n}\n"));
}

TEST(TypecheckCov13, IntTypeI16) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: I16 = 1\n  }\n}\n"));
}

TEST(TypecheckCov13, IntTypeI32) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: I32 = 1\n  }\n}\n"));
}

TEST(TypecheckCov13, IntTypeI64) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: I64 = 1\n  }\n}\n"));
}

TEST(TypecheckCov13, IntTypeUSize) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: USize = 1\n  }\n}\n"));
}

TEST(TypecheckCov13, IntTypeISize) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: ISize = 1\n  }\n}\n"));
}
