#include <gtest/gtest.h>
#include <ponypp/tool.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

extern "C" int tool_repl(void);

static int run_repl_with_input(const char *input) {
    char tmpl[] = "/tmp/ponypp_repl_c2_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) return -1;
    FILE *f = fdopen(fd, "w");
    if (!f) { close(fd); unlink(tmpl); return -1; }
    fprintf(f, "%s", input);
    fclose(f);
    
    FILE *in = freopen(tmpl, "r", stdin);
    if (!in) { unlink(tmpl); return -1; }
    
    int r = tool_repl();
    
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
    return r;
}

/* :file 加载有效文件 */
TEST(ReplCov2, FileLoadValid) {
    /* 创建临时 Pony++ 文件 */
    char src_tmpl[] = "/tmp/ponypp_repl_src_XXXXXX";
    int src_fd = mkstemp(src_tmpl);
    ASSERT_GE(src_fd, 0);
    FILE *src_f = fdopen(src_fd, "w");
    ASSERT_NE(src_f, nullptr);
    fprintf(src_f, "actor main {\n  new create() => {\n    print(\"hello from file\")\n  }\n}\n");
    fclose(src_f);
    
    char input[1024];
    snprintf(input, sizeof(input), ":file %s\n:quit\n", src_tmpl);
    
    int r = run_repl_with_input(input);
    EXPECT_EQ(r, 0);
    
    unlink(src_tmpl);
}

/* :file 加载空文件 */
TEST(ReplCov2, FileLoadEmpty) {
    char src_tmpl[] = "/tmp/ponypp_repl_empty_XXXXXX";
    int src_fd = mkstemp(src_tmpl);
    ASSERT_GE(src_fd, 0);
    close(src_fd);
    
    char input[1024];
    snprintf(input, sizeof(input), ":file %s\n:quit\n", src_tmpl);
    
    int r = run_repl_with_input(input);
    EXPECT_EQ(r, 0);
    
    unlink(src_tmpl);
}

