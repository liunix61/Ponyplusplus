/*
 * gtest_debugger.cpp - DAP 调试器单元测试
 *
 * Phase 3: debugger.c 测试
 */
#include <gtest/gtest.h>
extern "C" {
#include "ponypp/debugger.h"
}
#include <stdio.h>

/* ======================== 生命周期 ======================== */

TEST(DapLifecycle, NewFree) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    EXPECT_EQ(dap_get_state(dbg), DAP_STATE_IDLE);
    dap_free(dbg);
}

TEST(DapLifecycle, NewNullSafe) {
    dap_free(nullptr);
    EXPECT_EQ(dap_get_state(nullptr), DAP_STATE_TERMINATED);
    EXPECT_EQ(dap_get_breakpoint_count(nullptr), 0);
    EXPECT_EQ(dap_list_breakpoints(nullptr), nullptr);
}

/* ======================== 断点管理 ======================== */

TEST(DapBreakpoint, SetLineBreakpoint) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    int id1 = dap_set_breakpoint(dbg, "hello.pny", 10);
    int id2 = dap_set_breakpoint(dbg, "hello.pny", 20);
    int id3 = dap_set_breakpoint(dbg, "world.pny", 5);
    
    EXPECT_GT(id1, 0);
    EXPECT_GT(id2, 0);
    EXPECT_GT(id3, 0);
    EXPECT_NE(id1, id2);
    EXPECT_EQ(dap_get_breakpoint_count(dbg), 3);
    
    dap_free(dbg);
}

TEST(DapBreakpoint, SetFunctionBreakpoint) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    int id = dap_set_function_breakpoint(dbg, "main");
    EXPECT_GT(id, 0);
    EXPECT_EQ(dap_get_breakpoint_count(dbg), 1);
    
    dap_free(dbg);
}

TEST(DapBreakpoint, SetActorBreakpoint) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    int id = dap_set_actor_breakpoint(dbg, 42);
    EXPECT_GT(id, 0);
    
    /* 应命中 */
    EXPECT_EQ(dap_check_actor_breakpoint(dbg, 42), 1);
    /* 不应命中 */
    EXPECT_EQ(dap_check_actor_breakpoint(dbg, 99), 0);
    
    dap_free(dbg);
}

TEST(DapBreakpoint, SetMessageBreakpoint) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    int id = dap_set_message_breakpoint(dbg, "handle_message");
    EXPECT_GT(id, 0);
    
    EXPECT_EQ(dap_check_message_breakpoint(dbg, "handle_message"), 1);
    EXPECT_EQ(dap_check_message_breakpoint(dbg, "other_method"), 0);
    
    dap_free(dbg);
}

TEST(DapBreakpoint, CheckLineBreakpoint) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    dap_set_breakpoint(dbg, "test.pny", 42);
    
    EXPECT_EQ(dap_check_breakpoint(dbg, "test.pny", 42), 1);
    EXPECT_EQ(dap_check_breakpoint(dbg, "test.pny", 43), 0);
    EXPECT_EQ(dap_check_breakpoint(dbg, "other.pny", 42), 0);
    
    dap_free(dbg);
}

TEST(DapBreakpoint, RemoveBreakpoint) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    int id1 = dap_set_breakpoint(dbg, "a.pny", 1);
    int id2 = dap_set_breakpoint(dbg, "b.pny", 2);
    EXPECT_EQ(dap_get_breakpoint_count(dbg), 2);
    
    EXPECT_EQ(dap_remove_breakpoint(dbg, id1), 0);
    EXPECT_EQ(dap_get_breakpoint_count(dbg), 1);
    
    /* 删除不存在的 */
    EXPECT_EQ(dap_remove_breakpoint(dbg, 999), -1);
    
    dap_free(dbg);
}

TEST(DapBreakpoint, EnableDisable) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    int id = dap_set_breakpoint(dbg, "test.pny", 10);
    EXPECT_EQ(dap_check_breakpoint(dbg, "test.pny", 10), 1);
    
    /* 禁用后不应命中 */
    EXPECT_EQ(dap_enable_breakpoint(dbg, id, 0), 0);
    EXPECT_EQ(dap_check_breakpoint(dbg, "test.pny", 10), 0);
    
    /* 重新启用 */
    EXPECT_EQ(dap_enable_breakpoint(dbg, id, 1), 0);
    EXPECT_EQ(dap_check_breakpoint(dbg, "test.pny", 10), 1);
    
    /* 不存在的 ID */
    EXPECT_EQ(dap_enable_breakpoint(dbg, 999, 0), -1);
    
    dap_free(dbg);
}

