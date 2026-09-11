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

/* ==================== 更多类型推断 ==================== */

TEST(TypeCov3, LetIntLiteral) {
    check_code("fn main() { let x = 42; }");
}

TEST(TypeCov3, LetFloatLiteral) {
    check_code("fn main() { let x = 3.14; }");
}

TEST(TypeCov3, LetBoolLiteral) {
    check_code("fn main() { let x = true; }");
}

TEST(TypeCov3, LetStringLiteral) {
    check_code("fn main() { let x = \"hello\"; }");
}

TEST(TypeCov3, LetBinaryAdd) {
    check_code("fn main() { let x = 1 + 2; }");
}

TEST(TypeCov3, LetBinarySub) {
    check_code("fn main() { let x = 1 - 2; }");
}

TEST(TypeCov3, LetBinaryMul) {
    check_code("fn main() { let x = 1 * 2; }");
}

TEST(TypeCov3, LetBinaryDiv) {
    check_code("fn main() { let x = 1 / 2; }");
}

TEST(TypeCov3, LetUnaryNeg) {
    check_code("fn main() { let x = -42; }");
}

TEST(TypeCov3, LetUnaryNot) {
    check_code("fn main() { let x = !true; }");
}

/* ==================== 更多函数类型检查 ==================== */

TEST(TypeCov3, FuncWithTwoParams) {
    check_code("fn add(a: i32, b: i32) -> i32 { return a + b; }");
}

TEST(TypeCov3, FuncWithThreeParams) {
    check_code("fn add3(a: i32, b: i32, c: i32) -> i32 { return a + b + c; }");
}

TEST(TypeCov3, FuncCallWithArgs) {
    check_code("fn add(a: i32, b: i32) -> i32 { return a + b; } fn main() { add(1, 2); }");
}

TEST(TypeCov3, FuncRecursiveCall) {
    check_code("fn fact(n: i32) -> i32 { if n <= 1 { return 1; } return n * fact(n - 1); }");
}

/* ==================== 更多控制流 ==================== */

TEST(TypeCov3, IfElseIf) {
    check_code("fn main() { if true { print(1); } else if false { print(2); } }");
}

TEST(TypeCov3, IfElseIfElse) {
    check_code("fn main() { if true { print(1); } else if false { print(2); } else { print(3); } }");
}

TEST(TypeCov3, WhileWithCondition) {
    check_code("fn main() { while 1 < 2 { print(1); } }");
}

TEST(TypeCov3, NestedWhile) {
    check_code("fn main() { while false { while false { print(1); } } }");
}

/* ==================== 更多能力类型 ==================== */

TEST(TypeCov3, ActorWithIsoField) {
    check_code("actor Counter { var count: iso i32; }");
}

TEST(TypeCov3, ActorWithTrnField) {
    check_code("actor Buffer { var data: trn string; }");
}

TEST(TypeCov3, ActorWithRefField) {
    check_code("actor Logger { var messages: ref Array<string>; }");
}

TEST(TypeCov3, ActorWithValField) {
    check_code("actor Config { var settings: val Map<string, string>; }");
}

TEST(TypeCov3, ActorWithBoxField) {
    check_code("actor Cache { var items: box Map<string, i32>; }");
}

TEST(TypeCov3, ActorWithTagField) {
    check_code("actor Sender { var receiver: tag Receiver; }");
}

/* ==================== 更多泛型 ==================== */

TEST(TypeCov3, GenericFuncWithTwoParams) {
    check_code("fn pair<A, B>(a: A, b: B) -> (A, B) { return (a, b); }");
}

TEST(TypeCov3, GenericActorWithConstraint) {
    check_code("actor Stack<T: Comparable> { var items: Array<T>; }");
}

/* ==================== 更多错误情况 ==================== */

TEST(TypeCov3, TypeMismatchIntString) {
    check_code("fn main() { let x: i32 = \"hello\"; }");
}

TEST(TypeCov3, TypeMismatchBoolInt) {
    check_code("fn main() { let x: bool = 42; }");
}

TEST(TypeCov3, UndefinedVariable) {
    check_code("fn main() { print(undefined_var); }");
}

TEST(TypeCov3, UndefinedFunction) {
    check_code("fn main() { undefined_func(42); }");
}

TEST(TypeCov3, WrongArgCountTooFew) {
    check_code("fn add(a: i32, b: i32) -> i32 { return a + b; } fn main() { add(1); }");
}

TEST(TypeCov3, WrongArgCountTooMany) {
    check_code("fn add(a: i32, b: i32) -> i32 { return a + b; } fn main() { add(1, 2, 3); }");
}

TEST(TypeCov3, WrongArgType) {
    check_code("fn add(a: i32, b: i32) -> i32 { return a + b; } fn main() { add(1, \"hello\"); }");
}

TEST(TypeCov3, ReturnTypeMismatch) {
    check_code("fn main() -> i32 { return \"hello\"; }");
}

/* ==================== 更多复杂表达式 ==================== */

TEST(TypeCov3, ChainedAddSub) {
    check_code("fn main() -> i32 { return 1 + 2 - 3; }");
}

TEST(TypeCov3, ChainedMulDiv) {
    check_code("fn main() -> i32 { return 1 * 2 / 3; }");
}

TEST(TypeCov3, MixedPrecedence) {
    check_code("fn main() -> i32 { return 1 + 2 * 3; }");
}

TEST(TypeCov3, ComparisonChain) {
    check_code("fn main() -> bool { return 1 < 2 && 2 < 3; }");
}

TEST(TypeCov3, LogicalOr) {
    check_code("fn main() -> bool { return true || false; }");
}

TEST(TypeCov3, LogicalAnd) {
    check_code("fn main() -> bool { return true && false; }");
}

/* ==================== 边界情况 ==================== */

TEST(TypeCov3, EmptyProgram) {
    check_code("");
}

TEST(TypeCov3, SingleEmptyFunc) {
    check_code("fn main() { }");
}

TEST(TypeCov3, MultipleEmptyFuncs) {
    check_code("fn main() { } fn helper() { } fn another() { }");
}
