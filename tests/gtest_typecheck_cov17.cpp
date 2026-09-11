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

/* ==================== tc_is_builtin_func: all branches ==================== */

TEST(TypecheckCov17, BuiltinPrint) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { print(\"hi\") }\n}\n"));
}

TEST(TypecheckCov17, BuiltinPrintln) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { println(\"hi\") }\n}\n"));
}

TEST(TypecheckCov17, BuiltinParseJson) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { parse_json(\"{}\") }\n}\n"));
}

TEST(TypecheckCov17, BuiltinLogDebug) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { log_debug(\"msg\") }\n}\n"));
}

TEST(TypecheckCov17, BuiltinLogInfo) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { log_info(\"msg\") }\n}\n"));
}

TEST(TypecheckCov17, BuiltinLogWarn) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { log_warn(\"msg\") }\n}\n"));
}

TEST(TypecheckCov17, BuiltinLogError) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { log_error(\"msg\") }\n}\n"));
}

TEST(TypecheckCov17, BuiltinTimeNow) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { time_now() }\n}\n"));
}

TEST(TypecheckCov17, BuiltinTimeElapsed) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { time_elapsed() }\n}\n"));
}

TEST(TypecheckCov17, BuiltinMathPi) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { math_pi() }\n}\n"));
}

TEST(TypecheckCov17, BuiltinMathSqrt) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { math_sqrt(4.0) }\n}\n"));
}

TEST(TypecheckCov17, BuiltinMathSin) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { math_sin(1.0) }\n}\n"));
}

TEST(TypecheckCov17, BuiltinMathCos) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { math_cos(1.0) }\n}\n"));
}

/* ==================== tc_module_types: all modules ==================== */

TEST(TypecheckCov17, ModuleStd) {
    EXPECT_TRUE(tc_ok("use \"std\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov17, ModuleStdIo) {
    EXPECT_TRUE(tc_ok("use \"std.io\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov17, ModuleStdConcurrent) {
    EXPECT_TRUE(tc_ok("use \"std.concurrent\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov17, ModuleStdJson) {
    EXPECT_TRUE(tc_ok("use \"std.json\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov17, ModuleStdTime) {
    EXPECT_TRUE(tc_ok("use \"std.time\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov17, ModuleStdLog) {
    EXPECT_TRUE(tc_ok("use \"std.log\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov17, ModuleStdNet) {
    EXPECT_TRUE(tc_ok("use \"std.net\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov17, ModuleStdCrypto) {
    EXPECT_TRUE(tc_ok("use \"std.crypto\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov17, ModuleStdCollections) {
    EXPECT_TRUE(tc_ok("use \"std.collections\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov17, ModuleStdString) {
    EXPECT_TRUE(tc_ok("use \"std.string\"\nactor main {\n  new create() => { }\n}\n"));
}

TEST(TypecheckCov17, ModuleEmpty) {
    EXPECT_TRUE(tc_ok("use \"\"\nactor main {\n  new create() => { }\n}\n"));
}

/* ==================== NODE_CALL with print data ==================== */

TEST(TypecheckCov17, PrintOneArg) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { print(\"hello\") }\n}\n"));
}

TEST(TypecheckCov17, PrintTwoArgs) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => { print(\"a\", \"b\") }\n}\n"));
}

/* ==================== tc_is_int_type: all int types ==================== */

TEST(TypecheckCov17, IntTypeU8) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: U8 = 1\n  }\n}\n"));
}

TEST(TypecheckCov17, IntTypeU16) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: U16 = 1\n  }\n}\n"));
}

TEST(TypecheckCov17, IntTypeU32) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: U32 = 1\n  }\n}\n"));
}

TEST(TypecheckCov17, IntTypeU64) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: U64 = 1\n  }\n}\n"));
}

TEST(TypecheckCov17, IntTypeI8) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: I8 = 1\n  }\n}\n"));
}

TEST(TypecheckCov17, IntTypeI16) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: I16 = 1\n  }\n}\n"));
}

TEST(TypecheckCov17, IntTypeI32) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: I32 = 1\n  }\n}\n"));
}

TEST(TypecheckCov17, IntTypeI64) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: I64 = 1\n  }\n}\n"));
}

TEST(TypecheckCov17, IntTypeUSize) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: USize = 1\n  }\n}\n"));
}

TEST(TypecheckCov17, IntTypeISize) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: ISize = 1\n  }\n}\n"));
}

/* ==================== Cross-Actor Field ==================== */

TEST(TypecheckCov17, CrossActorField) {
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

TEST(TypecheckCov17, StringToU32) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let s: String = \"42\"\n    let n: U32 = s.u32()\n  }\n}\n"));
}

TEST(TypecheckCov17, U32ToString) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let n: U32 = 42\n    let s: String = n.string()\n  }\n}\n"));
}

/* ==================== Nested Function Call ==================== */

TEST(TypecheckCov17, NestedFunctionCall) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    print(parse_json(\"{}\"))\n  }\n}\n"));
}

/* ==================== Multiple Prints ==================== */

TEST(TypecheckCov17, MultiplePrints) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    print(\"a\")\n    println(\"b\")\n    log_info(\"c\")\n  }\n}\n"));
}

/* ==================== Lambda ==================== */

TEST(TypecheckCov17, LambdaNoCapture) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let f = lambda() { print(\"hi\") }\n  }\n}\n"));
}

/* ==================== Match Multiple Patterns ==================== */

TEST(TypecheckCov17, MatchMultiplePatterns) {
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

TEST(TypecheckCov17, TryCatchFinallyAllBranches) {
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
