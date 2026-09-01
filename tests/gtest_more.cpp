#include "ponypp.h"
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/codegen.h"
#include "ponypp/util.h"
#include "gtest_helpers.h"
#include <gtest/gtest.h>
#include <string.h>
#include <stdio.h>

TEST(UtilExtra, TokenTypeNames) {
    ASSERT_STREQ(token_type_name(TK_FLOAT), "FLOAT");
    ASSERT_STREQ(token_type_name(TK_STRING), "STRING");
    ASSERT_STREQ(token_type_name(TK_BOOL), "BOOL");
    ASSERT_STREQ(token_type_name(TK_TYPE), "TYPE");
    ASSERT_STREQ(token_type_name(TK_CAP), "CAP");
    ASSERT_STREQ(token_type_name(TK_PUNCT), "PUNCT");
    ASSERT_STREQ(token_type_name(TK_ARROW), "ARROW");
    ASSERT_STREQ(token_type_name(TK_HASH), "HASH");
    ASSERT_STREQ(token_type_name(TK_DOLLAR), "DOLLAR");
    ASSERT_STREQ(token_type_name(TK_AMP), "AMP");
    ASSERT_STREQ(token_type_name(TK_PERCENT), "PERCENT");
    ASSERT_STREQ(token_type_name(TK_AT), "AT");
    ASSERT_STREQ(token_type_name(TK_PIPE), "PIPE");
    ASSERT_STREQ(token_type_name(TK_QUESTION), "QUESTION");
    ASSERT_STREQ(token_type_name(TK_COLON), "COLON");
    ASSERT_STREQ(token_type_name(TK_BRACKET_L), "[");
    ASSERT_STREQ(token_type_name(TK_BRACKET_R), "]");
    ASSERT_STREQ(token_type_name(TK_BRACE_L), "{");
    ASSERT_STREQ(token_type_name(TK_BRACE_R), "}");
    ASSERT_STREQ(token_type_name(TK_PAREN_R), ")");
    ASSERT_STREQ(token_type_name(TK_SEMI), ";");
    ASSERT_STREQ(token_type_name(TK_COMMA), ",");
    ASSERT_STREQ(token_type_name(TK_DOT), ".");
    ASSERT_STREQ(token_type_name(TK_DASH), "-");
    ASSERT_STREQ(token_type_name(TK_PLUS), "+");
    ASSERT_STREQ(token_type_name(TK_STAR), "*");
    ASSERT_STREQ(token_type_name(TK_SLASH), "/");
    ASSERT_STREQ(token_type_name(TK_EQ), "=");
    ASSERT_STREQ(token_type_name(TK_BANG), "!");
    ASSERT_STREQ(token_type_name(TK_LT), "<");
    ASSERT_STREQ(token_type_name(TK_GT), ">");
    ASSERT_STREQ(token_type_name(TK_LE), "<=");
    ASSERT_STREQ(token_type_name(TK_GE), ">=");
    ASSERT_STREQ(token_type_name(TK_EQEQ), "==");
    ASSERT_STREQ(token_type_name(TK_NEQ), "!=");
    ASSERT_STREQ(token_type_name(TK_COLONCOLON), "::");
    ASSERT_STREQ(token_type_name(TK_ARROW_ARR), "=>");
    ASSERT_STREQ(token_type_name(TK_IDENT), "IDENT");
    ASSERT_STREQ(token_type_name(TK_INT), "INT");
    ASSERT_STREQ(token_type_name(TK_EOF), "EOF");
}

TEST(UtilExtra, FileReadMissing) {
    ASSERT_TRUE(s_file_read("/nonexistent_path_for_testing_ponypp.txt") == nullptr);
}

TEST(UtilExtra, FileReadWrite) {
    const char *p = "/tmp/ponypp_rt.txt";
    const char *data = "hello 42";
    ASSERT_EQ(s_file_write(p, data, strlen(data)), 0);
    char *rd = s_file_read(p);
    ASSERT_TRUE(rd != nullptr);
    ASSERT_STREQ(rd, "hello 42");
    s_free(rd);
    remove(p);
}

TEST(UtilExtra, Sprintf) {
    char *buf = (char*)s_malloc(64);
    snprintf(buf, 64, "hello %d", 42);
    char *rd = s_file_read("/tmp/ponypp_rt2.txt");
    s_free(buf);
    s_free(rd);
    const char *p2 = "/tmp/ponypp_rt2.txt";
    const char *data2 = "hello 42";
    s_file_write(p2, data2, strlen(data2));
    rd = s_file_read(p2);
    ASSERT_TRUE(rd != nullptr);
    ASSERT_STREQ(rd, "hello 42");
    s_free(rd);
    remove(p2);
}

