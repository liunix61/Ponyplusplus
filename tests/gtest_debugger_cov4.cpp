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

/* ==================== 更多断点管理组合 ==================== */

TEST(DebugCov4, SetBreakpointManyLines) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    for (int i = 1; i <= 10; i++) {
        int id = dap_set_breakpoint(dbg, "test.pny", i);
        EXPECT_GE(id, 0);
    }
    dap_free(dbg);
}

TEST(DebugCov4, SetBreakpointManyFiles) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    for (int i = 1; i <= 5; i++) {
        char file[32];
        snprintf(file, sizeof(file), "file%d.pny", i);
        int id = dap_set_breakpoint(dbg, file, i);
        EXPECT_GE(id, 0);
    }
    dap_free(dbg);
}

TEST(DebugCov4, SetFunctionBreakpointMany) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    for (int i = 1; i <= 5; i++) {
        char func[32];
        snprintf(func, sizeof(func), "func%d", i);
        int id = dap_set_function_breakpoint(dbg, func);
        EXPECT_GE(id, 0);
    }
    dap_free(dbg);
}

TEST(DebugCov4, SetActorBreakpointMany) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    for (int i = 1; i <= 5; i++) {
        int id = dap_set_actor_breakpoint(dbg, i);
        EXPECT_GE(id, 0);
    }
    dap_free(dbg);
}

TEST(DebugCov4, SetMessageBreakpointMany) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    for (int i = 1; i <= 5; i++) {
        char method[32];
        snprintf(method, sizeof(method), "method%d", i);
        int id = dap_set_message_breakpoint(dbg, method);
        EXPECT_GE(id, 0);
    }
    dap_free(dbg);
}

/* ==================== 更多断点检查组合 ==================== */

TEST(DebugCov4, CheckBreakpointAfterRemoveAll) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id1 = dap_set_breakpoint(dbg, "test.pny", 10);
    int id2 = dap_set_breakpoint(dbg, "test.pny", 20);
    dap_remove_breakpoint(dbg, id1);
    dap_remove_breakpoint(dbg, id2);
    int hit = dap_check_breakpoint(dbg, "test.pny", 10);
    EXPECT_EQ(hit, 0);
    hit = dap_check_breakpoint(dbg, "test.pny", 20);
    EXPECT_EQ(hit, 0);
    dap_free(dbg);
}

TEST(DebugCov4, CheckBreakpointAfterDisableAll) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    int id1 = dap_set_breakpoint(dbg, "test.pny", 10);
    int id2 = dap_set_breakpoint(dbg, "test.pny", 20);
    dap_enable_breakpoint(dbg, id1, 0);
    dap_enable_breakpoint(dbg, id2, 0);
    int hit = dap_check_breakpoint(dbg, "test.pny", 10);
    EXPECT_EQ(hit, 0);
    hit = dap_check_breakpoint(dbg, "test.pny", 20);
    EXPECT_EQ(hit, 0);
    dap_free(dbg);
}

/* ==================== 更多执行控制组合 ==================== */

TEST(DebugCov4, ContinueStepOverStepIntoStepOut) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    EXPECT_EQ(dap_continue(dbg), 0);
    EXPECT_EQ(dap_step_over(dbg), 0);
    EXPECT_EQ(dap_step_into(dbg), 0);
    EXPECT_EQ(dap_step_out(dbg), 0);
    dap_free(dbg);
}

/* ==================== 更多栈/变量查询组合 ==================== */

TEST(DebugCov4, GetStackTraceAfterMultipleBreakpoints) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_breakpoint(dbg, "test.pny", 10);
    dap_set_breakpoint(dbg, "test.pny", 20);
    int count = 0;
    DapStackFrame *frames = dap_get_stack_trace(dbg, &count);
    EXPECT_EQ(frames, nullptr);
    EXPECT_EQ(count, 0);
    dap_free(dbg);
}

TEST(DebugCov4, GetVariablesAfterMultipleBreakpoints) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_breakpoint(dbg, "test.pny", 10);
    dap_set_breakpoint(dbg, "test.pny", 20);
    int count = 0;
    DapVariable *vars = dap_get_variables(dbg, 1, &count);
    EXPECT_EQ(vars, nullptr);
    EXPECT_EQ(count, 0);
    dap_free(dbg);
}

/* ==================== 更多 Actor/Message 查询组合 ==================== */

TEST(DebugCov4, GetActorsAfterBreakpoints) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_actor_breakpoint(dbg, 1);
    dap_set_actor_breakpoint(dbg, 2);
    int count = 0;
    DapActorInfo *actors = dap_get_actors(dbg, &count);
    EXPECT_EQ(actors, nullptr);
    EXPECT_EQ(count, 0);
    dap_free(dbg);
}

TEST(DebugCov4, GetMessagesAfterBreakpoints) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_message_breakpoint(dbg, "receive");
    dap_set_message_breakpoint(dbg, "send");
    int count = 0;
    DapMessageInfo *msgs = dap_get_messages(dbg, 1, &count);
    EXPECT_EQ(msgs, nullptr);
    EXPECT_EQ(count, 0);
    dap_free(dbg);
}

/* ==================== 更多回调设置组合 ==================== */

TEST(DebugCov4, SetAllCallbacks) {
    DapDebugger *dbg = make_dap();
    if (!dbg) return;
    dap_set_on_breakpoint(dbg, nullptr);
    dap_set_on_actor_created(dbg, nullptr);
    dap_set_on_message_sent(dbg, nullptr);
    dap_free(dbg);
}

/* ==================== 更多空指针安全组合 ==================== */

TEST(DebugCov4, AllNullSafety) {
    EXPECT_EQ(dap_set_breakpoint(nullptr, "test.pny", 10), -1);
    EXPECT_EQ(dap_set_function_breakpoint(nullptr, "main"), -1);
    EXPECT_EQ(dap_set_actor_breakpoint(nullptr, 1), -1);
    EXPECT_EQ(dap_set_message_breakpoint(nullptr, "receive"), -1);
    EXPECT_EQ(dap_remove_breakpoint(nullptr, 1), -1);
    EXPECT_EQ(dap_enable_breakpoint(nullptr, 1, 1), -1);
    EXPECT_EQ(dap_check_breakpoint(nullptr, "test.pny", 10), 0);
    EXPECT_EQ(dap_check_actor_breakpoint(nullptr, 1), 0);
    EXPECT_EQ(dap_check_message_breakpoint(nullptr, "receive"), 0);
    EXPECT_EQ(dap_continue(nullptr), -1);
    EXPECT_EQ(dap_step_over(nullptr), -1);
    EXPECT_EQ(dap_step_into(nullptr), -1);
    EXPECT_EQ(dap_step_out(nullptr), -1);
    EXPECT_EQ(dap_pause(nullptr), -1);
    
    int count = 0;
    EXPECT_EQ(dap_get_stack_trace(nullptr, &count), nullptr);
    EXPECT_EQ(dap_get_variables(nullptr, 1, &count), nullptr);
    EXPECT_EQ(dap_get_actors(nullptr, &count), nullptr);
    EXPECT_EQ(dap_get_messages(nullptr, 1, &count), nullptr);
    
    dap_set_on_breakpoint(nullptr, nullptr);
    dap_set_on_actor_created(nullptr, nullptr);
    dap_set_on_message_sent(nullptr, nullptr);
}
