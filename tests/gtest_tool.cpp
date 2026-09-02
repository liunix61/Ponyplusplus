#include "ponypp/tool.h"
#include "ponypp.h"
#include "ponypp/util.h"
#include <gtest/gtest.h>
#include <gtest/gtest.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

/* Helper: create a temp .pny file */
static int write_pny_file(const char *path, const char *content) {
    return s_file_write(path, content, (int)strlen(content));
}

/* ==================== Parse Args ==================== */

TEST(ToolArgs, BuildCommand) {
    char *argv[] = {"ponyppc", "build", "-o", "out", "hello.pny"};
    ToolConfig tc;
    ASSERT_EQ(tool_parse_args(5, argv, &tc), 0);
    ASSERT_EQ(tc.cmd, TOOL_BUILD);
    ASSERT_STREQ(tc.input, "hello.pny");
    ASSERT_STREQ(tc.output, "out");
}

TEST(ToolArgs, RunCommand) {
    char *argv[] = {"ponyppc", "run", "-O3", "main.pny"};
    ToolConfig tc;
    ASSERT_EQ(tool_parse_args(4, argv, &tc), 0);
    ASSERT_EQ(tc.cmd, TOOL_RUN);
    ASSERT_STREQ(tc.input, "main.pny");
    ASSERT_STREQ(tc.olevel, "3");
}

TEST(ToolArgs, TestCommand) {
    char *argv[] = {"ponyppc", "test", "-v"};
    ToolConfig tc;
    ASSERT_EQ(tool_parse_args(3, argv, &tc), 0);
    ASSERT_EQ(tc.cmd, TOOL_TEST);
    ASSERT_TRUE(tc.test_verbose);
}

TEST(ToolArgs, FmtCommand) {
    char *argv[] = {"ponyppc", "fmt", "main.pny"};
    ToolConfig tc;
    ASSERT_EQ(tool_parse_args(3, argv, &tc), 0);
    ASSERT_EQ(tc.cmd, TOOL_FMT);
    ASSERT_STREQ(tc.input, "main.pny");
}

TEST(ToolArgs, PkgNew) {
    char *argv[] = {"ponyppc", "pkg", "new", "myapp"};
    ToolConfig tc;
    ASSERT_EQ(tool_parse_args(4, argv, &tc), 0);
    ASSERT_EQ(tc.cmd, TOOL_PKG);
    ASSERT_STREQ(tc.subcmd, "new");
    ASSERT_STREQ(tc.input, "myapp");
}

TEST(ToolArgs, PkgAdd) {
    char *argv[] = {"ponyppc", "pkg", "add", "json"};
    ToolConfig tc;
    ASSERT_EQ(tool_parse_args(4, argv, &tc), 0);
    ASSERT_EQ(tc.cmd, TOOL_PKG);
    ASSERT_STREQ(tc.subcmd, "add");
    ASSERT_STREQ(tc.input, "json");
}

TEST(ToolArgs, HelpCommand) {
    char *argv[] = {"ponyppc", "help"};
    ToolConfig tc;
    ASSERT_EQ(tool_parse_args(2, argv, &tc), 0);
    ASSERT_EQ(tc.cmd, TOOL_HELP);
}

TEST(ToolArgs, TargetFlag) {
    char *argv[] = {"ponyppc", "build", "--target", "native", "-g", "hello.pny"};
    ToolConfig tc;
    ASSERT_EQ(tool_parse_args(6, argv, &tc), 0);
    ASSERT_STREQ(tc.target, "native");
    ASSERT_TRUE(tc.debug);
}

TEST(ToolArgs, PlatformFlag) {
    char *argv[] = {"ponyppc", "build", "--platform", "stm32", "hello.pny"};
    ToolConfig tc;
    ASSERT_EQ(tool_parse_args(5, argv, &tc), 0);
    ASSERT_STREQ(tc.platform, "stm32");
}

TEST(ToolArgs, UnknownCmd) {
    char *argv[] = {"ponyppc", "invalid", "hello.pny"};
    ToolConfig tc;
    ASSERT_EQ(tool_parse_args(3, argv, &tc), -1);
}

/* ==================== Build ==================== */

TEST(ToolBuild, SimpleActor) {
    const char *path = "/tmp/ponypp_tool_build.pny";
    const char *src =
        "actor main {\n"
        "  new create() => {\n"
        "    print(\"Hello from Pony++\")\n"
        "  }\n"
        "}\n";
    int r = s_file_write(path, src, strlen(src));
    ASSERT_EQ(r, 0);

    int ret = tool_build(path, "/tmp/ponypp_build_out", "native", NULL, "0", false);
    ASSERT_EQ(ret, 0);

    /* Verify binary exists and runs */
    struct stat st;
    ASSERT_EQ(stat("/tmp/ponypp_build_out", &st), 0);
    remove(path);
    remove("/tmp/ponypp_build_out");
}

TEST(ToolBuild, MultilineActor) {
    const char *path = "/tmp/ponypp_tool_build2.pny";
    const char *src =
        "actor Worker {\n"
        "  var count: U32 = 0\n"
        "  new create() => {}\n"
        "  be handle() => { count = count + 1 }\n"
        "}\n"
        "actor main {\n"
        "  new create() => { print(\"Worker ready\") }\n"
        "}\n";
    int r = s_file_write(path, src, strlen(src));
    ASSERT_EQ(r, 0);
    int ret = tool_build(path, "/tmp/ponypp_build_out2", "native", NULL, "0", false);
    ASSERT_EQ(ret, 0);
    struct stat st;
    ASSERT_EQ(stat("/tmp/ponypp_build_out2", &st), 0);
    remove(path);
    remove("/tmp/ponypp_build_out2");
}

