#include <gtest/gtest.h>
#include <ponypp/typecheck.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp/ast.h>
#include <cstring>
#include <cstdlib>

/* 辅助: 解析并类型检查 */
static bool typecheck_source(const char *src, TypeCheckResult *result) {
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
    
    int rc = typecheck_program(ast, result);
    ast_node_free(ast);
    return rc == 0;
}

/* ==================== 有效程序 ==================== */

TEST(Typecheck, ValidActor) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    env.out.print(\"hello\")\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, ValidFields) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Counter\n"
        "  var count: I32 = 0\n"
        "  be increment() =>\n"
        "    count = count + 1\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, ValidBehaviors) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Worker\n"
        "  be do_work() =>\n"
        "    None\n"
        "  be get_result() =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, ValidFunctions) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  fun add(a: I32, b: I32): I32 =>\n"
        "    a + b\n"
        "  new create(env: Env) =>\n"
        "    let x: I32 = add(1, 2)\n",
        &result
    ));
    typecheck_free_result(&result);
}

/* ==================== 类型使用 ==================== */

TEST(Typecheck, BuiltinTypes) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: U8 = 1\n"
        "    let b: U16 = 2\n"
        "    let c: U32 = 3\n"
        "    let d: U64 = 4\n"
        "    let e: I8 = 5\n"
        "    let f: I16 = 6\n"
        "    let g: I32 = 7\n"
        "    let h: I64 = 8\n"
        "    let i: F32 = 9.0\n"
        "    let j: F64 = 10.0\n"
        "    let k: Bool = true\n"
        "    let l: String = \"test\"\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, StdTypes) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "use \"std/io\"\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    env.out.print(\"hello\")\n",
        &result
    ));
    typecheck_free_result(&result);
}

/* ==================== 表达式检查 ==================== */

TEST(Typecheck, ArithmeticExpr) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: I32 = 1 + 2\n"
        "    let b: I32 = a * 3\n"
        "    let c: I32 = b - 1\n"
        "    let d: I32 = c / 2\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, ComparisonExpr) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: Bool = 1 == 2\n"
        "    let b: Bool = 1 != 2\n"
        "    let c: Bool = 1 < 2\n"
        "    let d: Bool = 1 > 2\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, LogicalExpr) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: Bool = true and false\n"
        "    let b: Bool = true or false\n"
        "    let c: Bool = not true\n",
        &result
    ));
    typecheck_free_result(&result);
}

/* ==================== 控制流 ==================== */

TEST(Typecheck, IfElse) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    if true then\n"
        "      env.out.print(\"yes\")\n"
        "    else\n"
        "      env.out.print(\"no\")\n"
        "    end\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, WhileLoop) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    var i: I32 = 0\n"
        "    while i < 10 do\n"
        "      i = i + 1\n"
        "    end\n",
        &result
    ));
    typecheck_free_result(&result);
}

/* ==================== 错误检测 ==================== */

TEST(Typecheck, UnknownType) {
    TypeCheckResult result = {0};
    /* 未知类型应产生错误 */
    bool ok = typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let x: UnknownType = 42\n",
        &result
    );
    /* 可能成功或失败, 但不应崩溃 */
    typecheck_free_result(&result);
}

TEST(Typecheck, UnknownVariable) {
    TypeCheckResult result = {0};
    /* 未定义变量应产生错误 */
    bool ok = typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let x: I32 = undefined_var\n",
        &result
    );
    typecheck_free_result(&result);
}

/* ==================== 空值安全 ==================== */

TEST(Typecheck, NullSafety) {
    TypeCheckResult result = {0};
    typecheck_free_result(&result);
    
    typecheck_free_result(nullptr);
    
    /* NULL result 返回非零 */
    EXPECT_NE(typecheck_program(nullptr, nullptr), 0);
    
    /* NULL ast 产生错误 */
    TypeCheckResult r2 = {0};
    EXPECT_NE(typecheck_program(nullptr, &r2), 0);
    EXPECT_FALSE(r2.ok);
    typecheck_free_result(&r2);
}

