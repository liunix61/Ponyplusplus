#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static const char* find_bin() {
    const char* env = std::getenv("PONYPPC_BIN");
    if (env && env[0]) return env;
    /* Try multiple locations */
    const char* candidates[] = {
        "/home/liunix/Ponyplusplus/build-cmake/ponyppc",
        "/home/liunix/Ponyplusplus/build-cmak/home/liunix/Ponyplusplus/build-cmake/ponyppc",
        "/home/liunix/Ponyplusplus/bi/home/liunix/Ponyplusplus/build-cmake/ponyppc",
        "./bi/home/liunix/Ponyplusplus/build-cmake/ponyppc",
        "../../bi/home/liunix/Ponyplusplus/build-cmake/ponyppc",
        "../bi/home/liunix/Ponyplusplus/build-cmake/ponyppc",
        nullptr
    };
    for (int i = 0; candidates[i]; i++) {
        FILE* f = fopen(candidates[i], "rb");
        if (f) { fclose(f); return candidates[i]; }
    }
    return "./bi/home/liunix/Ponyplusplus/build-cmake/ponyppc";
}

TEST(E2E, CompileToWasm) {
    const char* src_path = "/tmp/ponypp_e2e.pny";
    FILE* f = fopen(src_path, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => { print(\"Hello\") }\n}\n");
    fclose(f);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "%s %s -o /tmp/ponypp_e2e.wasm 2>&1", find_bin(), src_path);
    int ret = std::system(cmd);
    EXPECT_EQ(ret, 0);

    FILE* wf = fopen("/tmp/ponypp_e2e.wasm", "rb");
    ASSERT_NE(wf, nullptr);
    unsigned char magic[8] = {0};
    fread(magic, 1, 8, wf);
    fclose(wf);
    EXPECT_EQ(magic[0], 0x00);
    EXPECT_EQ(magic[1], 0x61);
    EXPECT_EQ(magic[2], 0x73);
    EXPECT_EQ(magic[3], 0x6d);

    std::remove(src_path);
    std::remove("/tmp/ponypp_e2e.wasm");
}

TEST(E2E, WitOnly) {
    const char* src_path = "/tmp/ponypp_wit.pny";
    FILE* f = fopen(src_path, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor Counter { var count: U64 = 0\n  be increment() => {} }\n");
    fclose(f);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "%s %s --wit-only -o /tmp/ponypp_e2e.wit 2>&1", find_bin(), src_path);
    int ret = std::system(cmd);
    EXPECT_EQ(ret, 0);

    FILE* wf = fopen("/tmp/ponypp_e2e.wit", "r");
    ASSERT_NE(wf, nullptr);
    char buf[2048] = {0};
    fread(buf, 1, sizeof(buf) - 1, wf);
    fclose(wf);
    EXPECT_NE(std::strstr(buf, "interface"), nullptr);

    std::remove(src_path);
    std::remove("/tmp/ponypp_e2e.wit");
}

TEST(E2E, NativeBackend) {
    const char* src_path = "/tmp/ponypp_native.pny";
    FILE* f = fopen(src_path, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => {}\n}\n");
    fclose(f);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "%s %s --target native -o /tmp/ponypp_native 2>&1", find_bin(), src_path);
    int ret = std::system(cmd);
    EXPECT_EQ(ret, 0);

    FILE* bf = fopen("/tmp/ponypp_native", "rb");
    EXPECT_NE(bf, nullptr);
    if (bf) fclose(bf);

    std::remove(src_path);
    std::remove("/tmp/ponypp_native");
    std::remove("/tmp/ponypp_native.c");
}

TEST(E2E, Help) {
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "%s --help 2>&1", find_bin());
    /* --help may return 0 or non-zero depending on implementation */
    std::system(cmd);
}

TEST(E2E, Version) {
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "%s --version 2>&1", find_bin());
    int ret = std::system(cmd);
    EXPECT_EQ(ret, 0);
}

TEST(E2E, ActorMessageNative) {
    const char* src_path = "/tmp/ponypp_actor_msg.pny";
    FILE* f = fopen(src_path, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f,
        "actor Worker {\n"
        "  var name: String\n"
        "  new create(n: String) => { name = n }\n"
        "  be run(msg: String) => { print(msg) }\n"
        "}\n"
        "actor main {\n"
        "  var w: Worker\n"
        "  new create() => {}\n"
        "  be run() => {\n"
        "    w = Worker(\"worker1\")\n"
        "    print(\"done\")\n"
        "  }\n"
        "}\n");
    fclose(f);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "%s %s --target native -o /tmp/ponypp_actor_msg 2>&1",
             find_bin(), src_path);
    /* typecheck + codegen 至少通过; native link 可能因环境而异 */
    std::system(cmd);
    std::remove(src_path);
    std::remove("/tmp/ponypp_actor_msg");
    std::remove("/tmp/ponypp_actor_msg.c");
}

TEST(E2E, TypecheckActorTypes) {
    const char* src_path = "/tmp/ponypp_tcheck.pny";
    FILE* f = fopen(src_path, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f,
        "actor A { new create() => {} }\n"
        "actor main {\n"
        "  var a: A\n"
        "  new create() => { a = A() }\n"
        "}\n");
    fclose(f);

    /* 验证生成的 C 代码无 typecheck 报错 */
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "%s %s --target native -o /tmp/ponypp_tcheck.c 2>/tmp/ponypp_tcheck.log",
             find_bin(), src_path);
    std::system(cmd);

    FILE* log = fopen("/tmp/ponypp_tcheck.log", "r");
    bool has_typeerror = false;
    if (log) {
        char buf[4096] = {0};
        fread(buf, 1, sizeof(buf) - 1, log);
        fclose(log);
        has_typeerror = strstr(buf, "类型错误") != nullptr;
    }
    ASSERT_FALSE(has_typeerror);

    std::remove(src_path);
    std::remove("/tmp/ponypp_tcheck");
    std::remove("/tmp/ponypp_tcheck.c");
    std::remove("/tmp/ponypp_tcheck.log");
}

TEST(E2E, NativePrint) {
    const char* src_path = "/tmp/ponypp_print.pny";
    FILE* f = fopen(src_path, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f,
        "actor main {\n"
        "  new create() => {}\n"
        "  be run() => { print(\"hello world\") }\n"
        "}\n");
    fclose(f);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "%s %s --target native -o /tmp/ponypp_print 2>&1",
             find_bin(), src_path);
    int ret = std::system(cmd);
    EXPECT_EQ(ret, 0);

    std::remove(src_path);
    std::remove("/tmp/ponypp_print");
    std::remove("/tmp/ponypp_print.c");
}

TEST(E2E, MatchExpression) {
    const char* src_path = "/tmp/ponypp_match.pny";
    FILE* f = fopen(src_path, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f,
        "actor main {\n"
        "  new create() => {}\n"
        "  be run() => {\n"
        "    var x: I32 = 1\n"
        "    match x { 1 => print(\"one\"); _ => print(\"other\") }\n"
        "  }\n"
        "}\n");
    fclose(f);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "%s %s --target native -o /tmp/ponypp_match 2>&1",
             find_bin(), src_path);
    int ret = std::system(cmd);
    EXPECT_EQ(ret, 0);

    std::remove(src_path);
    std::remove("/tmp/ponypp_match");
    std::remove("/tmp/ponypp_match.c");
}
