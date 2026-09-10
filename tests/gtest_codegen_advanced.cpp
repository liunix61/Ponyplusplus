/*
 * Codegen 高级路径测试
 */

#include <gtest/gtest.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp/codegen.h>
#include <ponypp/ast.h>
#include <ponypp.h>
#include <cstdio>
#include <cstring>

static bool compile_source(const char *src) {
    Lexer *lex = lexer_new("test.pny", src, strlen(src));
    if (!lex) return false;
    Token *tokens = nullptr;
    size_t count = 0;
    if (!lexer_lex_all(lex, &tokens, &count)) {
        lexer_free(lex);
        return false;
    }
    Parser *p = parser_new("test.pny", tokens, count);
    if (!p) { lexer_free(lex); return false; }
    ASTNode *ast = parser_parse_program(p);
    parser_free(p);
    lexer_free(lex);
    if (!ast) return false;
    
    FILE *out = tmpfile();
    if (!out) { ast_node_free(ast); return false; }
    Codegen *cg = codegen_new(out);
    if (!cg) { fclose(out); ast_node_free(ast); return false; }
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(out);
    ast_node_free(ast);
    return true;
}

/* ==================== 复杂数据结构 ==================== */

TEST(CodegenAdvanced, ActorWithArrayField) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  var _items: Array[U64]\n"
        "  new create() {\n"
        "    _items = Array[U64]\n"
        "  }\n"
        "}"));
}

TEST(CodegenAdvanced, ActorWithMapField) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  var _map: Map[String, U64]\n"
        "  new create() {\n"
        "    _map = Map[String, U64]\n"
        "  }\n"
        "}"));
}

TEST(CodegenAdvanced, ActorWithSetField) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  var _set: Set[String]\n"
        "  new create() {\n"
        "    _set = Set[String]\n"
        "  }\n"
        "}"));
}

/* ==================== 复杂方法 ==================== */

TEST(CodegenAdvanced, MethodWithMultipleReturns) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun classify(x: U64): String {\n"
        "    if x < 10 then\n"
        "      return \"small\"\n"
        "    end\n"
        "    if x < 100 then\n"
        "      return \"medium\"\n"
        "    end\n"
        "    return \"large\"\n"
        "  }\n"
        "}"));
}

TEST(CodegenAdvanced, MethodWithLocalVars) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun calc(): U64 {\n"
        "    var a: U64 = 1\n"
        "    var b: U64 = 2\n"
        "    var c: U64 = a + b\n"
        "    var d: U64 = c * 2\n"
        "    d\n"
        "  }\n"
        "}"));
}

TEST(CodegenAdvanced, MethodWithNestedCalls) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun helper(x: U64): U64 { x * 2 }\n"
        "  fun test(): U64 {\n"
        "    helper(helper(helper(1)))\n"
        "  }\n"
        "}"));
}

/* ==================== 异步消息 ==================== */

TEST(CodegenAdvanced, AsyncSendWithArgs) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  be process(a: U64, b: String) { }\n"
        "  be run() {\n"
        "    var self: Foo = this\n"
        "    self!process(42, \"test\")\n"
        "  }\n"
        "}"));
}

TEST(CodegenAdvanced, SyncCallWithArgs) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun compute(x: U64): U64 { x * 2 }\n"
        "  be run() {\n"
        "    var result: U64 = this@compute(21)\n"
        "  }\n"
        "}"));
}

/* ==================== 复杂控制流 ==================== */

TEST(CodegenAdvanced, IfWithComplexCondition) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun check(a: U64, b: U64, c: U64): Bool {\n"
        "    if a > 10 and b < 20 or c == 30 then\n"
        "      true\n"
        "    else\n"
        "      false\n"
        "    end\n"
        "  }\n"
        "}"));
}

TEST(CodegenAdvanced, WhileWithComplexCondition) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun loop(): U64 {\n"
        "    var i: U64 = 0\n"
        "    var j: U64 = 10\n"
        "    while i < 10 and j > 0 do\n"
        "      i = i + 1\n"
        "      j = j - 1\n"
        "    end\n"
        "    i\n"
        "  }\n"
        "}"));
}

