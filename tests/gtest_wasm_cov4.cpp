#include <gtest/gtest.h>
#include <ponypp/wasm.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

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

static void wasm_write(const char *code, TargetKind target) {
    ASTNode *ast = parse_code(code);
    if (!ast) return;
    int ret = wasm_write_program(ast, "/tmp/test_wasm_cov4.wasm", target);
    (void)ret;
    unlink("/tmp/test_wasm_cov4.wasm");
    ast_node_free(ast);
}

/* ==================== 更多表达式类型组合 ==================== */

TEST(WasmCov4, WriteIntLiteral) {
    wasm_write("fn main() -> i32 { return 42; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteFloatLiteral) {
    wasm_write("fn main() -> f64 { return 3.14; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteBoolLiteral) {
    wasm_write("fn main() -> bool { return true; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteStringLiteral) {
    wasm_write("fn main() { print(\"hello\"); }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteBinaryAdd) {
    wasm_write("fn main() -> i32 { return 1 + 2; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteBinarySub) {
    wasm_write("fn main() -> i32 { return 1 - 2; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteBinaryMul) {
    wasm_write("fn main() -> i32 { return 1 * 2; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteBinaryDiv) {
    wasm_write("fn main() -> i32 { return 1 / 2; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteUnaryNeg) {
    wasm_write("fn main() -> i32 { return -42; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteUnaryNot) {
    wasm_write("fn main() -> bool { return !true; }", TARGET_WASI_P2);
}

/* ==================== 更多语句类型组合 ==================== */

TEST(WasmCov4, WriteLetInt) {
    wasm_write("fn main() { let x: i32 = 1; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteLetFloat) {
    wasm_write("fn main() { let x: f64 = 3.14; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteLetBool) {
    wasm_write("fn main() { let x: bool = true; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteLetString) {
    wasm_write("fn main() { let x: string = \"hello\"; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteLetBinaryOp) {
    wasm_write("fn main() { let x: i32 = 1 + 2; }", TARGET_WASI_P2);
}

/* ==================== 更多控制流组合 ==================== */

TEST(WasmCov4, WriteIfTrue) {
    wasm_write("fn main() { if true { print(1); } }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteIfFalse) {
    wasm_write("fn main() { if false { print(1); } }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteIfElse) {
    wasm_write("fn main() { if true { print(1); } else { print(2); } }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteWhileTrue) {
    wasm_write("fn main() { while true { break; } }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteWhileFalse) {
    wasm_write("fn main() { while false { print(1); } }", TARGET_WASI_P2);
}

/* ==================== 更多函数组合 ==================== */

TEST(WasmCov4, WriteFuncNoParams) {
    wasm_write("fn main() { }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteFuncOneParam) {
    wasm_write("fn f(x: i32) -> i32 { return x; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteFuncTwoParams) {
    wasm_write("fn add(a: i32, b: i32) -> i32 { return a + b; }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteFuncCall) {
    wasm_write("fn main() { helper(); } fn helper() { }", TARGET_WASI_P2);
}

TEST(WasmCov4, WriteFuncCallWithArgs) {
    wasm_write("fn add(a: i32, b: i32) -> i32 { return a + b; } fn main() { add(1, 2); }", TARGET_WASI_P2);
}

/* ==================== 更多目标组合 ==================== */

TEST(WasmCov4, WriteAllTargets) {
    wasm_write("fn main() { }", TARGET_WASI_P2);
    wasm_write("fn main() { }", TARGET_WASI_P3);
    wasm_write("fn main() { }", TARGET_COMPONENT);
    wasm_write("fn main() { }", TARGET_BROWSER);
    wasm_write("fn main() { }", TARGET_MCU_WASM);
    wasm_write("fn main() { }", TARGET_NATIVE);
}

/* ==================== 更多边界情况组合 ==================== */

TEST(WasmCov4, WriteEmptyProgram) {
    ASTNode *ast = parse_code("");
    if (ast) {
        int ret = wasm_write_program(ast, "/tmp/test_wasm_empty_cov4.wasm", TARGET_WASI_P2);
        (void)ret;
        unlink("/tmp/test_wasm_empty_cov4.wasm");
        ast_node_free(ast);
    }
}

TEST(WasmCov4, WriteNullOutput) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        EXPECT_NE(wasm_write_program(ast, nullptr, TARGET_WASI_P2), 0);
        ast_node_free(ast);
    }
}

TEST(WasmCov4, WriteNullAst) {
    EXPECT_EQ(wasm_write_program(nullptr, "/tmp/test.wasm", TARGET_WASI_P2), 0);
}
