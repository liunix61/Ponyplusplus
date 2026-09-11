#include <gtest/gtest.h>
#include <ponypp/debugger.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

static DapDebugger *make_dap() {
    FILE *devnull_in = fopen("/dev/null", "r");
    FILE *devnull_out = fopen("/dev/null", "w");
    if (!devnull_in || !devnull_out) {
        if (devnull_in) fclose(devnull_in);
        if (devnull_out) fclose(devnull_out);
        return nullptr;
    }
    DapDebugger *dbg = dap_new(devnull_in, devnull_out);
    return dbg;
}

/* ==================== 断点管理 ==================== */

TEST(DebugCov2, SetBreakpointValid) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id = dap_set_breakpoint(dbg, "test.pny", 10);
    EXPECT_GE(id, 0);
    dap_free(dbg);
}

TEST(DebugCov2, SetBreakpointMultiple) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id1 = dap_set_breakpoint(dbg, "test.pny", 10);
    int id2 = dap_set_breakpoint(dbg, "test.pny", 20);
    int id3 = dap_set_breakpoint(dbg, "test.pny", 30);
    EXPECT_GE(id1, 0);
    EXPECT_GE(id2, 0);
    EXPECT_GE(id3, 0);
    dap_free(dbg);
}

TEST(DebugCov2, SetFunctionBreakpoint) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id = dap_set_function_breakpoint(dbg, "main");
    EXPECT_GE(id, 0);
    dap_free(dbg);
}

TEST(DebugCov2, SetActorBreakpoint) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id = dap_set_actor_breakpoint(dbg, 1);
    EXPECT_GE(id, 0);
    dap_free(dbg);
}

TEST(DebugCov2, SetMessageBreakpoint) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id = dap_set_message_breakpoint(dbg, "receive");
    EXPECT_GE(id, 0);
    dap_free(dbg);
}

/* ==================== 断点检查 ==================== */

TEST(DebugCov2, CheckBreakpointHit) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_breakpoint(dbg, "test.pny", 10);
    int hit = dap_check_breakpoint(dbg, "test.pny", 10);
    EXPECT_NE(hit, 0);
    dap_free(dbg);
}

TEST(DebugCov2, CheckBreakpointMiss) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_breakpoint(dbg, "test.pny", 10);
    int hit = dap_check_breakpoint(dbg, "test.pny", 20);
    EXPECT_EQ(hit, 0);
    dap_free(dbg);
}

TEST(DebugCov2, CheckActorBreakpointHit) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_actor_breakpoint(dbg, 1);
    int hit = dap_check_actor_breakpoint(dbg, 1);
    EXPECT_NE(hit, 0);
    dap_free(dbg);
}

TEST(DebugCov2, CheckActorBreakpointMiss) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_actor_breakpoint(dbg, 1);
    int hit = dap_check_actor_breakpoint(dbg, 2);
    EXPECT_EQ(hit, 0);
    dap_free(dbg);
}

TEST(DebugCov2, CheckMessageBreakpointHit) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_message_breakpoint(dbg, "receive");
    int hit = dap_check_message_breakpoint(dbg, "receive");
    EXPECT_NE(hit, 0);
    dap_free(dbg);
}

TEST(DebugCov2, CheckMessageBreakpointMiss) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_message_breakpoint(dbg, "receive");
    int hit = dap_check_message_breakpoint(dbg, "send");
    EXPECT_EQ(hit, 0);
    dap_free(dbg);
}

/* ==================== 断点删除/禁用 ==================== */

TEST(DebugCov2, RemoveBreakpoint) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id = dap_set_breakpoint(dbg, "test.pny", 10);
    EXPECT_GE(id, 0);
    int ret = dap_remove_breakpoint(dbg, id);
    EXPECT_EQ(ret, 0);
    dap_free(dbg);
}

