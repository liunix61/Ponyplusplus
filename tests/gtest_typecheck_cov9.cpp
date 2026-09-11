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

/* std.concurrent + Channel */
TEST(TypecheckCov9, StdConcurrentChannel) {
    EXPECT_TRUE(tc_ok("use \"std.concurrent\"\n\nactor main {\n  new create() => {\n    let ch: Channel = Channel\n    print(\"channel\")\n  }\n}\n"));
}

/* std.concurrent + Future */
TEST(TypecheckCov9, StdConcurrentFuture) {
    EXPECT_TRUE(tc_ok("use \"std.concurrent\"\n\nactor main {\n  new create() => {\n    let f: Future = Future\n    print(\"future\")\n  }\n}\n"));
}

/* std.concurrent + Mutex */
TEST(TypecheckCov9, StdConcurrentMutex) {
    EXPECT_TRUE(tc_ok("use \"std.concurrent\"\n\nactor main {\n  new create() => {\n    let m: Mutex = Mutex\n    print(\"mutex\")\n  }\n}\n"));
}

/* std.concurrent + ActorGroup */
TEST(TypecheckCov9, StdConcurrentActorGroup) {
    EXPECT_TRUE(tc_ok("use \"std.concurrent\"\n\nactor main {\n  new create() => {\n    let g: ActorGroup = ActorGroup\n    print(\"group\")\n  }\n}\n"));
}

/* std.io + File */
TEST(TypecheckCov9, StdIoFile) {
    EXPECT_TRUE(tc_ok("use \"std.io\"\n\nactor main {\n  new create() => {\n    let f: File = File\n    print(\"file\")\n  }\n}\n"));
}

/* std.io + Path */
TEST(TypecheckCov9, StdIoPath) {
    EXPECT_TRUE(tc_ok("use \"std.io\"\n\nactor main {\n  new create() => {\n    let p: Path = Path\n    print(\"path\")\n  }\n}\n"));
}

/* std.json + JSON */
TEST(TypecheckCov9, StdJsonJSON) {
    EXPECT_TRUE(tc_ok("use \"std.json\"\n\nactor main {\n  new create() => {\n    let j: JSON = JSON\n    print(\"json\")\n  }\n}\n"));
}

/* std.time + Timer */
TEST(TypecheckCov9, StdTimeTimer) {
    EXPECT_TRUE(tc_ok("use \"std.time\"\n\nactor main {\n  new create() => {\n    let t: Timer = Timer\n    print(\"timer\")\n  }\n}\n"));
}

/* std.log + Logger */
TEST(TypecheckCov9, StdLogLogger) {
    EXPECT_TRUE(tc_ok("use \"std.log\"\n\nactor main {\n  new create() => {\n    let l: Logger = Logger\n    print(\"logger\")\n  }\n}\n"));
}

/* std wildcard + Channel */
TEST(TypecheckCov9, StdWildcardChannel) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let ch: Channel = Channel\n    print(\"channel\")\n  }\n}\n"));
}

/* std wildcard + File */
TEST(TypecheckCov9, StdWildcardFile) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let f: File = File\n    print(\"file\")\n  }\n}\n"));
}

/* std wildcard + JSON */
TEST(TypecheckCov9, StdWildcardJSON) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let j: JSON = JSON\n    print(\"json\")\n  }\n}\n"));
}

/* std wildcard + Timer */
TEST(TypecheckCov9, StdWildcardTimer) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let t: Timer = Timer\n    print(\"timer\")\n  }\n}\n"));
}

/* std wildcard + Logger */
TEST(TypecheckCov9, StdWildcardLogger) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let l: Logger = Logger\n    print(\"logger\")\n  }\n}\n"));
}

/* std wildcard + Future */
TEST(TypecheckCov9, StdWildcardFuture) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let f: Future = Future\n    print(\"future\")\n  }\n}\n"));
}

/* std wildcard + Mutex */
TEST(TypecheckCov9, StdWildcardMutex) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let m: Mutex = Mutex\n    print(\"mutex\")\n  }\n}\n"));
}

/* std wildcard + Path */
TEST(TypecheckCov9, StdWildcardPath) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let p: Path = Path\n    print(\"path\")\n  }\n}\n"));
}

/* print 带多参数（NODE_CALL child_count > 1） */
TEST(TypecheckCov9, PrintMultiArgCall) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    print(\"a\", \"b\", \"c\")\n  }\n}\n"));
}

/* println 带多参数 */
TEST(TypecheckCov9, PrintlnMultiArgCall) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    println(\"a\", \"b\")\n  }\n}\n"));
}

/* log_debug 多参数 */
TEST(TypecheckCov9, LogDebugMultiArg) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    log_debug(\"msg\", 42)\n  }\n}\n"));
}

/* log_info 多参数 */
TEST(TypecheckCov9, LogInfoMultiArg) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    log_info(\"msg\", 42)\n  }\n}\n"));
}

/* log_warn 多参数 */
TEST(TypecheckCov9, LogWarnMultiArg) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    log_warn(\"msg\", 42)\n  }\n}\n"));
}

