/*
 * test_codegen.c - Pony++ Native backend C 代码生成测试
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/codegen.h"

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

static int test_codegen_generates_c(void) {
    printf("test_codegen_generates_c\n");
    const char *src = "actor Main { be run() => { print(\"hi\") } }";
    ASTNode *ast = parse_to_ast(src);
    CHECK(ast != NULL, "解析成功");
    if (ast) {
        FILE *f = fopen("/tmp/ponypp_gen.c", "w");
        CHECK(f != NULL, "打开输出文件");
        if (f) {
            Codegen *cg = codegen_new(f);
            CHECK(cg != NULL, "创建 Codegen");
            if (cg) {
                codegen_program(cg, ast);
                codegen_free(cg);
            }
            fclose(f);
            FILE *rf = fopen("/tmp/ponypp_gen.c", "r");
            CHECK(rf != NULL, "生成文件可打开");
            if (rf) {
                char buf[8192] = {0};
                fread(buf, 1, sizeof(buf) - 1, rf);
                fclose(rf);
                CHECK(strstr(buf, "typedef struct") != NULL, "包含结构体定义");
                CHECK(strstr(buf, "int main") != NULL, "包含 main 函数");
                CHECK(strstr(buf, "PnyRuntime") != NULL, "包含运行时代码");
            }
        }
        ast_node_free(ast);
        remove("/tmp/ponypp_gen.c");
    }
    return tests_failed == 0;
}

int main(void) {
    printf("=== Pony++ Native 代码生成测试 ===\n\n");
    test_codegen_generates_c();
    printf("\n=== 测试结果 ===\n");
    printf("通过: %d\n", tests_passed);
    printf("失败: %d\n", tests_failed);
    printf("总计: %d\n", tests_passed + tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