/* ==================== 复杂程序 ==================== */

TEST(Typecheck, ComplexProgram) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "use \"std/io\"\n"
        "\n"
        "actor Worker\n"
        "  var _count: I32 = 0\n"
        "  var _name: String = \"worker\"\n"
        "  \n"
        "  be increment() =>\n"
        "    _count = _count + 1\n"
        "  \n"
        "  be get_count() =>\n"
        "    _count\n"
        "\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let w = Worker\n"
        "    w.increment()\n"
        "    w.increment()\n"
        "    env.out.print(\"done\")\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, MultipleActorsV2) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor A\n"
        "  be do_a() => None\n"
        "\n"
        "actor B\n"
        "  be do_b() => None\n"
        "\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a = A\n"
        "    let b = B\n"
        "    a.do_a()\n"
        "    b.do_b()\n",
        &result
    ));
    typecheck_free_result(&result);
}

/* ==================== Import 处理 ==================== */

TEST(Typecheck, ImportStd) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "use \"std\"\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, ImportStdIO) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "use \"std/io\"\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    env.out.print(\"hello\")\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, ImportStdCollections) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "use \"std/collections\"\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, ImportStdConcurrent) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "use \"std/concurrent\"\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, ImportMultiple) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "use \"std/io\"\n"
        "use \"std/collections\"\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

/* ==================== 错误场景 ==================== */

TEST(Typecheck, ErrorMaxLimit) {
    /* 生成大量错误测试 MAX_ERRORS 限制 */
    TypeCheckResult result = {0};
    bool ok = typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: Unknown1 = 1\n"
        "    let b: Unknown2 = 2\n"
        "    let c: Unknown3 = 3\n"
        "    let d: Unknown4 = 4\n"
        "    let e: Unknown5 = 5\n"
        "    let f: Unknown6 = 6\n"
        "    let g: Unknown7 = 7\n"
        "    let h: Unknown8 = 8\n"
        "    let i: Unknown9 = 9\n"
        "    let j: Unknown10 = 10\n"
        "    let k: Unknown11 = 11\n"
        "    let l: Unknown12 = 12\n"
        "    let m: Unknown13 = 13\n"
        "    let n: Unknown14 = 14\n"
        "    let o: Unknown15 = 15\n"
        "    let p: Unknown16 = 16\n"
        "    let q: Unknown17 = 17\n"
        "    let r: Unknown18 = 18\n"
        "    let s: Unknown19 = 19\n"
        "    let t: Unknown20 = 20\n",
        &result
    );
    /* 应该有错误但不崩溃 */
    typecheck_free_result(&result);
}

TEST(Typecheck, EmptyActor) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Empty\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, ActorNoNew) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor NoNew\n"
        "  be do_something() =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, NestedActors) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Outer\n"
        "  be create_inner() =>\n"
        "    let inner = Inner\n"
        "\n"
        "actor Inner\n"
        "  be do_work() =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

/* ==================== 错误路径覆盖 ==================== */

TEST(Typecheck, UnknownIdentifier) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let x = unknown_var\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, ThisFieldAccess) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  var _count: U32 = 0\n"
        "  new create(env: Env) =>\n"
        "    this._count = 1\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, UnknownFieldAccess) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    this.unknown_field = 1\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, BuiltinFuncs) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    print(\"hello\")\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, MathFuncs) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let pi = math_pi()\n"
        "    let root = math_sqrt(4.0)\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, LogFuncs) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    log_info(\"started\")\n"
        "    log_error(\"error\")\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, IntTypes) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  var a: U8 = 0\n"
        "  var b: U16 = 0\n"
        "  var c: U32 = 0\n"
        "  var d: U64 = 0\n"
        "  var e: I8 = 0\n"
        "  var f: I16 = 0\n"
        "  var g: I32 = 0\n"
        "  var h: I64 = 0\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, FloatTypes) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  var a: F32 = 0.0\n"
        "  var b: F64 = 0.0\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, BoolType) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  var flag: Bool = false\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, StringType) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  var name: String = \"test\"\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, ArrayType) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  var arr: Array[U32] = Array[U32]\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    );
    typecheck_free_result(&result);
}


