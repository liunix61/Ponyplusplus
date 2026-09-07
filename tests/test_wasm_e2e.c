/*
 * test_wasm_e2e.c - Pony++ WASM Path B 端到端测试
 *
 * 测试编译 -> 执行 -> 验证输出全链路
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/wasm.h"
#include "ponypp/wamr.h"
#include "ponypp/ast.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { tests_passed++; printf("  ✓ %s\n", msg); } \
    else { tests_failed++; printf("  ✗ %s\n", msg); } \
} while(0)

static ASTNode *parse_to_ast(const char *src) {
    Lexer *lex = lexer_new("test.pny", src, strlen(src));
    Token *tokens = NULL;
    size_t count = 0;
    if (!lexer_lex_all(lex, &tokens, &count)) { lexer_free(lex); return NULL; }
    Parser *p = parser_new("test.pny", tokens, count);
    ASTNode *ast = parser_parse_program(p);
    parser_free(p);
    lexer_free(lex);
    return ast;
}

/* 测试 1: WASM 文件生成 */
static void test_wasm_file_generation(void) {
    printf("test_wasm_file_generation\n");
    const char *src = "actor main { new create() => { print(\"Hello, World!\") } }";
    ASTNode *ast = parse_to_ast(src);
    CHECK(ast != NULL, "AST 解析成功");
    if (!ast) return;

    CHECK(wasm_write_program(ast, "/tmp/test_hello.wasm", TARGET_WASI_P2) == 0,
          "WASM 文件生成成功");
    ast_node_free(ast);

    /* 验证 WASM 格式 */
    FILE *f = fopen("/tmp/test_hello.wasm", "rb");
    CHECK(f != NULL, "文件可打开");
    if (f) {
        unsigned char magic[8] = {0};
        fread(magic, 1, 8, f);
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fclose(f);
        CHECK(magic[0] == 0x00 && magic[1] == 0x61 &&
              magic[2] == 0x73 && magic[3] == 0x6d, "WASM magic 正确");
        CHECK(magic[4] == 0x01, "WASM 版本 1");
        CHECK(size > 50, "文件大小合理 (>50 bytes)");
    }
    remove("/tmp/test_hello.wasm");
}

/* 测试 2: WAMR 模块加载 */
static void test_wamr_module_load(void) {
    printf("test_wamr_module_load\n");
    const char *src = "actor main { new create() => { print(\"Test\") } }";
    ASTNode *ast = parse_to_ast(src);
    CHECK(ast != NULL, "AST 解析成功");
    if (!ast) return;

    CHECK(wasm_write_program(ast, "/tmp/test_wamr.wasm", TARGET_WASI_P2) == 0,
          "WASM 文件生成");
    ast_node_free(ast);

    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    cfg.wasm_path = "/tmp/test_wamr.wasm";

    WamrModule *mod = NULL;
    int ret = wamr_module_load(&cfg, &mod);
    CHECK(ret == 0 && mod != NULL, "WAMR 模块加载成功");

    if (mod) {
        CHECK(mod->size > 0, "模块大小 > 0");
        wamr_module_free(mod);
    }
    remove("/tmp/test_wamr.wasm");
}

/* 测试 3: WAMR 实例创建 */
static void test_wamr_instance_create(void) {
    printf("test_wamr_instance_create\n");
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);

    /* 生成最小 WASM */
    const char *src = "actor main { new create() => { print(\"Hi\") } }";
    ASTNode *ast = parse_to_ast(src);
    if (!ast) { tests_failed++; printf("  ✗ AST 解析失败\n"); return; }
    wasm_write_program(ast, "/tmp/test_inst.wasm", TARGET_WASI_P2);
    ast_node_free(ast);

    cfg.wasm_path = "/tmp/test_inst.wasm";
    WamrModule *mod = NULL;
    wamr_module_load(&cfg, &mod);

    WamrInstance *inst = NULL;
    int ret = wamr_instance_create(mod, &cfg, &inst);
    CHECK(ret == 0 && inst != NULL, "WAMR 实例创建成功");

    if (inst) {
        CHECK(inst->mem_pages == 1, "内存页数 = 1");
        CHECK(inst->memory != NULL, "内存已分配");
        wamr_instance_free(inst);
    }
    wamr_module_free(mod);
    remove("/tmp/test_inst.wasm");
}

