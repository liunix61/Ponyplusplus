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
    int ret = wasm_write_program(ast, "/tmp/test_wasm_cov2.wasm", target);
    (void)ret;
    unlink("/tmp/test_wasm_cov2.wasm");
    ast_node_free(ast);
}

/* ==================== 不同目标 ==================== */

TEST(WasmCov2, WriteWasiP2) {
    wasm_write("fn main() { }", TARGET_WASI_P2);
}

TEST(WasmCov2, WriteWasiP3) {
    wasm_write("fn main() { }", TARGET_WASI_P3);
}

TEST(WasmCov2, WriteComponent) {
    wasm_write("fn main() { }", TARGET_COMPONENT);
}

TEST(WasmCov2, WriteBrowser) {
    wasm_write("fn main() { }", TARGET_BROWSER);
}

TEST(WasmCov2, WriteMCU) {
    wasm_write("fn main() { }", TARGET_MCU_WASM);
}

TEST(WasmCov2, WriteNative) {
    wasm_write("fn main() { }", TARGET_NATIVE);
}

/* ==================== 复杂程序 ==================== */

TEST(WasmCov2, WriteFuncWithParams) {
    wasm_write("fn add(a: i32, b: i32) -> i32 { return a + b; }", TARGET_WASI_P2);
}

TEST(WasmCov2, WriteFuncCall) {
    wasm_write("fn main() { helper(); } fn helper() { }", TARGET_WASI_P2);
}

TEST(WasmCov2, WriteMultipleFuncs) {
    wasm_write("fn main() { } fn helper() -> i32 { return 1; } fn another() { }", TARGET_WASI_P2);
}

TEST(WasmCov2, WriteNestedIf) {
    wasm_write("fn main() { if true { if false { print(1); } } }", TARGET_WASI_P2);
}

TEST(WasmCov2, WriteWhileWithBreak) {
    wasm_write("fn main() { while true { break; } }", TARGET_WASI_P2);
}

TEST(WasmCov2, WriteWhileWithContinue) {
    wasm_write("fn main() { while true { continue; } }", TARGET_WASI_P2);
}

/* ==================== 能力类型 ==================== */

TEST(WasmCov2, WriteActorIso) {
    wasm_write("actor Counter { var count: iso i32; }", TARGET_WASI_P2);
}

TEST(WasmCov2, WriteActorTrn) {
    wasm_write("actor Buffer { var data: trn string; }", TARGET_WASI_P2);
}

TEST(WasmCov2, WriteActorRef) {
    wasm_write("actor Logger { var messages: ref Array<string>; }", TARGET_WASI_P2);
}

/* ==================== 泛型 ==================== */

TEST(WasmCov2, WriteGenericFunc) {
    wasm_write("fn identity<T>(x: T) -> T { return x; }", TARGET_WASI_P2);
}

TEST(WasmCov2, WriteGenericActor) {
    wasm_write("actor Stack<T> { var items: Array<T>; }", TARGET_WASI_P2);
}

/* ==================== 复杂表达式 ==================== */

TEST(WasmCov2, WriteChainedBinaryOps) {
    wasm_write("fn main() -> i32 { return 1 + 2 * 3 - 4; }", TARGET_WASI_P2);
}

TEST(WasmCov2, WriteComparisonOps) {
    wasm_write("fn main() -> bool { return 1 < 2 && 3 > 4; }", TARGET_WASI_P2);
}

TEST(WasmCov2, WriteUnaryOps) {
    wasm_write("fn main() -> i32 { return -42; }", TARGET_WASI_P2);
}

/* ==================== 边界情况 ==================== */

TEST(WasmCov2, WriteEmptyProgram) {
    ASTNode *ast = parse_code("");
    if (ast) {
        int ret = wasm_write_program(ast, "/tmp/test_wasm_empty.wasm", TARGET_WASI_P2);
        (void)ret;
        unlink("/tmp/test_wasm_empty.wasm");
        ast_node_free(ast);
    }
}

TEST(WasmCov2, WriteNullOutput) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        EXPECT_NE(wasm_write_program(ast, nullptr, TARGET_WASI_P2), 0);
        ast_node_free(ast);
    }
}

/* ==================== wasm_target_name ==================== */

TEST(WasmCov2, TargetNames) {
    EXPECT_STREQ(wasm_target_name(TARGET_WASI_P2), "wasi-p2");
    EXPECT_STREQ(wasm_target_name(TARGET_WASI_P3), "wasi-p3");
    EXPECT_STREQ(wasm_target_name(TARGET_COMPONENT), "component");
    EXPECT_STREQ(wasm_target_name(TARGET_BROWSER), "browser");
    EXPECT_STREQ(wasm_target_name(TARGET_MCU_WASM), "mcu-wasm");
    EXPECT_STREQ(wasm_target_name(TARGET_NATIVE), "native");
}
