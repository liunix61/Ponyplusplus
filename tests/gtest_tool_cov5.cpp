#include <gtest/gtest.h>
#include <ponypp/tool.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <cstdio>

/* ==================== tool_build 更多组合 ==================== */

TEST(ToolCov5, BuildWasiP2O0) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "wasi-p2", nullptr, "0", false), 0);
}

TEST(ToolCov5, BuildWasiP2O1) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "wasi-p2", nullptr, "1", false), 0);
}

TEST(ToolCov5, BuildWasiP2O2) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "wasi-p2", nullptr, "2", false), 0);
}

TEST(ToolCov5, BuildWasiP2O3) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "wasi-p2", nullptr, "3", false), 0);
}

TEST(ToolCov5, BuildComponentO2) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "component", nullptr, "2", false), 0);
}

TEST(ToolCov5, BuildBrowserO2) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "browser", nullptr, "2", false), 0);
}

TEST(ToolCov5, BuildMCUO2) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "mcu-wasm", nullptr, "2", false), 0);
}

TEST(ToolCov5, BuildNativeO2Debug) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "native", nullptr, "2", true), 0);
}

/* ==================== tool_run 更多组合 ==================== */

TEST(ToolCov5, RunWasiP2O0) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "wasi-p2", "0"), 0);
}

TEST(ToolCov5, RunWasiP2O1) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "wasi-p2", "1"), 0);
}

TEST(ToolCov5, RunWasiP2O2) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "wasi-p2", "2"), 0);
}

TEST(ToolCov5, RunWasiP2O3) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "wasi-p2", "3"), 0);
}

TEST(ToolCov5, RunNativeO0) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "native", "0"), 0);
}

TEST(ToolCov5, RunNativeO1) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "native", "1"), 0);
}

TEST(ToolCov5, RunNativeO2) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "native", "2"), 0);
}

TEST(ToolCov5, RunNativeO3) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "native", "3"), 0);
}

/* ==================== tool_fmt 更多文件 ==================== */

TEST(ToolCov5, FmtFuncWithIf) {
    FILE *f = fopen("/tmp/test_fmt_if_cov5.pny", "w");
    if (f) {
        fprintf(f, "fn main() { if true { print(1); } }\n");
        fclose(f);
    }
    int ret = tool_fmt("/tmp/test_fmt_if_cov5.pny");
    unlink("/tmp/test_fmt_if_cov5.pny");
    (void)ret;
}

TEST(ToolCov5, FmtFuncWithWhile) {
    FILE *f = fopen("/tmp/test_fmt_while_cov5.pny", "w");
    if (f) {
        fprintf(f, "fn main() { while false { print(1); } }\n");
        fclose(f);
    }
    int ret = tool_fmt("/tmp/test_fmt_while_cov5.pny");
    unlink("/tmp/test_fmt_while_cov5.pny");
    (void)ret;
}

TEST(ToolCov5, FmtFuncWithParams) {
    FILE *f = fopen("/tmp/test_fmt_params_cov5.pny", "w");
    if (f) {
        fprintf(f, "fn add(a: i32, b: i32) -> i32 { return a + b; }\n");
        fclose(f);
    }
    int ret = tool_fmt("/tmp/test_fmt_params_cov5.pny");
    unlink("/tmp/test_fmt_params_cov5.pny");
    (void)ret;
}

TEST(ToolCov5, FmtMultipleFuncs) {
    FILE *f = fopen("/tmp/test_fmt_multi_cov5.pny", "w");
    if (f) {
        fprintf(f, "fn main() { }\nfn helper() -> i32 { return 1; }\n");
        fclose(f);
    }
    int ret = tool_fmt("/tmp/test_fmt_multi_cov5.pny");
    unlink("/tmp/test_fmt_multi_cov5.pny");
    (void)ret;
}

/* ==================== tool_pkg_new 更多情况 ==================== */

TEST(ToolCov5, PkgNewTempDir) {
    int ret = tool_pkg_new("/tmp/test_pkg_cov5");
    system("rm -rf /tmp/test_pkg_cov5");
    (void)ret;
}

/* ==================== tool_pkg_add 更多情况 ==================== */