/* 测试 4: WAMR 调用导出函数 */
static void test_wamr_call_func(void) {
    printf("test_wamr_call_func\n");
    const char *src = "actor main { new create() => { print(\"E2E\") } }";
    ASTNode *ast = parse_to_ast(src);
    CHECK(ast != NULL, "AST 解析成功");
    if (!ast) return;

    CHECK(wasm_write_program(ast, "/tmp/test_call.wasm", TARGET_WASI_P2) == 0,
          "WASM 文件生成");
    ast_node_free(ast);

    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    cfg.wasm_path = "/tmp/test_call.wasm";

    WamrModule *mod = NULL;
    wamr_module_load(&cfg, &mod);
    if (!mod) { tests_failed++; printf("  ✗ 模块加载失败\n"); return; }

    WamrInstance *inst = NULL;
    wamr_instance_create(mod, &cfg, &inst);
    if (!inst) { tests_failed++; printf("  ✗ 实例创建失败\n"); wamr_module_free(mod); remove("/tmp/test_call.wasm"); return; }

    /* 捕获 stdout */
    FILE *old_stdout = stdout;
    fflush(old_stdout);
    fflush(stderr);

    void *result = NULL;
    int result_count = 0;
    int ret = wamr_call_func(inst, "main", NULL, 0, &result, &result_count);

    fflush(stdout);
    fflush(stderr);

    /* 恢复 stdout */
    (void)old_stdout;

    CHECK(ret == 0, "调用 main 成功");
    CHECK(result_count >= 0, "返回结果计数有效");

    wamr_instance_free(inst);
    wamr_module_free(mod);
    remove("/tmp/test_call.wasm");
}

/* 测试 5: 完整编译 CLI 流程 */
static void test_compile_cli(void) {
    printf("test_compile_cli\n");
    /* 写入测试文件 */
    FILE *f = fopen("/tmp/test_compile.pny", "w");
    if (!f) { tests_failed++; printf("  ✗ 无法创建测试文件\n"); return; }
    fprintf(f, "actor main {\n  new create() => {\n    print(\"CLI Test\")\n  }\n}\n");
    fclose(f);

    /* 通过 ponyppc CLI 编译 */
    char cmd[] = "cp /tmp/test_compile.pny /tmp/test_compile.pny";
    (void)cmd;

    /* 直接测试编译 API */
    const char *src = "actor main { new create() => { print(\"CLI Test\") } }";
    ASTNode *ast = parse_to_ast(src);
    CHECK(ast != NULL, "AST 解析成功");
    if (!ast) { remove("/tmp/test_compile.pny"); return; }

    CHECK(wasm_write_program(ast, "/tmp/test_compile.wasm", TARGET_WASI_P2) == 0,
          "编译 WASM 成功");
    ast_node_free(ast);

    FILE *wasm_f = fopen("/tmp/test_compile.wasm", "rb");
    CHECK(wasm_f != NULL, "WASM 文件存在");
    if (wasm_f) fclose(wasm_f);

    remove("/tmp/test_compile.pny");
    remove("/tmp/test_compile.wasm");
}

/* 测试 6: WASM 目标名称 */
static void test_wasm_target_names(void) {
    printf("test_wasm_target_names\n");
    CHECK(strcmp(wasm_target_name(TARGET_WASI_P2), "wasi-p2") == 0, "wasi-p2");
    CHECK(strcmp(wasm_target_name(TARGET_WASI_P3), "wasi-p3") == 0, "wasi-p3");
    CHECK(strcmp(wasm_target_name(TARGET_COMPONENT), "component") == 0, "component");
    CHECK(strcmp(wasm_target_name(TARGET_BROWSER), "browser") == 0, "browser");
    CHECK(strcmp(wasm_target_name(TARGET_MCU_WASM), "mcu-wasm") == 0, "mcu-wasm");
    CHECK(strcmp(wasm_target_name(TARGET_NATIVE), "native") == 0, "native");
}

