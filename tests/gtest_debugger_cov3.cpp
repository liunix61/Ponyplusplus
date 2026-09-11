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

/* ==================== 更多断点管理 ==================== */

TEST(DebugCov3, SetBreakpointMultipleLines) {
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

TEST(DebugCov3, SetBreakpointDifferentFiles) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id1 = dap_set_breakpoint(dbg, "file1.pny", 10);
    int id2 = dap_set_breakpoint(dbg, "file2.pny", 20);
    EXPECT_GE(id1, 0);
    EXPECT_GE(id2, 0);
    dap_free(dbg);
}

TEST(DebugCov3, SetFunctionBreakpointMultiple) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id1 = dap_set_function_breakpoint(dbg, "main");
    int id2 = dap_set_function_breakpoint(dbg, "helper");
    EXPECT_GE(id1, 0);
    EXPECT_GE(id2, 0);
    dap_free(dbg);
}

TEST(DebugCov3, SetActorBreakpointMultiple) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id1 = dap_set_actor_breakpoint(dbg, 1);
    int id2 = dap_set_actor_breakpoint(dbg, 2);
    EXPECT_GE(id1, 0);
    EXPECT_GE(id2, 0);
    dap_free(dbg);
}

TEST(DebugCov3, SetMessageBreakpointMultiple) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id1 = dap_set_message_breakpoint(dbg, "receive");
    int id2 = dap_set_message_breakpoint(dbg, "send");
    EXPECT_GE(id1, 0);
    EXPECT_GE(id2, 0);
    dap_free(dbg);
}

/* ==================== 更多断点检查 ==================== */

TEST(DebugCov3, CheckBreakpointAfterRemove) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id = dap_set_breakpoint(dbg, "test.pny", 10);
    EXPECT_GE(id, 0);
    dap_remove_breakpoint(dbg, id);
    int hit = dap_check_breakpoint(dbg, "test.pny", 10);
    EXPECT_EQ(hit, 0);
    dap_free(dbg);
}

TEST(DebugCov3, CheckBreakpointAfterDisable) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id = dap_set_breakpoint(dbg, "test.pny", 10);
    EXPECT_GE(id, 0);
    dap_enable_breakpoint(dbg, id, 0);
    int hit = dap_check_breakpoint(dbg, "test.pny", 10);
    EXPECT_EQ(hit, 0);
    dap_free(dbg);
}

TEST(DebugCov3, CheckBreakpointAfterEnable) {
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

/* ==================== 更多执行控制 ==================== */

TEST(DebugCov3, ContinueAfterBreakpoint) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_breakpoint(dbg, "test.pny", 10);
    int ret = dap_continue(dbg);
    EXPECT_EQ(ret, 0);
    dap_free(dbg);
}

TEST(DebugCov3, StepOverAfterBreakpoint) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_breakpoint(dbg, "test.pny", 10);
    int ret = dap_step_over(dbg);
    EXPECT_EQ(ret, 0);
    dap_free(dbg);
}

TEST(DebugCov3, StepIntoAfterBreakpoint) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_breakpoint(dbg, "test.pny", 10);
    int ret = dap_step_into(dbg);
    EXPECT_EQ(ret, 0);
    dap_free(dbg);
}

TEST(DebugCov3, StepOutAfterBreakpoint) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_breakpoint(dbg, "test.pny", 10);
    int ret = dap_step_out(dbg);
    EXPECT_EQ(ret, 0);
    dap_free(dbg);
}

/* ==================== 更多栈/变量查询 ==================== */

TEST(DebugCov3, GetStackTraceAfterBreakpoint) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_breakpoint(dbg, "test.pny", 10);
    int count = 0;
    DapStackFrame *frames = dap_get_stack_trace(dbg, &count);
    EXPECT_EQ(frames, nullptr);
    EXPECT_EQ(count, 0);
    dap_free(dbg);
}

TEST(DebugCov3, GetVariablesAfterBreakpoint) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_breakpoint(dbg, "test.pny", 10);
    int count = 0;
    DapVariable *vars = dap_get_variables(dbg, 1, &count);
    EXPECT_EQ(vars, nullptr);
    EXPECT_EQ(count, 0);
    dap_free(dbg);
}

/* ==================== 更多 Actor/Message 查询 ==================== */

TEST(DebugCov3, GetActors) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int count = 0;
    DapActorInfo *actors = dap_get_actors(dbg, &count);
    EXPECT_EQ(actors, nullptr);
    EXPECT_EQ(count, 0);
    dap_free(dbg);
}

TEST(DebugCov3, GetMessages) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int count = 0;
    DapMessageInfo *msgs = dap_get_messages(dbg, 1, &count);
    EXPECT_EQ(msgs, nullptr);
    EXPECT_EQ(count, 0);
    dap_free(dbg);
}

/* ==================== 回调设置 ==================== */

TEST(DebugCov3, SetOnBreakpoint) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_on_breakpoint(dbg, nullptr);
    dap_free(dbg);
}

TEST(DebugCov3, SetOnActorCreated) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_on_actor_created(dbg, nullptr);
    dap_free(dbg);
}

TEST(DebugCov3, SetOnMessageSent) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_on_message_sent(dbg, nullptr);
    dap_free(dbg);
}

/* ==================== 更多空指针安全 ==================== */

TEST(DebugCov3, SetFunctionBreakpointNull) {
    EXPECT_EQ(dap_set_function_breakpoint(nullptr, "main"), -1);
}

TEST(DebugCov3, SetActorBreakpointNull) {
    EXPECT_EQ(dap_set_actor_breakpoint(nullptr, 1), -1);
}

TEST(DebugCov3, SetMessageBreakpointNull) {
    EXPECT_EQ(dap_set_message_breakpoint(nullptr, "receive"), -1);
}

TEST(DebugCov3, CheckActorBreakpointNull) {
    EXPECT_EQ(dap_check_actor_breakpoint(nullptr, 1), 0);
}

TEST(DebugCov3, CheckMessageBreakpointNull) {
    EXPECT_EQ(dap_check_message_breakpoint(nullptr, "receive"), 0);
}

TEST(DebugCov3, GetActorsNull) {
    int count = 0;
    EXPECT_EQ(dap_get_actors(nullptr, &count), nullptr);
}

TEST(DebugCov3, GetMessagesNull) {
    int count = 0;
    EXPECT_EQ(dap_get_messages(nullptr, 1, &count), nullptr);
}

TEST(DebugCov3, SetOnBreakpointNull) {
    dap_set_on_breakpoint(nullptr, nullptr);  /* 不崩溃 */
}

TEST(DebugCov3, SetOnActorCreatedNull) {
    dap_set_on_actor_created(nullptr, nullptr);  /* 不崩溃 */
}

TEST(DebugCov3, SetOnMessageSentNull) {
    dap_set_on_message_sent(nullptr, nullptr);  /* 不崩溃 */
}
