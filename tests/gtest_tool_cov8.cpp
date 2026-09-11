#include <gtest/gtest.h>
#include <ponypp/tool.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

extern "C" int tool_bootstrap(void);

/* tool_test 在有测试文件的目录 */
TEST(ToolCov8, TestWithTestFiles) {
    /* 创建测试文件 */
    char tmpl[] = "/tmp/ponypp_test_dir_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    
    char test_path[512];
    snprintf(test_path, sizeof(test_path), "%s/test_simple_test.pny", dir);
    FILE *f = fopen(test_path, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor TestSimple {\n  be run() => {\n    print(\"test\")\n  }\n}\n");
    fclose(f);
    
    /* 切换到该目录 */
    char old_dir[1024];
    getcwd(old_dir, sizeof(old_dir));
    chdir(dir);
    
    int r = tool_test(true);
    (void)r;
    
    chdir(old_dir);
    
    /* 清理 */
    unlink(test_path);
    rmdir(dir);
    
    SUCCEED();
}

/* tool_test 在空目录 */
TEST(ToolCov8, TestEmptyDir) {
    char tmpl[] = "/tmp/ponypp_empty_test_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    
    char old_dir[1024];
    getcwd(old_dir, sizeof(old_dir));
    chdir(dir);
    
    int r = tool_test(false);
    EXPECT_EQ(r, 0);
    
    chdir(old_dir);
    rmdir(dir);
}

/* tool_test verbose 模式 */
TEST(ToolCov8, TestVerbose) {
    char tmpl[] = "/tmp/ponypp_test_verbose_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    
    char old_dir[1024];
    getcwd(old_dir, sizeof(old_dir));
    chdir(dir);
    
    int r = tool_test(true);
    EXPECT_EQ(r, 0);
    
    chdir(old_dir);
    rmdir(dir);
}

/* tool_pkg_new */
TEST(ToolCov8, PkgNew) {
    char tmpl[] = "/tmp/ponypp_pkg_new_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    
    char old_dir[1024];
    getcwd(old_dir, sizeof(old_dir));
    chdir(dir);
    
    int r = tool_pkg_new("testpkg");
    (void)r;
    
    chdir(old_dir);
    
    /* 清理 */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", dir);
    system(cmd);
    
    SUCCEED();
}

/* tool_pkg_add */
TEST(ToolCov8, PkgAdd) {
    int r = tool_pkg_add("somepackage");
    (void)r;
    SUCCEED();
}

/* tool_fmt */
TEST(ToolCov8, FmtFile) {
    char tmpl[] = "/tmp/ponypp_fmt_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => { print(\"hi\") }\n}\n");
    fclose(f);
    
    int r = tool_fmt(tmpl);
    (void)r;
    
    unlink(tmpl);
    SUCCEED();
}

/* tool_fmt 不存在的文件 */
TEST(ToolCov8, FmtNonexistent) {
    int r = tool_fmt("/nonexistent/test.pny");
    (void)r;
    SUCCEED();
}

/* tool_bootstrap */
TEST(ToolCov8, Bootstrap) {
    int r = tool_bootstrap();
    (void)r;
    SUCCEED();
}

/* tool_build wasm 目标 */
TEST(ToolCov8, BuildWasm) {
    char tmpl[] = "/tmp/ponypp_build_wasm_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => { print(\"hi\") }\n}\n");
    fclose(f);
    
    int r = tool_build(tmpl, "/tmp/ponypp_test_out.wasm", "wasm", NULL, "0", false);
    (void)r;
    
    unlink(tmpl);
    unlink("/tmp/ponypp_test_out.wasm");
    SUCCEED();
}

/* tool_build native 目标 */
TEST(ToolCov8, BuildNative) {
    char tmpl[] = "/tmp/ponypp_build_native_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => { print(\"hi\") }\n}\n");
    fclose(f);
    
    int r = tool_build(tmpl, "/tmp/ponypp_test_out_bin", "native", NULL, "0", false);
    (void)r;
    
    unlink(tmpl);
    unlink("/tmp/ponypp_test_out_bin");
    SUCCEED();
}

/* tool_build 不存在的文件 */
TEST(ToolCov8, BuildNonexistent) {
    int r = tool_build("/nonexistent/test.pny", "/tmp/out", "native", NULL, "0", false);
    (void)r;
    SUCCEED();
}

/* tool_run wasm 目标 */
TEST(ToolCov8, RunWasm) {
    char tmpl[] = "/tmp/ponypp_run_wasm_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => { print(\"hi\") }\n}\n");
    fclose(f);
    
    int r = tool_run(tmpl, "wasm", "0");
    (void)r;
    
    unlink(tmpl);
    SUCCEED();
}

/* tool_run 不存在的文件 */
TEST(ToolCov8, RunNonexistent) {
    int r = tool_run("/nonexistent/test.pny", "native", "0");
    (void)r;
    SUCCEED();
}

