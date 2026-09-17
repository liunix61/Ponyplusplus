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


// extern fun FFI (0.2.16): 顶层 extern 声明 → C 原型 + 直调分派
TEST(Codegen, ExternFunFFI) {
    const char* src =
        "extern fun toy_hash(data: String, len: U32): I64\n"
        "actor main {\n"
        "  new create() => {\n"
        "    var h: I64 = toy_hash(\"blk\", 3)\n"
        "    print(h)\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    FILE* f = fopen("/tmp/ponypp_gen_ext.c", "w");
    ASSERT_NE(f, nullptr);
    Codegen* cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    FILE* rf = fopen("/tmp/ponypp_gen_ext.c", "r");
    ASSERT_NE(rf, nullptr);
    static char buf[262144] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, rf);
    fclose(rf);
    buf[n] = 0;
    EXPECT_NE(std::strstr(buf, "extern signed long long toy_hash(const char *, unsigned int);"), nullptr)
        << "extern fun 必须发射 C 原型";
    EXPECT_NE(std::strstr(buf, "toy_hash(\"blk\", 3)"), nullptr)
        << "调用点必须直发 C 调用";
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_ext.c");
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
        "  fun ref main() {\n"
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


// W1: wasm 后端赋值/复合赋值发射 local.set (此前静默丢弃)
TEST(WasmBackend, AssignEmitsLocalSet) {
    const char* src =
        "actor main {\n"
        "  fun ref main() {\n"
        "    var i: U32 = 0\n"
        "    i = i + 1\n"
        "    i += 2\n"
        "    print(\"x\")\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(wasm_write_program(ast, "/tmp/ponypp_gen_wassign.wasm", TARGET_WASI_P2), 0);
    FILE* rf = fopen("/tmp/ponypp_gen_wassign.wasm", "rb");
    ASSERT_NE(rf, nullptr);
    static unsigned char buf[65536] = {0};
    size_t n = fread(buf, 1, sizeof(buf), rf);
    fclose(rf);
    int set_count = 0;
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == 0x21) set_count++; /* local.set */
    }
    EXPECT_GE(set_count, 3) << "var 声明 + 赋值 + 复合赋值 各需一个 local.set";
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_wassign.wasm");
}


// W2: 字符串运行时 — a+"B" 必须分派到 concat (call idx 9), print 到 print_str (idx 13)
TEST(WasmBackend, StringRuntimeDispatch) {
    const char* src =
        "actor main {\n"
        "  fun ref main() {\n"
        "    var a: String = \"A\"\n"
        "    var b: String = a + \"B\"\n"
        "    print(b)\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(wasm_write_program(ast, "/tmp/ponypp_gen_wstr.wasm", TARGET_WASI_P2), 0);
    FILE* rf = fopen("/tmp/ponypp_gen_wstr.wasm", "rb");
    ASSERT_NE(rf, nullptr);
    static unsigned char buf[65536] = {0};
    size_t n = fread(buf, 1, sizeof(buf), rf);
    fclose(rf);
    bool call_concat = false, call_print_str = false, has_global = false;
    for (size_t i = 0; i + 1 < n; i++) {
        if (buf[i] == 0x10 && buf[i + 1] == 0x12) call_concat = true;   /* call concat=18 (b=15) */
        if (buf[i] == 0x10 && buf[i + 1] == 0x16) call_print_str = true;/* call print_str=22 (b=15) */
        if (buf[i] == 0x06) has_global = true;                          /* global section */
    }
    EXPECT_TRUE(call_concat) << "字符串 + 必须分派 concat 运行时";
    EXPECT_TRUE(call_print_str) << "print(String) 必须走 print_str (strlen+fd_write)";
    EXPECT_TRUE(has_global) << "堆指针 global 段必须存在";
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_wstr.wasm");
}


// W2: field/slice/find_from 内建分派 (call idx 16/11/15)
TEST(WasmBackend, StringBuiltinsDispatch) {
    const char* src =
        "actor main {\n"
        "  fun ref main() {\n"
        "    var s: String = \"k1;v1\"\n"
        "    print(field(s, 1, \";\"))\n"
        "    print(slice(s, 0, 2))\n"
        "    print(find_from(s, \"v\", 0))\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(wasm_write_program(ast, "/tmp/ponypp_gen_wbi.wasm", TARGET_WASI_P2), 0);
    FILE* rf = fopen("/tmp/ponypp_gen_wbi.wasm", "rb");
    ASSERT_NE(rf, nullptr);
    static unsigned char buf[65536] = {0};
    size_t n = fread(buf, 1, sizeof(buf), rf);
    fclose(rf);
    bool call_field = false, call_slice = false, call_ff = false;
    for (size_t i = 0; i + 1 < n; i++) {
        if (buf[i] == 0x10 && buf[i + 1] == 0x19) call_field = true;    /* call field=25 (b=15) */
        if (buf[i] == 0x10 && buf[i + 1] == 0x14) call_slice = true;    /* call slice=20 (b=15) */
        if (buf[i] == 0x10 && buf[i + 1] == 0x18) call_ff = true;       /* call find_from=24 (b=15) */
    }
    EXPECT_TRUE(call_field) << "field() 必须分派到 field 运行时";
    EXPECT_TRUE(call_slice) << "slice() 必须分派到 slice 运行时";
    EXPECT_TRUE(call_ff) << "find_from() 必须分派到 find_from 运行时";
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_wbi.wasm");
}

