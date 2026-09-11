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

/* std import + 类型使用 */
TEST(TypecheckCov8, StdImportTypeUsage) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let s = \"hello\"\n    let n = 42\n    print(s)\n    print(n)\n  }\n}\n"));
}

/* std.io import + IO 操作 */
TEST(TypecheckCov8, StdIoImportIO) {
    EXPECT_TRUE(tc_ok("use \"std.io\"\n\nactor main {\n  new create() => {\n    print(\"io test\")\n  }\n}\n"));
}

/* std.string import + 字符串操作 */
TEST(TypecheckCov8, StdStringImportOps) {
    EXPECT_TRUE(tc_ok("use \"std.string\"\n\nactor main {\n  new create() => {\n    let s = \"hello\"\n    print(s)\n  }\n}\n"));
}

/* std.math import + 数学操作 */
TEST(TypecheckCov8, StdMathImportOps) {
    EXPECT_TRUE(tc_ok("use \"std.math\"\n\nactor main {\n  new create() => {\n    let x = 42\n    print(x)\n  }\n}\n"));
}

/* std.time import */
TEST(TypecheckCov8, StdTimeImport) {
    EXPECT_TRUE(tc_ok("use \"std.time\"\n\nactor main {\n  new create() => {\n    print(\"time\")\n  }\n}\n"));
}

/* std.env import */
TEST(TypecheckCov8, StdEnvImport) {
    EXPECT_TRUE(tc_ok("use \"std.env\"\n\nactor main {\n  new create() => {\n    print(\"env\")\n  }\n}\n"));
}

/* std.collections import */
TEST(TypecheckCov8, StdCollectionsImport) {
    EXPECT_TRUE(tc_ok("use \"std.collections\"\n\nactor main {\n  new create() => {\n    print(\"collections\")\n  }\n}\n"));
}

/* 多个 std import */
TEST(TypecheckCov8, MultipleStdImports) {
    EXPECT_TRUE(tc_ok("use \"std.io\"\nuse \"std.string\"\nuse \"std.math\"\n\nactor main {\n  new create() => {\n    print(\"multi\")\n  }\n}\n"));
}

/* std import + actor 引用 */
TEST(TypecheckCov8, StdImportActorRef) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor Worker {\n  new create() => {}\n  be process() => {\n    print(\"processing\")\n  }\n}\n\nactor main {\n  new create() => {\n    let w = Worker\n    print(\"main\")\n  }\n}\n"));
}

/* print 多参数 */
TEST(TypecheckCov8, PrintMultiArg) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    print(\"a\", \"b\")\n  }\n}\n"));
}

/* print 嵌套调用 */
TEST(TypecheckCov8, PrintNestedCall) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    print(add(1, 2))\n  }\n  fun add(a: U64, b: U64): U64 => a + b\n}\n"));
}

/* println 多参数 */
TEST(TypecheckCov8, PrintlnMultiArg) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    println(\"a\", \"b\")\n  }\n}\n"));
}

/* 复杂 std 类型使用 */
TEST(TypecheckCov8, StdTypeComplexUsage) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let s = \"hello\"\n    let n = 42\n    let f = 3.14\n    let b = true\n    print(s)\n    print(n)\n    print(f)\n    print(b)\n  }\n}\n"));
}

/* actor 跨引用 */
TEST(TypecheckCov8, ActorCrossRef) {
    EXPECT_TRUE(tc_ok("actor A {\n  new create() => {}\n  be send_to_b(b: B) => {\n    b.receive()\n  }\n}\n\nactor B {\n  new create() => {}\n  be receive() => {\n    print(\"received\")\n  }\n}\n"));
}

/* 类型别名 + std */
TEST(TypecheckCov8, TypeAliasStd) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\ntype MyString = String\n\nactor main {\n  new create() => {\n    let s: MyString = \"hello\"\n    print(s)\n  }\n}\n"));
}

/* 多个 actor + std import */
TEST(TypecheckCov8, MultipleActorsStdImport) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor A {\n  new create() => {\n    print(\"A\")\n  }\n}\n\nactor B {\n  new create() => {\n    print(\"B\")\n  }\n}\n"));
}

/* 行为 + std import */
TEST(TypecheckCov8, BehaviorStdImport) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {}\n  be greet(name: String) => {\n    print(name)\n  }\n}\n"));
}

/* 函数 + std import */
TEST(TypecheckCov8, FunctionStdImport) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let x = compute()\n    print(x)\n  }\n  fun compute(): U64 => 42\n}\n"));
}

/* 递归 + std import */
TEST(TypecheckCov8, RecursiveStdImport) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let x = factorial(5)\n    print(x)\n  }\n  fun factorial(n: U64): U64 =>\n    if n <= 1 then 1 else n * factorial(n - 1)\n}\n"));
}

/* match + std import */
TEST(TypecheckCov8, MatchStdImport) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let x = 1\n    match x\n    | 1 => print(\"one\")\n    | 2 => print(\"two\")\n    else\n      print(\"other\")\n    end\n  }\n}\n"));
}

/* for + std import */
TEST(TypecheckCov8, ForStdImport) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    for i in Range(0, 10) do\n      print(i)\n    end\n  }\n}\n"));
}

/* while + std import */
TEST(TypecheckCov8, WhileStdImport) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    var i = 0\n    while i < 10 do\n      print(i)\n      i = i + 1\n    end\n  }\n}\n"));
}

/* if/else + std import */
TEST(TypecheckCov8, IfElseStdImport) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let x = 1\n    if x > 0 then\n      print(\"positive\")\n    else\n      print(\"negative\")\n    end\n  }\n}\n"));
}

/* lambda + std import */
TEST(TypecheckCov8, LambdaStdImport) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let f = lambda() => print(\"lambda\")\n    print(\"done\")\n  }\n}\n"));
}

/* list + std import */
TEST(TypecheckCov8, ListStdImport) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let list = [1, 2, 3]\n    print(\"list\")\n  }\n}\n"));
}

/* map + std import */
TEST(TypecheckCov8, MapStdImport) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let m = map(\"a\", 1)\n    print(\"map\")\n  }\n}\n"));
}

/* 空 std import */
TEST(TypecheckCov8, EmptyStdImport) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {}\n}\n"));
}

/* std import + 注释 */
TEST(TypecheckCov8, StdImportWithComments) {
    EXPECT_TRUE(tc_ok("// comment\nuse \"std\" // another\n\n/* block */\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* std import + 空行 */
TEST(TypecheckCov8, StdImportWithBlankLines) {
    EXPECT_TRUE(tc_ok("\n\nuse \"std\"\n\n\nactor main {\n\n  new create() => {\n\n    print(\"hi\")\n\n  }\n\n}\n"));
}
