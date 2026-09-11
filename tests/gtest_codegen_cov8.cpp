#include <gtest/gtest.h>
#include <ponypp/codegen.h>
#include <ponypp/ast.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

static ASTNode *parse_code(const char *src) {
    Lexer *lx = lexer_new("test", src, strlen(src));
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("test", toks, tc);
    return parser_parse_program(p);
}

static bool gen_ok(const char *src) {
    ASTNode *ast = parse_code(src);
    if (!ast) return false;
    char tmpl[] = "/tmp/ponypp_cg8_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) { ast_node_free(ast); return false; }
    close(fd);
    FILE *f = fopen(tmpl, "w");
    if (!f) { unlink(tmpl); ast_node_free(ast); return false; }
    Codegen *cg = codegen_new(f);
    if (!cg) { fclose(f); unlink(tmpl); ast_node_free(ast); return false; }
    codegen_set_source_file(cg, "test.pny");
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    unlink(tmpl);
    ast_node_free(ast);
    return true;
}

/* ==================== Float ==================== */

TEST(CodegenCov8, FloatLiteral) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print(3.14)\n  }\n}\n"));
}

/* ==================== String Methods ==================== */

TEST(CodegenCov8, StringSize) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let n: USize = \"hello\".size()\n  }\n}\n"));
}

TEST(CodegenCov8, StringApply) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let c = \"hello\".apply(0)\n  }\n}\n"));
}

TEST(CodegenCov8, StringSubstring) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let s = \"hello\".substring(1, 3)\n  }\n}\n"));
}

TEST(CodegenCov8, StringContains) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let b = \"hello\".contains(\"ell\")\n  }\n}\n"));
}

TEST(CodegenCov8, StringToUpper) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let s = \"hello\".upper()\n  }\n}\n"));
}

TEST(CodegenCov8, StringToLower) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let s = \"HELLO\".lower()\n  }\n}\n"));
}

TEST(CodegenCov8, StringStrip) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let s = \" hello \".strip()\n  }\n}\n"));
}

/* ==================== Field Access ==================== */

TEST(CodegenCov8, FieldAccessUnderscore) {
    EXPECT_TRUE(gen_ok("actor main {\n  var _x: U32\n  new create() => {\n    print(_x)\n  }\n}\n"));
}

/* ==================== Print Bool/Char ==================== */

TEST(CodegenCov8, PrintBoolTrue) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print(true)\n  }\n}\n"));
}

TEST(CodegenCov8, PrintBoolFalse) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print(false)\n  }\n}\n"));
}

TEST(CodegenCov8, PrintChar) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print('A')\n  }\n}\n"));
}

/* ==================== Return from Fun ==================== */

TEST(CodegenCov8, ReturnFromFun) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => { }\n  fun _helper(): U32 =>\n    return 42\n}\n"));
}
