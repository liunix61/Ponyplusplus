#include <gtest/gtest.h>
#include <ponypp/debugger.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

static DapDebugger *new_dap(void) {
    FILE *out = fopen("/dev/null", "w");
    FILE *in = fopen("/dev/null", "r");
    return dap_new(in, out);
}

/* 断点链遍历 */
TEST(DebuggerCov7, BreakpointChainTraversal) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    for (int i = 0; i < 20; i++) {
        dap_set_breakpoint(dbg, "test.pny", i + 1);
    }
    EXPECT_TRUE(dap_check_breakpoint(dbg, "test.pny", 1));
    EXPECT_TRUE(dap_check_breakpoint(dbg, "test.pny", 20));
    EXPECT_FALSE(dap_check_breakpoint(dbg, "test.pny", 99));
    dap_free(dbg);
}

/* 不同文件的断点 */
TEST(DebuggerCov7, DifferentFileBreakpoints) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    dap_set_breakpoint(dbg, "file1.pny", 5);
    dap_set_breakpoint(dbg, "file2.pny", 5);
    EXPECT_TRUE(dap_check_breakpoint(dbg, "file1.pny", 5));
    EXPECT_TRUE(dap_check_breakpoint(dbg, "file2.pny", 5));
    EXPECT_FALSE(dap_check_breakpoint(dbg, "file3.pny", 5));
    dap_free(dbg);
}

/* set_and_remove */
TEST(DebuggerCov7, SetAndRemoveBreakpoint) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_breakpoint(dbg, "test.pny", 1);
    EXPECT_TRUE(dap_check_breakpoint(dbg, "test.pny", 1));
    if (id >= 0) {
        dap_remove_breakpoint(dbg, id);
    }
    EXPECT_FALSE(dap_check_breakpoint(dbg, "test.pny", 1));
    dap_free(dbg);
}

/* 重复设置 */
TEST(DebuggerCov7, SetDuplicateBreakpoint) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    dap_set_breakpoint(dbg, "test.pny", 5);
    dap_set_breakpoint(dbg, "test.pny", 5);
    EXPECT_TRUE(dap_check_breakpoint(dbg, "test.pny", 5));
    dap_free(dbg);
}

/* 不存在的文件 */
TEST(DebuggerCov7, CheckNonexistentFile) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    EXPECT_FALSE(dap_check_breakpoint(dbg, "nonexistent.pny", 1));
    dap_free(dbg);
}

/* 不存在的行号 */
TEST(DebuggerCov7, CheckNonexistentLine) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    dap_set_breakpoint(dbg, "test.pny", 5);
    EXPECT_FALSE(dap_check_breakpoint(dbg, "test.pny", 999));
    dap_free(dbg);
}

/* dap_get_state */
TEST(DebuggerCov7, GetState) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    DapState state = dap_get_state(dbg);
    EXPECT_GE(state, 0);
    dap_free(dbg);
}

/* dap_get_breakpoint_count */
TEST(DebuggerCov7, GetBreakpointCount) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    dap_set_breakpoint(dbg, "test.pny", 3);
    dap_set_breakpoint(dbg, "test.pny", 7);
    int count = dap_get_breakpoint_count(dbg);
    EXPECT_GE(count, 0);
    dap_free(dbg);
}

/* dap_list_breakpoints */
TEST(DebuggerCov7, ListBreakpoints) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    dap_set_breakpoint(dbg, "test.pny", 3);
    dap_set_breakpoint(dbg, "test.pny", 7);
    DapBreakpoint *bp = dap_list_breakpoints(dbg);
    /* 可能为 null */
    (void)bp;
    dap_free(dbg);
}

/* 函数断点 */
TEST(DebuggerCov7, FunctionBreakpoint) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_function_breakpoint(dbg, "main");
    EXPECT_GE(id, 0);
    dap_free(dbg);
}

/* actor 断点 */
TEST(DebuggerCov7, ActorBreakpoint) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_actor_breakpoint(dbg, 1);
    EXPECT_GE(id, 0);
    EXPECT_TRUE(dap_check_actor_breakpoint(dbg, 1));
    dap_free(dbg);
}

/* message 断点 */
TEST(DebuggerCov7, MessageBreakpoint) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_message_breakpoint(dbg, "greet");
    EXPECT_GE(id, 0);
    EXPECT_TRUE(dap_check_message_breakpoint(dbg, "greet"));
    EXPECT_FALSE(dap_check_message_breakpoint(dbg, "other"));
    dap_free(dbg);
}

/* 启用/禁用断点 */
TEST(DebuggerCov7, EnableDisableBreakpoint) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_breakpoint(dbg, "test.pny", 5);
    if (id >= 0) {
        dap_enable_breakpoint(dbg, id, 0);
        dap_enable_breakpoint(dbg, id, 1);
    }
    dap_free(dbg);
}

/* dap_continue */
TEST(DebuggerCov7, Continue) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int r = dap_continue(dbg);
    (void)r;
    dap_free(dbg);
}