TEST(ToolCov5, PkgAddValidDep) {
    int ret = tool_pkg_add("another_dep_cov5");
    (void)ret;
}

/* ==================== tool_execute 更多命令 ==================== */

TEST(ToolCov5, ExecuteBuildWasiP2O0) {
    ToolConfig tc = {};
    tc.cmd = TOOL_BUILD;
    tc.input = "/nonexistent/test.pny";
    tc.target = "wasi-p2";
    tc.olevel = "0";
    EXPECT_NE(tool_execute(&tc), 0);
}

TEST(ToolCov5, ExecuteBuildWasiP2O3) {
    ToolConfig tc = {};
    tc.cmd = TOOL_BUILD;
    tc.input = "/nonexistent/test.pny";
    tc.target = "wasi-p2";
    tc.olevel = "3";
    EXPECT_NE(tool_execute(&tc), 0);
}

TEST(ToolCov5, ExecuteRunWasiP2O0) {
    ToolConfig tc = {};
    tc.cmd = TOOL_RUN;
    tc.input = "/nonexistent/test.pny";
    tc.target = "wasi-p2";
    tc.olevel = "0";
    EXPECT_NE(tool_execute(&tc), 0);
}

TEST(ToolCov5, ExecuteRunWasiP2O3) {
    ToolConfig tc = {};
    tc.cmd = TOOL_RUN;
    tc.input = "/nonexistent/test.pny";
    tc.target = "wasi-p2";
    tc.olevel = "3";
    EXPECT_NE(tool_execute(&tc), 0);
}

TEST(ToolCov5, ExecuteTestVerbose) {
    ToolConfig tc = {};
    tc.cmd = TOOL_TEST;
    tc.test_verbose = true;
    int ret = tool_execute(&tc);
    (void)ret;
}

TEST(ToolCov5, ExecutePkgNewTemp) {
    ToolConfig tc = {};
    tc.cmd = TOOL_PKG;
    tc.subcmd = "new";
    tc.input = "/tmp/test_pkg_exec_cov5";
    int ret = tool_execute(&tc);
    system("rm -rf /tmp/test_pkg_exec_cov5");
    (void)ret;
}

TEST(ToolCov5, ExecutePkgAddDep) {
    ToolConfig tc = {};
    tc.cmd = TOOL_PKG;
    tc.subcmd = "add";
    tc.input = "some_other_dep_cov5";
    int ret = tool_execute(&tc);
    (void)ret;
}

/* ==================== 解析更多组合 ==================== */

TEST(ToolCov5, ParseBuildAllFlags) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"test.pny", (char*)"-g", (char*)"--ast", (char*)"--pretty", (char*)"-O2", (char*)"--target", (char*)"native", (char*)"--wit-only", (char*)"-o", (char*)"out.wasm"};
    int ret = tool_parse_args(12, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(tc.debug);
    EXPECT_TRUE(tc.ast_dump);
    EXPECT_TRUE(tc.pretty);
    EXPECT_STREQ(tc.olevel, "2");
    EXPECT_STREQ(tc.target, "native");
    EXPECT_TRUE(tc.wit_only);
    EXPECT_STREQ(tc.output, "out.wasm");
}

TEST(ToolCov5, ParseRunWithAllFlags) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"run", (char*)"test.pny", (char*)"--target", (char*)"wasi-p2", (char*)"-O3"};
    int ret = tool_parse_args(6, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_RUN);
    EXPECT_STREQ(tc.target, "wasi-p2");
    EXPECT_STREQ(tc.olevel, "3");
}

TEST(ToolCov5, ParseTestWithVerboseLong) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"test", (char*)"--verbose"};
    int ret = tool_parse_args(3, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(tc.test_verbose);
}

TEST(ToolCov5, ParsePkgNewWithArg) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"pkg", (char*)"new", (char*)"my_new_pkg_cov5"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(tc.subcmd, "new");
    EXPECT_STREQ(tc.input, "my_new_pkg_cov5");
}

TEST(ToolCov5, ParsePkgAddWithArg) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"pkg", (char*)"add", (char*)"my_new_dep_cov5"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(tc.subcmd, "add");
    EXPECT_STREQ(tc.input, "my_new_dep_cov5");
}
