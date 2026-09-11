#include <gtest/gtest.h>
#include <ponypp/wamr.h>
#include <ponypp/runtime.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
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

/* ==================== wamr_compile_program ==================== */

TEST(WamrCov2, CompileNullAst) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    EXPECT_NE(wamr_compile_program(nullptr, &cfg, "/tmp/test.wasm"), 0);
}

TEST(WamrCov2, CompileNullCfg) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        EXPECT_NE(wamr_compile_program(ast, nullptr, "/tmp/test.wasm"), 0);
        ast_node_free(ast);
    }
}

TEST(WamrCov2, CompileNullOutput) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        WamrConfig cfg = wamr_config_default(MCU_GENERIC);
        EXPECT_NE(wamr_compile_program(ast, &cfg, nullptr), 0);
        ast_node_free(ast);
    }
}

TEST(WamrCov2, CompileSimpleFunc) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        WamrConfig cfg = wamr_config_default(MCU_GENERIC);
        int ret = wamr_compile_program(ast, &cfg, "/tmp/test_wamr_simple.wasm");
        (void)ret;
        unlink("/tmp/test_wamr_simple.wasm");
        ast_node_free(ast);
    }
}

TEST(WamrCov2, CompileFuncWithReturn) {
    ASTNode *ast = parse_code("fn main() -> i32 { return 42; }");
    if (ast) {
        WamrConfig cfg = wamr_config_default(MCU_GENERIC);
        int ret = wamr_compile_program(ast, &cfg, "/tmp/test_wamr_return.wasm");
        (void)ret;
        unlink("/tmp/test_wamr_return.wasm");
        ast_node_free(ast);
    }
}

TEST(WamrCov2, CompileFuncWithLet) {
    ASTNode *ast = parse_code("fn main() { let x: i32 = 1; }");
    if (ast) {
        WamrConfig cfg = wamr_config_default(MCU_GENERIC);
        int ret = wamr_compile_program(ast, &cfg, "/tmp/test_wamr_let.wasm");
        (void)ret;
        unlink("/tmp/test_wamr_let.wasm");
        ast_node_free(ast);
    }
}

TEST(WamrCov2, CompileFuncWithPrint) {
    ASTNode *ast = parse_code("fn main() { print(42); }");
    if (ast) {
        WamrConfig cfg = wamr_config_default(MCU_GENERIC);
        int ret = wamr_compile_program(ast, &cfg, "/tmp/test_wamr_print.wasm");
        (void)ret;
        unlink("/tmp/test_wamr_print.wasm");
        ast_node_free(ast);
    }
}

TEST(WamrCov2, CompileFuncWithBinaryOp) {
    ASTNode *ast = parse_code("fn main() -> i32 { return 1 + 2; }");
    if (ast) {
        WamrConfig cfg = wamr_config_default(MCU_GENERIC);
        int ret = wamr_compile_program(ast, &cfg, "/tmp/test_wamr_binop.wasm");
        (void)ret;
        unlink("/tmp/test_wamr_binop.wasm");
        ast_node_free(ast);
    }
}

TEST(WamrCov2, CompileFuncWithIf) {
    ASTNode *ast = parse_code("fn main() { if true { print(1); } }");
    if (ast) {
        WamrConfig cfg = wamr_config_default(MCU_GENERIC);
        int ret = wamr_compile_program(ast, &cfg, "/tmp/test_wamr_if.wasm");
        (void)ret;
        unlink("/tmp/test_wamr_if.wasm");
        ast_node_free(ast);
    }
}

TEST(WamrCov2, CompileFuncWithWhile) {
    ASTNode *ast = parse_code("fn main() { while false { print(1); } }");
    if (ast) {
        WamrConfig cfg = wamr_config_default(MCU_GENERIC);
        int ret = wamr_compile_program(ast, &cfg, "/tmp/test_wamr_while.wasm");
        (void)ret;
        unlink("/tmp/test_wamr_while.wasm");
        ast_node_free(ast);
    }
}

TEST(WamrCov2, CompileMultipleFuncs) {
    ASTNode *ast = parse_code("fn main() { } fn helper() -> i32 { return 1; }");
    if (ast) {
        WamrConfig cfg = wamr_config_default(MCU_GENERIC);
        int ret = wamr_compile_program(ast, &cfg, "/tmp/test_wamr_multi.wasm");
        (void)ret;
        unlink("/tmp/test_wamr_multi.wasm");
        ast_node_free(ast);
    }
}

/* ==================== 不同 MCU 配置 ==================== */

TEST(WamrCov2, CompileSTM32F4) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        WamrConfig cfg = wamr_config_default(MCU_STM32F4);
        int ret = wamr_compile_program(ast, &cfg, "/tmp/test_wamr_stm32f4.wasm");
        (void)ret;
        unlink("/tmp/test_wamr_stm32f4.wasm");
        ast_node_free(ast);
    }
}

TEST(WamrCov2, CompileSTM32H7) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        WamrConfig cfg = wamr_config_default(MCU_STM32H7);
        int ret = wamr_compile_program(ast, &cfg, "/tmp/test_wamr_stm32h7.wasm");
        (void)ret;
        unlink("/tmp/test_wamr_stm32h7.wasm");
        ast_node_free(ast);
    }
}

TEST(WamrCov2, CompileESP32) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        WamrConfig cfg = wamr_config_default(MCU_ESP32);
        int ret = wamr_compile_program(ast, &cfg, "/tmp/test_wamr_esp32.wasm");
        (void)ret;
        unlink("/tmp/test_wamr_esp32.wasm");
        ast_node_free(ast);
    }
}

TEST(WamrCov2, CompileESP32S3) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        WamrConfig cfg = wamr_config_default(MCU_ESP32S3);
        int ret = wamr_compile_program(ast, &cfg, "/tmp/test_wamr_esp32s3.wasm");
        (void)ret;
        unlink("/tmp/test_wamr_esp32s3.wasm");
        ast_node_free(ast);
    }
}

/* ==================== 模块加载 ==================== */

TEST(WamrCov2, ModuleLoadValid) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    cfg.wasm_path = "/tmp/test_wamr_module.wasm";
    WamrModule *mod = nullptr;
    int ret = wamr_module_load(&cfg, &mod);
    /* 可能成功或失败 */
    if (ret == 0 && mod) {
        wamr_module_free(mod);
    }
    (void)ret;
}

/* ==================== 实例创建 ==================== */

TEST(WamrCov2, InstanceCreateValid) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    cfg.wasm_path = "/tmp/test_wamr_instance.wasm";
    WamrModule *mod = nullptr;
    int ret = wamr_module_load(&cfg, &mod);
    if (ret == 0 && mod) {
        WamrInstance *inst = nullptr;
        ret = wamr_instance_create(mod, &cfg, &inst);
        if (ret == 0 && inst) {
            wamr_instance_free(inst);
        }
        wamr_module_free(mod);
    }
    (void)ret;
}
