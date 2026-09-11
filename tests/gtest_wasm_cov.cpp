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

/* ==================== wasm_write_program ==================== */

TEST(WasmCov, WriteNullAst) {
    EXPECT_EQ(wasm_write_program(nullptr, "/tmp/test.wasm", TARGET_WASI_P2), 0);
}

TEST(WasmCov, WriteNullOutput) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        EXPECT_NE(wasm_write_program(ast, nullptr, TARGET_WASI_P2), 0);
        ast_node_free(ast);
    }
}

TEST(WasmCov, WriteSimpleFunc) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        int ret = wasm_write_program(ast, "/tmp/test_simple.wasm", TARGET_WASI_P2);
        (void)ret;
        unlink("/tmp/test_simple.wasm");
        ast_node_free(ast);
    }
}

TEST(WasmCov, WriteFuncWithReturn) {
    ASTNode *ast = parse_code("fn main() -> i32 { return 42; }");
    if (ast) {
        int ret = wasm_write_program(ast, "/tmp/test_return.wasm", TARGET_WASI_P2);
        (void)ret;
        unlink("/tmp/test_return.wasm");
        ast_node_free(ast);
    }
}

TEST(WasmCov, WriteFuncWithLet) {
    ASTNode *ast = parse_code("fn main() { let x: i32 = 1; }");
    if (ast) {
        int ret = wasm_write_program(ast, "/tmp/test_let.wasm", TARGET_WASI_P2);
        (void)ret;
        unlink("/tmp/test_let.wasm");
        ast_node_free(ast);
    }
}

TEST(WasmCov, WriteFuncWithPrint) {
    ASTNode *ast = parse_code("fn main() { print(42); }");
    if (ast) {
        int ret = wasm_write_program(ast, "/tmp/test_print.wasm", TARGET_WASI_P2);
        (void)ret;
        unlink("/tmp/test_print.wasm");
        ast_node_free(ast);
    }
}

TEST(WasmCov, WriteFuncWithBinaryOp) {
    ASTNode *ast = parse_code("fn main() -> i32 { return 1 + 2; }");
    if (ast) {
        int ret = wasm_write_program(ast, "/tmp/test_binop.wasm", TARGET_WASI_P2);
        (void)ret;
        unlink("/tmp/test_binop.wasm");
        ast_node_free(ast);
    }
}

TEST(WasmCov, WriteFuncWithIf) {
    ASTNode *ast = parse_code("fn main() { if true { print(1); } }");
    if (ast) {
        int ret = wasm_write_program(ast, "/tmp/test_if.wasm", TARGET_WASI_P2);
        (void)ret;
        unlink("/tmp/test_if.wasm");
        ast_node_free(ast);
    }
}

TEST(WasmCov, WriteFuncWithWhile) {
    ASTNode *ast = parse_code("fn main() { while false { print(1); } }");
    if (ast) {
        int ret = wasm_write_program(ast, "/tmp/test_while.wasm", TARGET_WASI_P2);
        (void)ret;
        unlink("/tmp/test_while.wasm");
        ast_node_free(ast);
    }
}

TEST(WasmCov, WriteMultipleFuncs) {
    ASTNode *ast = parse_code("fn main() { } fn helper() -> i32 { return 1; }");
    if (ast) {
        int ret = wasm_write_program(ast, "/tmp/test_multi.wasm", TARGET_WASI_P2);
        (void)ret;
        unlink("/tmp/test_multi.wasm");
        ast_node_free(ast);
    }
}

TEST(WasmCov, WriteDifferentTarget) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        int ret = wasm_write_program(ast, "/tmp/test_target.wasm", TARGET_WASI_P3);
        (void)ret;
        unlink("/tmp/test_target.wasm");
        ast_node_free(ast);
    }
}

TEST(WasmCov, WriteTargetName) {
    EXPECT_STREQ(wasm_target_name(TARGET_WASI_P2), "wasi-p2");
}
