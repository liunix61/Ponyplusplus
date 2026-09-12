#include <gtest/gtest.h>
#include <ponypp/tool.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

extern "C" int tool_repl(void);

static int run_repl_with_input(const char *input) {
    char tmpl[] = "/tmp/ponypp_repl_c3_XXXXXX";
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

/* ==================== Error handling paths ==================== */

TEST(ReplCov3, EmptyInput) {
    int result = run_repl_with_input(":quit\n");
    EXPECT_EQ(result, 0);
}

TEST(ReplCov3, InvalidSyntax) {
    int result = run_repl_with_input("actor\n:quit\n");
    EXPECT_EQ(result, 0);
}

TEST(ReplCov3, IncompleteActor) {
    int result = run_repl_with_input("actor main {\n:quit\n");
    EXPECT_EQ(result, 0);
}

TEST(ReplCov3, InvalidToken) {
    int result = run_repl_with_input("@#$%\n:quit\n");
    EXPECT_EQ(result, 0);
}

/* ==================== File command error handling ==================== */

TEST(ReplCov3, FileNonexistent) {
    int result = run_repl_with_input(":file /nonexistent/path/file.pny\n:quit\n");
    EXPECT_EQ(result, 0);
}

TEST(ReplCov3, FileEmptyPath) {
    int result = run_repl_with_input(":file \n:quit\n");
    EXPECT_EQ(result, 0);
}

/* ==================== Reset and clear ==================== */

TEST(ReplCov3, ResetCommand) {
    int result = run_repl_with_input("actor main { new create() => { } }\n:reset\n:quit\n");
    EXPECT_EQ(result, 0);
}

TEST(ReplCov3, ClearCommand) {
    int result = run_repl_with_input("actor main { new create() => { } }\n:clear\n:quit\n");
    EXPECT_EQ(result, 0);
}

/* ==================== History ==================== */

TEST(ReplCov3, HistoryCommand) {
    int result = run_repl_with_input("actor main { new create() => { } }\n:history\n:quit\n");
    EXPECT_EQ(result, 0);
}

/* ==================== Multiple evaluations ==================== */

TEST(ReplCov3, MultipleEvaluations) {
    int result = run_repl_with_input(
        "actor main { new create() => { } }\n"
        "actor Worker { new create() => { } }\n"
        "actor Manager { new create() => { } }\n"
        ":quit\n");
    EXPECT_EQ(result, 0);
}

/* ==================== Long input ==================== */

TEST(ReplCov3, LongInput) {
    int result = run_repl_with_input(
        "actor main { new create() => { print(\"hello\"); print(\"world\"); print(\"test\"); } }\n"
        ":quit\n");
    EXPECT_EQ(result, 0);
}

/* ==================== Special characters ==================== */

TEST(ReplCov3, SpecialCharacters) {
    int result = run_repl_with_input(
        "actor main { new create() => { print(\"hello\\n\\t\\\"\") } }\n"
        ":quit\n");
    EXPECT_EQ(result, 0);
}

/* ==================== Mixed valid/invalid ==================== */

TEST(ReplCov3, MixedValidInvalid) {
    int result = run_repl_with_input(
        "actor main { new create() => { } }\n"
        "invalid syntax here\n"
        ":quit\n");
    EXPECT_EQ(result, 0);
}

/* ==================== Help command ==================== */

TEST(ReplCov3, HelpCommand) {
    int result = run_repl_with_input(":help\n:quit\n");
    EXPECT_EQ(result, 0);
}