TEST(DapBreakpoint, HitCount) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    int id = dap_set_breakpoint(dbg, "test.pny", 5);
    
    dap_check_breakpoint(dbg, "test.pny", 5);
    dap_check_breakpoint(dbg, "test.pny", 5);
    dap_check_breakpoint(dbg, "test.pny", 5);
    
    DapBreakpoint *bp = dap_list_breakpoints(dbg);
    ASSERT_NE(bp, nullptr);
    EXPECT_EQ(bp->hit_count, 3);
    
    dap_free(dbg);
}

TEST(DapBreakpoint, ListBreakpoints) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    dap_set_breakpoint(dbg, "a.pny", 1);
    dap_set_function_breakpoint(dbg, "foo");
    dap_set_actor_breakpoint(dbg, 1);
    dap_set_message_breakpoint(dbg, "bar");
    
    DapBreakpoint *bp = dap_list_breakpoints(dbg);
    ASSERT_NE(bp, nullptr);
    
    /* 链表遍历验证 */
    int count = 0;
    for (DapBreakpoint *p = bp; p; p = p->next) count++;
    EXPECT_EQ(count, 4);
    
    dap_free(dbg);
}

TEST(DapBreakpoint, InvalidArgs) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    EXPECT_EQ(dap_set_breakpoint(dbg, nullptr, 10), -1);
    EXPECT_EQ(dap_set_breakpoint(dbg, "test.pny", -1), -1);
    EXPECT_EQ(dap_set_breakpoint(dbg, "test.pny", 0), -1);
    EXPECT_EQ(dap_set_function_breakpoint(dbg, nullptr), -1);
    EXPECT_EQ(dap_set_actor_breakpoint(dbg, -1), -1);
    EXPECT_EQ(dap_set_message_breakpoint(dbg, nullptr), -1);
    
    dap_free(dbg);
}

/* ======================== 执行控制 ======================== */

TEST(DapExecution, ContinueStepPause) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    EXPECT_EQ(dap_continue(dbg), 0);
    EXPECT_EQ(dap_step_over(dbg), 0);
    EXPECT_EQ(dap_step_into(dbg), 0);
    EXPECT_EQ(dap_step_out(dbg), 0);
    EXPECT_EQ(dap_pause(dbg), 0);
    
    dap_free(dbg);
}

/* ======================== 运行时通知 ======================== */

TEST(DapNotify, ActorCreated) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    dap_notify_actor_created(dbg, 1, "Counter");
    dap_notify_actor_created(dbg, 2, "Logger");
    dap_notify_actor_created(dbg, 3, nullptr);
    
    int count = 0;
    DapActorInfo *actors = dap_get_actors(dbg, &count);
    EXPECT_EQ(count, 3);
    ASSERT_NE(actors, nullptr);
    EXPECT_EQ(actors[0].id, 1);
    EXPECT_STREQ(actors[0].name, "Counter");
    
    dap_free(dbg);
}

TEST(DapNotify, MessageSent) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    dap_notify_message(dbg, 1, 2, "increment");
    dap_notify_message(dbg, 2, 1, "get_value");
    
    int count = 0;
    DapMessageInfo *msgs = dap_get_messages(dbg, -1, &count);
    EXPECT_EQ(count, 2);
    ASSERT_NE(msgs, nullptr);
    EXPECT_STREQ(msgs[0].method, "increment");
    
    dap_free(dbg);
}

TEST(DapNotify, BreakpointHit) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    /* 设置断点前先设置状态为 RUNNING */
    dap_set_breakpoint(dbg, "test.pny", 10);
    
    /* 未处于 RUNNING 状态, notify 不应触发 stopped 事件 */
    dap_notify_breakpoint(dbg, "test.pny", 10);
    
    dap_free(dbg);
}

/* ======================== 状态检查 ======================== */

TEST(DapState, InitialState) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    EXPECT_EQ(dap_get_state(dbg), DAP_STATE_IDLE);
    
    int count = 0;
    EXPECT_EQ(dap_get_stack_trace(dbg, &count), nullptr);
    EXPECT_EQ(count, 0);
    
    EXPECT_EQ(dap_get_variables(dbg, 0, &count), nullptr);
    EXPECT_EQ(count, 0);
    
    EXPECT_EQ(dap_get_actors(dbg, &count), nullptr);
    EXPECT_EQ(count, 0);
    
    EXPECT_EQ(dap_get_messages(dbg, -1, &count), nullptr);
    EXPECT_EQ(count, 0);
    
    dap_free(dbg);
}

/* ======================== 回调注册 ======================== */

static int bp_callback_fired = 0;
static void bp_callback(DapDebugger *dbg, const DapBreakpoint *bp) {
    (void)dbg; (void)bp;
    bp_callback_fired++;
}

static void actor_callback(DapDebugger *dbg, int id, const char *name) {
    (void)dbg; (void)id; (void)name;
}

static void msg_callback(DapDebugger *dbg, int from, int to, const char *method) {
    (void)dbg; (void)from; (void)to; (void)method;
}

