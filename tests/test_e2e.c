/*
 * test_e2e.c - Pony++ 端到端测试
 *
 * 验证编译器可运行并生成有效输出文件
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { tests_passed++; printf("  \342\234\223 %s\n", msg); } \
    else { tests_failed++; printf("  \342\234\224 %s\n", msg); } \
} while(0)

static int compile_to_wasm(void) {
    printf("test_compile_to_wasm\n");
    const char *src_path = "/tmp/ponypp_e2e.pny";
    FILE *f = fopen(src_path, "w");
    CHECK(f != NULL, "写源文件");
    if (f) {
        fputs("actor main {\n  new create() => { print(\"Hello\") }\n}\n", f);
        fclose(f);
    }

    char cmd[1024];
    const char *bin = getenv("PONYPPC_BIN");
    if (!bin) bin = "./bin/ponyppc";
    snprintf(cmd, sizeof(cmd), "%s %s -o /tmp/ponypp_e2e.wasm 2>&1", bin, src_path);

    int ret = system(cmd);
    CHECK(ret == 0, "编译器退出码为 0");

    FILE *wf = fopen("/tmp/ponypp_e2e.wasm", "rb");
    CHECK(wf != NULL, "Wasm 文件存在");
    if (wf) {
        unsigned char magic[8] = {0};
        fread(magic, 1, 8, wf);
        fclose(wf);
        CHECK(magic[0] == 0x00 && magic[1] == 0x61 && magic[2] == 0x73 && magic[3] == 0x6d,
              "Wasm magic 正确");
    }
    remove(src_path);
    remove("/tmp/ponypp_e2e.wasm");
    return tests_failed == 0;
}

static int compile_wit_only(void) {
    printf("test_compile_wit_only\n");
    const char *src_path = "/tmp/ponypp_wit.pny";
    FILE *f = fopen(src_path, "w");
    CHECK(f != NULL, "写源文件");
    if (f) {
        fputs("actor Counter { var count: U64 = 0\n  be increment() => {} }\n", f);
        fclose(f);
    }

    char cmd[1024];
    const char *bin = getenv("PONYPPC_BIN");
    if (!bin) bin = "./bin/ponyppc";
    snprintf(cmd, sizeof(cmd), "%s %s --wit-only -o /tmp/ponypp_e2e.wit 2>&1", bin, src_path);

    int ret = system(cmd);
    CHECK(ret == 0, "编译器退出码为 0");

    FILE *wf = fopen("/tmp/ponypp_e2e.wit", "r");
    CHECK(wf != NULL, "WIT 文件存在");
    if (wf) {
        char buf[2048] = {0};
        fread(buf, 1, sizeof(buf) - 1, wf);
        fclose(wf);
        CHECK(strstr(buf, "interface") != NULL, "WIT 含 interface");
    }
    remove(src_path);
    remove("/tmp/ponypp_e2e.wit");
    return tests_failed == 0;
}

int main(void) {
    printf("=== Pony++ 端到端测试 ===\n\n");
    compile_to_wasm();
    compile_wit_only();
    printf("\n=== 测试结果 ===\n");
    printf("通过: %d\n", tests_passed);
    printf("失败: %d\n", tests_failed);
    printf("总计: %d\n", tests_passed + tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
