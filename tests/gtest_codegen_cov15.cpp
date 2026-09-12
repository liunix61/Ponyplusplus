#include <gtest/gtest.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp/codegen.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>

static bool emit_ok(const char *src) {
    Lexer *lx = lexer_new("t", src, strlen(src));
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("t", toks, tc);
    ASTNode *ast = parser_parse_program(p);
    bool ok = false;
    if (ast) {
        FILE *out = tmpfile();
        Codegen *cg = codegen_new(out);
        if (cg) {
            codegen_program(cg, ast);
            codegen_free(cg);
            ok = true;
        }
        fclose(out);
        ast_node_free(ast);
    }
    parser_free(p);
    free(toks);
    lexer_free(lx);
    return ok;
}

/* 针对性覆盖 src/codegen.c 未覆盖行:
 * - 319-326: String.charAt / to_string
 * - 330-343: String.startsWith / toUpperCase
 * - 347-360: List.append / length
 * - 370-388: List()/Set()/Map() 构造
 * - 390-416: ActorRef()/String() 构造 + 跨 actor 构造
 */

/* ==================== String methods ==================== */

TEST(CodegenCov15, StringCharAt) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hi\"\n"
        "    let c: U32 = s.charAt(0)\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov15, StringCharAtNoArg) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hi\"\n"
        "    let c: U32 = s.charAt()\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov15, StringToString) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hi\"\n"
        "    let t: String = s.to_string()\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov15, StringStartsWith) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hello\"\n"
        "    let b: Bool = s.startsWith(\"he\")\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov15, StringStartsWithNoArg) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hello\"\n"
        "    let b: Bool = s.startsWith()\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov15, StringToUpper) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = \"hi\"\n"
        "    let t: String = s.toUpperCase()\n"
        "  }\n"
        "}\n"));
}

/* ==================== List methods ==================== */

TEST(CodegenCov15, ListAppend) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let l: List = List()\n"
        "    l.append(42)\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov15, ListAppendNoArg) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let l: List = List()\n"
        "    l.append()\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov15, ListLength) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let l: List = List()\n"
        "    let n: U64 = l.length()\n"
        "  }\n"
        "}\n"));
}

/* ==================== Builtin constructors ==================== */

TEST(CodegenCov15, SetConstructor) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: Set = Set()\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov15, MapConstructor) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let m: Map = Map()\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov15, StringConstructorArg) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = String(\"abc\")\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov15, StringConstructorNoArg) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let s: String = String()\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov15, ActorRefConstructor) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let r: ActorRef = ActorRef()\n"
        "  }\n"
        "}\n"));
}

/* ==================== Cross-actor constructor ==================== */

TEST(CodegenCov15, SelfActorConstructor) {
    /* func == actor_name -> Main_create(...) */
    EXPECT_TRUE(emit_ok(
        "actor Main {\n"
        "  new create() => {}\n"
        "  be spawn() => {\n"
        "    let m = Main()\n"
        "  }\n"
        "}\n"));
}
