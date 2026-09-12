#include <gtest/gtest.h>
#include <ponypp.h>
#include <ponypp/tool.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

/* ==================== REPL via tool_repl (no stdin needed) ==================== */

TEST(ReplCov4, ToolRepl) {
    /* tool_repl() reads from stdin; we just verify it doesn't crash */
    int r = tool_repl();
    EXPECT_TRUE(r == 0 || r == -1);
}