/* :file 加载不存在的文件 */
TEST(ReplCov2, FileLoadNonexistent) {
    int r = run_repl_with_input(":file /nonexistent/test.pny\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* :file 加载无效代码 */
TEST(ReplCov2, FileLoadInvalidCode) {
    char src_tmpl[] = "/tmp/ponypp_repl_invalid_XXXXXX";
    int src_fd = mkstemp(src_tmpl);
    ASSERT_GE(src_fd, 0);
    FILE *src_f = fdopen(src_fd, "w");
    ASSERT_NE(src_f, nullptr);
    fprintf(src_f, "this is not valid pony++ code\n");
    fclose(src_f);
    
    char input[1024];
    snprintf(input, sizeof(input), ":file %s\n:quit\n", src_tmpl);
    
    int r = run_repl_with_input(input);
    EXPECT_EQ(r, 0);
    
    unlink(src_tmpl);
}

/* :file 加载语法错误 */
TEST(ReplCov2, FileLoadSyntaxError) {
    char src_tmpl[] = "/tmp/ponypp_repl_syntax_XXXXXX";
    int src_fd = mkstemp(src_tmpl);
    ASSERT_GE(src_fd, 0);
    FILE *src_f = fdopen(src_fd, "w");
    ASSERT_NE(src_f, nullptr);
    fprintf(src_f, "actor main {\n  new create() => {\n    print(\"unclosed\n  }\n");
    fclose(src_f);
    
    char input[1024];
    snprintf(input, sizeof(input), ":file %s\n:quit\n", src_tmpl);
    
    int r = run_repl_with_input(input);
    EXPECT_EQ(r, 0);
    
    unlink(src_tmpl);
}

/* :file 加载带注释的文件 */
TEST(ReplCov2, FileLoadWithComments) {
    char src_tmpl[] = "/tmp/ponypp_repl_comments_XXXXXX";
    int src_fd = mkstemp(src_tmpl);
    ASSERT_GE(src_fd, 0);
    FILE *src_f = fdopen(src_fd, "w");
    ASSERT_NE(src_f, nullptr);
    fprintf(src_f, "// comment\n/* block */\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    fclose(src_f);
    
    char input[1024];
    snprintf(input, sizeof(input), ":file %s\n:quit\n", src_tmpl);
    
    int r = run_repl_with_input(input);
    EXPECT_EQ(r, 0);
    
    unlink(src_tmpl);
}

/* :file 加载多个 actor */
TEST(ReplCov2, FileLoadMultipleActors) {
    char src_tmpl[] = "/tmp/ponypp_repl_multi_XXXXXX";
    int src_fd = mkstemp(src_tmpl);
    ASSERT_GE(src_fd, 0);
    FILE *src_f = fdopen(src_fd, "w");
    ASSERT_NE(src_f, nullptr);
    fprintf(src_f, "actor A {\n  new create() => {}\n}\n\nactor B {\n  new create() => {}\n}\n");
    fclose(src_f);
    
    char input[1024];
    snprintf(input, sizeof(input), ":file %s\n:quit\n", src_tmpl);
    
    int r = run_repl_with_input(input);
    EXPECT_EQ(r, 0);
    
    unlink(src_tmpl);
}

/* :file 加载带 import 的文件 */
TEST(ReplCov2, FileLoadWithImport) {
    char src_tmpl[] = "/tmp/ponypp_repl_import_XXXXXX";
    int src_fd = mkstemp(src_tmpl);
    ASSERT_GE(src_fd, 0);
    FILE *src_f = fdopen(src_fd, "w");
    ASSERT_NE(src_f, nullptr);
    fprintf(src_f, "use \"std\"\n\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    fclose(src_f);
    
    char input[1024];
    snprintf(input, sizeof(input), ":file %s\n:quit\n", src_tmpl);
    
    int r = run_repl_with_input(input);
    EXPECT_EQ(r, 0);
    
    unlink(src_tmpl);
}

/* REPL 代码求值成功 */
TEST(ReplCov2, EvalSuccess) {
    int r = run_repl_with_input("let x = 1\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL 代码求值失败 */
TEST(ReplCov2, EvalFailure) {
    int r = run_repl_with_input("invalid syntax here\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL 多行代码 */
TEST(ReplCov2, MultiLineCode) {
    int r = run_repl_with_input("let x = 1\nlet y = 2\nlet z = x + y\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL 空输入 */
TEST(ReplCov2, EmptyInput) {
    int r = run_repl_with_input("\n\n\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL 只有空格 */
TEST(ReplCov2, WhitespaceOnly) {
    int r = run_repl_with_input("   \n\t\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL :reset 后继续 */
TEST(ReplCov2, ResetThenContinue) {
    int r = run_repl_with_input(":reset\nlet x = 1\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL :history 多次 */
TEST(ReplCov2, HistoryMultiple) {
    int r = run_repl_with_input("let x = 1\nlet y = 2\n:history\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL :clear 后继续 */
TEST(ReplCov2, ClearThenContinue) {
    int r = run_repl_with_input(":clear\nlet x = 1\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL 未知命令后继续 */
TEST(ReplCov2, UnknownThenContinue) {
    int r = run_repl_with_input(":unknown\nlet x = 1\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL :file 后继续 */
TEST(ReplCov2, FileThenContinue) {
    int r = run_repl_with_input(":file /nonexistent.pny\nlet x = 1\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL EOF */
TEST(ReplCov2, EndOfFile) {
    int r = run_repl_with_input("let x = 1\n");
    EXPECT_EQ(r, 0);
}

/* REPL :help 后继续 */
TEST(ReplCov2, HelpThenContinue) {
    int r = run_repl_with_input(":help\nlet x = 1\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL 多次 :reset */
TEST(ReplCov2, MultipleResets) {
    int r = run_repl_with_input(":reset\n:reset\n:reset\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL 多次 :clear */
TEST(ReplCov2, MultipleClears) {
    int r = run_repl_with_input(":clear\n:clear\n:clear\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL 多次 :history */
TEST(ReplCov2, MultipleHistories) {
    int r = run_repl_with_input(":history\n:history\n:history\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL 代码 + 命令混合 */
TEST(ReplCov2, CodeAndCommandsMixed) {
    int r = run_repl_with_input("let x = 1\n:help\nlet y = 2\n:history\nlet z = 3\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL 长代码行 */
TEST(ReplCov2, LongCodeLine) {
    int r = run_repl_with_input("let x = 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL 特殊字符 */
TEST(ReplCov2, SpecialChars) {
    int r = run_repl_with_input("let s = \"hello world\"\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL :file 无参数 */
TEST(ReplCov2, FileNoArg) {
    int r = run_repl_with_input(":file\n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL :file 空路径 */
TEST(ReplCov2, FileEmptyPath) {
    int r = run_repl_with_input(":file \n:quit\n");
    EXPECT_EQ(r, 0);
}

/* REPL :file 目录 */