TEST(DapCallback, RegisterCallbacks) {
    DapDebugger *dbg = dap_new(nullptr, nullptr);
    ASSERT_NE(dbg, nullptr);
    
    dap_set_on_breakpoint(dbg, bp_callback);
    dap_set_on_actor_created(dbg, actor_callback);
    dap_set_on_message_sent(dbg, msg_callback);
    
    /* NULL 回调也应安全 */
    dap_set_on_breakpoint(dbg, nullptr);
    dap_set_on_actor_created(dbg, nullptr);
    dap_set_on_message_sent(dbg, nullptr);
    
    dap_free(dbg);
}

/* ======================== DAP 消息处理 (文件流) ======================== */

TEST(DapMessage, ProcessFromPipe) {
    /* 通过临时文件模拟 stdin */
    const char *msg_body = "{\"seq\":1,\"type\":\"request\",\"command\":\"initialize\"}";
    char header[128];
    snprintf(header, sizeof(header), "Content-Length: %zu\r\n\r\n", strlen(msg_body));
    
    FILE *tmp = tmpfile();
    ASSERT_NE(tmp, nullptr);
    fputs(header, tmp);
    fputs(msg_body, tmp);
    rewind(tmp);
    
    FILE *out = tmpfile();
    ASSERT_NE(out, nullptr);
    
    DapDebugger *dbg = dap_new(tmp, out);
    ASSERT_NE(dbg, nullptr);
    
    int ret = dap_process_message(dbg);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(dap_get_state(dbg), DAP_STATE_INITIALIZED);
    
    /* 验证输出包含 initialize 响应 */
    rewind(out);
    char buf[4096] = {0};
    fread(buf, 1, sizeof(buf) - 1, out);
    EXPECT_NE(strstr(buf, "initialize"), nullptr);
    EXPECT_NE(strstr(buf, "\"success\":true"), nullptr);
    
    dap_free(dbg);
    fclose(tmp);
    fclose(out);
}

TEST(DapMessage, ProcessSetBreakpoints) {
    const char *body = "{\"seq\":2,\"type\":\"request\",\"command\":\"setBreakpoints\","
                       "\"arguments\":{\"source\":{\"path\":\"test.pny\"},"
                       "\"breakpoints\":[{\"line\":42}]}}";
    char header[128];
    snprintf(header, sizeof(header), "Content-Length: %zu\r\n\r\n", strlen(body));
    
    FILE *tmp = tmpfile();
    ASSERT_NE(tmp, nullptr);
    fputs(header, tmp);
    fputs(body, tmp);
    rewind(tmp);
    
    FILE *out = tmpfile();
    ASSERT_NE(out, nullptr);
    
    DapDebugger *dbg = dap_new(tmp, out);
    ASSERT_NE(dbg, nullptr);
    
    int ret = dap_process_message(dbg);
    EXPECT_EQ(ret, 0);
    
    /* 断点应被设置 */
    EXPECT_GE(dap_get_breakpoint_count(dbg), 1);
    EXPECT_EQ(dap_check_breakpoint(dbg, "test.pny", 42), 1);
    
    dap_free(dbg);
    fclose(tmp);
    fclose(out);
}

TEST(DapMessage, ProcessUnknownCommand) {
    const char *body = "{\"seq\":3,\"type\":\"request\",\"command\":\"nonexistent\"}";
    char header[128];
    snprintf(header, sizeof(header), "Content-Length: %zu\r\n\r\n", strlen(body));
    
    FILE *tmp = tmpfile();
    ASSERT_NE(tmp, nullptr);
    fputs(header, tmp);
    fputs(body, tmp);
    rewind(tmp);
    
    FILE *out = tmpfile();
    ASSERT_NE(out, nullptr);
    
    DapDebugger *dbg = dap_new(tmp, out);
    ASSERT_NE(dbg, nullptr);
    
    int ret = dap_process_message(dbg);
    EXPECT_EQ(ret, 0);
    
    /* 应返回 error 响应 */
    rewind(out);
    char buf[4096] = {0};
    fread(buf, 1, sizeof(buf) - 1, out);
    EXPECT_NE(strstr(buf, "unsupported"), nullptr);
    
    dap_free(dbg);
    fclose(tmp);
    fclose(out);
}

TEST(DapMessage, ProcessEmptyInput) {
    FILE *tmp = tmpfile();
    ASSERT_NE(tmp, nullptr);
    
    FILE *out = tmpfile();
    ASSERT_NE(out, nullptr);
    
    DapDebugger *dbg = dap_new(tmp, out);
    ASSERT_NE(dbg, nullptr);
    
    /* 空输入应返回 -1 */
    int ret = dap_process_message(dbg);
    EXPECT_EQ(ret, -1);
    
    dap_free(dbg);
    fclose(tmp);
    fclose(out);
}
