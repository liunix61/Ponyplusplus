#include <gtest/gtest.h>
#include <ponypp/codegen.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

static ASTNode *parse_code(const char *code) {
    Lexer *lx = lexer_new("test.pny", code, strlen(code));
    if (!lx) return nullptr;
    Token *tokens = nullptr;
    size_t count = 0;
    lexer_lex_all(lx, &tokens, &count);
    lexer_free(lx);
    Parser *ps = parser_new("test.pny", tokens, count);
    if (!ps) return nullptr;
    ASTNode *ast = parser_parse_program(ps);
    parser_free(ps);
    return ast;
}

static void codegen_code(const char *code) {
    ASTNode *ast = parse_code(code);
    if (!ast) return;
    FILE *out = fopen("/dev/null", "w");
    if (!out) { ast_node_free(ast); return; }
    Codegen *cg = codegen_new(out);
    if (cg) {
        codegen_program(cg, ast);
        codegen_free(cg);
    }
    fclose(out);
    ast_node_free(ast);
}

/* ==================== codegen_program ==================== */

TEST(CodegenCov2, EmptyProgram) {
    codegen_code("");
}

TEST(CodegenCov2, EmptyFunc) {
    codegen_code("fn main() { }");
}

TEST(CodegenCov2, FuncWithLet) {
    codegen_code("fn main() { let x: i32 = 1; }");
}

TEST(CodegenCov2, FuncWithReturn) {
    codegen_code("fn main() -> i32 { return 42; }");
}

TEST(CodegenCov2, FuncWithPrint) {
    codegen_code("fn main() { print(42); }");
}

TEST(CodegenCov2, FuncWithBinaryOp) {
    codegen_code("fn main() -> i32 { return 1 + 2; }");
}

TEST(CodegenCov2, FuncWithUnaryOp) {
    codegen_code("fn main() -> i32 { return -42; }");
}

TEST(CodegenCov2, FuncWithIf) {
    codegen_code("fn main() { if true { print(1); } }");
}

TEST(CodegenCov2, FuncWithIfElse) {
    codegen_code("fn main() { if true { print(1); } else { print(2); } }");
}

TEST(CodegenCov2, FuncWithWhile) {
    codegen_code("fn main() { while false { print(1); } }");
}

TEST(CodegenCov2, FuncWithParams) {
    codegen_code("fn add(a: i32, b: i32) -> i32 { return a + b; }");
}

TEST(CodegenCov2, FuncCall) {
    codegen_code("fn main() { helper(); } fn helper() { }");
}

TEST(CodegenCov2, MultipleFuncs) {
    codegen_code("fn main() { } fn helper() -> i32 { return 1; } fn another() { }");
}

/* ==================== 复杂表达式 ==================== */

TEST(CodegenCov2, ChainedBinaryOps) {
    codegen_code("fn main() -> i32 { return 1 + 2 * 3 - 4; }");
}

TEST(CodegenCov2, NestedFuncCalls) {
    codegen_code("fn f(x: i32) -> i32 { return x; } fn main() { f(f(f(42))); }");
}

TEST(CodegenCov2, ComparisonOps) {
    codegen_code("fn main() -> bool { return 1 < 2 && 3 > 4; }");
}

/* ==================== 能力类型 ==================== */

TEST(CodegenCov2, ActorWithIso) {
    codegen_code("actor Counter { var count: iso i32; }");
}

TEST(CodegenCov2, ActorWithTrn) {
    codegen_code("actor Buffer { var data: trn string; }");
}

TEST(CodegenCov2, ActorWithRef) {
    codegen_code("actor Logger { var messages: ref Array<string>; }");
}

/* ==================== 泛型 ==================== */

TEST(CodegenCov2, GenericFunc) {
    codegen_code("fn identity<T>(x: T) -> T { return x; }");
}

TEST(CodegenCov2, GenericActor) {
    codegen_code("actor Stack<T> { var items: Array<T>; }");
}

/* ==================== codegen_new_with_map ==================== */

TEST(CodegenCov2, NewWithMap) {
    ASTNode *ast = parse_code("fn main() { }");
    if (!ast) return;
    FILE *out = fopen("/dev/null", "w");
    if (!out) { ast_node_free(ast); return; }
    SourceMap *sm = sourcemap_new();
    Codegen *cg = codegen_new_with_map(out, sm);
    if (cg) {
        codegen_program(cg, ast);
        codegen_free(cg);
    }
    sourcemap_free(sm);
    fclose(out);
    ast_node_free(ast);
}

/* ==================== codegen_set_source_file ==================== */

TEST(CodegenCov2, SetSourceFile) {
    FILE *out = fopen("/dev/null", "w");
    if (!out) return;
    Codegen *cg = codegen_new(out);
    if (cg) {
        codegen_set_source_file(cg, "test.pny");
        codegen_free(cg);
    }
    fclose(out);
}

/* ==================== codegen_emit_line_directive ==================== */



/* ==================== sourcemap_save/load ==================== */

TEST(CodegenCov2, SourceMapSaveLoad) {
    SourceMap *sm = sourcemap_new();
    EXPECT_NE(sm, nullptr);
    sourcemap_add(sm, 1, "test.pny", 1, 1);
    sourcemap_add(sm, 2, "test.pny", 2, 1);
    int ret = sourcemap_save_json(sm, "/tmp/test_sourcemap_cov2.json");
    EXPECT_EQ(ret, 0);
    SourceMap *loaded = sourcemap_load_json("/tmp/test_sourcemap_cov2.json");
    EXPECT_NE(loaded, nullptr);
    if (loaded) {
        SourceMapEntry entry;
        int found = sourcemap_lookup(loaded, 1, &entry);
        EXPECT_EQ(found, 0);
        sourcemap_free(loaded);
    }
    unlink("/tmp/test_sourcemap_cov2.json");
    sourcemap_free(sm);
}
