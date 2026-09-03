/*
 * test_lexer.c - Pony++ 词法分析器测试
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ponypp/lexer.h"
#include "ponypp/util.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { tests_passed++; printf("  ✓ %s\n", msg); } \
    else { tests_failed++; printf("  ✗ %s\n", msg); } \
} while(0)

static int test_basic_lexing(void) {
    printf("test_basic_lexing\n");
    const char *src = "actor main { be hello() => {} }";
    Lexer *lex = lexer_new("test.pny", src, strlen(src));
    CHECK(lex != NULL, "创建 Lexer");

    Token *tokens = NULL;
    size_t count = 0;
    bool ok = lexer_lex_all(lex, &tokens, &count);
    CHECK(ok, "词法分析成功");

    if (ok && count > 0) {
        CHECK(tokens[0].type == TK_KEYWORD, "actor 是关键字");
        CHECK(strcmp(tokens[0].value, "actor") == 0, "actor 值正确");
        CHECK(tokens[1].type == TK_IDENT, "Main 是标识符");
        CHECK(strcmp(tokens[1].value, "Main") == 0, "Main 值正确");
        CHECK(tokens[2].type == TK_BRACE_L, "{ 是左花括号");
        CHECK(tokens[3].type == TK_KEYWORD, "be 是关键字");
        CHECK(strcmp(tokens[3].value, "be") == 0, "be 值正确");
    }

    lexer_free(lex);
    return tests_failed == 0;
}

static int test_numbers(void) {
    printf("test_numbers\n");
    const char *src = "42 3.14 0xFF 123456789";
    Lexer *lex = lexer_new("test.pny", src, strlen(src));

    Token *tokens = NULL;
    size_t count = 0;
    bool ok = lexer_lex_all(lex, &tokens, &count);
    CHECK(ok, "词法分析成功");

    if (ok && count >= 3) {
        CHECK(tokens[0].type == TK_INT, "42 是整数");
        CHECK(strcmp(tokens[0].value, "42") == 0, "42 值正确");
        CHECK(tokens[1].type == TK_INT, "3.14 是数字");
        CHECK(tokens[2].type == TK_INT, "0xFF 是数字");
    }

    lexer_free(lex);
    return tests_failed == 0;
}

static int test_strings(void) {
    printf("test_strings\n");
    const char *src = "\"hello\" 'world' \"test\"";
    Lexer *lex = lexer_new("test.pny", src, strlen(src));

    Token *tokens = NULL;
    size_t count = 0;
    bool ok = lexer_lex_all(lex, &tokens, &count);
    CHECK(ok, "词法分析成功");

    if (ok && count >= 3) {
        CHECK(tokens[0].type == TK_STRING, "\"hello\" 是字符串");
        CHECK(tokens[1].type == TK_STRING, "'world' 是字符串");
    }

    lexer_free(lex);
    return tests_failed == 0;
}

static int test_capabilities(void) {
    printf("test_capabilities\n");
    const char *src = "iso trn ref val box tag";
    Lexer *lex = lexer_new("test.pny", src, strlen(src));

    Token *tokens = NULL;
    size_t count = 0;
    bool ok = lexer_lex_all(lex, &tokens, &count);
    CHECK(ok, "词法分析成功");

    if (ok && count >= 6) {
        CHECK(tokens[0].type == TK_CAP, "iso 是能力");
        CHECK(tokens[1].type == TK_CAP, "trn 是能力");
        CHECK(tokens[2].type == TK_CAP, "ref 是能力");
        CHECK(tokens[3].type == TK_CAP, "val 是能力");
        CHECK(tokens[4].type == TK_CAP, "box 是能力");
        CHECK(tokens[5].type == TK_CAP, "tag 是能力");
    }

    lexer_free(lex);
    return tests_failed == 0;
}

static int test_types(void) {
    printf("test_types\n");
    const char *src = "U64 I64 String Bool Any None";
    Lexer *lex = lexer_new("test.pny", src, strlen(src));

    Token *tokens = NULL;
    size_t count = 0;
    bool ok = lexer_lex_all(lex, &tokens, &count);
    CHECK(ok, "词法分析成功");

    if (ok && count >= 6) {
        CHECK(tokens[0].type == TK_TYPE, "U64 是类型");
        CHECK(tokens[1].type == TK_TYPE, "I64 是类型");
        CHECK(tokens[2].type == TK_TYPE, "String 是类型");
        CHECK(tokens[3].type == TK_TYPE, "Bool 是类型");
    }

    lexer_free(lex);
    return tests_failed == 0;
}

static int test_operators(void) {
    printf("test_operators\n");
    const char *src = "+ - * / = == != <= >= => :";
    Lexer *lex = lexer_new("test.pny", src, strlen(src));

    Token *tokens = NULL;
    size_t count = 0;
    bool ok = lexer_lex_all(lex, &tokens, &count);
    CHECK(ok, "词法分析成功");

    if (ok) {
        int op_count = 0;
        for (size_t i = 0; i < count; i++) {
            if (tokens[i].type != TK_EOF) op_count++;
        }
        CHECK(op_count == 11, "11 个运算符");
    }

    lexer_free(lex);
    return tests_failed == 0;
}

static int test_comments(void) {
    printf("test_comments\n");
    const char *src = "// 注释\nactor main {}";
    Lexer *lex = lexer_new("test.pny", src, strlen(src));

    Token *tokens = NULL;
    size_t count = 0;
    bool ok = lexer_lex_all(lex, &tokens, &count);
    CHECK(ok, "词法分析成功");

    if (ok) {
        CHECK(tokens[0].type == TK_KEYWORD, "跳过注释后第一个 token 是 actor");
        CHECK(strcmp(tokens[0].value, "actor") == 0, "注释被正确跳过");
    }

    lexer_free(lex);
    return tests_failed == 0;
}

static int test_full_actor(void) {
    printf("test_full_actor\n");
    const char *src =
        "actor Counter(val initial: U64 = 0) {\n"
        "  var count: U64 = initial\n"
        "  new create(initial: U64) => {\n"
        "    count = initial\n"
        "  }\n"
        "  fun value(): U64 => {\n"
        "    count\n"
        "  }\n"
        "  be increment() => {\n"
        "    count = count + 1\n"
        "  }\n"
        "}";
    Lexer *lex = lexer_new("test.pny", src, strlen(src));

    Token *tokens = NULL;
    size_t count = 0;
    bool ok = lexer_lex_all(lex, &tokens, &count);
    CHECK(ok, "词法分析成功");

    if (ok) {
        CHECK(tokens[0].type == TK_KEYWORD, "第一个 token 是 actor");
        CHECK(strcmp(tokens[0].value, "actor") == 0, "actor 关键字正确");
        CHECK(tokens[1].type == TK_IDENT, "第二个 token 是标识符");
        CHECK(strcmp(tokens[1].value, "Counter") == 0, "Counter 名称正确");
    }

    lexer_free(lex);
    return tests_failed == 0;
}

int main(void) {
    printf("=== Pony++ 词法分析器测试 ===\n\n");

    test_basic_lexing();
    test_numbers();
    test_strings();
    test_capabilities();
    test_types();
    test_operators();
    test_comments();
    test_full_actor();

    printf("\n=== 测试结果 ===\n");
    printf("通过: %d\n", tests_passed);
    printf("失败: %d\n", tests_failed);
    printf("总计: %d\n", tests_passed + tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
