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

// Bug#44: \xNN 十六进制转义 — 此前无 'x' 分支, 反斜杠被吞成字面 x
TEST(Lexer, HexEscape) {
    const char* src = "actor main { new create() => { var s: String = \"a\\x1fb\" } }";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    // 深度找字符串字面量, 断言长度 3 且中间字节 0x1f
    ASTNode* stack[256]; int sp = 0; stack[sp++] = ast;
    bool found = false;
    while (sp > 0) {
        ASTNode* n = stack[--sp];
        if (n->type == NODE_STRING && n->data) {
            const char* v = (const char*)n->data;
            if (v[0] == 'a' && (unsigned char)v[1] == 0x1f && v[2] == 'b' && v[3] == 0) found = true;
        }
        for (size_t i = 0; i < n->child_count && sp < 250; i++)
            if (n->children[i]) stack[sp++] = n->children[i];
    }
    EXPECT_TRUE(found) << "字符串字面量必须含真 0x1f 字节";
    ast_node_free(ast);
}
