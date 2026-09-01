/*
 * test_parser.c - Pony++ 语法分析器测试
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/util.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { tests_passed++; printf("  ✓ %s\n", msg); } \
    else { tests_failed++; printf("  ✗ %s\n", msg); } \
} while(0)

static Parser *parse_source(const char *src) {
    Lexer *lex = lexer_new("test.pny", src, strlen(src));
    Token *tokens = NULL;
    size_t count = 0;
    if (!lexer_lex_all(lex, &tokens, &count)) {
        lexer_free(lex);
        return NULL;
    }
    Parser *parser = parser_new("test.pny", tokens, count);
    lexer_free(lex);
    return parser;
}

static int test_empty_program(void) {
    printf("test_empty_program\n");
    Parser *p = parse_source("");
    CHECK(p != NULL, "创建 Parser");
    if (p) {
        ASTNode *ast = parser_parse_program(p);
        CHECK(ast != NULL, "解析空程序");
        if (ast) {
            CHECK(ast->type == NODE_PROGRAM, "根节点是 Program");
            CHECK(ast->child_count == 0, "无子节点");
        }
    }
    parser_free(p);
    return tests_failed == 0;
}

static int test_simple_actor(void) {
    printf("test_simple_actor\n");
    Parser *p = parse_source("actor Main { }");
    CHECK(p != NULL, "创建 Parser");
    if (p) {
        ASTNode *ast = parser_parse_program(p);
        CHECK(ast != NULL, "解析成功");
        if (ast && ast->child_count > 0) {
            ASTNode *actor = ast->children[0];
            CHECK(actor->type == NODE_ACTOR, "节点类型是 Actor");
            CHECK(actor->data && strcmp((char *)actor->data, "Main") == 0,
                  "Actor 名称是 Main");
        }
    }
    parser_free(p);
    return tests_failed == 0;
}

static int test_actor_with_methods(void) {
    printf("test_actor_with_methods\n");
    const char *src =
        "actor Counter {\n"
        "  var count: U64 = 0\n"
        "  be increment() => { }\n"
        "  fun value(): U64 => { }\n"
        "}";
    Parser *p = parse_source(src);
    CHECK(p != NULL, "创建 Parser");
    if (p) {
        ASTNode *ast = parser_parse_program(p);
        CHECK(ast != NULL, "解析成功");
        if (ast && ast->child_count > 0) {
            ASTNode *actor = ast->children[0];
            CHECK(actor->child_count >= 2, "至少 2 个子节点 (var + be + fun)");
        }
    }
    parser_free(p);
    return tests_failed == 0;
}

static int test_actor_with_constructor(void) {
    printf("test_actor_with_constructor\n");
    const char *src =
        "actor Counter(val initial: U64) {\n"
        "  var count: U64 = initial\n"
        "  new create(initial: U64) => { }\n"
        "}";
    Parser *p = parse_source(src);
    CHECK(p != NULL, "创建 Parser");
    if (p) {
        ASTNode *ast = parser_parse_program(p);
        CHECK(ast != NULL, "解析成功");
        if (ast && ast->child_count > 0) {
            ASTNode *actor = ast->children[0];
            int has_new = 0;
            for (size_t i = 0; i < actor->child_count; i++) {
                if (actor->children[i]->type == NODE_NEW) has_new = 1;
            }
            CHECK(has_new == 1, "包含构造函数");
        }
    }
    parser_free(p);
    return tests_failed == 0;
}

static int test_multiple_actors(void) {
    printf("test_multiple_actors\n");
    const char *src =
        "actor A { }\n"
        "actor B { }\n"
        "actor C { }";
    Parser *p = parse_source(src);
    CHECK(p != NULL, "创建 Parser");
    if (p) {
        ASTNode *ast = parser_parse_program(p);
        CHECK(ast != NULL, "解析成功");
        if (ast) {
            int actor_count = 0;
            for (size_t i = 0; i < ast->child_count; i++) {
                if (ast->children[i]->type == NODE_ACTOR) actor_count++;
            }
            CHECK(actor_count == 3, "3 个 Actor");
        }
    }
    parser_free(p);
    return tests_failed == 0;
}

static int test_supervise(void) {
    printf("test_supervise\n");
    const char *src = "supervise worker one_for_one";
    Parser *p = parse_source(src);
    CHECK(p != NULL, "创建 Parser");
    if (p) {
        ASTNode *ast = parser_parse_program(p);
        CHECK(ast != NULL, "解析成功");
        if (ast && ast->child_count > 0) {
            CHECK(ast->children[0]->type == NODE_SUPERVISE, "监督节点存在");
        }
    }
    parser_free(p);
    return tests_failed == 0;
}

static int test_error_handling(void) {
    printf("test_error_handling\n");
    Parser *p = parse_source("actor {");
    CHECK(p != NULL, "创建 Parser");
    if (p) {
        (void)parser_parse_program(p);
        CHECK(1, "不完整代码不崩溃");
    }
    parser_free(p);
    return tests_failed == 0;
}

int main(void) {
    printf("=== Pony++ 语法分析器测试 ===\n\n");

    test_empty_program();
    test_simple_actor();
    test_actor_with_methods();
    test_actor_with_constructor();
    test_multiple_actors();
    test_supervise();
    test_error_handling();

    printf("\n=== 测试结果 ===\n");
    printf("通过: %d\n", tests_passed);
    printf("失败: %d\n", tests_failed);
    printf("总计: %d\n", tests_passed + tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
