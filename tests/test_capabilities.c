/*
 * test_capabilities.c - Pony++ 引用能力验证测试
 */

#include <stdio.h>
#include <string.h>
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/capabilities.h"

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

static int test_empty(void) {
    printf("test_empty\n");
    CapCheckResult *r = (CapCheckResult *)calloc(1, sizeof(CapCheckResult));
    CHECK(capabilities_check_program(NULL, r) != 0, "空程序返回错误");
    CHECK(r->ok == 0, "ok 为 false");
    cap_check_free_result(r);
    return tests_failed == 0;
}

static int test_valid_actor(void) {
    printf("test_valid_actor\n");
    const char *src = "actor Main {\n  var count: U64 = 0\n  be run() => {}\n}";
    ASTNode *ast = parse_to_ast(src);
    CHECK(ast != NULL, "解析成功");
    if (ast) {
        CapCheckResult *r = (CapCheckResult *)calloc(1, sizeof(CapCheckResult));
        CHECK(capabilities_check_program(ast, r) == 0, "能力检查通过");
        cap_check_free_result(r);
        ast_node_free(ast);
    }
    return tests_failed == 0;
}

int main(void) {
    printf("=== Pony++ 引用能力验证测试 ===\n\n");
    test_empty();
    test_valid_actor();
    printf("\n=== 测试结果 ===\n");
    printf("通过: %d\n", tests_passed);
    printf("失败: %d\n", tests_failed);
    printf("总计: %d\n", tests_passed + tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
