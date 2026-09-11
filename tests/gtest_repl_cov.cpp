#include <gtest/gtest.h>
#include <ponypp/tool.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

extern "C" int tool_repl(void);

/* REPL :help + :quit */
TEST(ReplCov, HelpQuit) {
    char tmpl[] = "/tmp/ponypp_repl_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    FILE *f = fdopen(fd, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, ":help\n:quit\n");
    fclose(f);
    
    FILE *in = freopen(tmpl, "r", stdin);
    ASSERT_NE(in, nullptr);
    
    int r = tool_repl();
    EXPECT_EQ(r, 0);
    
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
}

/* REPL :h + :q */
TEST(ReplCov, HelpQuitShort) {
    char tmpl[] = "/tmp/ponypp_repl2_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    FILE *f = fdopen(fd, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, ":h\n:q\n");
    fclose(f);
    
    FILE *in = freopen(tmpl, "r", stdin);
    ASSERT_NE(in, nullptr);
    
    int r = tool_repl();
    EXPECT_EQ(r, 0);
    
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
}

/* REPL :exit */
TEST(ReplCov, ExitCommand) {
    char tmpl[] = "/tmp/ponypp_repl3_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    FILE *f = fdopen(fd, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, ":exit\n");
    fclose(f);
    
    FILE *in = freopen(tmpl, "r", stdin);
    ASSERT_NE(in, nullptr);
    
    int r = tool_repl();
    EXPECT_EQ(r, 0);
    
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
}

/* REPL :clear + :quit */
TEST(ReplCov, ClearQuit) {
    char tmpl[] = "/tmp/ponypp_repl4_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    FILE *f = fdopen(fd, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, ":clear\n:quit\n");
    fclose(f);
    
    FILE *in = freopen(tmpl, "r", stdin);
    ASSERT_NE(in, nullptr);
    
    int r = tool_repl();
    EXPECT_EQ(r, 0);
    
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
}

/* REPL :history + :quit */
TEST(ReplCov, HistoryQuit) {
    char tmpl[] = "/tmp/ponypp_repl5_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    FILE *f = fdopen(fd, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, ":history\n:quit\n");
    fclose(f);
    
    FILE *in = freopen(tmpl, "r", stdin);
    ASSERT_NE(in, nullptr);
    
    int r = tool_repl();
    EXPECT_EQ(r, 0);
    
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
}

/* REPL :reset + :quit */
TEST(ReplCov, ResetQuit) {
    char tmpl[] = "/tmp/ponypp_repl6_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    FILE *f = fdopen(fd, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, ":reset\n:quit\n");
    fclose(f);
    
    FILE *in = freopen(tmpl, "r", stdin);
    ASSERT_NE(in, nullptr);
    
    int r = tool_repl();
    EXPECT_EQ(r, 0);
    
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
}

/* REPL 未知命令 + :quit */
TEST(ReplCov, UnknownCommandQuit) {
    char tmpl[] = "/tmp/ponypp_repl7_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    FILE *f = fdopen(fd, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, ":unknown\n:quit\n");
    fclose(f);
    
    FILE *in = freopen(tmpl, "r", stdin);
    ASSERT_NE(in, nullptr);
    
    int r = tool_repl();
    EXPECT_EQ(r, 0);
    
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
}

/* REPL 代码输入 + :quit */
TEST(ReplCov, CodeInputQuit) {
    char tmpl[] = "/tmp/ponypp_repl8_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    FILE *f = fdopen(fd, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "let x = 1\n:quit\n");
    fclose(f);
    
    FILE *in = freopen(tmpl, "r", stdin);
    ASSERT_NE(in, nullptr);
    
    int r = tool_repl();
    EXPECT_EQ(r, 0);
    
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
}

/* REPL 空行 + :quit */
TEST(ReplCov, EmptyLinesQuit) {
    char tmpl[] = "/tmp/ponypp_repl9_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    FILE *f = fdopen(fd, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "\n\n\n:quit\n");
    fclose(f);
    
    FILE *in = freopen(tmpl, "r", stdin);
    ASSERT_NE(in, nullptr);
    
    int r = tool_repl();
    EXPECT_EQ(r, 0);
    
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
}

/* REPL :file + :quit */
TEST(ReplCov, FileCommandQuit) {
    char tmpl[] = "/tmp/ponypp_repl10_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    FILE *f = fdopen(fd, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, ":file /nonexistent.pny\n:quit\n");
    fclose(f);
    
    FILE *in = freopen(tmpl, "r", stdin);
    ASSERT_NE(in, nullptr);
    
    int r = tool_repl();
    EXPECT_EQ(r, 0);
    
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
}

/* REPL :hist + :c */
TEST(ReplCov, HistClearShort) {
    char tmpl[] = "/tmp/ponypp_repl11_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    FILE *f = fdopen(fd, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, ":hist\n:c\n:q\n");
    fclose(f);
    
    FILE *in = freopen(tmpl, "r", stdin);
    ASSERT_NE(in, nullptr);
    
    int r = tool_repl();
    EXPECT_EQ(r, 0);
    
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
}

/* REPL EOF (no :quit) */
TEST(ReplCov, EOFQuit) {
    char tmpl[] = "/tmp/ponypp_repl12_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    FILE *f = fdopen(fd, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, ":help\n");
    fclose(f);
    
    FILE *in = freopen(tmpl, "r", stdin);
    ASSERT_NE(in, nullptr);
    
    int r = tool_repl();
    EXPECT_EQ(r, 0);
    
    freopen("/dev/null", "r", stdin);
    unlink(tmpl);
}
