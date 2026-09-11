#include <gtest/gtest.h>
#include <ponypp/tool.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <cstdio>

/* ==================== tool_build 不同平台 ==================== */

TEST(ToolCov3, BuildPlatformLinux) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "native", "linux", "2", false), 0);
}

TEST(ToolCov3, BuildPlatformMacOS) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "native", "macos", "2", false), 0);
}

TEST(ToolCov3, BuildPlatformWindows) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "native", "windows", "2", false), 0);
}

/* ==================== tool_build 不同优化级别 ==================== */

TEST(ToolCov3, BuildO0) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "native", nullptr, "0", false), 0);
}

TEST(ToolCov3, BuildO1) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "native", nullptr, "1", false), 0);
}

TEST(ToolCov3, BuildO2) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "native", nullptr, "2", false), 0);
}

TEST(ToolCov3, BuildO3) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "native", nullptr, "3", false), 0);
}

TEST(ToolCov3, BuildOs) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "native", nullptr, "s", false), 0);
}

/* ==================== tool_run 不同优化级别 ==================== */

TEST(ToolCov3, RunO0) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "native", "0"), 0);
}

TEST(ToolCov3, RunO1) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "native", "1"), 0);
}

TEST(ToolCov3, RunO2) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "native", "2"), 0);
}

TEST(ToolCov3, RunO3) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "native", "3"), 0);
}

/* ==================== tool_fmt 不同文件 ==================== */

TEST(ToolCov3, FmtEmptyFile) {
    FILE *f = fopen("/tmp/test_fmt_empty.pny", "w");
    if (f) fclose(f);
    int ret = tool_fmt("/tmp/test_fmt_empty.pny");
    unlink("/tmp/test_fmt_empty.pny");
    (void)ret;
}

TEST(ToolCov3, FmtSimpleFunc) {
    FILE *f = fopen("/tmp/test_fmt_simple.pny", "w");
    if (f) {
        fprintf(f, "fn main() { }\n");
        fclose(f);
    }
    int ret = tool_fmt("/tmp/test_fmt_simple.pny");
    unlink("/tmp/test_fmt_simple.pny");
    (void)ret;
}

TEST(ToolCov3, FmtFuncWithLet) {
    FILE *f = fopen("/tmp/test_fmt_let.pny", "w");
    if (f) {
        fprintf(f, "fn main() { let x: i32 = 1; }\n");
        fclose(f);
    }
    int ret = tool_fmt("/tmp/test_fmt_let.pny");
    unlink("/tmp/test_fmt_let.pny");
    (void)ret;
}

TEST(ToolCov3, FmtFuncWithReturn) {
    FILE *f = fopen("/tmp/test_fmt_return.pny", "w");
    if (f) {
        fprintf(f, "fn main() -> i32 { return 42; }\n");
        fclose(f);
    }
    int ret = tool_fmt("/tmp/test_fmt_return.pny");
    unlink("/tmp/test_fmt_return.pny");
    (void)ret;
}

/* ==================== tool_pkg_new ==================== */

TEST(ToolCov3, PkgNewTempDir) {
    int ret = tool_pkg_new("/tmp/test_pkg_cov3");
    system("rm -rf /tmp/test_pkg_cov3");
    (void)ret;
}

/* ==================== tool_pkg_add ==================== */

TEST(ToolCov3, PkgAddValidDep) {
    int ret = tool_pkg_add("some_dep");
    (void)ret;
}

/* ==================== tool_execute 不同命令 ==================== */

TEST(ToolCov3, ExecuteBuildWasiP2) {
    ToolConfig tc = {};
    tc.cmd = TOOL_BUILD;
    tc.input = "/nonexistent/test.pny";
    tc.target = "wasi-p2";
    tc.olevel = "2";
    EXPECT_NE(tool_execute(&tc), 0);
}

