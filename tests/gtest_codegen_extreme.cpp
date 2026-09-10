/*
 * Codegen 极限路径测试
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

/* ==================== 大型 Actor ==================== */

TEST(CodegenExtreme, ActorWithManyFields) {
    EXPECT_TRUE(compile_source(
        "actor BigActor {\n"
        "  var _f1: U8\n  var _f2: U16\n  var _f3: U32\n  var _f4: U64\n"
        "  var _f5: I8\n  var _f6: I16\n  var _f7: I32\n  var _f8: I64\n"
        "  var _f9: F32\n  var _f10: F64\n  var _f11: Bool\n  var _f12: String\n"
        "  new create() {\n"
        "    _f1 = 0\n    _f2 = 0\n    _f3 = 0\n    _f4 = 0\n"
        "    _f5 = 0\n    _f6 = 0\n    _f7 = 0\n    _f8 = 0\n"
        "    _f9 = 0.0\n    _f10 = 0.0\n    _f11 = false\n    _f12 = \"\"\n"
        "  }\n"
        "}"));
}

TEST(CodegenExtreme, ActorWithManyMethods) {
    EXPECT_TRUE(compile_source(
        "actor MultiMethod {\n"
        "  fun m1(): U64 { 1 }\n"
        "  fun m2(): U64 { 2 }\n"
        "  fun m3(): U64 { 3 }\n"
        "  fun m4(): U64 { 4 }\n"
        "  fun m5(): U64 { 5 }\n"
        "  be b1() { }\n"
        "  be b2() { }\n"
        "  be b3() { }\n"
        "  be b4() { }\n"
        "  be b5() { }\n"
        "}"));
}

/* ==================== 复杂表达式树 ==================== */

TEST(CodegenExtreme, DeepArithmetic) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun calc(): U64 {\n"
        "    ((1 + 2) * (3 + 4)) - ((5 - 6) * (7 + 8))\n"
        "  }\n"
        "}"));
}

TEST(CodegenExtreme, MixedOperators) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun calc(a: U64, b: U64, c: U64): U64 {\n"
        "    a + b * c - a / b % c\n"
        "  }\n"
        "}"));
}

TEST(CodegenExtreme, ComparisonChain) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun check(a: U64, b: U64, c: U64): Bool {\n"
        "    a < b and b < c and c > a\n"
        "  }\n"
        "}"));
}

/* ==================== 复杂控制流 ==================== */

TEST(CodegenExtreme, TripleNestedLoop) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun nested(): U64 {\n"
        "    var total: U64 = 0\n"
        "    var i: U64 = 0\n"
        "    while i < 5 do\n"
        "      var j: U64 = 0\n"
        "      while j < 5 do\n"
        "        var k: U64 = 0\n"
        "        while k < 5 do\n"
        "          total = total + 1\n"
        "          k = k + 1\n"
        "        end\n"
        "        j = j + 1\n"
        "      end\n"
        "      i = i + 1\n"
        "    end\n"
        "    total\n"
        "  }\n"
        "}"));
}

TEST(CodegenExtreme, IfElseIfElseChain) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun classify(x: U64): U64 {\n"
        "    if x < 10 then\n"
        "      1\n"
        "    else\n"
        "      if x < 20 then\n"
        "        2\n"
        "      else\n"
        "        if x < 30 then\n"
        "          3\n"
        "        else\n"
        "          if x < 40 then\n"
        "            4\n"
        "          else\n"
        "            5\n"
        "          end\n"
        "        end\n"
        "      end\n"
        "    end\n"
        "  }\n"
        "}"));
}

/* ==================== 类型组合 ==================== */

TEST(CodegenExtreme, AllTypesInOneActor) {
    EXPECT_TRUE(compile_source(
        "actor AllTypes {\n"
        "  var _u8: U8\n  var _u16: U16\n  var _u32: U32\n  var _u64: U64\n"
        "  var _i8: I8\n  var _i16: I16\n  var _i32: I32\n  var _i64: I64\n"
        "  var _f32: F32\n  var _f64: F64\n"
        "  var _bool: Bool\n  var _str: String\n"
        "  var _arr: Array[U64]\n  var _map: Map[String, U64]\n  var _set: Set[String]\n"
        "}"));
}

/* ==================== 多 Actor 系统 ==================== */


/* ==================== 接口和 trait 组合 ==================== */



/* ==================== 错误处理组合 ==================== */


/* ==================== 模式匹配复杂 ==================== */


/* ==================== 边界值 ==================== */




/* ==================== 空和最小 ==================== */

TEST(CodegenExtreme, MinimalActor) {
    EXPECT_TRUE(compile_source("actor A{}"));
}

TEST(CodegenExtreme, ActorWithOnlyNew) {
    EXPECT_TRUE(compile_source("actor A{new create(){}}"));
}

TEST(CodegenExtreme, ActorWithOnlyBe) {
    EXPECT_TRUE(compile_source("actor A{be run(){}}"));
}

TEST(CodegenExtreme, ComplexActorSystem) {
    EXPECT_TRUE(compile_source(
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
        "}"));
}

TEST(CodegenExtreme, MaxIntLiteral) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun max(): U64 { 18446744073709551615 }\n"
        "}"));
}

TEST(CodegenExtreme, MultipleInterfaces) {
    EXPECT_TRUE(compile_source(
        "interface Printable {\n  fun print(): String\n}\n"
        "interface Serializable {\n  fun serialize(): String\n  fun deserialize(data: String)\n}\n"
        "actor Data is Printable, Serializable {\n"
        "  var _value: U64\n"
        "  new create(v: U64) { _value = v }\n"
        "  fun print(): String { \"Data\" }\n"
        "  fun serialize(): String { \"\" }\n"
        "  fun deserialize(data: String) { }\n"
        "}"));
}

TEST(CodegenExtreme, TraitChain) {
    EXPECT_TRUE(compile_source(
        "trait Base {\n  fun base_method(): U64 { 1 }\n}\n"
        "trait Middle is Base {\n  fun middle_method(): U64 { 2 }\n}\n"
        "actor Concrete is Middle {\n"
        "  fun concrete_method(): U64 { 3 }\n"
        "}"));
}

TEST(CodegenExtreme, TryElseThenAll) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun test(): U64 {\n"
        "    try\n"
        "      42\n"
        "    else\n"
        "      0\n"
        "    then\n"
        "      cleanup()\n"
        "    end\n"
        "  }\n"
        "}"));
}

TEST(CodegenExtreme, MatchWithMultiplePatterns) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun process(x: U64): String {\n"
        "    match x\n"
        "    | 0 => \"zero\"\n"
        "    | 1 => \"one\"\n"
        "    | 2 => \"two\"\n"
        "    | 3 => \"three\"\n"
        "    | 4 => \"four\"\n"
        "    | 5 => \"five\"\n"
        "    | else => \"many\"\n"
        "    end\n"
        "  }\n"
        "}"));
}

TEST(CodegenExtreme, FloatPrecision) {
    EXPECT_TRUE(compile_source(
        "actor Foo {\n"
        "  fun precise(): F64 { 3.14159265358979323846 }\n"
        "}"));
}