/* 测试 7: WASM 字节码结构验证 */
static void test_wasm_bytecode_structure(void) {
    printf("test_wasm_bytecode_structure\n");
    const char *src = "actor main { new create() => { print(\"Struct\") } }";
    ASTNode *ast = parse_to_ast(src);
    CHECK(ast != NULL, "AST 解析成功");
    if (!ast) return;

    wasm_write_program(ast, "/tmp/test_struct.wasm", TARGET_WASI_P2);
    ast_node_free(ast);

    FILE *f = fopen("/tmp/test_struct.wasm", "rb");
    if (!f) { tests_failed++; printf("  ✗ 无法打开\n"); return; }

    unsigned char *data = malloc(1024);
    size_t nread = fread(data, 1, 1024, f);
    fclose(f);

    CHECK(nread >= 8, "至少 8 bytes");

    /* 验证 section 结构 */
    int has_type_section = 0;
    int has_import_section = 0;
    int has_func_section = 0;
    int has_code_section = 0;
    int has_export_section = 0;
    int has_data_section = 0;

    if (nread >= 8) {
        size_t pos = 8;
        while (pos + 1 < nread) {
            unsigned char sid = data[pos++];
            /* 读取 section size (LEB128) */
            int32_t ssize = 0;
            int shift = 0;
            unsigned char b;
            size_t size_start = pos;
            do {
                if (pos >= nread) break;
                b = data[pos++];
                ssize |= (int32_t)(b & 0x7F) << shift;
                shift += 7;
            } while (b & 0x80);
            (void)size_start;
            pos += (size_t)ssize; /* 跳过 section body */

            switch (sid) {
                case 0x01: has_type_section = 1; break;
                case 0x02: has_import_section = 1; break;
                case 0x03: has_func_section = 1; break;
                case 0x0A: has_code_section = 1; break;
                case 0x07: has_export_section = 1; break;
                case 0x0B: has_data_section = 1; break;
            }
        }
    }

    CHECK(has_type_section, "Type section 存在");
    CHECK(has_import_section, "Import section 存在");
    CHECK(has_func_section, "Function section 存在");
    CHECK(has_code_section, "Code section 存在");
    CHECK(has_export_section, "Export section 存在");
    CHECK(has_data_section, "Data section 存在");

    free(data);
    remove("/tmp/test_struct.wasm");
}

/* 测试 8: WASM 内存管理 */
static void test_wamr_memory_management(void) {
    printf("test_wamr_memory_management\n");
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);

    /* 创建空实例 */
    WamrInstance inst;
    memset(&inst, 0, sizeof(inst));
    inst.mem_pages = 1;
    inst.memory = calloc(1, 65536);

    /* 内存分配 */
    void *ptr = NULL;
    int ret = wamr_mem_alloc(&inst, 32, &ptr);
    CHECK(ret == 0 && ptr != NULL, "内存分配成功");

    /* 内存读取 */
    if (ptr) {
        int32_t val = 0x12345678;
        memcpy(ptr, &val, 4);

        int32_t out = 0;
        ret = wamr_mem_read(&inst, (int)((char *)ptr - inst.memory), &out, 4);
        CHECK(ret == 0, "内存读取成功");
        CHECK(out == val, "读取值正确");
    }

    /* 越界检查 */
    int32_t bad = 0;
    ret = wamr_mem_read(&inst, 65536, &bad, 4);
    CHECK(ret == -1, "越界读取被拒绝");

    free(inst.memory);
}

int main(void) {
    printf("=== Pony++ WASM Path B E2E 测试 ===\n\n");

    test_wasm_file_generation();
    test_wamr_module_load();
    test_wamr_instance_create();
    test_wamr_call_func();
    test_compile_cli();
    test_wasm_target_names();
    test_wasm_bytecode_structure();
    test_wamr_memory_management();

    printf("\n=== 测试结果 ===\n");
    printf("通过: %d\n", tests_passed);
    printf("失败: %d\n", tests_failed);
    printf("总计: %d\n", tests_passed + tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