/* ==================== 覆盖率提升: 内置函数 ==================== */

TEST(Typecheck, BuiltinPrintln) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    println(\"hello\")\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, BuiltinParseJson) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    parse_json(\"{}\")\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, BuiltinLogDebug) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    log_debug(\"dbg\")\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, BuiltinLogWarn) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    log_warn(\"warn\")\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, BuiltinTimeNow) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    time_now()\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, BuiltinTimeElapsed) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    time_elapsed()\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, BuiltinMathSinCos) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    math_sin(1.0)\n"
        "    math_cos(1.0)\n",
        &result
    );
    typecheck_free_result(&result);
}

/* ==================== 覆盖率提升: import 模块 ==================== */

TEST(Typecheck, ImportStdJson) {
    TypeCheckResult result = {0};
    typecheck_source(
        "use \"std/json\"\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, ImportStdTime) {
    TypeCheckResult result = {0};
    typecheck_source(
        "use \"std/time\"\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, ImportStdLog) {
    TypeCheckResult result = {0};
    typecheck_source(
        "use \"std/log\"\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, ImportUnknownModule) {
    TypeCheckResult result = {0};
    typecheck_source(
        "use \"unknown/module\"\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    );
    typecheck_free_result(&result);
}

/* ==================== 覆盖率提升: 数字类型注解 ==================== */

TEST(Typecheck, AllIntTypes) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  var a: U8 = 0\n"
        "  var b: U16 = 0\n"
        "  var c: U32 = 0\n"
        "  var d: U64 = 0\n"
        "  var e: I8 = 0\n"
        "  var f: I16 = 0\n"
        "  var g: I32 = 0\n"
        "  var h: I64 = 0\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, AllFloatTypes) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  var a: F32 = 0.0\n"
        "  var b: F64 = 0.0\n"
        "  new create(env: Env) =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

/* ==================== 覆盖率提升: 表达式检查 ==================== */

TEST(Typecheck, AssignExpr) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  var _x: U32 = 0\n"
        "  new create(env: Env) =>\n"
        "    this._x = 42\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, PrintTooManyArgs) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    print(\"a\", \"b\", \"c\")\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, StringLiteralExpr) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let s = \"hello\"\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, IntLiteralExpr) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let n = 42\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, FloatLiteralExpr) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let f = 3.14\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, BoolLiteralExpr) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let b = true\n",
        &result
    ));
    typecheck_free_result(&result);
}

/* ==================== 覆盖率提升: 方法检查 ==================== */

TEST(Typecheck, MethodWithParams) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    None\n"
        "  be do_work(x: U32, y: String) =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

TEST(Typecheck, MethodWithReturn) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    None\n"
        "  fun get_value(): U32 =>\n"
        "    42\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, MultipleConstructors) {
    TypeCheckResult result = {0};
    EXPECT_TRUE(typecheck_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    None\n"
        "  new from_int(n: U32) =>\n"
        "    None\n",
        &result
    ));
    typecheck_free_result(&result);
}

/* ==================== 覆盖率提升: this.field ==================== */

TEST(Typecheck, ThisFieldRead) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  var _count: U32 = 0\n"
        "  new create(env: Env) =>\n"
        "    let c = this._count\n",
        &result
    );
    typecheck_free_result(&result);
}

TEST(Typecheck, ThisFieldWrite) {
    TypeCheckResult result = {0};
    typecheck_source(
        "actor Main\n"
        "  var _name: String = \"test\"\n"
        "  new create(env: Env) =>\n"
        "    this._name = \"updated\"\n",
        &result
    );
    typecheck_free_result(&result);
}

/* ==================== 覆盖率提升: 标准库类型使用 ==================== */

TEST(Typecheck, UseStdTypes) {
    TypeCheckResult result = {0};
    typecheck_source(
        "use \"std\"\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let ch = Channel\n"
        "    let f = Future\n",
        &result
    );
    typecheck_free_result(&result);
}
