#include <gtest/gtest.h>
#include <cstdio>
#include "gtest_helpers.h"
#include "ponypp/codegen.h"
#include "ponypp/wasm.h"

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

// M2: char_code 内建 — 首字节 ASCII (页日志校验和用)
TEST(Codegen, CharCodeBuiltin) {
    const char* src =
        "actor main {\n"
        "  new create() => {\n"
        "    var a: U32 = char_code(\"A\")\n"
        "    print(\"a=\" + a)\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_gen_cc.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    FILE* rf = fopen("/tmp/ponypp_gen_cc.c", "r");
    ASSERT_NE(rf, nullptr);
    static char buf[262144] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
    fclose(rf);
    buf[n] = 0;
    EXPECT_NE(std::strstr(buf, "pny_char_code("), nullptr);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_cc.c");
}

// M3: env_get 内建 — 环境变量读取 (native/wasi-libc getenv), 返回 String
TEST(Codegen, EnvGetBuiltin) {
    const char* src =
        "actor main {\n"
        "  new create() => {\n"
        "    var v: String = env_get(\"HOME\")\n"
        "    print(\"v=\" + v)\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_gen_env.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    FILE* rf = fopen("/tmp/ponypp_gen_env.c", "r");
    ASSERT_NE(rf, nullptr);
    static char buf[262144] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
    fclose(rf);
    buf[n] = 0;
    EXPECT_NE(std::strstr(buf, "pny_env_get("), nullptr);
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_env.c");
}


// PONYPP_GC=1: Boehm GC 模式 — 运行时分配全部走 GC_malloc (ponydb 泄漏修复实证)
TEST(Codegen, GcModeMacros) {
    const char* src =
        "actor main {\n"
        "  new create() => {\n"
        "    print(\"gc\")\n"
        "  }\n"
        "}\n";
    setenv("PONYPP_GC", "1", 1);
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_gen_gc.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    unsetenv("PONYPP_GC");
    FILE* rf = fopen("/tmp/ponypp_gen_gc.c", "r");
    ASSERT_NE(rf, nullptr);
    static char buf[262144] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
    fclose(rf);
    buf[n] = 0;
    EXPECT_NE(std::strstr(buf, "#include <gc/gc.h>"), nullptr) << "GC 模式必须注入 gc.h";
    EXPECT_NE(std::strstr(buf, "GC_malloc"), nullptr) << "GC 模式 malloc 必须映射 GC_malloc";
    EXPECT_NE(std::strstr(buf, "GC_free"), nullptr) << "GC 模式 free 必须映射 GC_free";
    // 关闭开关时不得注入
    setenv("PONYPP_GC", "", 1);
    unsetenv("PONYPP_GC");
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_gc.c");
}


// Bug#47: 裸局部变量被同名类字段劫持 — `var cur` + 传参 `page_get(cur)` 曾生成 page_get(self, self->cur)
TEST(Codegen, LocalShadowsField) {
    const char* src =
        "class Foo {\n"
        "  var cur: String = \"WRONG\"\n"
        "  new create() => {\n"
        "    var z: U32 = 0\n"
        "  }\n"
        "  fun page_get(id: String): String => {\n"
        "    return id\n"
        "  }\n"
        "  fun get(): String => {\n"
        "    var cur: String = \"RIGHT\"\n"
        "    return this.page_get(cur)\n"
        "  }\n"
        "}\n"
        "actor main {\n"
        "  new create() => {\n"
        "    var f: Foo = Foo()\n"
        "    print(f.get())\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_gen47.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    FILE* rf = fopen("/tmp/ponypp_gen47.c", "r");
    ASSERT_NE(rf, nullptr);
    static char buf[262144] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
    fclose(rf);
    buf[n] = 0;
    EXPECT_NE(std::strstr(buf, "Foo_page_get(self, cur)"), nullptr) << "局部变量必须裸名传参";
    EXPECT_EQ(std::strstr(buf, "Foo_page_get(self, self->cur)"), nullptr) << "局部变量不得被同名字段劫持";
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen47.c");
}


