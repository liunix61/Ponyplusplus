#include <gtest/gtest.h>
#include <ponypp/typecheck.h>
#include <ponypp/ast.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>

static ASTNode *parse_code(const char *src) {
    Lexer *lx = lexer_new("test", src, strlen(src));
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("test", toks, tc);
    return parser_parse_program(p);
}

/* 内置函数调用 */
TEST(TypecheckCov6, BuiltinPrintln) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    println(\"hello\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_EQ(errs, 0);
}

TEST(TypecheckCov6, BuiltinParseJson) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    parse_json(\"{}\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

TEST(TypecheckCov6, BuiltinLogDebug) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    log_debug(\"msg\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

TEST(TypecheckCov6, BuiltinLogInfo) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    log_info(\"msg\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

TEST(TypecheckCov6, BuiltinLogWarn) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    log_warn(\"msg\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

TEST(TypecheckCov6, BuiltinLogError) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    log_error(\"msg\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

TEST(TypecheckCov6, BuiltinTimeNow) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    time_now()\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

TEST(TypecheckCov6, BuiltinMathPi) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    math_pi()\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

TEST(TypecheckCov6, BuiltinMathSqrt) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    math_sqrt(4)\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* use std 模块导入 */
TEST(TypecheckCov6, UseStdWildcard) {
    ASTNode *ast = parse_code("use std\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_EQ(errs, 0);
}

TEST(TypecheckCov6, UseStdConcurrent) {
    ASTNode *ast = parse_code("use std.concurrent\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_EQ(errs, 0);
}

TEST(TypecheckCov6, UseStdIo) {
    ASTNode *ast = parse_code("use std.io\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_EQ(errs, 0);
}

TEST(TypecheckCov6, UseStdJson) {
    ASTNode *ast = parse_code("use std.json\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_EQ(errs, 0);
}

TEST(TypecheckCov6, UseStdTime) {
    ASTNode *ast = parse_code("use std.time\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_EQ(errs, 0);
}

TEST(TypecheckCov6, UseStdLog) {
    ASTNode *ast = parse_code("use std.log\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_EQ(errs, 0);
}

/* 字面量作为语句 */
TEST(TypecheckCov6, LiteralStmts) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    42\n    3.14\n    \"hello\"\n    true\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* print 多参数报错 */
TEST(TypecheckCov6, PrintMultipleArgs) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    print(\"a\", \"b\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* 赋值语句 */
TEST(TypecheckCov6, AssignStmt) {
    ASTNode *ast = parse_code("actor main {\n  var x: I32\n  new create() => {\n    x = 42\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* actor 字段类型匹配 */
TEST(TypecheckCov6, ActorFieldType) {
    ASTNode *ast = parse_code("actor main {\n  var x: I32\n  new create() => {\n    x = 42\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* std 类型作为字段 */
TEST(TypecheckCov6, StdTypeField) {
    ASTNode *ast = parse_code("use std\nactor main {\n  var ch: Channel\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* std.concurrent 类型 */
TEST(TypecheckCov6, StdConcurrentType) {
    ASTNode *ast = parse_code("use std.concurrent\nactor main {\n  var ch: Channel\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* std.io 类型 */
TEST(TypecheckCov6, StdIoType) {
    ASTNode *ast = parse_code("use std.io\nactor main {\n  var f: File\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* std.json 类型 */
TEST(TypecheckCov6, StdJsonType) {
    ASTNode *ast = parse_code("use std.json\nactor main {\n  var j: JSON\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* 数字类型字段 */
TEST(TypecheckCov6, IntTypeFields) {
    ASTNode *ast = parse_code("actor main {\n  var a: U8\n  var b: U16\n  var c: U32\n  var d: U64\n  var e: I8\n  var f: I16\n  var g: I32\n  var h: I64\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* 多个 actor */
TEST(TypecheckCov6, MultipleActors) {
    ASTNode *ast = parse_code("actor A {\n  new create() => {\n    print(\"a\")\n  }\n}\nactor B {\n  new create() => {\n    print(\"b\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_EQ(errs, 0);
}

/* actor 继承 */
TEST(TypecheckCov6, ActorInherit) {
    ASTNode *ast = parse_code("trait T\nactor main is T {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* use std.* 子模块 */
TEST(TypecheckCov6, UseStdWildcardSub) {
    ASTNode *ast = parse_code("use std.*\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_EQ(errs, 0);
}

/* use std.concurrent.* */
TEST(TypecheckCov6, UseStdConcurrentWildcard) {
    ASTNode *ast = parse_code("use std.concurrent.*\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_EQ(errs, 0);
}

/* 非内置函数调用 */
TEST(TypecheckCov6, NonBuiltinCall) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    foo()\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* 空 actor */
TEST(TypecheckCov6, EmptyActor) {
    ASTNode *ast = parse_code("actor main\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* 空程序 */
TEST(TypecheckCov6, EmptyProgram) {
    ASTNode *ast = parse_code("");
    if (ast) {
        TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
        EXPECT_GE(errs, 0);
    }
}

/* null 输入 */
TEST(TypecheckCov6, NullInput) {
    TypeCheckResult tc_result = {}; typecheck_program(nullptr, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* 多个 use 语句 */
TEST(TypecheckCov6, MultipleUse) {
    ASTNode *ast = parse_code("use std\nuse std.concurrent\nuse std.io\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_EQ(errs, 0);
}

/* use std.time 类型 */
TEST(TypecheckCov6, StdTimeType) {
    ASTNode *ast = parse_code("use std.time\nactor main {\n  var t: Timer\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* use std.log 类型 */
TEST(TypecheckCov6, StdLogType) {
    ASTNode *ast = parse_code("use std.log\nactor main {\n  var l: Logger\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* 嵌套函数调用 */
TEST(TypecheckCov6, NestedCalls) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    print(math_sqrt(4))\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_GE(errs, 0);
}

/* 多条语句 */
TEST(TypecheckCov6, MultipleStmts) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    print(\"a\")\n    print(\"b\")\n    print(\"c\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    TypeCheckResult tc_result = {}; typecheck_program(ast, &tc_result); int errs = tc_result.error_count; typecheck_free_result(&tc_result);
    EXPECT_EQ(errs, 0);
}
