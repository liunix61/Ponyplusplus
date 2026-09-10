/*
 * Debugger DAP 全面测试
 */

#include <gtest/gtest.h>
#include <ponypp/debugger.h>
#include <cstdio>
#include <cstring>

static DapDebugger *create_debugger(FILE **in_out, FILE **out_out) {
    FILE *in = tmpfile();
    FILE *out = tmpfile();
    if (!in || !out) return nullptr;
    *in_out = in;
    *out_out = out;
    return dap_new(in, out);
}

/* ==================== 生命周期 ==================== */

TEST(DebuggerFull, CreateAndFree) {
    FILE *in, *out;
    DapDebugger *dbg = create_debugger(&in, &out);
    ASSERT_NE(dbg, nullptr);
    dap_free(dbg);
    fclose(in);
    fclose(out);
}

TEST(DebuggerFull, FreeNull) {
    dap_free(nullptr);
}

/* ==================== DAP 消息格式 ==================== */

TEST(DebuggerFull, ProcessInvalidMessage) {
    FILE *in, *out;
    DapDebugger *dbg = create_debugger(&in, &out);
    ASSERT_NE(dbg, nullptr);
    
    /* 写入无效消息 */
    fprintf(in, "invalid message");
    rewind(in);
    
    int r = dap_process_message(dbg);
    (void)r;
    
    dap_free(dbg);
    fclose(in);
    fclose(out);
}

TEST(DebuggerFull, ProcessInitialize) {
    FILE *in, *out;
    DapDebugger *dbg = create_debugger(&in, &out);
    ASSERT_NE(dbg, nullptr);
    
    /* 写入 DAP initialize 请求 */
    const char *body = "{\"seq\":1,\"type\":\"request\",\"command\":\"initialize\"}";
    fprintf(in, "Content-Length: %zu\r\n\r\n%s", strlen(body), body);
    rewind(in);
    
    int r = dap_process_message(dbg);
    (void)r;
    
    dap_free(dbg);
    fclose(in);
    fclose(out);
}

TEST(DebuggerFull, ProcessLaunch) {
    FILE *in, *out;
    DapDebugger *dbg = create_debugger(&in, &out);
    ASSERT_NE(dbg, nullptr);
    
    const char *body = "{\"seq\":2,\"type\":\"request\",\"command\":\"launch\"}";
    fprintf(in, "Content-Length: %zu\r\n\r\n%s", strlen(body), body);
    rewind(in);
    
    int r = dap_process_message(dbg);
    (void)r;
    
    dap_free(dbg);
    fclose(in);
    fclose(out);
}

TEST(DebuggerFull, ProcessSetBreakpoints) {
    FILE *in, *out;
    DapDebugger *dbg = create_debugger(&in, &out);
    ASSERT_NE(dbg, nullptr);
    
    const char *body = "{\"seq\":3,\"type\":\"request\",\"command\":\"setBreakpoints\"}";
    fprintf(in, "Content-Length: %zu\r\n\r\n%s", strlen(body), body);
    rewind(in);
    
    int r = dap_process_message(dbg);
    (void)r;
    
    dap_free(dbg);
    fclose(in);
    fclose(out);
}

TEST(DebuggerFull, ProcessConfigurationDone) {
    FILE *in, *out;
    DapDebugger *dbg = create_debugger(&in, &out);
    ASSERT_NE(dbg, nullptr);
    
    const char *body = "{\"seq\":4,\"type\":\"request\",\"command\":\"configurationDone\"}";
    fprintf(in, "Content-Length: %zu\r\n\r\n%s", strlen(body), body);
    rewind(in);
    
    int r = dap_process_message(dbg);
    (void)r;
    
    dap_free(dbg);
    fclose(in);
    fclose(out);
}

/* ==================== 断点类型 ==================== */

TEST(DebuggerFull, LineBreakpoint) {
    EXPECT_EQ(DAP_BP_LINE, 0);
}

TEST(DebuggerFull, FunctionBreakpoint) {
    EXPECT_NE(DAP_BP_FUNCTION, DAP_BP_LINE);
}

TEST(DebuggerFull, ActorBreakpoint) {
    EXPECT_NE(DAP_BP_ACTOR, DAP_BP_FUNCTION);
}

TEST(DebuggerFull, MessageBreakpoint) {
    EXPECT_NE(DAP_BP_MESSAGE, DAP_BP_ACTOR);
}

/* ==================== 状态转换 ==================== */

TEST(DebuggerFull, StateIdle) {
    EXPECT_EQ(DAP_STATE_IDLE, 0);
}

TEST(DebuggerFull, StateInitialized) {
    EXPECT_NE(DAP_STATE_INITIALIZED, DAP_STATE_IDLE);
}

TEST(DebuggerFull, StateRunning) {
    EXPECT_NE(DAP_STATE_RUNNING, DAP_STATE_INITIALIZED);
}

TEST(DebuggerFull, StateStopped) {
    EXPECT_NE(DAP_STATE_STOPPED, DAP_STATE_RUNNING);
}

TEST(DebuggerFull, StateTerminated) {
    EXPECT_NE(DAP_STATE_TERMINATED, DAP_STATE_STOPPED);
}

/* ==================== 事件类型 ==================== */

TEST(DebuggerFull, EventInitialized) {
    EXPECT_NE(DAP_EVENT_INITIALIZED, DAP_EVENT_STOPPED);
}

TEST(DebuggerFull, EventStopped) {
    EXPECT_NE(DAP_EVENT_STOPPED, DAP_EVENT_CONTINUED);
}

TEST(DebuggerFull, EventContinued) {
    EXPECT_NE(DAP_EVENT_CONTINUED, DAP_EVENT_TERMINATED);
}

TEST(DebuggerFull, EventOutput) {
    EXPECT_NE(DAP_EVENT_OUTPUT, DAP_EVENT_BREAKPOINT);
}

/* ==================== 消息类型 ==================== */

TEST(DebuggerFull, MsgRequest) {
    EXPECT_EQ(DAP_MSG_REQUEST, 0);
}

TEST(DebuggerFull, MsgResponse) {
    EXPECT_NE(DAP_MSG_RESPONSE, DAP_MSG_REQUEST);
}

TEST(DebuggerFull, MsgEvent) {
    EXPECT_NE(DAP_MSG_EVENT, DAP_MSG_RESPONSE);
}
