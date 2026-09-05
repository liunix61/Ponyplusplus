/*
 * test_wit.c - Pony++ WIT 接口生成测试
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/wit.h"

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

static int test_wit_generates(void) {
    printf("test_wit_generates\n");
    const char *src = "actor Counter { var count: U64 = 0\n  be increment() => {} }";
    ASTNode *ast = parse_to_ast(src);
    CHECK(ast != NULL, "解析成功");
    if (ast) {
        CHECK(wit_write_program(ast, "/tmp/ponypp_test.wit", TARGET_WASI_P2) == 0,
              "WIT 文件生成成功");
        FILE *f = fopen("/tmp/ponypp_test.wit", "r");
        CHECK(f != NULL, "文件可打开");
        if (f) {
            char buf[1024] = {0};
            fread(buf, 1, sizeof(buf) - 1, f);
            fclose(f);
            CHECK(strstr(buf, "package ponypp") != NULL, "包含 package 声明");
            CHECK(strstr(buf, "interface") != NULL, "包含 interface 声明");
            CHECK(strstr(buf, "world") != NULL, "包含 world 声明");
        }
        ast_node_free(ast);
        remove("/tmp/ponypp_test.wit");
    }
    return tests_failed == 0;
}

static int test_wit_empty(void) {
    printf("test_wit_empty\n");
    ASTNode *ast = parse_to_ast("");
    CHECK(ast != NULL, "空程序解析成功");
    if (ast) {
        CHECK(wit_write_program(ast, "/tmp/ponypp_empty.wit", TARGET_WASI_P2) == 0,
              "空程序也生成 WIT");
        ast_node_free(ast);
        remove("/tmp/ponypp_empty.wit");
    }
    return tests_failed == 0;
}

int main(void) {
    printf("=== Pony++ WIT 接口生成测试 ===\n\n");
    test_wit_generates();
    test_wit_empty();
    printf("\n=== 测试结果 ===\n");
    printf("通过: %d\n", tests_passed);
    printf("失败: %d\n", tests_failed);
    printf("总计: %d\n", tests_passed + tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