TEST(CodegenAdvanced, ForWithRange) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun sum(): U64 {\n"
        "    var total: U64 = 0\n"
        "    for i in 0..100 do\n"
        "      total = total + i\n"
        "    end\n"
        "    total\n"
        "  }\n"
        "}"));
}

/* ==================== 类型系统 ==================== */

TEST(CodegenAdvanced, TypeAliases) {
    EXPECT_TRUE(compile_source(
        "type UserId is U64\n"
        "type UserName is String\n"
        "actor Foo {\n"
        "  var _id: UserId\n"
        "  var _name: UserName\n"
        "}"));
}

TEST(CodegenAdvanced, GenericTypes) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  var _list: List[U64]\n"
        "  var _dict: Map[String, List[U64]]\n"
        "}"));
}

/* ==================== 多 Actor 系统 ==================== */

TEST(CodegenAdvanced, ActorSystem) {
    EXPECT_TRUE(compile_source(
        "actor Logger {\n"
        "  be log(msg: String) { }\n"
        "}\n"
        "actor Worker {\n"
        "  var _logger: Logger\n"
        "  new create(logger: Logger) {\n"
        "    _logger = logger\n"
        "  }\n"
        "  be process(data: U64) {\n"
        "    _logger!log(\"processing\")\n"
        "  }\n"
        "}\n"
        "actor Main {\n"
        "  new create() {\n"
        "    var logger = Logger\n"
        "    var worker = Worker(logger)\n"
        "    worker!process(42)\n"
        "  }\n"
        "}"));
}

/* ==================== 错误处理 ==================== */

TEST(CodegenAdvanced, TryWithRecovery) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun risky(): U64 {\n"
        "    try\n"
        "      42\n"
        "    else\n"
        "      0\n"
        "    end\n"
        "  }\n"
        "}"));
}

TEST(CodegenAdvanced, TryWithFinally) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun test() {\n"
        "    try\n"
        "      risky()\n"
        "    then\n"
        "      cleanup()\n"
        "    end\n"
        "  }\n"
        "}"));
}

/* ==================== 模式匹配 ==================== */

TEST(CodegenAdvanced, MatchWithBindings) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun process(x: U64): U64 {\n"
        "    match x\n"
        "    | 0 => 100\n"
        "    | 1 => 200\n"
        "    | else => 300\n"
        "    end\n"
        "  }\n"
        "}"));
}

/* ==================== 复杂继承 ==================== */

TEST(CodegenAdvanced, InterfaceImplementation) {
    EXPECT_TRUE(compile_source(
        "interface Shape {\n"
        "  fun area(): F64\n"
        "  fun perimeter(): F64\n"
        "}\n"
        "actor Circle is Shape {\n"
        "  var _radius: F64\n"
        "  new create(r: F64) { _radius = r }\n"
        "  fun area(): F64 { 3.14159 * _radius * _radius }\n"
        "  fun perimeter(): F64 { 2 * 3.14159 * _radius }\n"
        "}\n"
        "actor Rectangle is Shape {\n"
        "  var _width: F64\n"
        "  var _height: F64\n"
        "  new create(w: F64, h: F64) {\n"
        "    _width = w\n"
        "    _height = h\n"
        "  }\n"
        "  fun area(): F64 { _width * _height }\n"
        "  fun perimeter(): F64 { 2 * (_width + _height) }\n"
        "}"));
}

/* ==================== 边界情况 ==================== */

TEST(CodegenDeeplyNested, DeeplyNestedIf) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun deep(x: U64): U64 {\n"
        "    if x > 100 then\n"
        "      if x > 200 then\n"
        "        if x > 300 then\n"
        "          3\n"
        "        else\n"
        "          2\n"
        "        end\n"
        "      else\n"
        "        1\n"
        "      end\n"
        "    else\n"
        "      0\n"
        "    end\n"
        "  }\n"
        "}"));
}

TEST(CodegenAdvanced, LongMethodChain) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun a(): U64 { 1 }\n"
        "  fun b(): U64 { a() + 1 }\n"
        "  fun c(): U64 { b() + 1 }\n"
        "  fun d(): U64 { c() + 1 }\n"
        "  fun e(): U64 { d() + 1 }\n"
        "}"));
}