// P1: find_from(s, sub, off) — 从偏移起查找 (单趟增量扫描, ponydb 解析热点)
TEST(Codegen, FindFromBuiltin) {
    const char* src =
        "actor main {\n"
        "  new create() => {\n"
        "    var s: String = \"a|b|c\"\n"
        "    var i: U32 = s.find_from(\"|\", 1)\n"
        "    var t: String = s.slice(i + 1, s.len())\n"
        "    print(t)\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_gen_ff.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    FILE* rf = fopen("/tmp/ponypp_gen_ff.c", "r");
    ASSERT_NE(rf, nullptr);
    static char buf[262144] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
    fclose(rf);
    buf[n] = 0;
    EXPECT_NE(std::strstr(buf, "pny_str_find_from(s, \"|\", 1)"), nullptr) << "find_from 必须分派到 pny_str_find_from";
    EXPECT_NE(std::strstr(buf, "static long long pny_str_find_from"), nullptr) << "运行时必须包含 find_from 实现";
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_ff.c");
}


// Bug#48: % 取模 — parser 乘法层此前不含 TK_PERCENT, `pos % 2` 静默编译成 `pos`
TEST(Codegen, ModuloOperator) {
    const char* src =
        "actor main {\n"
        "  new create() => {\n"
        "    var a: U32 = 7\n"
        "    var b: U32 = a % 2\n"
        "    print(\"m\")\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_gen_mod.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    FILE* rf = fopen("/tmp/ponypp_gen_mod.c", "r");
    ASSERT_NE(rf, nullptr);
    static char buf[262144] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
    fclose(rf);
    buf[n] = 0;
    EXPECT_NE(std::strstr(buf, "% (int)(2)"), nullptr) << "% 必须生成 C 取模";
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_mod.c");
}


// P1-4: GC 模式下纯字符缓冲走 GC_malloc_atomic (免保守扫描)
TEST(Codegen, GcAtomicStrings) {
    const char* src =
        "actor main {\n"
        "  new create() => {\n"
        "    print(\"gc-atomic\")\n"
        "  }\n"
        "}\n";
    setenv("PONYPP_GC", "1", 1);
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_gen_gca.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    unsetenv("PONYPP_GC");
    FILE* rf = fopen("/tmp/ponypp_gen_gca.c", "r");
    ASSERT_NE(rf, nullptr);
    static char buf[262144] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
    fclose(rf);
    buf[n] = 0;
    EXPECT_NE(std::strstr(buf, "#define pny_xmalloc(n) GC_malloc_atomic(n)"), nullptr)
        << "GC 模式必须注入 atomic 分配宏";
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_gca.c");
}

// 非 GC 模式: pny_xmalloc 退化为 malloc
TEST(Codegen, NoGcXmallocFallback) {
    const char* src =
        "actor main {\n"
        "  new create() => {\n"
        "    print(\"plain\")\n"
        "  }\n"
        "}\n";
    unsetenv("PONYPP_GC");
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_gen_gcn.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    FILE* rf = fopen("/tmp/ponypp_gen_gcn.c", "r");
    ASSERT_NE(rf, nullptr);
    static char buf[262144] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
    fclose(rf);
    buf[n] = 0;
    EXPECT_NE(std::strstr(buf, "#define pny_xmalloc(n) malloc(n)"), nullptr)
        << "非 GC 模式 pny_xmalloc 必须退化为 malloc";
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_gcn.c");
}


// Bug#46 W0: wasi-p2 必须同时导出 _start (wasmtime run 命令入口) 与 main (WAMR/兼容)
TEST(WasmBackend, WasiStartExport) {
    const char* src =
        "actor main {\n"
        "  new create() => {\n"
        "    print(\"hi\")\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    int rc = wasm_write_program(ast, "/tmp/ponypp_gen_wstart.wasm", TARGET_WASI_P2);
    ASSERT_EQ(rc, 0);
    FILE* rf = fopen("/tmp/ponypp_gen_wstart.wasm", "rb");
    ASSERT_NE(rf, nullptr);
    static unsigned char buf[65536] = {0};
    size_t n = fread(buf, 1, sizeof(buf), rf);
    fclose(rf);
    bool has_start = false, has_main = false;
    for (size_t i = 0; i + 6 < n; i++) {
        if (memcmp(buf + i, "_start", 6) == 0) has_start = true;
        if (memcmp(buf + i, "main", 4) == 0) has_main = true;
    }
    EXPECT_TRUE(has_start) << "wasi-p2 必须导出 _start";
    EXPECT_TRUE(has_main) << "wasi-p2 必须保留 main 导出 (WAMR e2e 依赖)";
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_wstart.wasm");
}