TEST(WasmBackend, ClassSystemDispatch) {
    /* W3: 类系统 — 构造器/字段读写/方法分派 字节级校验 */
    const char* src =
        "class Counter {\n"
        "  var count: U32\n"
        "  new create() => {\n"
        "    count = 42\n"
        "  }\n"
        "  fun ref add(v: U32): U32 => {\n"
        "    count = count + v\n"
        "    count\n"
        "  }\n"
        "}\n"
        "actor main {\n"
        "  fun ref main() {\n"
        "    var c = Counter.create()\n"
        "    print(c.add(5))\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(wasm_write_program(ast, "/tmp/ponypp_gen_w3.wasm", TARGET_WASI_P2), 0);
    FILE* rf = fopen("/tmp/ponypp_gen_w3.wasm", "rb");
    ASSERT_NE(rf, nullptr);
    static unsigned char buf[65536] = {0};
    size_t n = fread(buf, 1, sizeof(buf), rf);
    fclose(rf);
    bool call_ctor = false, call_add = false, has_store = false, has_load = false;
    /* Bug#58: fn_base b+21 (b+20=strcmp), wasi-p2(b=15) → 类方法从 36 起 */
    for (size_t i = 0; i + 1 < n; i++) {
        if (buf[i] == 0x10 && buf[i + 1] == 0x24) call_ctor = true;  /* call create=36 (b=15) */
        if (buf[i] == 0x10 && buf[i + 1] == 0x25) call_add = true;   /* call add=37 (b=15) */
        if (buf[i] == 0x36) has_store = true;                        /* i32.store */
        if (buf[i] == 0x28) has_load = true;                         /* i32.load */
    }
    EXPECT_TRUE(call_ctor) << "Counter.create() 必须分派到构造器函数";
    EXPECT_TRUE(call_add) << "c.add(5) 必须分派到类方法函数";
    EXPECT_TRUE(has_store) << "字段赋值必须发射 i32.store";
    EXPECT_TRUE(has_load) << "字段读取必须发射 i32.load";
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_w3.wasm");
}

TEST(WasmBackend, W4BuiltinsAndReturn) {
    /* W4(bug#46): 内建全集 — chr/repl/json 分派 + Bug#51 return 语句发射 */
    const char* src =
        "class Holder {\n"
        "  var name: String\n"
        "  new create() => {\n"
        "    name = \"PonyDB\"\n"
        "  }\n"
        "  fun ref greet(): String => {\n"
        "    return \"Hi,\" + this.name\n"
        "  }\n"
        "}\n"
        "actor main {\n"
        "  fun ref main() {\n"
        "    var h: Holder = Holder()\n"
        "    print(h.greet())\n"
        "    print(str_from_char(65))\n"
        "    print(str_replace_all(\"aXbXc\", \"X\", \"-\"))\n"
        "    print(json_raw_get(\"{\\\"key\\\":\\\"k1\\\"}\", \"key\"))\n"
        "  }\n"
        "}\n";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(wasm_write_program(ast, "/tmp/ponypp_gen_w4.wasm", TARGET_WASI_P2), 0);
    FILE* rf = fopen("/tmp/ponypp_gen_w4.wasm", "rb");
    ASSERT_NE(rf, nullptr);
    static unsigned char buf[65536] = {0};
    size_t n = fread(buf, 1, sizeof(buf), rf);
    fclose(rf);
    /* chr=b+11=26, repl=b+12=27, json=b+13=28 (b=15) */
    bool call_chr = false, call_repl = false, call_json = false;
    bool has_return = false, call_ctor = false, call_greet = false;
    for (size_t i = 0; i + 1 < n; i++) {
        if (buf[i] == 0x10 && buf[i + 1] == 0x1A) call_chr = true;
        if (buf[i] == 0x10 && buf[i + 1] == 0x1B) call_repl = true;
        if (buf[i] == 0x10 && buf[i + 1] == 0x1C) call_json = true;
        if (buf[i] == 0x0f) has_return = true;                       /* return */
        if (buf[i] == 0x10 && buf[i + 1] == 0x24) call_ctor = true;  /* Bug#58: Holder()=36 (b=15) */
        if (buf[i] == 0x10 && buf[i + 1] == 0x25) call_greet = true; /* Bug#58: greet=37 (b=15) */
    }
    EXPECT_TRUE(call_chr) << "str_from_char 必须分派到 rt_chr";
    EXPECT_TRUE(call_repl) << "str_replace_all 必须分派到 rt_repl";
    EXPECT_TRUE(call_json) << "json_raw_get 必须分派到 rt_json";
    EXPECT_TRUE(has_return) << "Bug#51: NODE_EMPTY(data=return) 必须发射 return 指令";
    EXPECT_TRUE(call_ctor) << "Holder() 必须分派到构造器";
    EXPECT_TRUE(call_greet) << "h.greet() 必须分派到类方法";
    ast_node_free(ast);
    std::remove("/tmp/ponypp_gen_w4.wasm");
}
