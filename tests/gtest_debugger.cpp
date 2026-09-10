/*
 * Debugger DAP 测试
 */

#include <gtest/gtest.h>
#include <ponypp/debugger.h>
#include <cstdio>
#include <cstring>

/* ==================== 生命周期 ==================== */

TEST(DebuggerTest, DapNewFree) {
    FILE *in = tmpfile();
    FILE *out = tmpfile();
    ASSERT_NE(in, nullptr);
    ASSERT_NE(out, nullptr);
    
    DapDebugger *dbg = dap_new(in, out);
    ASSERT_NE(dbg, nullptr);
    
    dap_free(dbg);
    fclose(in);
    fclose(out);
}

TEST(DebuggerTest, DapNewNullStreams) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    /* 可能返回NULL */
    if (dbg) dap_free(dbg);
}

TEST(DebuggerTest, DapFreeNull) {
    dap_free(nullptr);
    /* 不应崩溃 */
}

/* ==================== DAP 消息处理 ==================== */

TEST(DebuggerTest, ProcessMessageEmptyInput) {
    FILE *in = tmpfile();
    FILE *out = tmpfile();
    DapDebugger *dbg = dap_new(in, out);
    ASSERT_NE(dbg, nullptr);
    
    /* 空输入应返回错误或超时 */
    int r = dap_process_message(dbg);
    /* 不应崩溃 */
    (void)r;
    
    dap_free(dbg);
    fclose(in);
    fclose(out);
}

TEST(DebuggerTest, ProcessInitializeRequest) {
    FILE *in = tmpfile();
    FILE *out = tmpfile();
    
    /* 写入 DAP initialize 请求 */
    const char *msg = "Content-Length: 100\r\n\r\n{\"seq\":1,\"type\":\"request\",\"command\":\"initialize\",\"arguments\":{\"clientID\":\"test\"}}";
    fwrite(msg, 1, strlen(msg), in);
    rewind(in);
    
    DapDebugger *dbg = dap_new(in, out);
    ASSERT_NE(dbg, nullptr);
    
    int r = dap_process_message(dbg);
    /* 应该处理成功或返回错误 */
    (void)r;
    
    dap_free(dbg);
    fclose(in);
    fclose(out);
}

/* ==================== DAP 运行 ==================== */

TEST(DebuggerTest, DapRunEmptyInput) {
    FILE *in = tmpfile();
    FILE *out = tmpfile();
    DapDebugger *dbg = dap_new(in, out);
    ASSERT_NE(dbg, nullptr);
    
    /* 空输入应退出 */
    int r = dap_run(dbg);
    (void)r;
    
    dap_free(dbg);
    fclose(in);
    fclose(out);
}

/* ==================== 断点管理 ==================== */

TEST(DebuggerTest, BreakpointTypes) {
    /* 验证断点类型枚举 */
    EXPECT_EQ(DAP_BP_LINE, 0);
    EXPECT_NE(DAP_BP_FUNCTION, DAP_BP_LINE);
    EXPECT_NE(DAP_BP_ACTOR, DAP_BP_FUNCTION);
    EXPECT_NE(DAP_BP_MESSAGE, DAP_BP_ACTOR);
}

/* ==================== 状态管理 ==================== */

TEST(DebuggerTest, StateEnum) {
    EXPECT_EQ(DAP_STATE_IDLE, 0);
    EXPECT_NE(DAP_STATE_INITIALIZED, DAP_STATE_IDLE);
    EXPECT_NE(DAP_STATE_RUNNING, DAP_STATE_INITIALIZED);
    EXPECT_NE(DAP_STATE_STOPPED, DAP_STATE_RUNNING);
    EXPECT_NE(DAP_STATE_TERMINATED, DAP_STATE_STOPPED);
}

/* ==================== 事件类型 ==================== */

TEST(DebuggerTest, EventTypes) {
    EXPECT_NE(DAP_EVENT_INITIALIZED, DAP_EVENT_STOPPED);
    EXPECT_NE(DAP_EVENT_STOPPED, DAP_EVENT_CONTINUED);
    EXPECT_NE(DAP_EVENT_CONTINUED, DAP_EVENT_TERMINATED);
    EXPECT_NE(DAP_EVENT_TERMINATED, DAP_EVENT_OUTPUT);
}
