#include <gtest/gtest.h>
#include <ponypp/debugger.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

static DapDebugger *make_dap() {
    FILE *devnull_in = fopen("/dev/null", "r");
    FILE *devnull_out = fopen("/dev/null", "w");
    DapDebugger *dbg = dap_new(devnull_in, devnull_out);
    return dbg;
}

/* ==================== 断点管理 ==================== */

TEST(DebuggerCov, SetBreakpoint) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_breakpoint(dbg, "test.pny", 10);
    EXPECT_GE(id, 0);
    dap_free(dbg);
}

TEST(DebuggerCov, SetMultipleBreakpoints) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    int id1 = dap_set_breakpoint(dbg, "test.pny", 10);
    int id2 = dap_set_breakpoint(dbg, "test.pny", 20);
    int id3 = dap_set_breakpoint(dbg, "other.pny", 5);
    EXPECT_GE(id1, 0);
    EXPECT_GE(id2, 0);
    EXPECT_GE(id3, 0);
    EXPECT_NE(id1, id2);
    dap_free(dbg);
}

TEST(DebuggerCov, SetFunctionBreakpoint) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_function_breakpoint(dbg, "main");
    EXPECT_GE(id, 0);
    dap_free(dbg);
}

TEST(DebuggerCov, SetActorBreakpoint) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_actor_breakpoint(dbg, 1);
    EXPECT_GE(id, 0);
    dap_free(dbg);
}

TEST(DebuggerCov, SetMessageBreakpoint) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_message_breakpoint(dbg, "ping");
    EXPECT_GE(id, 0);
    dap_free(dbg);
}

TEST(DebuggerCov, RemoveBreakpoint) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_breakpoint(dbg, "test.pny", 10);
    EXPECT_GE(id, 0);
    EXPECT_EQ(dap_remove_breakpoint(dbg, id), 0);
    /* 已移除的断点 */
    EXPECT_NE(dap_remove_breakpoint(dbg, id), 0);
    dap_free(dbg);
}

TEST(DebuggerCov, EnableDisableBreakpoint) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_breakpoint(dbg, "test.pny", 10);
    EXPECT_GE(id, 0);
    EXPECT_EQ(dap_enable_breakpoint(dbg, id, 0), 0);  /* 禁用 */
    EXPECT_EQ(dap_enable_breakpoint(dbg, id, 1), 0);  /* 启用 */
    dap_free(dbg);
}

TEST(DebuggerCov, CheckBreakpoint) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    dap_set_breakpoint(dbg, "test.pny", 10);
    EXPECT_TRUE(dap_check_breakpoint(dbg, "test.pny", 10));
    EXPECT_FALSE(dap_check_breakpoint(dbg, "test.pny", 20));
    EXPECT_FALSE(dap_check_breakpoint(dbg, "other.pny", 10));
    dap_free(dbg);
}

TEST(DebuggerCov, CheckActorBreakpoint) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    dap_set_actor_breakpoint(dbg, 1);
    EXPECT_TRUE(dap_check_actor_breakpoint(dbg, 1));
    EXPECT_FALSE(dap_check_actor_breakpoint(dbg, 2));
    dap_free(dbg);
}

TEST(DebuggerCov, CheckMessageBreakpoint) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    dap_set_message_breakpoint(dbg, "ping");
    EXPECT_TRUE(dap_check_message_breakpoint(dbg, "ping"));
    EXPECT_FALSE(dap_check_message_breakpoint(dbg, "pong"));
    dap_free(dbg);
}

/* ==================== 执行控制 ==================== */

TEST(DebuggerCov, Continue) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    dap_continue(dbg);
    dap_free(dbg);
}

TEST(DebuggerCov, StepOver) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    dap_step_over(dbg);
    dap_free(dbg);
}

TEST(DebuggerCov, StepInto) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    dap_step_into(dbg);
    dap_free(dbg);
}

TEST(DebuggerCov, StepOut) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    dap_step_out(dbg);
    dap_free(dbg);
}

TEST(DebuggerCov, Pause) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    dap_pause(dbg);
    dap_free(dbg);
}

/* ==================== 栈和变量 ==================== */

TEST(DebuggerCov, GetStackTrace) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    int count = 0;
    DapStackFrame *frames = dap_get_stack_trace(dbg, &count);
    /* 无栈时返回 NULL 或空 */
    if (frames) free(frames);
    dap_free(dbg);
}

TEST(DebuggerCov, GetVariables) {
    DapDebugger *dbg = make_dap();
    ASSERT_NE(dbg, nullptr);
    int count = 0;
    DapVariable *vars = dap_get_variables(dbg, 0, &count);
    if (vars) free(vars);
    dap_free(dbg);
}

/* ==================== NULL 安全 ==================== */

TEST(DebuggerCov, NullSafety) {
    dap_free(nullptr);
    EXPECT_NE(dap_set_breakpoint(nullptr, "test.pny", 10), 0);
    EXPECT_NE(dap_set_function_breakpoint(nullptr, "main"), 0);
    EXPECT_NE(dap_set_actor_breakpoint(nullptr, 1), 0);
    EXPECT_NE(dap_set_message_breakpoint(nullptr, "ping"), 0);
    EXPECT_NE(dap_remove_breakpoint(nullptr, 0), 0);
    EXPECT_NE(dap_enable_breakpoint(nullptr, 0, 1), 0);
    EXPECT_FALSE(dap_check_breakpoint(nullptr, "test.pny", 10));
    EXPECT_FALSE(dap_check_actor_breakpoint(nullptr, 1));
    EXPECT_FALSE(dap_check_message_breakpoint(nullptr, "ping"));
    EXPECT_NE(dap_continue(nullptr), 0);
    EXPECT_NE(dap_step_over(nullptr), 0);
    EXPECT_NE(dap_step_into(nullptr), 0);
    EXPECT_NE(dap_step_out(nullptr), 0);
    EXPECT_NE(dap_pause(nullptr), 0);
}
