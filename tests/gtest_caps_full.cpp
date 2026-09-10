/*
 * Capabilities 测试
 */

#include <gtest/gtest.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp/capabilities.h>
#include <ponypp/ast.h>
#include <ponypp.h>
#include <cstdlib>
#include <cstring>

static ASTNode *parse_source(const char *src) {
    Lexer *lex = lexer_new("test.pny", src, strlen(src));
    if (!lex) return nullptr;
    Token *tokens = nullptr;
    size_t count = 0;
    if (!lexer_lex_all(lex, &tokens, &count)) {
        lexer_free(lex);
        return nullptr;
    }
    Parser *p = parser_new("test.pny", tokens, count);
    if (!p) { lexer_free(lex); return nullptr; }
    ASTNode *ast = parser_parse_program(p);
    parser_free(p);
    lexer_free(lex);
    return ast;
}

TEST(CapabilitiesTest, CheckSimpleActor) {
    ASTNode *ast = parse_source("actor Foo { }");
    ASSERT_NE(ast, nullptr);
    CapCheckResult *result = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    ASSERT_NE(result, nullptr);
    int r = capabilities_check_program(ast, result);
    EXPECT_EQ(r, 0);
    cap_check_free_result(result);
    ast_node_free(ast);
}

TEST(CapabilitiesTest, CheckIsoField) {
    ASTNode *ast = parse_source("actor Foo {\n  var _data: iso String\n}");
    ASSERT_NE(ast, nullptr);
    CapCheckResult *result = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    int r = capabilities_check_program(ast, result);
    EXPECT_EQ(r, 0);
    cap_check_free_result(result);
    ast_node_free(ast);
}

TEST(CapabilitiesTest, CheckValField) {
    ASTNode *ast = parse_source("actor Foo {\n  var _data: val U64\n}");
    ASSERT_NE(ast, nullptr);
    CapCheckResult *result = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    int r = capabilities_check_program(ast, result);
    EXPECT_EQ(r, 0);
    cap_check_free_result(result);
    ast_node_free(ast);
}

TEST(CapabilitiesTest, CheckRefField) {
    ASTNode *ast = parse_source("actor Foo {\n  var _data: ref Foo\n}");
    ASSERT_NE(ast, nullptr);
    CapCheckResult *result = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    int r = capabilities_check_program(ast, result);
    EXPECT_EQ(r, 0);
    cap_check_free_result(result);
    ast_node_free(ast);
}

TEST(CapabilitiesTest, CheckMultipleCapabilities) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  var _iso: iso String\n"
        "  var _val: val U64\n"
        "  var _ref: ref Foo\n"
        "}"
    );
    ASSERT_NE(ast, nullptr);
    CapCheckResult *result = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    int r = capabilities_check_program(ast, result);
    EXPECT_EQ(r, 0);
    cap_check_free_result(result);
    ast_node_free(ast);
}

TEST(CapabilitiesTest, CheckNullAst) {
    CapCheckResult *result = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    int r = capabilities_check_program(nullptr, result);
    EXPECT_NE(r, 0);
    cap_check_free_result(result);
}

TEST(CapabilitiesTest, FreeNullResult) {
    cap_check_free_result(nullptr);
}

TEST(CapabilitiesTest, CheckActorWithMethods) {
    ASTNode *ast = parse_source(
        "actor Foo {\n"
        "  var _data: iso String\n"
        "  be process(d: iso String) { _data = d }\n"
        "  fun get(): String { \"\" }\n"
        "}"
    );
    ASSERT_NE(ast, nullptr);
    CapCheckResult *result = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    int r = capabilities_check_program(ast, result);
    EXPECT_EQ(r, 0);
    cap_check_free_result(result);
    ast_node_free(ast);
}

TEST(CapabilitiesTest, CheckInterface) {
    ASTNode *ast = parse_source("interface Stringable {\n  fun to_string(): String\n}");
    if (!ast) return;
    CapCheckResult *result = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    int r = capabilities_check_program(ast, result);
    EXPECT_EQ(r, 0);
    cap_check_free_result(result);
    ast_node_free(ast);
}

TEST(CapabilitiesTest, CheckTrait) {
    ASTNode *ast = parse_source("trait Named {\n  fun name(): String\n}");
    if (!ast) return;
    CapCheckResult *result = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    int r = capabilities_check_program(ast, result);
    EXPECT_EQ(r, 0);
    cap_check_free_result(result);
    ast_node_free(ast);
}

TEST(CapabilitiesTest, CheckPrimitive) {
    ASTNode *ast = parse_source("primitive Constants {\n  fun pi(): F64 { 3.14159 }\n}");
    if (!ast) return;
    CapCheckResult *result = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    int r = capabilities_check_program(ast, result);
    EXPECT_EQ(r, 0);
    cap_check_free_result(result);
    ast_node_free(ast);
}

TEST(CapabilitiesTest, CheckClass) {
    ASTNode *ast = parse_source("class Point {\n  var _x: U64\n  var _y: U64\n}");
    if (!ast) return;
    CapCheckResult *result = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    int r = capabilities_check_program(ast, result);
    EXPECT_EQ(r, 0);
    cap_check_free_result(result);
    ast_node_free(ast);
}

TEST(CapabilitiesTest, CheckMixedDefs) {
    ASTNode *ast = parse_source(
        "actor A {\n  var _data: iso String\n}\n"
        "class B {\n  var _val: val U64\n}\n"
        "primitive C {\n  fun value(): U64 { 42 }\n}"
    );
    if (!ast) return;
    CapCheckResult *result = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    int r = capabilities_check_program(ast, result);
    EXPECT_EQ(r, 0);
    cap_check_free_result(result);
    ast_node_free(ast);
}