TEST(UtilExtra, Memset) {
    char buf[8];
    s_memset(buf, 0, 8);
    for (int i = 0; i < 8; i++) ASSERT_EQ(buf[i], 0);
    s_memset(buf, 'A', 8);
    for (int i = 0; i < 8; i++) ASSERT_EQ(buf[i], 'A');
}

TEST(CodegenExtra5, PrintInt) {
    ASTNode *ast = parse_to_ast(
        "actor A { new create() => {}\n"
        "  be run() => { print(42) }\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    const char *path = "/tmp/ponypp_cge5_int.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    codegen_program(codegen_new(f), ast);
    fclose(f);
    /* just verify codegen runs without crashing */
    char *out = s_file_read(path);
    ASSERT_TRUE(out != nullptr);
    s_free(out);
    remove(path);
    ast_node_free(ast);
}

TEST(CodegenExtra5, PrintFloatBool) {
    ASTNode *ast = parse_to_ast(
        "actor A { new create() => {}\n"
        "  be run() => { print(3.14); print(true) }\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    const char *path = "/tmp/ponypp_cge5_float.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    codegen_program(codegen_new(f), ast);
    fclose(f);
    s_free(s_file_read(path));
    remove(path);
    ast_node_free(ast);
}

TEST(CodegenExtra5, PrintIdent) {
    ASTNode *ast = parse_to_ast(
        "actor A { var count: U64 = 0; new create() => {}\n"
        "  be run() => { print(count) }\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    const char *path = "/tmp/ponypp_cge5_ident.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    codegen_program(codegen_new(f), ast);
    fclose(f);
    s_free(s_file_read(path));
    remove(path);
    ast_node_free(ast);
}

TEST(CodegenExtra5, PrintEmpty) {
    ASTNode *ast = parse_to_ast(
        "actor A { new create() => {}\n"
        "  be run() => { print() }\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    const char *path = "/tmp/ponypp_cge5_empty.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    codegen_program(codegen_new(f), ast);
    fclose(f);
    s_free(s_file_read(path));
    remove(path);
    ast_node_free(ast);
}

TEST(CodegenExtra5, PrintMulti) {
    ASTNode *ast = parse_to_ast(
        "actor A { new create() => {}\n"
        "  be run() => { print(\"a\", \"b\") }\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    const char *path = "/tmp/ponypp_cge5_multi.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    codegen_program(codegen_new(f), ast);
    fclose(f);
    s_free(s_file_read(path));
    remove(path);
    ast_node_free(ast);
}

TEST(CodegenExtra5, NoneType) {
    ASTNode *ast = parse_to_ast(
        "actor A { new create() => {}\n"
        "  be run() => { var x: None = 0 }\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    const char *path = "/tmp/ponypp_cge5_none.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    codegen_program(codegen_new(f), ast);
    fclose(f);
    char *out = s_file_read(path);
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE(strstr(out, "void"), nullptr);
    s_free(out);
    remove(path);
    ast_node_free(ast);
}

TEST(ParserExtra, TypeArgs) {
    ASTNode *ast = parse_to_ast(
        "actor A { var x: Box<U64>; new create() => {}\n"
        "  be run() => {}\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}

TEST(ParserExtra, IfElseIf) {
    ASTNode *ast = parse_to_ast(
        "actor A { new create() => {}\n"
        "  be run() => {\n"
        "    if 1 => { print(\"a\") } else if 2 => { print(\"b\") } else => { print(\"c\") }\n"
        "  }\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}

TEST(ParserExtra, WhileLoop) {
    ASTNode *ast = parse_to_ast(
        "actor A { var i: U64 = 0; new create() => {}\n"
        "  be run() => { while i < 10 => { i = i + 1 } }\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}

TEST(ParserExtra, MultiField) {
    ASTNode *ast = parse_to_ast(
        "actor A { var a: U64 = 0; var b: U64 = 1; var c: U64 = 2;\n"
        "  new create() => {}\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}

TEST(ParserExtra, LetInit) {
    ASTNode *ast = parse_to_ast(
        "actor A { new create() => {}\n"
        "  be run() => { let x = 42 }\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}