/* log_error 多参数 */
TEST(TypecheckCov9, LogErrorMultiArg) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    log_error(\"msg\", 42)\n  }\n}\n"));
}

/* parse_json 带参数 */
TEST(TypecheckCov9, ParseJsonWithArg) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x = parse_json(\"{}\")\n    print(x)\n  }\n}\n"));
}

/* 数字类型 U8 */
TEST(TypecheckCov9, TypeU8) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: U8 = 1\n    print(x)\n  }\n}\n"));
}

/* 数字类型 I64 */
TEST(TypecheckCov9, TypeI64) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: I64 = -1\n    print(x)\n  }\n}\n"));
}

/* 数字类型 F64 */
TEST(TypecheckCov9, TypeF64) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: F64 = 3.14\n    print(x)\n  }\n}\n"));
}

/* 数字类型 F32 */
TEST(TypecheckCov9, TypeF32) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: F32 = 1.0\n    print(x)\n  }\n}\n"));
}

/* 布尔类型 */
TEST(TypecheckCov9, TypeBool) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: Bool = true\n    print(x)\n  }\n}\n"));
}

/* 字符串类型 */
TEST(TypecheckCov9, TypeString) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let x: String = \"hello\"\n    print(x)\n  }\n}\n"));
}

/* 字段访问 */
TEST(TypecheckCov9, FieldAccess) {
    EXPECT_TRUE(tc_ok("actor main {\n  var count: U64 = 0\n  new create() => {\n    this.count = 1\n    print(this.count)\n  }\n}\n"));
}

/* 多个字段 */
TEST(TypecheckCov9, MultipleFields) {
    EXPECT_TRUE(tc_ok("actor main {\n  var a: U64 = 0\n  var b: String = \"\"\n  var c: Bool = false\n  new create() => {\n    this.a = 1\n    this.b = \"hello\"\n    this.c = true\n  }\n}\n"));
}

/* let 字段 */
TEST(TypecheckCov9, LetField) {
    EXPECT_TRUE(tc_ok("actor main {\n  let name: String = \"test\"\n  new create() => {\n    print(this.name)\n  }\n}\n"));
}

/* 数组类型 */
TEST(TypecheckCov9, ArrayType) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let arr: Array = Array\n    print(\"array\")\n  }\n}\n"));
}

/* 元组类型 */
TEST(TypecheckCov9, TupleType) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let t: Tuple = Tuple\n    print(\"tuple\")\n  }\n}\n"));
}

/* union 类型 */
TEST(TypecheckCov9, UnionType) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let u: Union = Union\n    print(\"union\")\n  }\n}\n"));
}

/* None 类型 */
TEST(TypecheckCov9, NoneType) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let n: None = None\n    print(\"none\")\n  }\n}\n"));
}

/* Any 类型 */
TEST(TypecheckCov9, AnyType) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let a: Any = Any\n    print(\"any\")\n  }\n}\n"));
}

/* Generic 类型 */
TEST(TypecheckCov9, GenericType) {
    EXPECT_TRUE(tc_ok("actor main {\n  new create() => {\n    let g: Generic = Generic\n    print(\"generic\")\n  }\n}\n"));
}

/* 多个 std 模块导入 */
TEST(TypecheckCov9, MultipleStdModules) {
    EXPECT_TRUE(tc_ok("use \"std.concurrent\"\nuse \"std.io\"\nuse \"std.json\"\nuse \"std.time\"\nuse \"std.log\"\n\nactor main {\n  new create() => {\n    print(\"multi\")\n  }\n}\n"));
}

/* std 模块 + actor 跨引用 */
TEST(TypecheckCov9, StdModuleActorCrossRef) {
    EXPECT_TRUE(tc_ok("use \"std.concurrent\"\n\nactor Worker {\n  new create() => {}\n  be process() => {\n    print(\"processing\")\n  }\n}\n\nactor main {\n  new create() => {\n    let w = Worker\n    print(\"main\")\n  }\n}\n"));
}

/* std 模块 + 函数 */

/* std 模块 + 递归 */
TEST(TypecheckCov9, StdModuleRecursive) {
    EXPECT_TRUE(tc_ok("use \"std.math\"\n\nactor main {\n  new create() => {\n    let x = factorial(5)\n    print(x)\n  }\n  fun factorial(n: U64): U64 =>\n    if n <= 1 then 1 else n * factorial(n - 1)\n}\n"));
}

/* std 模块 + match */
TEST(TypecheckCov9, StdModuleMatch) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    let x = 1\n    match x\n    | 1 => print(\"one\")\n    | 2 => print(\"two\")\n    else\n      print(\"other\")\n    end\n  }\n}\n"));
}

/* std 模块 + for */
TEST(TypecheckCov9, StdModuleFor) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    for i in Range(0, 10) do\n      print(i)\n    end\n  }\n}\n"));
}

/* std 模块 + while */
TEST(TypecheckCov9, StdModuleWhile) {
    EXPECT_TRUE(tc_ok("use \"std\"\n\nactor main {\n  new create() => {\n    var i = 0\n    while i < 10 do\n      print(i)\n      i = i + 1\n    end\n  }\n}\n"));
}
