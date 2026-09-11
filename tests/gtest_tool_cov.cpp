#include <gtest/gtest.h>
#include <ponypp/tool.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <cstdio>

/* ==================== tool_build ==================== */

TEST(ToolCov, BuildNullInput) {
    EXPECT_NE(tool_build(nullptr, nullptr, nullptr, nullptr, nullptr, false), 0);
}

TEST(ToolCov, BuildNonexistentFile) {
    EXPECT_NE(tool_build("/nonexistent/test.pny", nullptr, "native", nullptr, "2", false), 0);
}

/* ==================== tool_run ==================== */

TEST(ToolCov, RunNullInput) {
    EXPECT_NE(tool_run(nullptr, nullptr, nullptr), 0);
}

TEST(ToolCov, RunNonexistentFile) {
    EXPECT_NE(tool_run("/nonexistent/test.pny", "native", "2"), 0);
}

/* ==================== tool_test ==================== */

TEST(ToolCov, TestVerbose) {
    /* 在项目目录运行 */
    int ret = tool_test(true);
    /* 可能成功或失败, 但不崩溃 */
    (void)ret;
}

TEST(ToolCov, TestQuiet) {
    int ret = tool_test(false);
    (void)ret;
}

/* ==================== tool_fmt ==================== */

TEST(ToolCov, FmtNullInput) {
    EXPECT_NE(tool_fmt(nullptr), 0);
}

TEST(ToolCov, FmtNonexistentFile) {
    EXPECT_NE(tool_fmt("/nonexistent/test.pny"), 0);
}

/* ==================== tool_pkg ==================== */

TEST(ToolCov, PkgNewNullName) {
    EXPECT_NE(tool_pkg_new(nullptr), 0);
}

TEST(ToolCov, PkgAddNullDep) {
    EXPECT_NE(tool_pkg_add(nullptr), 0);
}

/* ==================== tool_execute ==================== */

TEST(ToolCov, ExecuteBuildNonexistent) {
    ToolConfig tc = {};
    tc.cmd = TOOL_BUILD;
    tc.input = "/nonexistent/test.pny";
    tc.target = "native";
    tc.olevel = "2";
    EXPECT_NE(tool_execute(&tc), 0);
}

TEST(ToolCov, ExecuteRunNonexistent) {
    ToolConfig tc = {};
    tc.cmd = TOOL_RUN;
    tc.input = "/nonexistent/test.pny";
    tc.target = "native";
    tc.olevel = "2";
    EXPECT_NE(tool_execute(&tc), 0);
}

TEST(ToolCov, ExecuteFmtNonexistent) {
    ToolConfig tc = {};
    tc.cmd = TOOL_FMT;
    tc.input = "/nonexistent/test.pny";
    EXPECT_NE(tool_execute(&tc), 0);
}

TEST(ToolCov, ExecutePkgNew) {
    ToolConfig tc = {};
    tc.cmd = TOOL_PKG;
    tc.subcmd = "new";
    tc.input = "/tmp/test_pkg_12345";
    int ret = tool_execute(&tc);
    /* 清理 */
    system("rm -rf /tmp/test_pkg_12345");
    (void)ret;
}

TEST(ToolCov, ExecuteHelp) {
    ToolConfig tc = {};
    tc.cmd = TOOL_HELP;
    EXPECT_EQ(tool_execute(&tc), 0);
}

/* ==================== 解析更多参数组合 ==================== */

TEST(ToolCov, ParseBuildMultipleFlags) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"test.pny", (char*)"-g", (char*)"--ast", (char*)"--pretty", (char*)"-O2"};
    int ret = tool_parse_args(7, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(tc.debug);
    EXPECT_TRUE(tc.ast_dump);
    EXPECT_TRUE(tc.pretty);
    EXPECT_STREQ(tc.olevel, "2");
}

TEST(ToolCov, ParseRunWithOptions) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"run", (char*)"test.pny", (char*)"--target", (char*)"native", (char*)"-O3"};
    int ret = tool_parse_args(6, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_RUN);
    EXPECT_STREQ(tc.target, "native");
    EXPECT_STREQ(tc.olevel, "3");
}

TEST(ToolCov, ParseDocs) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"docs"};
    int ret = tool_parse_args(2, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_DOCS);
}

TEST(ToolCov, ParseRepl) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"repl"};
    int ret = tool_parse_args(2, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_REPL);
}

TEST(ToolCov, ParseBootstrap) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"bootstrap"};
    int ret = tool_parse_args(2, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_BOOTSTRAP);
}

TEST(ToolCov, ParseShortHelp) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"-h"};
    int ret = tool_parse_args(2, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_HELP);
}

TEST(ToolCov, ParseLongHelp) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"--help"};
    int ret = tool_parse_args(2, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_HELP);
}

TEST(ToolCov, ParseWitOnly) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"test.pny", (char*)"--wit-only"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(tc.wit_only);
}

TEST(ToolCov, ParseVerboseLong) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"test", (char*)"--verbose"};
    int ret = tool_parse_args(3, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(tc.test_verbose);
}

TEST(ToolCov, ParseOutputShort) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"test.pny", (char*)"-oout.wasm"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(tc.output, "out.wasm");
}
