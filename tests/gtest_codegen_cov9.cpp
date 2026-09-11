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
    char tmpl[] = "/tmp/ponypp_cg9_XXXXXX";
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

/* ==================== Print Field Access ==================== */

TEST(CodegenCov9, PrintFieldAccess) {
    EXPECT_TRUE(gen_ok("actor main {\n  var _x: U32\n  new create() => {\n    _x = 42\n    print(_x)\n  }\n}\n"));
}

/* ==================== Print Int Expression ==================== */

TEST(CodegenCov9, PrintIntExpr) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print(42)\n  }\n}\n"));
}

/* ==================== Print String Data ==================== */

TEST(CodegenCov9, PrintStringData) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print(\"hello\")\n  }\n}\n"));
}

/* ==================== String Method Calls ==================== */

TEST(CodegenCov9, StringSize) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let n: USize = \"hello\".size()\n  }\n}\n"));
}

TEST(CodegenCov9, StringApply) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let c = \"hello\".apply(0)\n  }\n}\n"));
}

TEST(CodegenCov9, StringSubstring) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let s = \"hello\".substring(1, 3)\n  }\n}\n"));
}

TEST(CodegenCov9, StringContains) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let b = \"hello\".contains(\"ell\")\n  }\n}\n"));
}

TEST(CodegenCov9, StringStartsWith) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let b = \"hello\".startswith(\"he\")\n  }\n}\n"));
}

TEST(CodegenCov9, StringEndsWith) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let b = \"hello\".endswith(\"lo\")\n  }\n}\n"));
}

TEST(CodegenCov9, StringFind) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let n: USize = \"hello\".find(\"l\")\n  }\n}\n"));
}

TEST(CodegenCov9, StringUpper) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let s = \"hello\".upper()\n  }\n}\n"));
}

TEST(CodegenCov9, StringLower) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let s = \"HELLO\".lower()\n  }\n}\n"));
}

TEST(CodegenCov9, StringStrip) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let s = \" hello \".strip()\n  }\n}\n"));
}

TEST(CodegenCov9, StringRepeat) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let s = \"ab\".repeat(3)\n  }\n}\n"));
}

TEST(CodegenCov9, StringSplit) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let a = \"a,b,c\".split(\",\")\n  }\n}\n"));
}

TEST(CodegenCov9, StringReplace) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let s = \"hello\".replace(\"l\", \"L\")\n  }\n}\n"));
}

TEST(CodegenCov9, StringToString) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    let s = \"hello\".string()\n  }\n}\n"));
}

/* ==================== Float Literal ==================== */

TEST(CodegenCov9, FloatLiteral) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print(3.14)\n  }\n}\n"));
}

/* ==================== Bool Literal ==================== */

TEST(CodegenCov9, BoolTrue) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print(true)\n  }\n}\n"));
}

TEST(CodegenCov9, BoolFalse) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print(false)\n  }\n}\n"));
}

/* ==================== Char Literal ==================== */

TEST(CodegenCov9, CharLiteral) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print('A')\n  }\n}\n"));
}

/* ==================== Return from Fun ==================== */

TEST(CodegenCov9, ReturnFromFun) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => { }\n  fun _helper(): U32 =>\n    return 42\n}\n"));
}

/* ==================== Field Access Underscore ==================== */

TEST(CodegenCov9, FieldAccessUnderscore) {
    EXPECT_TRUE(gen_ok("actor main {\n  var _x: U32\n  new create() => {\n    print(_x)\n  }\n}\n"));
}

/* ==================== Complex Program ==================== */

TEST(CodegenCov9, ActorWithFieldsAndMethods) {
    EXPECT_TRUE(gen_ok(
        "actor Counter {\n"
        "  var _count: U32\n"
        "  new create() => { _count = 0 }\n"
        "  be increment() => { _count = _count + 1 }\n"
        "  be get() => { print(_count) }\n"
        "}\n"
        "actor main {\n"
        "  new create() => {\n"
        "    let c = Counter.create()\n"
        "    c.increment()\n"
        "    c.get()\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov9, NestedIfElse) {
    EXPECT_TRUE(gen_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: U32 = 5\n"
        "    if x > 10 then print(\"big\")\n"
        "    elseif x > 3 then print(\"medium\")\n"
        "    elseif x > 1 then print(\"small\")\n"
        "    else print(\"tiny\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov9, TryCatchFinally) {
    EXPECT_TRUE(gen_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    try\n"
        "      print(\"try\")\n"
        "    then\n"
        "      print(\"then\")\n"
        "    else\n"
        "      print(\"else\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov9, WhileWithBreakContinue) {
    EXPECT_TRUE(gen_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    var i: U32 = 0\n"
        "    while i < 10 do\n"
        "      i = i + 1\n"
        "      if i == 5 then continue end\n"
        "      if i == 8 then break end\n"
        "    end\n"
        "  }\n"
        "}\n"));
}
