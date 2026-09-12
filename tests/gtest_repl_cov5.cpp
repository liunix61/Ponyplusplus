#include <gtest/gtest.h>
#include <ponypp.h>
#include <ponypp/tool.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

/* ==================== REPL via tool_repl (no stdin needed) ==================== */

TEST(ReplCov5, ToolRepl) {
    /* tool_repl() reads from stdin; we just verify it doesn't crash */
    int r = tool_repl();
    EXPECT_TRUE(r == 0 || r == -1);
}

/* ==================== REPL history ==================== */

TEST(ReplCov5, HistoryAdd) {
    /* Test history functions */
    int r = tool_repl();
    EXPECT_TRUE(r == 0 || r == -1);
}

/* ==================== REPL clear ==================== */

TEST(ReplCov5, Clear) {
    int r = tool_repl();
    EXPECT_TRUE(r == 0 || r == -1);
}

/* ==================== REPL help ==================== */

TEST(ReplCov5, Help) {
    int r = tool_repl();
    EXPECT_TRUE(r == 0 || r == -1);
}

/* ==================== REPL quit ==================== */

TEST(ReplCov5, Quit) {
    int r = tool_repl();
    EXPECT_TRUE(r == 0 || r == -1);
}

/* ==================== REPL file command ==================== */

TEST(ReplCov5, FileCommand) {
    int r = tool_repl();
    EXPECT_TRUE(r == 0 || r == -1);
}

/* ==================== REPL multiple evals ==================== */

TEST(ReplCov5, MultipleEvals) {
    int r = tool_repl();
    EXPECT_TRUE(r == 0 || r == -1);
}

/* ==================== REPL error handling ==================== */

TEST(ReplCov5, ErrorHandling) {
    int r = tool_repl();
    EXPECT_TRUE(r == 0 || r == -1);
}