TEST(DebugCov2, DisableBreakpoint) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id = dap_set_breakpoint(dbg, "test.pny", 10);
    EXPECT_GE(id, 0);
    int ret = dap_enable_breakpoint(dbg, id, 0);
    EXPECT_EQ(ret, 0);
    int hit = dap_check_breakpoint(dbg, "test.pny", 10);
    EXPECT_EQ(hit, 0);
    dap_free(dbg);
}

TEST(DebugCov2, EnableBreakpoint) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id = dap_set_breakpoint(dbg, "test.pny", 10);
    EXPECT_GE(id, 0);
    dap_enable_breakpoint(dbg, id, 0);
    dap_enable_breakpoint(dbg, id, 1);
    int hit = dap_check_breakpoint(dbg, "test.pny", 10);
    EXPECT_NE(hit, 0);
    dap_free(dbg);
}

/* ==================== 执行控制 ==================== */

TEST(DebugCov2, Continue) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int ret = dap_continue(dbg);
    EXPECT_EQ(ret, 0);
    dap_free(dbg);
}

TEST(DebugCov2, StepOver) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int ret = dap_step_over(dbg);
    EXPECT_EQ(ret, 0);
    dap_free(dbg);
}

TEST(DebugCov2, StepInto) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int ret = dap_step_into(dbg);
    EXPECT_EQ(ret, 0);
    dap_free(dbg);
}

TEST(DebugCov2, StepOut) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int ret = dap_step_out(dbg);
    EXPECT_EQ(ret, 0);
    dap_free(dbg);
}

TEST(DebugCov2, Pause) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int ret = dap_pause(dbg);
    EXPECT_EQ(ret, 0);
    dap_free(dbg);
}

/* ==================== 栈/变量查询 ==================== */

TEST(DebugCov2, GetStackTrace) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int count = 0;
    DapStackFrame *frames = dap_get_stack_trace(dbg, &count);
    EXPECT_EQ(frames, nullptr);
    EXPECT_EQ(count, 0);
    dap_free(dbg);
}

TEST(DebugCov2, GetVariables) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int count = 0;
    DapVariable *vars = dap_get_variables(dbg, 1, &count);
    EXPECT_EQ(vars, nullptr);
    EXPECT_EQ(count, 0);
    dap_free(dbg);
}

/* ==================== 空指针安全 ==================== */

TEST(DebugCov2, SetBreakpointNull) {
    EXPECT_EQ(dap_set_breakpoint(nullptr, "test.pny", 10), -1);
}

TEST(DebugCov2, CheckBreakpointNull) {
    EXPECT_EQ(dap_check_breakpoint(nullptr, "test.pny", 10), 0);
}

TEST(DebugCov2, ContinueNull) {
    EXPECT_EQ(dap_continue(nullptr), -1);
}

TEST(DebugCov2, StepOverNull) {
    EXPECT_EQ(dap_step_over(nullptr), -1);
}

TEST(DebugCov2, StepIntoNull) {
    EXPECT_EQ(dap_step_into(nullptr), -1);
}

TEST(DebugCov2, StepOutNull) {
    EXPECT_EQ(dap_step_out(nullptr), -1);
}

TEST(DebugCov2, PauseNull) {
    EXPECT_EQ(dap_pause(nullptr), -1);
}

TEST(DebugCov2, GetStackTraceNull) {
    int count = 0;
    EXPECT_EQ(dap_get_stack_trace(nullptr, &count), nullptr);
}

TEST(DebugCov2, GetVariablesNull) {
    int count = 0;
    EXPECT_EQ(dap_get_variables(nullptr, 1, &count), nullptr);
}

TEST(DebugCov2, RemoveBreakpointNull) {
    EXPECT_EQ(dap_remove_breakpoint(nullptr, 1), -1);
}

TEST(DebugCov2, EnableBreakpointNull) {
    EXPECT_EQ(dap_enable_breakpoint(nullptr, 1, 1), -1);
}
