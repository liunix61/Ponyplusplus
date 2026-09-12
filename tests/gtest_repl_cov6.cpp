#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

extern "C" int tool_repl(void);

/* 针对性覆盖 src/ponypp/repl.c 未覆盖行:
 * - 94-97:  repl_eval_string lexer 错误路径 (未知字符)
 * - 150-154: repl_eval_file lexer 错误路径
 * 注: lexer 仅对"未知字符"(如反引号)报错, 未闭合字符串不报错。
 */

static int run_repl_with_input(const char *content) {
    char tmpl[] = "/tmp/repl_cov6_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) return -1;
    write(fd, content, strlen(content));
    close(fd);
    FILE *in = freopen(tmpl, "r", stdin);
    if (!in) { unlink(tmpl); return -1; }
    int r = tool_repl();
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
    return r;
}

TEST(ReplCov6, LexErrorBacktickInString) {
    /* 反引号 = 未知字符 -> lexer_lex_all 失败 -> repl_eval_string 94-97 */
    int r = run_repl_with_input("var x = `\n:quit\n");
    EXPECT_EQ(r, 0);
}

TEST(ReplCov6, LexErrorInFile) {
    /* 写一个含未知字符的文件 -> repl_eval_file 150-154 */
    char path[] = "/tmp/repl_cov6_bad_XXXXXX";
    int fd = mkstemp(path);
    ASSERT_GE(fd, 0);
    const char *bad = "actor Bad\n  fun f() => `\n";
    write(fd, bad, strlen(bad));
    close(fd);

    char cmd[512];
    snprintf(cmd, sizeof(cmd), ":file %s\n:quit\n", path);
    int r = run_repl_with_input(cmd);
    unlink(path);
    EXPECT_EQ(r, 0);
}

TEST(ReplCov6, MultipleLexErrors) {
    /* 多行含未知字符 */
    int r = run_repl_with_input("`\n``\n:quit\n");
    EXPECT_EQ(r, 0);
}

TEST(ReplCov6, BacktickOnly) {
    int r = run_repl_with_input("`\n:quit\n");
    EXPECT_EQ(r, 0);
}
