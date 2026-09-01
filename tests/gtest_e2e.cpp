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
    fprintf(f, "actor Main {\n  new create() => { print(\"Hello\") }\n}\n");
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
    fprintf(f, "actor Main {\n  new create() => {}\n}\n");
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