TEST(ToolCov3, ExecuteBuildComponent) {
    ToolConfig tc = {};
    tc.cmd = TOOL_BUILD;
    tc.input = "/nonexistent/test.pny";
    tc.target = "component";
    tc.olevel = "2";
    EXPECT_NE(tool_execute(&tc), 0);
}

TEST(ToolCov3, ExecuteBuildBrowser) {
    ToolConfig tc = {};
    tc.cmd = TOOL_BUILD;
    tc.input = "/nonexistent/test.pny";
    tc.target = "browser";
    tc.olevel = "2";
    EXPECT_NE(tool_execute(&tc), 0);
}

TEST(ToolCov3, ExecuteBuildMCU) {
    ToolConfig tc = {};
    tc.cmd = TOOL_BUILD;
    tc.input = "/nonexistent/test.pny";
    tc.target = "mcu-wasm";
    tc.olevel = "2";
    EXPECT_NE(tool_execute(&tc), 0);
}

TEST(ToolCov3, ExecuteRunWasiP2) {
    ToolConfig tc = {};
    tc.cmd = TOOL_RUN;
    tc.input = "/nonexistent/test.pny";
    tc.target = "wasi-p2";
    tc.olevel = "2";
    EXPECT_NE(tool_execute(&tc), 0);
}

TEST(ToolCov3, ExecuteFmt) {
    ToolConfig tc = {};
    tc.cmd = TOOL_FMT;
    tc.input = "/nonexistent/test.pny";
    EXPECT_NE(tool_execute(&tc), 0);
}

TEST(ToolCov3, ExecuteTest) {
    ToolConfig tc = {};
    tc.cmd = TOOL_TEST;
    tc.test_verbose = false;
    int ret = tool_execute(&tc);
    (void)ret;
}

TEST(ToolCov3, ExecutePkgNew) {
    ToolConfig tc = {};
    tc.cmd = TOOL_PKG;
    tc.subcmd = "new";
    tc.input = "/tmp/test_pkg_exec_cov3";
    int ret = tool_execute(&tc);
    system("rm -rf /tmp/test_pkg_exec_cov3");
    (void)ret;
}

TEST(ToolCov3, ExecutePkgAdd) {
    ToolConfig tc = {};
    tc.cmd = TOOL_PKG;
    tc.subcmd = "add";
    tc.input = "some_dep";
    int ret = tool_execute(&tc);
    (void)ret;
}

TEST(ToolCov3, ExecuteDocs) {
    ToolConfig tc = {};
    tc.cmd = TOOL_DOCS;
    int ret = tool_execute(&tc);
    (void)ret;
}

TEST(ToolCov3, ExecuteHelp) {
    ToolConfig tc = {};
    tc.cmd = TOOL_HELP;
    EXPECT_EQ(tool_execute(&tc), 0);
}

/* ==================== 解析边界情况 ==================== */

TEST(ToolCov3, ParseBuildWithOutput) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"test.pny", (char*)"-o", (char*)"out.wasm"};
    int ret = tool_parse_args(5, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(tc.output, "out.wasm");
}

TEST(ToolCov3, ParseBuildWithTarget) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"test.pny", (char*)"--target", (char*)"wasi-p2"};
    int ret = tool_parse_args(5, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(tc.target, "wasi-p2");
}

TEST(ToolCov3, ParseBuildWithOlevel) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"test.pny", (char*)"-O3"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(tc.olevel, "3");
}

TEST(ToolCov3, ParseRunWithTarget) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"run", (char*)"test.pny", (char*)"--target", (char*)"native"};
    int ret = tool_parse_args(5, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(tc.target, "native");
}

TEST(ToolCov3, ParseTestVerbose) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"test", (char*)"--verbose"};
    int ret = tool_parse_args(3, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(tc.test_verbose);
}

TEST(ToolCov3, ParsePkgNewCmd) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"pkg", (char*)"new", (char*)"my_pkg"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(tc.subcmd, "new");
}

TEST(ToolCov3, ParsePkgAddCmd) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"pkg", (char*)"add", (char*)"my_dep"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(tc.subcmd, "add");
}
