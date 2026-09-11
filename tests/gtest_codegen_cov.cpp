#include <gtest/gtest.h>
#include <ponypp/codegen.h>
#include <ponypp/ast.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

static Codegen *make_codegen() {
    return codegen_new(stdout);
}

/* ==================== codegen_new/free ==================== */

TEST(CodegenCov, NewFree) {
    Codegen *cg = make_codegen();
    ASSERT_NE(cg, nullptr);
    codegen_free(cg);
}

TEST(CodegenCov, NewNullOutput) {
    Codegen *cg = codegen_new(nullptr);
    ASSERT_NE(cg, nullptr);
    codegen_free(cg);
}

TEST(CodegenCov, NewWithMap) {
    Codegen *cg = codegen_new_with_map(stdout, nullptr);
    ASSERT_NE(cg, nullptr);
    codegen_free(cg);
}

/* ==================== 代码生成 ==================== */

TEST(CodegenCov, EmitEmptyProgram) {
    Codegen *cg = make_codegen();
    ASSERT_NE(cg, nullptr);
    ASTNode *prog = ast_program_new(1, 1);
    codegen_program(cg, prog);
    codegen_free(cg);
    ast_node_free(prog);
}

TEST(CodegenCov, EmitActor) {
    Codegen *cg = make_codegen();
    ASSERT_NE(cg, nullptr);
    ASTNode *prog = ast_program_new(1, 1);
    ASTNode *actor = ast_actor_new("Main", 1, 1);
    ast_node_add_child(prog, actor);
    codegen_program(cg, prog);
    codegen_free(cg);
    ast_node_free(prog);
}

TEST(CodegenCov, EmitActorWithField) {
    Codegen *cg = make_codegen();
    ASSERT_NE(cg, nullptr);
    ASTNode *prog = ast_program_new(1, 1);
    ASTNode *actor = ast_actor_new("Main", 1, 1);
    ASTNode *field = ast_field_new("_count", true, 2, 1);
    ast_node_add_child(actor, field);
    ast_node_add_child(prog, actor);
    codegen_program(cg, prog);
    codegen_free(cg);
    ast_node_free(prog);
}

TEST(CodegenCov, EmitActorWithBehavior) {
    Codegen *cg = make_codegen();
    ASSERT_NE(cg, nullptr);
    ASTNode *prog = ast_program_new(1, 1);
    ASTNode *actor = ast_actor_new("Main", 1, 1);
    ASTNode *be = ast_method_new("ping", true, 2, 1);
    ast_node_add_child(actor, be);
    ast_node_add_child(prog, actor);
    codegen_program(cg, prog);
    codegen_free(cg);
    ast_node_free(prog);
}

TEST(CodegenCov, EmitActorWithFunction) {
    Codegen *cg = make_codegen();
    ASSERT_NE(cg, nullptr);
    ASTNode *prog = ast_program_new(1, 1);
    ASTNode *actor = ast_actor_new("Main", 1, 1);
    ASTNode *fun = ast_method_new("apply", false, 2, 1);
    ast_node_add_child(actor, fun);
    ast_node_add_child(prog, actor);
    codegen_program(cg, prog);
    codegen_free(cg);
    ast_node_free(prog);
}

/* ==================== SourceMap ==================== */

TEST(CodegenCov, SourceMapAdd) {
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);
    EXPECT_EQ(sourcemap_add(sm, 1, "test.pny", 1, 1), 0);
    EXPECT_EQ(sourcemap_add(sm, 2, "test.pny", 2, 1), 0);
    EXPECT_EQ(sourcemap_add(sm, 3, "test.pny", 3, 1), 0);
    sourcemap_free(sm);
}

TEST(CodegenCov, SourceMapLookup) {
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);
    sourcemap_add(sm, 1, "test.pny", 10, 5);
    sourcemap_add(sm, 2, "test.pny", 20, 10);

    SourceMapEntry entry;
    EXPECT_EQ(sourcemap_lookup(sm, 1, &entry), 0);
    EXPECT_EQ(entry.source_line, 10);
    EXPECT_EQ(entry.source_col, 5);

    EXPECT_EQ(sourcemap_lookup(sm, 2, &entry), 0);
    EXPECT_EQ(entry.source_line, 20);

    /* 不存在的行: 返回最近的条目 */
    (void)sourcemap_lookup(sm, 99, &entry);

    sourcemap_free(sm);
}

TEST(CodegenCov, SourceMapSaveJson) {
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);
    sourcemap_add(sm, 1, "test.pny", 1, 1);
    sourcemap_add(sm, 2, "test.pny", 2, 1);

    const char *path = "/tmp/test_sourcemap.json";
    EXPECT_EQ(sourcemap_save_json(sm, path), 0);

    /* 验证文件存在 */
    FILE *f = fopen(path, "r");
    ASSERT_NE(f, nullptr);
    fclose(f);
    unlink(path);

    sourcemap_free(sm);
}

TEST(CodegenCov, SourceMapNullSafety) {
    sourcemap_free(nullptr);
    EXPECT_NE(sourcemap_add(nullptr, 1, "test.pny", 1, 1), 0);

    SourceMapEntry entry;
    EXPECT_NE(sourcemap_lookup(nullptr, 1, &entry), 0);
    EXPECT_NE(sourcemap_save_json(nullptr, "/tmp/x.json"), 0);
}

/* ==================== 辅助函数 ==================== */

TEST(CodegenCov, SetSourceFile) {
    Codegen *cg = make_codegen();
    ASSERT_NE(cg, nullptr);
    codegen_set_source_file(cg, "test.pny");
    codegen_free(cg);
}

TEST(CodegenCov, EmitLineDirective) {
    Codegen *cg = make_codegen();
    ASSERT_NE(cg, nullptr);
    codegen_emit_line_directive(cg, 42);
    codegen_free(cg);
}

TEST(CodegenCov, EmitLineDirectiveZero) {
    Codegen *cg = make_codegen();
    ASSERT_NE(cg, nullptr);
    codegen_emit_line_directive(cg, 0);
    codegen_free(cg);
}
