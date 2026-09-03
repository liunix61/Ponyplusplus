/*
 * test_typecheck.c - Pony++ 类型检查器测试
 */

#include <stdio.h>
#include <string.h>
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/typecheck.h"

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

static int test_empty_program(void) {
    printf("test_empty_program\n");
    TypeCheckResult *r = (TypeCheckResult *)calloc(1, sizeof(TypeCheckResult));
    CHECK(typecheck_program(NULL, r) != 0, "空程序返回错误");
    CHECK(r->ok == 0, "ok 为 false");
    CHECK(r->error_count == 1, "有 1 个错误");
    typecheck_free_result(r);
    return tests_failed == 0;
}

static int test_valid_actor(void) {
    printf("test_valid_actor\n");
    const char *src = "actor main {\n  var count: U64 = 0\n  be run() => {}\n}";
    ASTNode *ast = parse_to_ast(src);
    CHECK(ast != NULL, "解析成功");
    if (ast) {
        TypeCheckResult *r = (TypeCheckResult *)calloc(1, sizeof(TypeCheckResult));
        CHECK(typecheck_program(ast, r) == 0, "类型检查通过");
        CHECK(r->ok == 1, "ok 为 true");
        CHECK(r->error_count == 0, "无错误");
        typecheck_free_result(r);
        ast_node_free(ast);
    }
    return tests_failed == 0;
}

static int test_multiple_actors(void) {
    printf("test_multiple_actors\n");
    const char *src =
        "actor A { var x: U64 = 0 }\n"
        "actor B { var y: I64 = 42 }\n"
        "actor C { fun hello(): String => { } }";
    ASTNode *ast = parse_to_ast(src);
    CHECK(ast != NULL, "解析成功");
    if (ast) {
        TypeCheckResult *r = (TypeCheckResult *)calloc(1, sizeof(TypeCheckResult));
        CHECK(typecheck_program(ast, r) == 0, "类型检查通过");
        typecheck_free_result(r);
        ast_node_free(ast);
    }
    return tests_failed == 0;
}

int main(void) {
    printf("=== Pony++ 类型检查器测试 ===\n\n");
    test_empty_program();
    test_valid_actor();
    test_multiple_actors();
    printf("\n=== 测试结果 ===\n");
    printf("通过: %d\n", tests_passed);
    printf("失败: %d\n", tests_failed);
    printf("总计: %d\n", tests_passed + tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
