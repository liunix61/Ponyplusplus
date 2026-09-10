/*
 * Codegen 全路径覆盖测试
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

/* ==================== Source Map ==================== */

TEST(CodegenFullpath, SourceMapApi) {
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);
    int r = sourcemap_add(sm, 1, "test.pny", 1, 1);
    EXPECT_EQ(r, 0);
    r = sourcemap_add(sm, 2, "test.pny", 2, 1);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(sourcemap_count(sm), 2u);
    SourceMapEntry entry = {};
    r = sourcemap_lookup(sm, 1, &entry);
    EXPECT_EQ(r, 0);
    sourcemap_free(sm);
}

/* ==================== 所有能力 ==================== */

TEST(CodegenFullpath, AllCapabilities) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  var _iso: iso String\n"
        "  var _trn: trn String\n"
        "  var _ref: ref Foo\n"
        "  var _val: val U64\n"
        "  var _box: box Foo\n"
        "  var _tag: tag Foo\n"
        "}"));
}

/* ==================== 所有类型转换 ==================== */

TEST(CodegenFullpath, NumericConversions) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun convert() {\n"
        "    var a: U8 = 255\n"
        "    var b: U16 = 65535\n"
        "    var c: U32 = 4294967295\n"
        "    var d: U64 = 18446744073709551615\n"
        "    var e: I8 = -128\n"
        "    var f: I16 = -32768\n"
        "    var g: I32 = -2147483648\n"
        "    var h: I64 = -9223372036854775808\n"
        "    var i: F32 = 1.5\n"
        "    var j: F64 = 1.5\n"
        "  }\n"
        "}"));
}

/* ==================== 复杂 Actor 继承 ==================== */

TEST(CodegenFullpath, ActorInheritance) {
    EXPECT_TRUE(compile_source(
        "trait Greeter {\n"
        "  fun greet(): String { \"hello\" }\n"
        "}\n"
        "actor Person is Greeter {\n"
        "  var _name: String\n"
        "  new create(name: String) { _name = name }\n"
        "  fun greet(): String { \"hi \" + _name }\n"
        "}"));
}

/* ==================== 复杂消息传递 ==================== */

TEST(CodegenFullpath, MessagePassing) {
    EXPECT_TRUE(compile_source(
        "actor Sender {\n"
        "  be send(receiver: Receiver, msg: String) {\n"
        "    receiver!receive(msg)\n"
        "  }\n"
        "}\n"
        "actor Receiver {\n"
        "  be receive(msg: String) { }\n"
        "}"));
}

/* ==================== 复杂数据操作 ==================== */

TEST(CodegenFullpath, DataOperations) {
    EXPECT_TRUE(compile_source(
        "actor DataStore {\n"
        "  var _items: Array[U64]\n"
        "  var _lookup: Map[String, U64]\n"
        "  var _tags: Set[String]\n"
        "  new create() {\n"
        "    _items = Array[U64]\n"
        "    _lookup = Map[String, U64]\n"
        "    _tags = Set[String]\n"
        "  }\n"
        "  be add(item: U64) { }\n"
        "  be remove(item: U64) { }\n"
        "  be find(key: String) { }\n"
        "}"));
}

/* ==================== 复杂控制流组合 ==================== */

TEST(CodegenFullpath, ComplexControlFlow) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun process(data: Array[U64]): U64 {\n"
        "    var total: U64 = 0\n"
        "    var i: U64 = 0\n"
        "    while i < 10 do\n"
        "      if i % 2 == 0 then\n"
        "        total = total + i\n"
        "      else\n"
        "        if i % 3 == 0 then\n"
        "          total = total + i * 2\n"
        "        else\n"
        "          total = total + 1\n"
        "        end\n"
        "      end\n"
        "      i = i + 1\n"
        "    end\n"
        "    total\n"
        "  }\n"
        "}"));
}

/* ==================== 错误处理组合 ==================== */

TEST(CodegenFullpath, ErrorHandling) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun safe_divide(a: U64, b: U64): U64 {\n"
        "    try\n"
        "      if b == 0 then\n"
        "        error\n"
        "      end\n"
        "      a / b\n"
        "    else\n"
        "      0\n"
        "    end\n"
        "  }\n"
        "}"));
}

/* ==================== 复杂匹配 ==================== */


/* ==================== 大型系统 ==================== */

TEST(CodegenFullpath, LargeSystem) {
    EXPECT_TRUE(compile_source(
        "actor Event { }\n"
        "actor EventBus {\n"
        "  var _subscribers: Array[Event]\n"
        "  new create() { _subscribers = Array[Event] }\n"
        "  be subscribe(sub: Event) { }\n"
        "  be publish(event: Event) { }\n"
        "}\n"
        "actor EventHandler {\n"
        "  var _bus: EventBus\n"
        "  new create(bus: EventBus) { _bus = bus }\n"
        "  be handle(event: Event) { }\n"
        "}\n"
        "actor App {\n"
        "  var _bus: EventBus\n"
        "  var _handler: EventHandler\n"
        "  new create() {\n"
        "    _bus = EventBus\n"
        "    _handler = EventHandler(_bus)\n"
        "  }\n"
        "  be start() {\n"
        "    _bus!subscribe(_handler)\n"
        "  }\n"
        "}"));
}

/* ==================== 边界值 ==================== */

TEST(CodegenFullpath, BoundaryValues) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun test() {\n"
        "    var min_u8: U8 = 0\n"
        "    var max_u8: U8 = 255\n"
        "    var min_i8: I8 = -128\n"
        "    var max_i8: I8 = 127\n"
        "    var zero: U64 = 0\n"
        "    var one: U64 = 1\n"
        "    var neg: I64 = -1\n"
        "    var pi: F64 = 3.14159\n"
        "  }\n"
        "}"));
}

/* ==================== 空值处理 ==================== */

TEST(CodegenFullpath, NullHandling) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  var _data: String\n"
        "  new create() { _data = \"\" }\n"
        "  fun is_empty(): Bool { _data == \"\" }\n"
        "}"));
}