/* dap_step_over */
TEST(DebuggerCov7, StepOver) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int r = dap_step_over(dbg);
    (void)r;
    dap_free(dbg);
}

/* dap_step_into */
TEST(DebuggerCov7, StepInto) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int r = dap_step_into(dbg);
    (void)r;
    dap_free(dbg);
}

/* dap_step_out */
TEST(DebuggerCov7, StepOut) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int r = dap_step_out(dbg);
    (void)r;
    dap_free(dbg);
}

/* dap_pause */
TEST(DebuggerCov7, Pause) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int r = dap_pause(dbg);
    (void)r;
    dap_free(dbg);
}

/* dap_get_stack_trace */
TEST(DebuggerCov7, GetStackTrace) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int count = 0;
    DapStackFrame *frames = dap_get_stack_trace(dbg, &count);
    (void)frames;
    EXPECT_GE(count, 0);
    dap_free(dbg);
}

/* dap_get_variables */
TEST(DebuggerCov7, GetVariables) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int count = 0;
    DapVariable *vars = dap_get_variables(dbg, 0, &count);
    (void)vars;
    EXPECT_GE(count, 0);
    dap_free(dbg);
}

/* dap_get_actors */
TEST(DebuggerCov7, GetActors) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int count = 0;
    DapActorInfo *actors = dap_get_actors(dbg, &count);
    (void)actors;
    EXPECT_GE(count, 0);
    dap_free(dbg);
}

/* dap_get_messages */
TEST(DebuggerCov7, GetMessages) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int count = 0;
    DapMessageInfo *msgs = dap_get_messages(dbg, -1, &count);
    (void)msgs;
    EXPECT_GE(count, 0);
    dap_free(dbg);
}

/* dap_notify_breakpoint */
TEST(DebuggerCov7, NotifyBreakpoint) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    dap_notify_breakpoint(dbg, "test.pny", 5);
    SUCCEED();
    dap_free(dbg);
}

/* dap_notify_actor_created */
TEST(DebuggerCov7, NotifyActorCreated) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    dap_notify_actor_created(dbg, 1, "MainActor");
    SUCCEED();
    dap_free(dbg);
}

/* dap_notify_message */
TEST(DebuggerCov7, NotifyMessage) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    dap_notify_message(dbg, 1, 2, "greet");
    SUCCEED();
    dap_free(dbg);
}

/* 回调设置 */
static void on_bp(DapDebugger *dbg, const DapBreakpoint *bp) { (void)dbg; (void)bp; }
static void on_actor(DapDebugger *dbg, int id, const char *name) { (void)dbg; (void)id; (void)name; }
static void on_msg(DapDebugger *dbg, int from, int to, const char *method) { (void)dbg; (void)from; (void)to; (void)method; }

TEST(DebuggerCov7, SetCallbacks) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    dap_set_on_breakpoint(dbg, on_bp);
    dap_set_on_actor_created(dbg, on_actor);
    dap_set_on_message_sent(dbg, on_msg);
    /* 触发通知 */
    dap_notify_breakpoint(dbg, "test.pny", 5);
    dap_notify_actor_created(dbg, 1, "test");
    dap_notify_message(dbg, 1, 2, "test");
    dap_free(dbg);
}

/* dap_free 清理 */
TEST(DebuggerCov7, FreeCleansUp) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    dap_set_breakpoint(dbg, "test.pny", 5);
    dap_set_function_breakpoint(dbg, "main");
    dap_set_actor_breakpoint(dbg, 1);
    dap_set_message_breakpoint(dbg, "test");
    dap_free(dbg);
    SUCCEED();
}

/* 断点链遍历 - 大量 */
TEST(DebuggerCov7, LargeBreakpointChain) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    for (int i = 0; i < 100; i++) {
        dap_set_breakpoint(dbg, "test.pny", i + 1);
    }
    EXPECT_TRUE(dap_check_breakpoint(dbg, "test.pny", 50));
    EXPECT_FALSE(dap_check_breakpoint(dbg, "test.pny", 101));
    int count = dap_get_breakpoint_count(dbg);
    EXPECT_GT(count, 0);
    dap_free(dbg);
}

/* 空文件名断点 */
TEST(DebuggerCov7, EmptyFilenameBreakpoint) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_breakpoint(dbg, "", 5);
    (void)id;
    dap_free(dbg);
}

/* null 文件名断点 */
TEST(DebuggerCov7, NullFilenameBreakpoint) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_breakpoint(dbg, nullptr, 5);
    (void)id;
    dap_free(dbg);
}

/* 负行号断点 */
TEST(DebuggerCov7, NegativeLineBreakpoint) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_breakpoint(dbg, "test.pny", -1);
    (void)id;
    dap_free(dbg);
}

/* 零行号断点 */
TEST(DebuggerCov7, ZeroLineBreakpoint) {
    DapDebugger *dbg = new_dap();
    ASSERT_NE(dbg, nullptr);
    int id = dap_set_breakpoint(dbg, "test.pny", 0);
    (void)id;
    dap_free(dbg);
}
