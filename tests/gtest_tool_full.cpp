#include <gtest/gtest.h>
#include <ponypp/tool.h>
#include <cstring>
#include <cstdlib>

/* ==================== tool_parse_args ==================== */

TEST(ToolFull, ParseBuild) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"hello.pny"};
    int ret = tool_parse_args(3, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_BUILD);
    EXPECT_STREQ(tc.input, "hello.pny");
}

TEST(ToolFull, ParseRun) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"run", (char*)"test.pny"};
    int ret = tool_parse_args(3, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_RUN);
    EXPECT_STREQ(tc.input, "test.pny");
}

TEST(ToolFull, ParseTest) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"test"};
    int ret = tool_parse_args(2, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_TEST);
}

TEST(ToolFull, ParseHelp) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"help"};
    int ret = tool_parse_args(2, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_HELP);
}

TEST(ToolFull, ParseFmt) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"fmt", (char*)"style.pny"};
    int ret = tool_parse_args(3, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_FMT);
    EXPECT_STREQ(tc.input, "style.pny");
}

TEST(ToolFull, ParsePkgNew) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"pkg", (char*)"new", (char*)"mylib"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_PKG);
    EXPECT_STREQ(tc.subcmd, "new");
}

TEST(ToolFull, ParsePkgAdd) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"pkg", (char*)"add", (char*)"dep"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_PKG);
    EXPECT_STREQ(tc.subcmd, "add");
}

TEST(ToolFull, ParseBuildWithOutput) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"hello.pny", (char*)"-o", (char*)"out.wasm"};
    int ret = tool_parse_args(5, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(tc.cmd, TOOL_BUILD);
    EXPECT_STREQ(tc.output, "out.wasm");
}

TEST(ToolFull, ParseBuildWithTargetSpace) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"hello.pny", (char*)"--target", (char*)"wasm"};
    int ret = tool_parse_args(5, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(tc.target, "wasm");
}

TEST(ToolFull, ParseBuildWithPlatformSpace) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"hello.pny", (char*)"--platform", (char*)"stm32"};
    int ret = tool_parse_args(5, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(tc.platform, "stm32");
}

TEST(ToolFull, ParseBuildWithOptLevelAttached) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"hello.pny", (char*)"-O3"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(tc.olevel, "3");
}

TEST(ToolFull, ParseBuildWithDebug) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"hello.pny", (char*)"-g"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(tc.debug);
}

TEST(ToolFull, ParseBuildWithAstDump) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"hello.pny", (char*)"--ast"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(tc.ast_dump);
}

TEST(ToolFull, ParseBuildWithPretty) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"build", (char*)"hello.pny", (char*)"--pretty"};
    int ret = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(tc.pretty);
}

TEST(ToolFull, ParseTestVerbose) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"test", (char*)"-v"};
    int ret = tool_parse_args(3, argv, &tc);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(tc.test_verbose);
}

TEST(ToolFull, ParseNoArgs) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc"};
    int ret = tool_parse_args(1, argv, &tc);
    EXPECT_NE(ret, 0);
}

TEST(ToolFull, ParseUnknownCommand) {
    ToolConfig tc = {};
    char *argv[] = {(char*)"ponyppc", (char*)"unknown"};
    int ret = tool_parse_args(2, argv, &tc);
    EXPECT_NE(ret, 0);
}





/* ==================== tool_execute ==================== */



TEST(ToolFull, ExecuteHelp) {
    ToolConfig tc = {};
    tc.cmd = TOOL_HELP;
    /* help 应该返回 0 */
    EXPECT_EQ(tool_execute(&tc), 0);
}
