#include <gtest/gtest.h>
#include <ponypp/tool.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <cstdio>

/* ==================== tool_bootstrap ==================== */



/* ==================== tool_repl ==================== */

TEST(ToolCov2, Repl) {
    /* tool_repl 会启动 REPL */
    /* 由于需要交互输入, 只测试不崩溃 */
    /* 实际测试中跳过 */
}

/* ==================== 解析边界情况 ==================== */

TEST(ToolCov2, ParseSingleArg) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc"};
    int ret = tool_parse_args(1, argv, &tc);
    EXPECT_EQ(ret, -1);
}

TEST(ToolCov2, ParseEmptyArgs) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc"};
    int ret = tool_parse_args(0, argv, &tc);
    /* 可能返回错误或默认 help */
    (void)ret;
}

TEST(ToolCov2, ParseBuildWithAllFlags) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"test.pny", (char*)"-g", (char*)"--ast", (char*)"--pretty", (char*)"-O3", (char*)"--target", (char*)"native", (char*)"--wit-only", (char*)"-o", (char*)"out.wasm"};
    int ret = tool_parse_args(12, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(tc.debug);
    EXPECT_TRUE(tc.ast_dump);
    EXPECT_TRUE(tc.pretty);
    EXPECT_STREQ(tc.olevel, "3");
    EXPECT_STREQ(tc.target, "native");
    EXPECT_TRUE(tc.wit_only);
    EXPECT_STREQ(tc.output, "out.wasm");
}

TEST(ToolCov2, ParseTestWithVerbose) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"test", (char*)"-v"};
    int ret = tool_parse_args(3, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_TEST);
    EXPECT_TRUE(tc.test_verbose);
}

TEST(ToolCov2, ParsePkgNew) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"pkg", (char*)"new", (char*)"my_package"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_PKG);
    EXPECT_STREQ(tc.subcmd, "new");
    EXPECT_STREQ(tc.input, "my_package");
}

TEST(ToolCov2, ParsePkgAdd) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"pkg", (char*)"add", (char*)"some_dep"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_PKG);
    EXPECT_STREQ(tc.subcmd, "add");
    EXPECT_STREQ(tc.input, "some_dep");
}

TEST(ToolCov2, ParsePkgUnknownSubcmd) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"pkg", (char*)"unknown"};
    int ret = tool_parse_args(3, argv, &tc);
    /* 可能返回错误 */
    (void)ret;
}

TEST(ToolCov2, ParseUnknownCommand) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"unknown_cmd"};
    int ret = tool_parse_args(2, argv, &tc);
    /* 可能返回错误或默认 help */
    (void)ret;
}

/* ==================== tool_execute 不同命令 ==================== */

TEST(ToolCov2, ExecuteTest) {
    ToolConfig tc = {};
    tc.cmd = TOOL_TEST;
    tc.test_verbose = false;
    int ret = tool_execute(&tc);
    /* 可能成功或失败 */
    (void)ret;
}

TEST(ToolCov2, ExecutePkgAdd) {
    ToolConfig tc = {};
    tc.cmd = TOOL_PKG;
    tc.subcmd = "add";
    tc.input = "some_dep";
    int ret = tool_execute(&tc);
    (void)ret;
}

TEST(ToolCov2, ExecutePkgNewTemp) {
    ToolConfig tc = {};
    tc.cmd = TOOL_PKG;
    tc.subcmd = "new";
    tc.input = "/tmp/test_pkg_cov2";
    int ret = tool_execute(&tc);
    /* 清理 */
    system("rm -rf /tmp/test_pkg_cov2");
    (void)ret;
}

TEST(ToolCov2, ExecuteDocs) {
    ToolConfig tc = {};
    tc.cmd = TOOL_DOCS;
    int ret = tool_execute(&tc);
    (void)ret;
}

/* ==================== tool_build 不同目标 ==================== */

TEST(ToolCov2, BuildTargetWasiP2) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "wasi-p2", nullptr, "2", false), 0);
}

TEST(ToolCov2, BuildTargetComponent) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "component", nullptr, "2", false), 0);
}

TEST(ToolCov2, BuildTargetBrowser) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "browser", nullptr, "2", false), 0);
}

TEST(ToolCov2, BuildTargetMCU) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "mcu-wasm", nullptr, "2", false), 0);
}

TEST(ToolCov2, BuildTargetNative) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "native", nullptr, "2", false), 0);
}

TEST(ToolCov2, BuildWithDebug) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "native", nullptr, "2", true), 0);
}

/* ==================== tool_run 不同目标 ==================== */

TEST(ToolCov2, RunTargetWasiP2) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "wasi-p2", "2"), 0);
}

TEST(ToolCov2, RunTargetNative) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "native", "2"), 0);
}

/* ==================== tool_fmt ==================== */

TEST(ToolCov2, FmtEmptyFile) {
    /* 创建空文件 */
    FILE *f = fopen("/tmp/test_empty.pny", "w");
    if (f) fclose(f);
    int ret = tool_fmt("/tmp/test_empty.pny");
    unlink("/tmp/test_empty.pny");
    (void)ret;
}

TEST(ToolCov2, FmtSimpleFile) {
    FILE *f = fopen("/tmp/test_simple.pny", "w");
    if (f) {
        fprintf(f, "fn main() { }\n");
        fclose(f);
    }
    int ret = tool_fmt("/tmp/test_simple.pny");
    unlink("/tmp/test_simple.pny");
    (void)ret;
}
