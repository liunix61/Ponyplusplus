#include <gtest/gtest.h>
#include "gtest_helpers.h"
#include "ponypp/lexer.h"
#include "ponypp/util.h"

TEST(Lexer, BasicLexing) {
    const char* src = "actor main { be hello() => {} }";
    Lexer* lex = lexer_new("test.pny", src, std::strlen(src));
    ASSERT_NE(lex, nullptr);

    Token* tokens = nullptr;
    size_t count = 0;
    bool ok = lexer_lex_all(lex, &tokens, &count);
    EXPECT_TRUE(ok);
    if (ok && count >= 4) {
        EXPECT_EQ(tokens[0].type, TK_KEYWORD);
        EXPECT_STREQ(tokens[0].value, "actor");
        EXPECT_EQ(tokens[1].type, TK_IDENT);
        EXPECT_STREQ(tokens[1].value, "main");
        EXPECT_EQ(tokens[2].type, TK_BRACE_L);
    }
    if (tokens) { for (size_t i = 0; i < count; i++) free(tokens[i].value); free(tokens); }
    lexer_free(lex);
}

TEST(Lexer, Numbers) {
    const char* src = "42 3.14 0xFF";
    Lexer* lex = lexer_new("test.pny", src, std::strlen(src));
    Token* tokens = nullptr;
    size_t count = 0;
    EXPECT_TRUE(lexer_lex_all(lex, &tokens, &count));
    EXPECT_GE(count, 3);
    if (tokens) { for (size_t i = 0; i < count; i++) free(tokens[i].value); free(tokens); }
    lexer_free(lex);
}

TEST(Lexer, Strings) {
    const char* src = "\"hello\" 'world'";
    Lexer* lex = lexer_new("test.pny", src, std::strlen(src));
    Token* tokens = nullptr;
    size_t count = 0;
    EXPECT_TRUE(lexer_lex_all(lex, &tokens, &count));
    EXPECT_GE(count, 2);
    if (count >= 2) {
        EXPECT_EQ(tokens[0].type, TK_STRING);
    }
    if (tokens) { for (size_t i = 0; i < count; i++) free(tokens[i].value); free(tokens); }
    lexer_free(lex);
}

TEST(Lexer, Capabilities) {
    const char* src = "iso trn ref val box tag";
    Lexer* lex = lexer_new("test.pny", src, std::strlen(src));
    Token* tokens = nullptr;
    size_t count = 0;
    EXPECT_TRUE(lexer_lex_all(lex, &tokens, &count));
    EXPECT_GE(count, 6);
    if (count >= 6) {
        for (size_t i = 0; i < 6; i++) {
            EXPECT_EQ(tokens[i].type, TK_CAP);
        }
    }
    if (tokens) { for (size_t i = 0; i < count; i++) free(tokens[i].value); free(tokens); }
    lexer_free(lex);
}

TEST(Lexer, Types) {
    const char* src = "U64 I64 String Bool";
    Lexer* lex = lexer_new("test.pny", src, std::strlen(src));
    Token* tokens = nullptr;
    size_t count = 0;
    EXPECT_TRUE(lexer_lex_all(lex, &tokens, &count));
    EXPECT_GE(count, 4);
    if (tokens) { for (size_t i = 0; i < count; i++) free(tokens[i].value); free(tokens); }
    lexer_free(lex);
}

TEST(Lexer, Operators) {
    const char* src = "+ - * / = == != <= >= => :";
    Lexer* lex = lexer_new("test.pny", src, std::strlen(src));
    Token* tokens = nullptr;
    size_t count = 0;
    EXPECT_TRUE(lexer_lex_all(lex, &tokens, &count));
    EXPECT_GE(count, 11);
    if (tokens) { for (size_t i = 0; i < count; i++) free(tokens[i].value); free(tokens); }
    lexer_free(lex);
}

TEST(Lexer, Comments) {
    const char* src = "// 注释\nactor main {}";
    Lexer* lex = lexer_new("test.pny", src, std::strlen(src));
    Token* tokens = nullptr;
    size_t count = 0;
    EXPECT_TRUE(lexer_lex_all(lex, &tokens, &count));
    EXPECT_EQ(tokens[0].type, TK_KEYWORD);
    EXPECT_STREQ(tokens[0].value, "actor");
    if (tokens) { for (size_t i = 0; i < count; i++) free(tokens[i].value); free(tokens); }
    lexer_free(lex);
}

TEST(Lexer, FullActor) {
    const char* src =
        "actor Counter(val initial: U64 = 0) {\n"
        "  var count: U64 = initial\n"
        "  new create(initial: U64) => {}\n"
        "  fun value(): U64 => {}\n"
        "  be increment() => {}\n"
        "}";
    Lexer* lex = lexer_new("test.pny", src, std::strlen(src));
    Token* tokens = nullptr;
    size_t count = 0;
    EXPECT_TRUE(lexer_lex_all(lex, &tokens, &count));
    EXPECT_EQ(tokens[0].type, TK_KEYWORD);
    EXPECT_STREQ(tokens[1].value, "Counter");
    if (tokens) { for (size_t i = 0; i < count; i++) free(tokens[i].value); free(tokens); }
    lexer_free(lex);
}

TEST(Lexer, EmptyInput) {
    Lexer* lex = lexer_new("empty.pny", "", 0);
    ASSERT_NE(lex, nullptr);
    Token* tokens = nullptr;
    size_t count = 0;
    bool ok = lexer_lex_all(lex, &tokens, &count);
    EXPECT_TRUE(ok);
    if (tokens) { for (size_t i = 0; i < count; i++) free(tokens[i].value); free(tokens); }
    lexer_free(lex);
}

TEST(Lexer, LargeInput) {
    char* src = (char*)calloc(10001, 1);
    if (!src) return;
    /* fill with 'a' */
    memset(src, 'a', 10000);
    src[10000] = '\0';
    Lexer* lex = lexer_new("big.pny", src, 10000);
    ASSERT_NE(lex, nullptr);
    Token* tokens = nullptr;
    size_t count = 0;
    bool ok = lexer_lex_all(lex, &tokens, &count);
    EXPECT_TRUE(ok);
    if (tokens) { for (size_t i = 0; i < count; i++) free(tokens[i].value); free(tokens); }
    lexer_free(lex);
    free(src);
}