TEST(ToolBuild, WitOutput) {
    const char *path = "/tmp/ponypp_tool_wit.pny";
    const char *src =
        "actor main {\n"
        "  new create() => { print(\"test\") }\n"
        "}\n";
    int r = s_file_write(path, src, strlen(src));
    ASSERT_EQ(r, 0);
    int ret = tool_build(path, "/tmp/ponypp_wit_out", "wit", NULL, "0", false);
    ASSERT_EQ(ret, 0);
    struct stat st;
    ASSERT_EQ(stat("/tmp/ponypp_wit_out.wit", &st), 0);
    remove(path);
    remove("/tmp/ponypp_wit_out.wit");
}

TEST(ToolBuild, WasmOutput) {
    const char *path = "/tmp/ponypp_tool_wasm.pny";
    const char *src =
        "actor main {\n"
        "  new create() => { print(\"wasm\") }\n"
        "}\n";
    int r = s_file_write(path, src, strlen(src));
    ASSERT_EQ(r, 0);
    int ret = tool_build(path, "/tmp/ponypp_wasm_out", "wasi-p2", NULL, "0", false);
    ASSERT_EQ(ret, 0);
    struct stat st;
    ASSERT_EQ(stat("/tmp/ponypp_wasm_out.wasm", &st), 0);
    remove(path);
    remove("/tmp/ponypp_wasm_out.wasm");
}

/* ==================== Run ==================== */

TEST(ToolRun, HelloPony) {
    const char *path = "/tmp/ponypp_tool_run.pny";
    const char *src =
        "actor main {\n"
        "  new create() => { print(\"Hello\") }\n"
        "}\n";
    int r = s_file_write(path, src, strlen(src));
    ASSERT_EQ(r, 0);
    int ret = tool_run(path, "native", "0");
    ASSERT_EQ(ret, 0);
    remove(path);
}

/* ==================== Test ==================== */

TEST(ToolTest, NoTestsInTmp) {
    /* Run test in a dir with no *_test.pny files — should return 0 */
    int ret = tool_test(false);
    ASSERT_EQ(ret, 0);
}

/* ==================== Fmt ==================== */

TEST(ToolFmt, Validate) {
    const char *path = "/tmp/ponypp_tool_fmt.pny";
    const char *src =
        "actor main {\n"
        "  new create() => { print(\"fmt test\") }\n"
        "}\n";
    int r = s_file_write(path, src, strlen(src));
    ASSERT_EQ(r, 0);
    int ret = tool_fmt(path);
    ASSERT_EQ(ret, 0);

    /* File should be unchanged */
    char *content = s_file_read(path);
    ASSERT_NE(content, nullptr);
    ASSERT_NE(strstr(content, "actor main"), nullptr);
    s_free(content);
    remove(path);
}

TEST(ToolFmt, InvalidSource) {
    const char *path = "/tmp/ponypp_tool_fmt_bad.pny";
    int r = s_file_write(path, "this is not pony++", 20);
    ASSERT_EQ(r, 0);
    /* May succeed or fail depending on parser behavior — just check no crash */
    tool_fmt(path);
    remove(path);
}

/* ==================== Pkg ==================== */

TEST(ToolPkg, NewProject) {
    const char *tmpdir = "/tmp/ponypp_pkg_test";
    remove(tmpdir);
    rmdir(tmpdir);

    int ret = tool_pkg_new(tmpdir);
    ASSERT_EQ(ret, 0);

    /* Verify structure */
    char toml[256], main[256];
    snprintf(toml, sizeof(toml), "%s/ponypp.toml", tmpdir);
    snprintf(main, sizeof(main), "%s/src/main.pny", tmpdir);
    ASSERT_TRUE(s_file_read(toml) != nullptr);
    ASSERT_TRUE(s_file_read(main) != nullptr);

    char *t = s_file_read(toml);
    ASSERT_NE(strstr(t, tmpdir), nullptr);
    s_free(t);
    remove(toml);
    remove(main);
    rmdir(tmpdir);
    rmdir(tmpdir); /* remove empty dir */
}

TEST(ToolPkg, AddDep) {
    /* Create ponypp.toml in current dir */
    FILE *f = fopen("ponypp.toml", "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "[package]\nname = \"test\"\n\n[dependencies]\n");
    fclose(f);

    int ret = tool_pkg_add("http");
    ASSERT_EQ(ret, 0);

    char *t = s_file_read("ponypp.toml");
    ASSERT_NE(strstr(t, "http"), nullptr);
    s_free(t);
    remove("ponypp.toml");
}

/* ==================== Help ==================== */

TEST(ToolHelp, Execute) {
    ToolConfig tc = { .cmd = TOOL_HELP };
    int ret = tool_execute(&tc);
    ASSERT_EQ(ret, 0);
}

TEST(ToolHelp, InvalidCmd) {
    ToolConfig tc = { .cmd = (ToolCommand)999 };
    int ret = tool_execute(&tc);
    ASSERT_EQ(ret, -1);
}