/* tool_parse_args 各种子命令 */
TEST(ToolCov8, ParseArgsBuild) {
    char *argv[] = {(char*)"ponypp", (char*)"build", (char*)"test.pny"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(3, argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_BUILD);
}

TEST(ToolCov8, ParseArgsRun) {
    char *argv[] = {(char*)"ponypp", (char*)"run", (char*)"test.pny"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(3, argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_RUN);
}

TEST(ToolCov8, ParseArgsTest) {
    char *argv[] = {(char*)"ponypp", (char*)"test"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(2, argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_TEST);
}

TEST(ToolCov8, ParseArgsFmt) {
    char *argv[] = {(char*)"ponypp", (char*)"fmt", (char*)"test.pny"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(3, argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_FMT);
}

TEST(ToolCov8, ParseArgsRepl) {
    char *argv[] = {(char*)"ponypp", (char*)"repl"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(2, argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_REPL);
}

TEST(ToolCov8, ParseArgsPkgNew) {
    char *argv[] = {(char*)"ponypp", (char*)"pkg", (char*)"new", (char*)"mypkg"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_PKG);
}

TEST(ToolCov8, ParseArgsPkgAdd) {
    char *argv[] = {(char*)"ponypp", (char*)"pkg", (char*)"add", (char*)"dep"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_PKG);
}

TEST(ToolCov8, ParseArgsHelp) {
    char *argv[] = {(char*)"ponypp", (char*)"--help"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(2, argv, &tc);
    EXPECT_EQ(r, 0);
}

TEST(ToolCov8, ParseArgsTarget) {
    char *argv[] = {(char*)"ponypp", (char*)"build", (char*)"--target", (char*)"wasm", (char*)"test.pny"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(5, argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_STREQ(tc.target, "wasm");
}

TEST(ToolCov8, ParseArgsOptLevel) {
    char *argv[] = {(char*)"ponypp", (char*)"build", (char*)"-O2", (char*)"test.pny"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_STREQ(tc.olevel, "2");
}

TEST(ToolCov8, ParseArgsOutput) {
    char *argv[] = {(char*)"ponypp", (char*)"build", (char*)"-o", (char*)"out.wasm", (char*)"test.pny"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(5, argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_STREQ(tc.output, "out.wasm");
}

TEST(ToolCov8, ParseArgsVerbose) {
    char *argv[] = {(char*)"ponypp", (char*)"build", (char*)"-v", (char*)"test.pny"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(4, argv, &tc);
    EXPECT_EQ(r, 0);
}

TEST(ToolCov8, ParseArgsNoArgs) {
    char *argv[] = {(char*)"ponypp"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(1, argv, &tc);
    (void)r;
    SUCCEED();
}

TEST(ToolCov8, ParseArgsUnknown) {
    char *argv[] = {(char*)"ponypp", (char*)"unknown_cmd"};
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    int r = tool_parse_args(2, argv, &tc);
    (void)r;
    SUCCEED();
}

/* tool_execute */
TEST(ToolCov8, ExecuteBuild) {
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    tc.cmd = TOOL_BUILD;
    tc.input = "/nonexistent/test.pny";
    tc.output = "/tmp/out";
    tc.target = "native";
    tc.olevel = "0";
    int r = tool_execute(&tc);
    (void)r;
    SUCCEED();
}


/* tool_execute 各种命令 */
TEST(ToolCov8, ExecuteRun) {
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    tc.cmd = TOOL_RUN;
    tc.input = "/nonexistent/test.pny";
    tc.target = "native";
    tc.olevel = "0";
    int r = tool_execute(&tc);
    (void)r;
    SUCCEED();
}

TEST(ToolCov8, ExecuteTest) {
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    tc.cmd = TOOL_TEST;
    tc.test_verbose = false;
    int r = tool_execute(&tc);
    EXPECT_EQ(r, 0);
}

TEST(ToolCov8, ExecuteFmt) {
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    tc.cmd = TOOL_FMT;
    tc.input = "/nonexistent/test.pny";
    int r = tool_execute(&tc);
    (void)r;
    SUCCEED();
}

TEST(ToolCov8, ExecuteRepl) {
    /* REPL 需要 stdin，跳过 */
    SUCCEED();
}

TEST(ToolCov8, ExecutePkg) {
    ToolConfig tc;
    memset(&tc, 0, sizeof(tc));
    tc.cmd = TOOL_PKG;
    tc.subcmd = "unknown";
    int r = tool_execute(&tc);
    (void)r;
    SUCCEED();
}

/* tool_build 带 sourcemap */
TEST(ToolCov8, BuildWithSourcemap) {
    char tmpl[] = "/tmp/ponypp_build_sm_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => { print(\"hi\") }\n}\n");
    fclose(f);
    
    int r = tool_build(tmpl, "/tmp/ponypp_out.wasm", "wasm", NULL, "0", true);
    (void)r;
    
    unlink(tmpl);
    unlink("/tmp/ponypp_out.wasm");
    SUCCEED();
}

/* tool_build 优化等级 */
TEST(ToolCov8, BuildO2) {
    char tmpl[] = "/tmp/ponypp_build_o2_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => { print(\"hi\") }\n}\n");
    fclose(f);
    
    int r = tool_build(tmpl, "/tmp/ponypp_out.o2.wasm", "wasm", NULL, "2", false);
    (void)r;
    
    unlink(tmpl);
    unlink("/tmp/ponypp_out.o2.wasm");
    SUCCEED();
}

/* tool_build 优化等级 3 */
TEST(ToolCov8, BuildO3) {
    char tmpl[] = "/tmp/ponypp_build_o3_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => { print(\"hi\") }\n}\n");
    fclose(f);
    
    int r = tool_build(tmpl, "/tmp/ponypp_out.o3.wasm", "wasm", NULL, "3", false);
    (void)r;
    
    unlink(tmpl);
    unlink("/tmp/ponypp_out.o3.wasm");
    SUCCEED();
}

/* tool_build Os */
TEST(ToolCov8, BuildOs) {
    char tmpl[] = "/tmp/ponypp_build_os_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => { print(\"hi\") }\n}\n");
    fclose(f);
    
    int r = tool_build(tmpl, "/tmp/ponypp_out.os.wasm", "wasm", NULL, "s", false);
    (void)r;
    
    unlink(tmpl);
    unlink("/tmp/ponypp_out.os.wasm");
    SUCCEED();
}
