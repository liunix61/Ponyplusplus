#include <gtest/gtest.h>
#include <cstdio>
#include "gtest_helpers.h"
#include "ponypp/codegen.h"

TEST(Codegen, GeneratesC) {
    const char* src = "actor main { be run() => { print(\"hi\") } }";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_gen.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    ASSERT_NE(cg, nullptr);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);

    FILE* rf = fopen("/tmp/ponypp_gen.c", "r");
    ASSERT_NE(rf, nullptr);
    static char buf[262144] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
    fclose(rf);
    EXPECT_GT(n, 0);
    EXPECT_NE(std::strstr(buf, "typedef struct"), nullptr);
    EXPECT_NE(std::strstr(buf, "int main"), nullptr);
    EXPECT_NE(std::strstr(buf, "PnyRuntime"), nullptr);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen.c");
}

TEST(Codegen, EmptyProgram) {
    ASTNode* ast = parse_to_ast("");
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_empty_gen.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    ASSERT_NE(cg, nullptr);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_empty_gen.c");
}

TEST(Codegen, ActorWithFields) {
    const char* src =
        "actor Counter {\n"
        "  var count: U64 = 0\n"
        "  be run() => {}\n"
        "}";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_fields.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    ASSERT_NE(cg, nullptr);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_fields.c");
}

// Bug#43: 字符串 < > <= >= 此前走裸指针比较 (仅 ==/!= 有 strcmp)
TEST(Codegen, StrRelationalCmp) {
    const char* src =
        "actor main {\n"
        "  new create() => {\n"
        "    var b: String = str_field(sys_exec(\"printf x\"), 0, \"\\n\")\n"
        "    var c: String = str_field(sys_exec(\"printf y\"), 0, \"\\n\")\n"
        "    if b < c { print(\"lt\") }\n"
        "    if c > b { print(\"gt\") }\n"
        "    if b <= b { print(\"le\") }\n"
        "    if c >= b { print(\"ge\") }\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_gen43.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    FILE* rf = fopen("/tmp/ponypp_gen43.c", "r");
    ASSERT_NE(rf, nullptr);
    static char buf[262144] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
    fclose(rf);
    buf[n] = 0;
    int cnt = 0;
    for (const char* p = std::strstr(buf, "strcmp("); p; p = std::strstr(p + 1, "strcmp(")) cnt++;
    EXPECT_GE(cnt, 4) << "字符串关系比较必须全部走 strcmp";
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen43.c");
}

// M2: file_size/file_append 内建 (COW 页文件需要) — file_append 复用 PEX 时代运行时
TEST(Codegen, FileSizeAppendBuiltins) {
    const char* src =
        "actor main {\n"
        "  new create() => {\n"
        "    var r: U32 = file_append(\"/tmp/x.db\", \"data\")\n"
        "    var sz: U32 = file_size(\"/tmp/x.db\")\n"
        "    var ex: U32 = file_exists(\"/tmp/x.db\")\n"
        "    print(\"r=\" + r)\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_gen_fio.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    FILE* rf = fopen("/tmp/ponypp_gen_fio.c", "r");
    ASSERT_NE(rf, nullptr);
    static char buf[262144] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
    fclose(rf);
    buf[n] = 0;
    EXPECT_NE(std::strstr(buf, "pny_file_append("), nullptr);
    EXPECT_NE(std::strstr(buf, "pny_file_size("), nullptr);
    EXPECT_NE(std::strstr(buf, "pny_file_exists("), nullptr);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_fio.c");
}
