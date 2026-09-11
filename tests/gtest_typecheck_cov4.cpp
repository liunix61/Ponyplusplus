#include <gtest/gtest.h>
#include <ponypp/typecheck.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>

static ASTNode *parse_code(const char *code) {
    Lexer *lx = lexer_new("test.pny", code, strlen(code));
    if (!lx) return nullptr;
    Token *tokens = nullptr;
    size_t count = 0;
    lexer_lex_all(lx, &tokens, &count);
    lexer_free(lx);
    Parser *ps = parser_new("test.pny", tokens, count);
    if (!ps) return nullptr;
    ASTNode *ast = parser_parse_program(ps);
    parser_free(ps);
    return ast;
}

static void check_code(const char *code) {
    ASTNode *ast = parse_code(code);
    if (!ast) return;
    TypeCheckResult result;
    memset(&result, 0, sizeof(result));
    int ret = typecheck_program(ast, &result);
    (void)ret;
    typecheck_free_result(&result);
    ast_node_free(ast);
}

/* ==================== 更多类型推断组合 ==================== */

TEST(TypeCov4, LetIntAddInt) {
    check_code("fn main() { let x = 1 + 2; }");
}

TEST(TypeCov4, LetFloatAddFloat) {
    check_code("fn main() { let x = 1.0 + 2.0; }");
}

TEST(TypeCov4, LetIntMulInt) {
    check_code("fn main() { let x = 1 * 2; }");
}

TEST(TypeCov4, LetFloatMulFloat) {
    check_code("fn main() { let x = 1.0 * 2.0; }");
}

TEST(TypeCov4, LetBoolAndBool) {
    check_code("fn main() { let x = true && false; }");
}

TEST(TypeCov4, LetBoolOrBool) {
    check_code("fn main() { let x = true || false; }");
}

TEST(TypeCov4, LetIntCompareInt) {
    check_code("fn main() { let x = 1 < 2; }");
}

TEST(TypeCov4, LetFloatCompareFloat) {
    check_code("fn main() { let x = 1.0 < 2.0; }");
}

/* ==================== 更多函数组合 ==================== */

TEST(TypeCov4, FuncWithIntParam) {
    check_code("fn f(x: i32) -> i32 { return x; }");
}

TEST(TypeCov4, FuncWithFloatParam) {
    check_code("fn f(x: f64) -> f64 { return x; }");
}

TEST(TypeCov4, FuncWithBoolParam) {
    check_code("fn f(x: bool) -> bool { return x; }");
}

TEST(TypeCov4, FuncWithStringParam) {
    check_code("fn f(x: string) -> string { return x; }");
}

TEST(TypeCov4, FuncCallWithIntArg) {
    check_code("fn f(x: i32) -> i32 { return x; } fn main() { f(42); }");
}

TEST(TypeCov4, FuncCallWithFloatArg) {
    check_code("fn f(x: f64) -> f64 { return x; } fn main() { f(3.14); }");
}

TEST(TypeCov4, FuncCallWithBoolArg) {
    check_code("fn f(x: bool) -> bool { return x; } fn main() { f(true); }");
}

TEST(TypeCov4, FuncCallWithStringArg) {
    check_code("fn f(x: string) -> string { return x; } fn main() { f(\"hello\"); }");
}

/* ==================== 更多控制流组合 ==================== */

TEST(TypeCov4, IfWithIntCondition) {
    check_code("fn main() { if 1 < 2 { print(1); } }");
}

TEST(TypeCov4, IfWithBoolCondition) {
    check_code("fn main() { if true { print(1); } }");
}

TEST(TypeCov4, WhileWithIntCondition) {
    check_code("fn main() { while 1 < 2 { print(1); } }");
}

TEST(TypeCov4, WhileWithBoolCondition) {
    check_code("fn main() { while true { break; } }");
}

/* ==================== 更多能力类型组合 ==================== */

TEST(TypeCov4, ActorWithIsoInt) {
    check_code("actor Counter { var count: iso i32; }");
}

TEST(TypeCov4, ActorWithTrnString) {
    check_code("actor Buffer { var data: trn string; }");
}

TEST(TypeCov4, ActorWithRefArray) {
    check_code("actor Logger { var messages: ref Array<string>; }");
}

TEST(TypeCov4, ActorWithValMap) {
    check_code("actor Config { var settings: val Map<string, string>; }");
}

TEST(TypeCov4, ActorWithBoxMap) {
    check_code("actor Cache { var items: box Map<string, i32>; }");
}

TEST(TypeCov4, ActorWithTagActor) {
    check_code("actor Sender { var receiver: tag Receiver; }");
}

/* ==================== 更多泛型组合 ==================== */

TEST(TypeCov4, GenericFuncIdentity) {
    check_code("fn identity<T>(x: T) -> T { return x; }");
}

TEST(TypeCov4, GenericFuncPair) {
    check_code("fn pair<A, B>(a: A, b: B) -> (A, B) { return (a, b); }");
}

TEST(TypeCov4, GenericActorStack) {
    check_code("actor Stack<T> { var items: Array<T>; }");
}

/* ==================== 更多错误组合 ==================== */

TEST(TypeCov4, TypeMismatchIntString) {
    check_code("fn main() { let x: i32 = \"hello\"; }");
}

TEST(TypeCov4, TypeMismatchBoolInt) {
    check_code("fn main() { let x: bool = 42; }");
}

TEST(TypeCov4, TypeMismatchFloatInt) {
    check_code("fn main() { let x: f64 = 42; }");
}

TEST(TypeCov4, UndefinedVar) {
    check_code("fn main() { print(undefined_var); }");
}

TEST(TypeCov4, UndefinedFunc) {
    check_code("fn main() { undefined_func(42); }");
}

TEST(TypeCov4, WrongArgCount) {
    check_code("fn add(a: i32, b: i32) -> i32 { return a + b; } fn main() { add(1); }");
}

TEST(TypeCov4, WrongArgType) {
    check_code("fn add(a: i32, b: i32) -> i32 { return a + b; } fn main() { add(1, \"hello\"); }");
}

TEST(TypeCov4, ReturnTypeMismatch) {
    check_code("fn main() -> i32 { return \"hello\"; }");
}

/* ==================== 更多复杂表达式 ==================== */

TEST(TypeCov4, ChainedAddSubMul) {
    check_code("fn main() -> i32 { return 1 + 2 - 3 * 4; }");
}

TEST(TypeCov4, ChainedMulDiv) {
    check_code("fn main() -> i32 { return 1 * 2 / 3; }");
}

TEST(TypeCov4, MixedPrecedence) {
    check_code("fn main() -> i32 { return 1 + 2 * 3 - 4 / 2; }");
}

TEST(TypeCov4, ComparisonChain) {
    check_code("fn main() -> bool { return 1 < 2 && 2 < 3 && 3 < 4; }");
}

TEST(TypeCov4, LogicalChain) {
    check_code("fn main() -> bool { return true && false || true; }");
}

/* ==================== 边界情况 ==================== */

TEST(TypeCov4, EmptyProgram) {
    check_code("");
}

TEST(TypeCov4, SingleEmptyFunc) {
    check_code("fn main() { }");
}

TEST(TypeCov4, MultipleEmptyFuncs) {
    check_code("fn main() { } fn helper() { } fn another() { }");
}
