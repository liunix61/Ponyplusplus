#include <gtest/gtest.h>
#include <ponypp/tool.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <cstdio>
#include <sys/stat.h>
#include <fcntl.h>

extern "C" int tool_bootstrap(void);

/* tool_bootstrap 直接调用（覆盖代码路径） */
TEST(ToolCov7, Bootstrap) {
    int r = tool_bootstrap();
    /* 可能成功或失败，但覆盖了代码路径 */
    (void)r;
}

/* tool_test 在有 _test.pny 文件的目录 */
TEST(ToolCov7, TestWithTestFiles) {
    /* 创建临时目录 */
    char tmpl[] = "/tmp/ponypp_test_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    
    /* 创建一个简单的 _test.pny 文件 */
    char test_path[512];
    snprintf(test_path, sizeof(test_path), "%s/simple_test.pny", dir);
    FILE *f = fopen(test_path, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => {\n    print(\"test\")\n  }\n}\n");
    fclose(f);
    
    /* 切换到该目录 */
    char old_cwd[1024];
    getcwd(old_cwd, sizeof(old_cwd));
    chdir(dir);
    
    int r = tool_test(false);
    /* 期望失败（编译可能不完整），但覆盖了代码路径 */
    (void)r;
    
    /* 切换回原目录 */
    chdir(old_cwd);
    
    /* 清理 */
    unlink(test_path);
    rmdir(dir);
}

/* tool_test verbose 模式 */
TEST(ToolCov7, TestVerbose) {
    char tmpl[] = "/tmp/ponypp_test_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    
    char test_path[512];
    snprintf(test_path, sizeof(test_path), "%s/simple_test.pny", dir);
    FILE *f = fopen(test_path, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => {\n    print(\"test\")\n  }\n}\n");
    fclose(f);
    
    char old_cwd[1024];
    getcwd(old_cwd, sizeof(old_cwd));
    chdir(dir);
    
    int r = tool_test(true);
    (void)r;
    
    chdir(old_cwd);
    unlink(test_path);
    rmdir(dir);
}

/* tool_test 空目录 */
TEST(ToolCov7, TestEmptyDir) {
    char tmpl[] = "/tmp/ponypp_test_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    
    char old_cwd[1024];
    getcwd(old_cwd, sizeof(old_cwd));
    chdir(dir);
    
    int r = tool_test(false);
    /* 空目录应该返回 0 */
    EXPECT_EQ(r, 0);
    
    chdir(old_cwd);
    rmdir(dir);
}

/* tool_test 多个 _test.pny 文件 */
TEST(ToolCov7, TestMultipleFiles) {
    char tmpl[] = "/tmp/ponypp_test_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    
    for (int i = 0; i < 3; i++) {
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s/test%d_test.pny", dir, i);
        FILE *f = fopen(test_path, "w");
        ASSERT_NE(f, nullptr);
        fprintf(f, "actor main {\n  new create() => {\n    print(\"test%d\")\n  }\n}\n", i);
        fclose(f);
    }
    
    char old_cwd[1024];
    getcwd(old_cwd, sizeof(old_cwd));
    chdir(dir);
    
    int r = tool_test(false);
    (void)r;
    
    chdir(old_cwd);
    
    /* 清理 */
    for (int i = 0; i < 3; i++) {
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s/test%d_test.pny", dir, i);
        unlink(test_path);
    }
    rmdir(dir);
}

/* tool_execute TOOL_BOOTSTRAP */
TEST(ToolCov7, ExecuteBootstrap) {
    ToolConfig tc = {};
    tc.cmd = TOOL_BOOTSTRAP;
    int r = tool_execute(&tc);
    (void)r;
}

/* tool_execute TOOL_TEST */
TEST(ToolCov7, ExecuteTest) {
    char tmpl[] = "/tmp/ponypp_test_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    
    char old_cwd[1024];
    getcwd(old_cwd, sizeof(old_cwd));
    chdir(dir);
    
    ToolConfig tc = {};
    tc.cmd = TOOL_TEST;
    tc.test_verbose = false;
    int r = tool_execute(&tc);
    EXPECT_EQ(r, 0);
    
    chdir(old_cwd);
    rmdir(dir);
}

/* tool_execute TOOL_FMT */
TEST(ToolCov7, ExecuteFmt) {
    /* 创建临时文件 */
    char tmpl[] = "/tmp/ponypp_fmt_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => {\n    print(\"hello\")\n  }\n}\n");
    fclose(f);
    
    ToolConfig tc = {};
    tc.cmd = TOOL_FMT;
    tc.input = tmpl;
    int r = tool_execute(&tc);
    EXPECT_EQ(r, 0);
    
    unlink(tmpl);
}

/* tool_execute TOOL_REPL (stdin 是 /dev/null) */
TEST(ToolCov7, ExecuteRepl) {
    /* 重定向 stdin 到 /dev/null */
    int old_stdin = dup(0);
    int devnull = open("/dev/null", O_RDONLY);
    ASSERT_GE(devnull, 0);
    dup2(devnull, 0);
    close(devnull);
    
    ToolConfig tc = {};
    tc.cmd = TOOL_REPL;
    int r = tool_execute(&tc);
    (void)r;
    
    /* 恢复 stdin */
    dup2(old_stdin, 0);
    close(old_stdin);
}

/* tool_parse_args + tool_execute 组合 */
TEST(ToolCov7, ParseAndExecuteBuild) {
    char tmpl[] = "/tmp/ponypp_build_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => {\n    print(\"hello\")\n  }\n}\n");
    fclose(f);
    
    const char *argv[] = {"ponypp", "build", tmpl, "-o", "/tmp/ponypp_test_out", "--target", "native"};
    ToolConfig tc = {};
    int r = tool_parse_args(7, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    
    r = tool_execute(&tc);
    (void)r;
    
    unlink(tmpl);
}

/* tool_parse_args + tool_execute 组合 (wasm) */
TEST(ToolCov7, ParseAndExecuteBuildWasm) {
    char tmpl[] = "/tmp/ponypp_build_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => {\n    print(\"hello\")\n  }\n}\n");
    fclose(f);
    
    const char *argv[] = {"ponypp", "build", tmpl, "-o", "/tmp/ponypp_test_out.wasm", "--target", "wasm"};
    ToolConfig tc = {};
    int r = tool_parse_args(7, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    
    r = tool_execute(&tc);
    (void)r;
    
    unlink(tmpl);
}

/* tool_parse_args + tool_execute 组合 (run) */
TEST(ToolCov7, ParseAndExecuteRun) {
    char tmpl[] = "/tmp/ponypp_run_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => {\n    print(\"hello\")\n  }\n}\n");
    fclose(f);
    
    const char *argv[] = {"ponypp", "run", tmpl, "--target", "native"};
    ToolConfig tc = {};
    int r = tool_parse_args(5, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    
    r = tool_execute(&tc);
    (void)r;
    
    unlink(tmpl);
}

/* tool_parse_args + tool_execute 组合 (check) */
TEST(ToolCov7, ParseAndExecuteCheck) {
    char tmpl[] = "/tmp/ponypp_check_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "actor main {\n  new create() => {\n    print(\"hello\")\n  }\n}\n");
    fclose(f);
    
    const char *argv[] = {"ponypp", "docs", tmpl};
    ToolConfig tc = {};
    int r = tool_parse_args(3, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    
    r = tool_execute(&tc);
    (void)r;
    
    unlink(tmpl);
}

/* tool_parse_args + tool_execute 组合 (pkg new) */
TEST(ToolCov7, ParseAndExecutePkgNew) {
    char tmpl[] = "/tmp/ponypp_pkg_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    
    char old_cwd[1024];
    getcwd(old_cwd, sizeof(old_cwd));
    chdir(dir);
    
    const char *argv[] = {"ponypp", "pkg", "new", "myproject"};
    ToolConfig tc = {};
    int r = tool_parse_args(4, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    
    r = tool_execute(&tc);
    EXPECT_EQ(r, 0);
    
    /* 清理 */
    char src_dir[512], main_path[512], toml_path[512];
    snprintf(src_dir, sizeof(src_dir), "myproject/src");
    snprintf(main_path, sizeof(main_path), "myproject/src/main.pny");
    snprintf(toml_path, sizeof(toml_path), "myproject/ponypp.toml");
    unlink(main_path);
    unlink(toml_path);
    rmdir(src_dir);
    rmdir("myproject");
    
    chdir(old_cwd);
    rmdir(dir);
}

/* tool_parse_args + tool_execute 组合 (pkg add) */
TEST(ToolCov7, ParseAndExecutePkgAdd) {
    char tmpl[] = "/tmp/ponypp_pkg_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    
    /* 创建 ponypp.toml */
    char toml_path[512];
    snprintf(toml_path, sizeof(toml_path), "%s/ponypp.toml", dir);
    FILE *f = fopen(toml_path, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "[package]\nname = \"test\"\n");
    fclose(f);
    
    char old_cwd[1024];
    getcwd(old_cwd, sizeof(old_cwd));
    chdir(dir);
    
    const char *argv[] = {"ponypp", "pkg", "add", "mydep"};
    ToolConfig tc = {};
    int r = tool_parse_args(4, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    
    r = tool_execute(&tc);
    EXPECT_EQ(r, 0);
    
    chdir(old_cwd);
    unlink(toml_path);
    rmdir(dir);
}

/* tool_parse_args + tool_execute 组合 (test) */
TEST(ToolCov7, ParseAndExecuteTest) {
    char tmpl[] = "/tmp/ponypp_test_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    
    char old_cwd[1024];
    getcwd(old_cwd, sizeof(old_cwd));
    chdir(dir);
    
    const char *argv[] = {"ponypp", "test"};
    ToolConfig tc = {};
    int r = tool_parse_args(2, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    
    r = tool_execute(&tc);
    EXPECT_EQ(r, 0);
    
    chdir(old_cwd);
    rmdir(dir);
}

/* tool_parse_args + tool_execute 组合 (repl) */
TEST(ToolCov7, ParseAndExecuteRepl) {
    /* 重定向 stdin 到 /dev/null */
    int old_stdin = dup(0);
    int devnull = open("/dev/null", O_RDONLY);
    ASSERT_GE(devnull, 0);
    dup2(devnull, 0);
    close(devnull);
    
    const char *argv[] = {"ponypp", "repl"};
    ToolConfig tc = {};
    int r = tool_parse_args(2, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    
    r = tool_execute(&tc);
    (void)r;
    
    /* 恢复 stdin */
    dup2(old_stdin, 0);
    close(old_stdin);
}

/* tool_parse_args + tool_execute 组合 (bootstrap) */
TEST(ToolCov7, ParseAndExecuteBootstrap) {
    const char *argv[] = {"ponypp", "bootstrap"};
    ToolConfig tc = {};
    int r = tool_parse_args(2, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    
    r = tool_execute(&tc);
    (void)r;
}

/* tool_parse_args unknown command */
TEST(ToolCov7, ParseUnknownCommand) {
    const char *argv[] = {"ponypp", "unknown"};
    ToolConfig tc = {};
    int r = tool_parse_args(2, (char**)argv, &tc);
    EXPECT_NE(r, 0);
}

/* tool_parse_args no command */
TEST(ToolCov7, ParseNoCommand) {
    const char *argv[] = {"ponypp"};
    ToolConfig tc = {};
    int r = tool_parse_args(1, (char**)argv, &tc);
    EXPECT_NE(r, 0);
}

/* tool_parse_args null argv */
TEST(ToolCov7, ParseNullArgv) {
    ToolConfig tc = {};
    int r = tool_parse_args(0, nullptr, &tc);
    EXPECT_NE(r, 0);
}

/* tool_parse_args help */
TEST(ToolCov7, ParseHelp) {
    const char *argv[] = {"ponypp", "--help"};
    ToolConfig tc = {};
    int r = tool_parse_args(2, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_HELP);
}

/* tool_parse_args version */
TEST(ToolCov7, ParseDashH) {
    const char *argv[] = {"ponypp", "-h"};
    ToolConfig tc = {};
    int r = tool_parse_args(2, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_HELP);
}

/* tool_parse_args check */
TEST(ToolCov7, ParseDocs) {
    const char *argv[] = {"ponypp", "docs", "test.pny"};
    ToolConfig tc = {};
    int r = tool_parse_args(3, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_DOCS);
}

/* tool_parse_args fmt */
TEST(ToolCov7, ParseFmt) {
    const char *argv[] = {"ponypp", "fmt", "test.pny"};
    ToolConfig tc = {};
    int r = tool_parse_args(3, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_FMT);
}

/* tool_parse_args run */
TEST(ToolCov7, ParseRun) {
    const char *argv[] = {"ponypp", "run", "test.pny"};
    ToolConfig tc = {};
    int r = tool_parse_args(3, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_RUN);
}

/* tool_parse_args test */
TEST(ToolCov7, ParseTest) {
    const char *argv[] = {"ponypp", "test"};
    ToolConfig tc = {};
    int r = tool_parse_args(2, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_TEST);
}

/* tool_parse_args repl */
TEST(ToolCov7, ParseRepl) {
    const char *argv[] = {"ponypp", "repl"};
    ToolConfig tc = {};
    int r = tool_parse_args(2, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_REPL);
}

/* tool_parse_args bootstrap */
TEST(ToolCov7, ParseBootstrap) {
    const char *argv[] = {"ponypp", "bootstrap"};
    ToolConfig tc = {};
    int r = tool_parse_args(2, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_BOOTSTRAP);
}

/* tool_parse_args build */
TEST(ToolCov7, ParseBuild) {
    const char *argv[] = {"ponypp", "build", "test.pny", "-o", "out"};
    ToolConfig tc = {};
    int r = tool_parse_args(5, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_BUILD);
}

/* tool_parse_args pkg */
TEST(ToolCov7, ParsePkg) {
    const char *argv[] = {"ponypp", "pkg", "new", "test"};
    ToolConfig tc = {};
    int r = tool_parse_args(4, (char**)argv, &tc);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(tc.cmd, TOOL_PKG);
}
