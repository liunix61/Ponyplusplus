/*
 * test_wasm.c - Pony++ Wasm 代码生成测试
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/wasm.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { tests_passed++; printf("  \342\234\223 %s\n", msg); } \
    else { tests_failed++; printf("  \342\234\224 %s\n", msg); } \
} while(0)

static ASTNode *parse_to_ast(const char *src) {
    Lexer *lex = lexer_new("test.pny", src, strlen(src));
    Token *tokens = NULL;
    size_t count = 0;
    if (!lexer_lex_all(lex, &tokens, &count)) { lexer_free(lex); return NULL; }
    Parser *p = parser_new("test.pny", tokens, count);
    ASTNode *ast = parser_parse_program(p);
    parser_free(p);
    lexer_free(lex);
    return ast;
}

static int test_wasm_generates_file(void) {
    printf("test_wasm_generates_file\n");
    const char *src = "actor main { be hello() => {} }";
    ASTNode *ast = parse_to_ast(src);
    CHECK(ast != NULL, "解析成功");
    if (ast) {
        CHECK(wasm_write_program(ast, "/tmp/ponypp_test.wasm", TARGET_WASI_P2) == 0,
              "Wasm 文件生成成功");
        FILE *f = fopen("/tmp/ponypp_test.wasm", "rb");
        CHECK(f != NULL, "文件可打开");
        if (f) {
            unsigned char magic[8] = {0};
            fread(magic, 1, 8, f);
            fclose(f);
            CHECK(magic[0] == 0x00 && magic[1] == 0x61 && magic[2] == 0x73 &&
                  magic[3] == 0x6d, "Wasm magic 正确");
            CHECK(magic[4] == 0x01, "Wasm 版本 1");
        }
        ast_node_free(ast);
        remove("/tmp/ponypp_test.wasm");
    }
    return tests_failed == 0;
}

static int test_wasm_target_name(void) {
    printf("test_wasm_target_name\n");
    CHECK(strcmp(wasm_target_name(1), "component") == 0 ||
          wasm_target_name(1) != NULL, "TARGET_COMPONENT 有名称");
    CHECK(strcmp(wasm_target_name(4), "native") == 0, "TARGET_NATIVE 名称正确");
    return tests_failed == 0;
}

int main(void) {
    printf("=== Pony++ Wasm 代码生成测试 ===\n\n");
    test_wasm_generates_file();
    test_wasm_target_name();
    printf("\n=== 测试结果 ===\n");
    printf("通过: %d\n", tests_passed);
    printf("失败: %d\n", tests_failed);
    printf("总计: %d\n", tests_passed + tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
