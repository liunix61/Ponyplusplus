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

/* ==================== 基础类型检查 ==================== */

TEST(TypeCov2, EmptyProgram) {
    check_code("");
}

TEST(TypeCov2, EmptyFunc) {
    check_code("fn main() { }");
}

TEST(TypeCov2, FuncWithLet) {
    check_code("fn main() { let x: i32 = 1; }");
}

TEST(TypeCov2, FuncWithReturn) {
    check_code("fn main() -> i32 { return 42; }");
}

TEST(TypeCov2, FuncWithPrint) {
    check_code("fn main() { print(42); }");
}

/* ==================== 类型推断 ==================== */

TEST(TypeCov2, LetIntInfer) {
    check_code("fn main() { let x = 42; }");
}

TEST(TypeCov2, LetFloatInfer) {
    check_code("fn main() { let x = 3.14; }");
}

TEST(TypeCov2, LetBoolInfer) {
    check_code("fn main() { let x = true; }");
}

TEST(TypeCov2, LetStringInfer) {
    check_code("fn main() { let x = \"hello\"; }");
}

TEST(TypeCov2, LetBinaryOp) {
    check_code("fn main() { let x = 1 + 2; }");
}

TEST(TypeCov2, LetUnaryOp) {
    check_code("fn main() { let x = -42; }");
}

/* ==================== 函数类型检查 ==================== */

TEST(TypeCov2, FuncParams) {
    check_code("fn add(a: i32, b: i32) -> i32 { return a + b; }");
}

TEST(TypeCov2, FuncCall) {
    check_code("fn main() { add(1, 2); } fn add(a: i32, b: i32) -> i32 { return a + b; }");
}

TEST(TypeCov2, FuncRecursive) {
    check_code("fn fact(n: i32) -> i32 { if n <= 1 { return 1; } return n * fact(n - 1); }");
}

/* ==================== 控制流类型检查 ==================== */

TEST(TypeCov2, IfElse) {
    check_code("fn main() { if true { print(1); } else { print(2); } }");
}

TEST(TypeCov2, WhileLoop) {
    check_code("fn main() { while false { print(1); } }");
}

TEST(TypeCov2, NestedIf) {
    check_code("fn main() { if true { if false { print(1); } } }");
}

TEST(TypeCov2, WhileWithBreak) {
    check_code("fn main() { while true { break; } }");
}

TEST(TypeCov2, WhileWithContinue) {
    check_code("fn main() { while true { continue; } }");
}

/* ==================== 能力类型检查 ==================== */

TEST(TypeCov2, ActorWithIso) {
    check_code("actor Counter { var count: iso i32; }");
}

TEST(TypeCov2, ActorWithTrn) {
    check_code("actor Buffer { var data: trn string; }");
}

TEST(TypeCov2, ActorWithRef) {
    check_code("actor Logger { var messages: ref Array<string>; }");
}

TEST(TypeCov2, ActorWithVal) {
    check_code("actor Config { var settings: val Map<string, string>; }");
}

TEST(TypeCov2, ActorWithBox) {
    check_code("actor Cache { var items: box Map<string, i32>; }");
}

TEST(TypeCov2, ActorWithTag) {
    check_code("actor Sender { var receiver: tag Receiver; }");
}

/* ==================== 泛型类型检查 ==================== */

TEST(TypeCov2, GenericFunc) {
    check_code("fn identity<T>(x: T) -> T { return x; }");
}

TEST(TypeCov2, GenericActor) {
    check_code("actor Stack<T> { var items: Array<T>; }");
}

/* ==================== 错误情况 ==================== */

TEST(TypeCov2, TypeMismatch) {
    check_code("fn main() { let x: i32 = \"hello\"; }");
}

TEST(TypeCov2, UndefinedVar) {
    check_code("fn main() { print(undefined_var); }");
}

TEST(TypeCov2, UndefinedFunc) {
    check_code("fn main() { undefined_func(42); }");
}

TEST(TypeCov2, WrongArgCount) {
    check_code("fn add(a: i32, b: i32) -> i32 { return a + b; } fn main() { add(1); }");
}

TEST(TypeCov2, WrongArgType) {
    check_code("fn add(a: i32, b: i32) -> i32 { return a + b; } fn main() { add(1, \"hello\"); }");
}

TEST(TypeCov2, ReturnTypeMismatch) {
    check_code("fn main() -> i32 { return \"hello\"; }");
}

/* ==================== 复杂表达式 ==================== */

TEST(TypeCov2, ChainedBinaryOps) {
    check_code("fn main() -> i32 { return 1 + 2 * 3 - 4; }");
}

TEST(TypeCov2, NestedFuncCalls) {
    check_code("fn f(x: i32) -> i32 { return x; } fn main() { f(f(f(42))); }");
}

TEST(TypeCov2, ComparisonOps) {
    check_code("fn main() -> bool { return 1 < 2 && 3 > 4 || 5 == 5; }");
}

TEST(TypeCov2, UnaryOps) {
    check_code("fn main() -> i32 { return -42; } fn main2() -> bool { return !true; }");
}
