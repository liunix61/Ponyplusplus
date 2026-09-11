#include <gtest/gtest.h>
#include <ponypp/typecheck.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>

static bool tc_ok(const char *src) {
    Lexer *lx = lexer_new("test", src, strlen(src));
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("test", toks, tc);
    ASTNode *ast = parser_parse_program(p);
    if (!ast) return false;
    TypeCheckResult result;
    memset(&result, 0, sizeof(result));
    typecheck_program(ast, &result);
    return result.error_count == 0;
}

/* 内置函数 println */
TEST(TypecheckCov7, BuiltinPrintln) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    println(\"hello\")\n  }\n}\n"));
}

/* 内置函数 parse_json */
TEST(TypecheckCov7, BuiltinParseJson) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x = parse_json(\"{}\")\n  }\n}\n"));
}

/* 内置函数 log_debug */
TEST(TypecheckCov7, BuiltinLogDebug) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    log_debug(\"debug msg\")\n  }\n}\n"));
}

/* 内置函数 log_info */
TEST(TypecheckCov7, BuiltinLogInfo) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    log_info(\"info msg\")\n  }\n}\n"));
}

/* 内置函数 log_warn */
TEST(TypecheckCov7, BuiltinLogWarn) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    log_warn(\"warn msg\")\n  }\n}\n"));
}

/* 内置函数 log_error */
TEST(TypecheckCov7, BuiltinLogError) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    log_error(\"error msg\")\n  }\n}\n"));
}

/* std 导入 */
TEST(TypecheckCov7, StdImport) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* std.io 导入 */
TEST(TypecheckCov7, StdIoImport) {
    EXPECT_TRUE(tc_ok("use \"std.io\"\n\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* std.string 导入 */
TEST(TypecheckCov7, StdStringImport) {
    EXPECT_TRUE(tc_ok("use \"std.string\"\n\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* std.math 导入 */
TEST(TypecheckCov7, StdMathImport) {
    EXPECT_TRUE(tc_ok("use \"std.math\"\n\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* std.time 导入 */
TEST(TypecheckCov7, StdTimeImport) {
    EXPECT_TRUE(tc_ok("use \"std.time\"\n\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* std.env 导入 */
TEST(TypecheckCov7, StdEnvImport) {
    EXPECT_TRUE(tc_ok("use \"std.env\"\n\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* std.collections 导入 */
TEST(TypecheckCov7, StdCollectionsImport) {
    EXPECT_TRUE(tc_ok("use \"std.collections\"\n\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* 字符串字面量 */
TEST(TypecheckCov7, StringLiteral) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let s = \"hello\"\n  }\n}\n"));
}

/* 整数字面量 */
TEST(TypecheckCov7, IntLiteral) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let n = 42\n  }\n}\n"));
}

/* 浮点字面量 */
TEST(TypecheckCov7, FloatLiteral) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let f = 3.14\n  }\n}\n"));
}

/* 布尔字面量 */
TEST(TypecheckCov7, BoolLiteral) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let b = true\n  }\n}\n"));
}

/* char 字面量 */
TEST(TypecheckCov7, CharLiteral) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let c = 'A'\n  }\n}\n"));
}

/* 赋值语句 */
TEST(TypecheckCov7, Assignment) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    var x = 1\n    x = 2\n  }\n}\n"));
}

/* 复杂表达式 */
TEST(TypecheckCov7, ComplexExpression) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let a = 1\n    let b = 2\n    let c = a + b * 3 - 1\n  }\n}\n"));
}

/* 比较表达式 */
TEST(TypecheckCov7, ComparisonExpression) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let a = 1\n    let b = 2\n    let c = a < b\n  }\n}\n"));
}

/* 逻辑表达式 */
TEST(TypecheckCov7, LogicalExpression) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let a = true\n    let b = false\n    let c = a and b\n  }\n}\n"));
}

/* if 表达式 */
TEST(TypecheckCov7, IfExpression) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x = if true then 1 else 2\n  }\n}\n"));
}

/* match 表达式 */
TEST(TypecheckCov7, MatchExpression) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x = 1\n    let y = match x | 1 => \"one\" | 2 => \"two\" else \"other\"\n  }\n}\n"));
}

/* 函数定义和调用 */
TEST(TypecheckCov7, FunctionDefAndCall) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x = add(1, 2)\n  }\n  fun add(a: U64, b: U64): U64 => a + b\n}\n"));
}

/* 多个 actor */
TEST(TypecheckCov7, MultipleActors) {
    EXPECT_TRUE(tc_ok("actor A {\n  new create() => {}\n}\n\nactor B {\n  new create() => {}\n}\n"));
}

/* 行为定义 */
TEST(TypecheckCov7, BehaviorDef) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {}\n  be greet() => {\n    print(\"hello\")\n  }\n}\n"));
}

/* 行为调用 */
TEST(TypecheckCov7, BehaviorCall) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    this.greet()\n  }\n  be greet() => {\n    print(\"hello\")\n  }\n}\n"));
}

/* 注释不影响类型检查 */
TEST(TypecheckCov7, CommentsIgnored) {
    EXPECT_TRUE(tc_ok("// comment\nactor main {\n  /* block */\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* 空程序 */
TEST(TypecheckCov7, EmptyProgram) {
    Lexer *lx = lexer_new("test", "", 0);
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("test", toks, tc);
    ASTNode *ast = parser_parse_program(p);
    TypeCheckResult result;
    memset(&result, 0, sizeof(result));
    if (ast) typecheck_program(ast, &result);
    SUCCEED();
}

/* 类型别名 */
TEST(TypecheckCov7, TypeAlias) {
    EXPECT_TRUE(tc_ok("type MyInt = U64\n\nactor main {\n  new create() => {\n    let x: MyInt = 42\n  }\n}\n"));
}

/* let 嵌套 */
TEST(TypecheckCov7, NestedLet) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let a = 1\n    let b = a\n    let c = b\n    print(c)\n  }\n}\n"));
}

/* 字符串拼接 */
TEST(TypecheckCov7, StringConcat) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let a = \"hello\"\n    let b = \"world\"\n    let c = a + b\n  }\n}\n"));
}

/* for 循环 */
TEST(TypecheckCov7, ForLoop) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    for i in Range(0, 10) do\n      print(i)\n    end\n  }\n}\n"));
}

/* while 循环 */
TEST(TypecheckCov7, WhileLoop) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    var i = 0\n    while i < 10 do\n      i = i + 1\n    end\n  }\n}\n"));
}

/* 嵌套 if */
TEST(TypecheckCov7, NestedIf) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x = 1\n    if x > 0 then\n      if x > 5 then\n        print(\"big\")\n      else\n        print(\"small\")\n      end\n    end\n  }\n}\n"));
}

/* return 语句 */
TEST(TypecheckCov7, ReturnStatement) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x = compute()\n  }\n  fun compute(): U64 =>\n    return 42\n}\n"));
}

/* 多参数函数 */
TEST(TypecheckCov7, MultiParamFunction) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x = add3(1, 2, 3)\n  }\n  fun add3(a: U64, b: U64, c: U64): U64 => a + b + c\n}\n"));
}

/* 递归函数 */
TEST(TypecheckCov7, RecursiveFunction) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x = factorial(5)\n  }\n  fun factorial(n: U64): U64 =>\n    if n <= 1 then 1 else n * factorial(n - 1)\n}\n"));
}

/* 链式调用 */
TEST(TypecheckCov7, ChainedCalls) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x = compute().add(1)\n  }\n  fun compute(): U64 => 0\n}\n"));
}
