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

/* ==================== 更多程序代码生成 ==================== */

TEST(CodegenCov4, EmptyProgram) {
    codegen_code("");
}

TEST(CodegenCov4, EmptyFunc) {
    codegen_code("fn main() { }");
}

TEST(CodegenCov4, FuncWithLet) {
    codegen_code("fn main() { let x: i32 = 1; }");
}

TEST(CodegenCov4, FuncWithReturn) {
    codegen_code("fn main() -> i32 { return 42; }");
}

TEST(CodegenCov4, FuncWithPrint) {
    codegen_code("fn main() { print(42); }");
}

TEST(CodegenCov4, FuncWithBinaryOp) {
    codegen_code("fn main() -> i32 { return 1 + 2; }");
}

TEST(CodegenCov4, FuncWithUnaryOp) {
    codegen_code("fn main() -> i32 { return -42; }");
}

TEST(CodegenCov4, FuncWithIf) {
    codegen_code("fn main() { if true { print(1); } }");
}

TEST(CodegenCov4, FuncWithIfElse) {
    codegen_code("fn main() { if true { print(1); } else { print(2); } }");
}

TEST(CodegenCov4, FuncWithWhile) {
    codegen_code("fn main() { while false { print(1); } }");
}

TEST(CodegenCov4, FuncWithParams) {
    codegen_code("fn add(a: i32, b: i32) -> i32 { return a + b; }");
}

TEST(CodegenCov4, FuncCall) {
    codegen_code("fn main() { helper(); } fn helper() { }");
}

TEST(CodegenCov4, MultipleFuncs) {
    codegen_code("fn main() { } fn helper() -> i32 { return 1; } fn another() { }");
}

/* ==================== 更多复杂表达式 ==================== */

TEST(CodegenCov4, ChainedBinaryOps) {
    codegen_code("fn main() -> i32 { return 1 + 2 * 3 - 4; }");
}

TEST(CodegenCov4, NestedFuncCalls) {
    codegen_code("fn f(x: i32) -> i32 { return x; } fn main() { f(f(f(42))); }");
}

TEST(CodegenCov4, ComparisonOps) {
    codegen_code("fn main() -> bool { return 1 < 2 && 3 > 4; }");
}

/* ==================== 更多能力类型 ==================== */

TEST(CodegenCov4, ActorIso) {
    codegen_code("actor Counter { var count: iso i32; }");
}

TEST(CodegenCov4, ActorTrn) {
    codegen_code("actor Buffer { var data: trn string; }");
}

TEST(CodegenCov4, ActorRef) {
    codegen_code("actor Logger { var messages: ref Array<string>; }");
}

/* ==================== 更多泛型 ==================== */

TEST(CodegenCov4, GenericFunc) {
    codegen_code("fn identity<T>(x: T) -> T { return x; }");
}

TEST(CodegenCov4, GenericActor) {
    codegen_code("actor Stack<T> { var items: Array<T>; }");
}

/* ==================== sourcemap 更多情况 ==================== */

TEST(CodegenCov4, SourceMapSaveLoadMultiple) {
    SourceMap *sm = sourcemap_new();
    EXPECT_NE(sm, nullptr);
    sourcemap_add(sm, 1, "test.pny", 1, 1);
    sourcemap_add(sm, 2, "test.pny", 2, 1);
    sourcemap_add(sm, 3, "test.pny", 3, 1);
    int ret = sourcemap_save_json(sm, "/tmp/test_sourcemap_cov4.json");
    EXPECT_EQ(ret, 0);
    SourceMap *loaded = sourcemap_load_json("/tmp/test_sourcemap_cov4.json");
    EXPECT_NE(loaded, nullptr);
    if (loaded) {
        SourceMapEntry entry;
        int found = sourcemap_lookup(loaded, 1, &entry);
        EXPECT_EQ(found, 0);
        found = sourcemap_lookup(loaded, 2, &entry);
        EXPECT_EQ(found, 0);
        found = sourcemap_lookup(loaded, 3, &entry);
        EXPECT_EQ(found, 0);
        sourcemap_free(loaded);
    }
    unlink("/tmp/test_sourcemap_cov4.json");
    sourcemap_free(sm);
}

TEST(CodegenCov4, SourceMapSaveLoadEmpty) {
    SourceMap *sm = sourcemap_new();
    EXPECT_NE(sm, nullptr);
    int ret = sourcemap_save_json(sm, "/tmp/test_sourcemap_empty_cov4.json");
    EXPECT_EQ(ret, 0);
    SourceMap *loaded = sourcemap_load_json("/tmp/test_sourcemap_empty_cov4.json");
    EXPECT_NE(loaded, nullptr);
    if (loaded) {
        sourcemap_free(loaded);
    }
    unlink("/tmp/test_sourcemap_empty_cov4.json");
    sourcemap_free(sm);
}

/* ==================== codegen_set_source_file 更多情况 ==================== */

TEST(CodegenCov4, SetSourceFileEmpty) {
    FILE *out = fopen("/dev/null", "w");
    if (!out) return;
    Codegen *cg = codegen_new(out);
    if (cg) {
        codegen_set_source_file(cg, "");
        codegen_free(cg);
    }
    fclose(out);
}

TEST(CodegenCov4, SetSourceFilePath) {
    FILE *out = fopen("/dev/null", "w");
    if (!out) return;
    Codegen *cg = codegen_new(out);
    if (cg) {
        codegen_set_source_file(cg, "/path/to/test.pny");
        codegen_free(cg);
    }
    fclose(out);
}

/* ==================== codegen_new_with_map 更多情况 ==================== */

TEST(CodegenCov4, NewWithMapAndSourceFile) {
    ASTNode *ast = parse_code("fn main() { }");
    if (!ast) return;
    FILE *out = fopen("/dev/null", "w");
    if (!out) { ast_node_free(ast); return; }
    SourceMap *sm = sourcemap_new();
    Codegen *cg = codegen_new_with_map(out, sm);
    if (cg) {
        codegen_set_source_file(cg, "test.pny");
        codegen_program(cg, ast);
        codegen_free(cg);
    }
    sourcemap_free(sm);
    fclose(out);
    ast_node_free(ast);
}
