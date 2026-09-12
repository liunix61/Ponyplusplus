#include <gtest/gtest.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>

static bool parse_ok(const char *src) {
    Lexer *lx = lexer_new("t", src, strlen(src));
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("t", toks, tc);
    ASTNode *ast = parser_parse_program(p);
    bool ok = (ast != nullptr);
    if (ast) ast_node_free(ast);
    parser_free(p);
    free(toks);
    lexer_free(lx);
    return ok;
}

/* ==================== Float literals ==================== */

TEST(ParserFloatCov, FloatLiteral) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: F64 = 3.14\n"
        "  }\n"
        "}\n"));
}

TEST(ParserFloatCov, FloatZero) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: F64 = 0.0\n"
        "  }\n"
        "}\n"));
}

/* ==================== Print field ==================== */

TEST(ParserFloatCov, PrintField) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  var _name: String\n"
        "  new create() => {\n"
        "    _name = \"test\"\n"
        "    print(_name)\n"
        "  }\n"
        "}\n"));
}

/* ==================== Print int/string ==================== */

TEST(ParserFloatCov, PrintInt) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: U32 = 42\n"
        "    print(x)\n"
        "  }\n"
        "}\n"));
}

TEST(ParserFloatCov, PrintString) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    print(\"hello\")\n"
        "  }\n"
        "}\n"));
}

/* ==================== String size ==================== */

TEST(ParserFloatCov, StringSize) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hello\"\n"
        "    let len: USize = s.size()\n"
        "  }\n"
        "}\n"));
}

/* ==================== Self field access ==================== */

TEST(ParserFloatCov, SelfFieldAccess) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  var _id: U32\n"
        "  new create() => {\n"
        "    _id = 1\n"
        "    print(self._id)\n"
        "  }\n"
        "}\n"));
}

/* ==================== If-else ==================== */

TEST(ParserFloatCov, IfElse) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: U32 = 1\n"
        "    if x > 0 then\n"
        "      print(\"positive\")\n"
        "    else\n"
        "      print(\"non-positive\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

/* ==================== While loop ==================== */

TEST(ParserFloatCov, WhileLoop) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    var i: U32 = 0\n"
        "    while i < 10 do\n"
        "      i = i + 1\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

/* ==================== Match ==================== */

TEST(ParserFloatCov, Match) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: U32 = 1\n"
        "    match x\n"
        "    | 1 => print(\"one\")\n"
        "    else print(\"other\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

/* ==================== Array literal ==================== */

TEST(ParserFloatCov, ArrayLiteral) {
    EXPECT_TRUE(parse_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let arr: Array[U32] = [1, 2, 3]\n"
        "  }\n"
        "}\n"));
}

/* ==================== Complex program ==================== */

TEST(ParserFloatCov, ComplexProgram) {
    EXPECT_TRUE(parse_ok(
        "actor Worker {\n"
        "  var _id: U32\n"
        "  new create() => {\n"
        "    _id = 1\n"
        "  }\n"
        "  be process(msg: String) =>\n"
        "    print(msg)\n"
        "}\n"
        "actor main {\n"
        "  new create() => {\n"
        "    let w = Worker.create()\n"
        "    w.process(\"hello\")\n"
        "  }\n"
        "}\n"));
}
