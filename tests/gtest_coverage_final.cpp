/*
 * 覆盖率最终提升测试
 */

#include <gtest/gtest.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp/codegen.h>
#include <ponypp/ast.h>
#include <ponypp/debugger.h>
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

/* ==================== Codegen 综合 ==================== */

TEST(CoverageFinal, ActorWithConstructorChaining) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  var _x: U64\n"
        "  new create() { _x = 0 }\n"
        "  new with_value(v: U64) { _x = v }\n"
        "}"));
}

TEST(CoverageFinal, ActorWithSelfReference) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  var _self: Foo\n"
        "  new create() { _self = this }\n"
        "}"));
}

TEST(CoverageFinal, ActorWithMethodCallChain) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun a(): U64 { 1 }\n"
        "  fun b(): U64 { a() + 1 }\n"
        "  fun c(): U64 { b() + 1 }\n"
        "  fun d(): U64 { c() + 1 }\n"
        "  fun e(): U64 { d() + 1 }\n"
        "  fun f(): U64 { e() + 1 }\n"
        "}"));
}

TEST(CoverageFinal, ActorWithComplexFields) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  var _a: U64 = 0\n"
        "  var _b: String = \"\"\n"
        "  var _c: Bool = false\n"
        "  var _d: F64 = 0.0\n"
        "  var _e: Array[U64] = Array[U64]\n"
        "  var _f: Map[String, U64] = Map[String, U64]\n"
        "}"));
}

TEST(CoverageFinal, ActorWithAllControlFlow) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun test(x: U64): U64 {\n"
        "    var result: U64 = 0\n"
        "    if x > 10 then\n"
        "      result = 1\n"
        "    else\n"
        "      if x > 5 then\n"
        "        result = 2\n"
        "      else\n"
        "        result = 3\n"
        "      end\n"
        "    end\n"
        "    var i: U64 = 0\n"
        "    while i < x do\n"
        "      result = result + i\n"
        "      i = i + 1\n"
        "    end\n"
        "    result\n"
        "  }\n"
        "}"));
}

TEST(CoverageFinal, ActorWithAllOperators) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun test(a: U64, b: U64): U64 {\n"
        "    var add: U64 = a + b\n"
        "    var sub: U64 = a - b\n"
        "    var mul: U64 = a * b\n"
        "    var div: U64 = a / b\n"
        "    var mod: U64 = a % b\n"
        "    add + sub + mul + div + mod\n"
        "  }\n"
        "}"));
}

TEST(CoverageFinal, ActorWithAllComparisons) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun test(a: U64, b: U64): Bool {\n"
        "    var eq: Bool = a == b\n"
        "    var ne: Bool = a != b\n"
        "    var lt: Bool = a < b\n"
        "    var gt: Bool = a > b\n"
        "    var le: Bool = a <= b\n"
        "    var ge: Bool = a >= b\n"
        "    eq and ne and lt and gt and le and ge\n"
        "  }\n"
        "}"));
}

TEST(CoverageFinal, ActorWithAllLogicalOps) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun test(a: Bool, b: Bool): Bool {\n"
        "    var and_result: Bool = a and b\n"
        "    var or_result: Bool = a or b\n"
        "    var not_result: Bool = not a\n"
        "    and_result or or_result or not_result\n"
        "  }\n"
        "}"));
}

/* ==================== Debugger 综合 ==================== */

TEST(CoverageFinal, DebuggerLifecycle) {
    FILE *in = tmpfile();
    FILE *out = tmpfile();
    ASSERT_NE(in, nullptr);
    ASSERT_NE(out, nullptr);
    
    DapDebugger *dbg = dap_new(in, out);
    ASSERT_NE(dbg, nullptr);
    
    dap_free(dbg);
    fclose(in);
    fclose(out);
}

TEST(CoverageFinal, DebuggerMultipleMessages) {
    FILE *in = tmpfile();
    FILE *out = tmpfile();
    DapDebugger *dbg = dap_new(in, out);
    ASSERT_NE(dbg, nullptr);
    
    /* 写入多个消息 */
    const char *msg1 = "{\"seq\":1,\"type\":\"request\",\"command\":\"initialize\"}";
    const char *msg2 = "{\"seq\":2,\"type\":\"request\",\"command\":\"launch\"}";
    const char *msg3 = "{\"seq\":3,\"type\":\"request\",\"command\":\"configurationDone\"}";
    
    fprintf(in, "Content-Length: %zu\r\n\r\n%s", strlen(msg1), msg1);
    fprintf(in, "Content-Length: %zu\r\n\r\n%s", strlen(msg2), msg2);
    fprintf(in, "Content-Length: %zu\r\n\r\n%s", strlen(msg3), msg3);
    rewind(in);
    
    /* 处理消息 */
    for (int i = 0; i < 3; i++) {
        int r = dap_process_message(dbg);
        (void)r;
    }
    
    dap_free(dbg);
    fclose(in);
    fclose(out);
}

/* ==================== 综合场景 ==================== */

TEST(CoverageFinal, FullCompilationPipeline) {
    /* 完整编译流程: lex -> parse -> codegen */
    const char *src =
        "actor Main {\n"
        "  var _counter: U64\n"
        "  var _name: String\n"
        "  new create() {\n"
        "    _counter = 0\n"
        "    _name = \"Main\"\n"
        "  }\n"
        "  be increment() {\n"
        "    _counter = _counter + 1\n"
        "  }\n"
        "  be decrement() {\n"
        "    _counter = _counter - 1\n"
        "  }\n"
        "  fun get_count(): U64 { _counter }\n"
        "  fun get_name(): String { _name }\n"
        "  fun reset() {\n"
        "    _counter = 0\n"
        "  }\n"
        "}";
    
    EXPECT_TRUE(compile_source(src));
}

TEST(CoverageFinal, ComplexActorSystem) {
    const char *src =
        "actor Logger {\n"
        "  be log(msg: String) { }\n"
        "}\n"
        "actor Cache {\n"
        "  var _data: Map[String, U64]\n"
        "  new create() { _data = Map[String, U64] }\n"
        "  be set(key: String, value: U64) { }\n"
        "  be get(key: String) { }\n"
        "}\n"
        "actor Service {\n"
        "  var _logger: Logger\n"
        "  var _cache: Cache\n"
        "  new create(logger: Logger, cache: Cache) {\n"
        "    _logger = logger\n"
        "    _cache = cache\n"
        "  }\n"
        "  be process(key: String) {\n"
        "    _cache!get(key)\n"
        "    _logger!log(\"processing\")\n"
        "  }\n"
        "}\n"
        "actor Main {\n"
        "  new create() {\n"
        "    var logger = Logger\n"
        "    var cache = Cache\n"
        "    var service = Service(logger, cache)\n"
        "    service!process(\"key1\")\n"
        "  }\n"
        "}";
    
    EXPECT_TRUE(compile_source(src));
}

TEST(CoverageFinal, InterfaceTraitImplementation) {
    const char *src =
        "interface Shape {\n"
        "  fun area(): F64\n"
        "  fun perimeter(): F64\n"
        "}\n"
        "trait Named {\n"
        "  fun name(): String\n"
        "}\n"
        "actor Circle is Shape, Named {\n"
        "  var _radius: F64\n"
        "  new create(r: F64) { _radius = r }\n"
        "  fun area(): F64 { 3.14159 * _radius * _radius }\n"
        "  fun perimeter(): F64 { 2 * 3.14159 * _radius }\n"
        "  fun name(): String { \"Circle\" }\n"
        "}";
    
    EXPECT_TRUE(compile_source(src));
}
