#include <gtest/gtest.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp/ast.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>

static ASTNode *parse_src(const char *src) {
    Lexer *lx = lexer_new("t", src, strlen(src));
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("t", toks, tc);
    ASTNode *ast = parser_parse_program(p);
    parser_free(p);
    free(toks);
    lexer_free(lx);
    return ast;
}

static bool parse_ok(const char *src) {
    ASTNode *ast = parse_src(src);
    bool ok = (ast != nullptr);
    ast_node_free(ast);
    return ok;
}

/* ==================== Try-Else-Then (lines 414-421) ==================== */

TEST(ParserCov16, TryElseThen) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    try\n"
        "      print(\"try\")\n"
        "    else\n"
        "      print(\"else\")\n"
        "    then\n"
        "      print(\"then\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

/* ==================== Return with expression (lines 481-489) ==================== */

TEST(ParserCov16, ReturnWithExpr) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  fun foo(): U32 =>\n"
        "    return 42\n"
        "  new create() => { }\n"
        "}\n"));
}

TEST(ParserCov16, ReturnNoExpr) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  fun foo() =>\n"
        "    return\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== Method call on field (lines 579-596) ==================== */

TEST(ParserCov16, MethodCallOnField) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hello\"\n"
        "    s.to_upper()\n"
        "  }\n"
        "}\n"));
}

TEST(ParserCov16, MethodCallOnFieldWithArgs) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hello\"\n"
        "    s.substr(1, 3)\n"
        "  }\n"
        "}\n"));
}

/* ==================== Import variants (lines 611-627) ==================== */

TEST(ParserCov16, ImportWithAlias) {
    EXPECT_TRUE(parse_ok(
        "use \"std.io\" as io\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== Nested match (lines 646-657) ==================== */

TEST(ParserCov16, NestedMatchInIf) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: U32 = 1\n"
        "    if x > 0 then\n"
        "      match x\n"
        "      | 1 => print(\"one\")\n"
        "      else print(\"other\")\n"
        "      end\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

/* ==================== Complex expressions (lines 699-722) ==================== */

TEST(ParserCov16, ChainedMethodCalls) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hello\"\n"
        "    s.to_upper().to_lower()\n"
        "  }\n"
        "}\n"));
}

TEST(ParserCov16, NestedFunctionCalls) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    print(parse_json(\"{}\"))\n"
        "  }\n"
        "}\n"));
}

/* ==================== For loop (lines 900-901) ==================== */

TEST(ParserCov16, ForLoop) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    for i in Range(0, 10) do\n"
        "      print(i.string())\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

/* ==================== Actor with fields (lines 933-940) ==================== */

TEST(ParserCov16, ActorWithFields) {
    EXPECT_TRUE(parse_ok(
        "actor Worker {\n"
        "  var _id: U32\n"
        "  var _name: String\n"
        "  new create() => {\n"
        "    _id = 1\n"
        "    _name = \"test\"\n"
        "  }\n"
        "}\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== Supervise (lines 950-951) ==================== */

TEST(ParserCov16, SuperviseActor) {
    EXPECT_TRUE(parse_ok(
        "supervise Worker\n"
        "actor Worker {\n"
        "  new create() => { }\n"
        "}\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== Class definition (lines 972-982) ==================== */

TEST(ParserCov16, ClassDefinition) {
    EXPECT_TRUE(parse_ok(
        "class Point {\n"
        "  var _x: U32\n"
        "  var _y: U32\n"
        "  new create() => {\n"
        "    _x = 0\n"
        "    _y = 0\n"
        "  }\n"
        "}\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}

/* ==================== Trait definition ==================== */

TEST(ParserCov16, TraitDefinition) {
    EXPECT_TRUE(parse_ok(
        "trait Drawable {\n"
        "  fun draw()\n"
        "}\n"
        "actor main {\n"
        "  new create() => { }\n"
        "}\n"));
}
