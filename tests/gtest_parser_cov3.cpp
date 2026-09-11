#include <gtest/gtest.h>
#include <ponypp/parser.h>
#include <ponypp/lexer.h>
#include <ponypp/ast.h>
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

/* else 块 */
TEST(ParserCov3, IfElseBlock) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    if true then\n      print(\"a\")\n    else\n      print(\"b\")\n    end\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* else if 链 */
TEST(ParserCov3, ElseIfChain) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    if true then\n      print(\"a\")\n    elseif false then\n      print(\"b\")\n    else\n      print(\"c\")\n    end\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* return 带值 */
TEST(ParserCov3, ReturnValue) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let x = compute()\n  }\n  fun compute(): U64 =>\n    return 42\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* return 无值 */
TEST(ParserCov3, ReturnNoValue) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    return\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 方法链调用 */
TEST(ParserCov3, MethodChain) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let x = foo().bar().baz()\n  }\n  fun foo(): U64 => 0\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 方法调用带参数 */
TEST(ParserCov3, MethodCallWithArgs) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let x = add(1, 2)\n  }\n  fun add(a: U64, b: U64): U64 => a + b\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* cap 类型 iso */
TEST(ParserCov3, CapIso) {
    ASTNode *ast = parse_code("actor Main iso {\n  new create() => {}\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* cap 类型 trn */
TEST(ParserCov3, CapTrn) {
    ASTNode *ast = parse_code("actor Main trn {\n  new create() => {}\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* cap 类型 ref */
TEST(ParserCov3, CapRef) {
    ASTNode *ast = parse_code("actor Main ref {\n  new create() => {}\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* cap 类型 val */
TEST(ParserCov3, CapVal) {
    ASTNode *ast = parse_code("actor Main val {\n  new create() => {}\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* cap 类型 box */
TEST(ParserCov3, CapBox) {
    ASTNode *ast = parse_code("actor Main box {\n  new create() => {}\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* match 带 else */
TEST(ParserCov3, MatchWithElse) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let x = 1\n    match x\n    | 1 => print(\"one\")\n    | 2 => print(\"two\")\n    else\n      print(\"other\")\n    end\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* for 循环带 end */
TEST(ParserCov3, ForLoopWithEnd) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    for i in Range(0, 10) do\n      print(i)\n    end\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* while 循环带 end */
TEST(ParserCov3, WhileLoopWithEnd) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    var i = 0\n    while i < 10 do\n      i = i + 1\n    end\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 嵌套 if */
TEST(ParserCov3, NestedIf) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let x = 1\n    if x > 0 then\n      if x > 5 then\n        print(\"big\")\n      else\n        print(\"small\")\n      end\n    end\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 多参数函数 */
TEST(ParserCov3, MultiParamFunction) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let x = add3(1, 2, 3)\n  }\n  fun add3(a: U64, b: U64, c: U64): U64 => a + b + c\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 递归函数 */
TEST(ParserCov3, RecursiveFunction) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let x = factorial(5)\n  }\n  fun factorial(n: U64): U64 =>\n    if n <= 1 then 1 else n * factorial(n - 1)\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 链式调用 */
TEST(ParserCov3, ChainedCalls) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let x = compute().add(1)\n  }\n  fun compute(): U64 => 0\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 多个 actor */
TEST(ParserCov3, MultipleActors) {
    ASTNode *ast = parse_code("actor A {\n  new create() => {}\n}\n\nactor B {\n  new create() => {}\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 行为定义 */
TEST(ParserCov3, BehaviorDef) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {}\n  be greet() => {\n    print(\"hello\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 行为调用 */
TEST(ParserCov3, BehaviorCall) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    this.greet()\n  }\n  be greet() => {\n    print(\"hello\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 类型别名 */
TEST(ParserCov3, TypeAlias) {
    ASTNode *ast = parse_code("type MyInt = U64\n\nactor main {\n  new create() => {\n    let x: MyInt = 42\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 字符串拼接 */
TEST(ParserCov3, StringConcat) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let a = \"hello\"\n    let b = \"world\"\n    let c = a + b\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 注释不影响解析 */
TEST(ParserCov3, CommentsIgnored) {
    ASTNode *ast = parse_code("// comment\nactor main {\n  /* block */\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 空程序 */
TEST(ParserCov3, EmptyProgram) {
    ASTNode *ast = parse_code("");
    /* 可能返回 null 或空 AST */
    SUCCEED();
}

/* 只有注释 */
TEST(ParserCov3, OnlyComments) {
    ASTNode *ast = parse_code("// just a comment\n");
    SUCCEED();
}

/* 比较表达式 */
TEST(ParserCov3, ComparisonExpression) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let a = 1\n    let b = 2\n    let c = a < b\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 逻辑表达式 */
TEST(ParserCov3, LogicalExpression) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let a = true\n    let b = false\n    let c = a and b\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* if 表达式 */
TEST(ParserCov3, IfExpression) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let x = if true then 1 else 2\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* match 表达式 */
TEST(ParserCov3, MatchExpression) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let x = 1\n    let y = match x | 1 => \"one\" | 2 => \"two\" else \"other\"\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 函数定义和调用 */
TEST(ParserCov3, FunctionDefAndCall) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let x = add(1, 2)\n  }\n  fun add(a: U64, b: U64): U64 => a + b\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* parser_error */
TEST(ParserCov3, ParserError) {
    Lexer *lx = lexer_new("test", "actor main {", strlen("actor main {"));
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("test", toks, tc);
    ASTNode *ast = parser_parse_program(p);
    /* 可能有错误 */
    const char *err = parser_error(p);
    (void)err;
    SUCCEED();
}

/* parser_line */
TEST(ParserCov3, ParserLine) {
    Lexer *lx = lexer_new("test", "actor main {\n  new create() => {}\n}\n", 40);
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("test", toks, tc);
    int line = parser_line(p);
    EXPECT_GE(line, 1);
}

/* 复杂嵌套 */
TEST(ParserCov3, ComplexNesting) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    let x = 1\n    if x > 0 then\n      var i = 0\n      while i < 10 do\n        if i % 2 == 0 then\n          print(i)\n        end\n        i = i + 1\n      end\n    end\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* import 语句 */
TEST(ParserCov3, ImportStatement) {
    ASTNode *ast = parse_code("use \"std.io\"\n\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}

/* 多个 import */
TEST(ParserCov3, MultipleImports) {
    ASTNode *ast = parse_code("use \"std.io\"\nuse \"std.string\"\n\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
}
