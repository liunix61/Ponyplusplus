#include <gtest/gtest.h>
#include <ponypp/debugger.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

static DapDebugger *make_dap() {
    FILE *devnull_in = fopen("/dev/null", "r");
    FILE *devnull_out = fopen("/dev/null", "w");
    return dap_new(devnull_in, devnull_out);
}

static DapDebugger *make_dap_with_input(const char *json_msg) {
    int pipefd[2];
    if (pipe(pipefd) != 0) return nullptr;
    char header[256];
    int content_len = (int)strlen(json_msg);
    snprintf(header, sizeof(header), "Content-Length: %d\r\n\r\n", content_len);
    write(pipefd[1], header, strlen(header));
    write(pipefd[1], json_msg, content_len);
    close(pipefd[1]);
    FILE *in = fdopen(pipefd[0], "r");
    FILE *out = fopen("/dev/null", "w");
    if (!in || !out) {
        if (in) fclose(in);
        if (out) fclose(out);
        close(pipefd[0]);
        return nullptr;
    }
    return dap_new(in, out);
}

/* JSON 转义序列 */
TEST(DebuggerCov6, JsonEscapes) {
    DapDebugger *dbg = make_dap_with_input(
        "{\"seq\":1,\"type\":\"request\",\"command\":\"initialize\",\"arguments\":{\"clientID\":\"test\\n\\t\\r\\\"\\\\\"}}");
    ASSERT_NE(dbg, nullptr);
    int r = dap_process_message(dbg);
    EXPECT_NE(r, -1);
    dap_free(dbg);
}

/* setBreakpoints with actual breakpoints array */
TEST(DebuggerCov6, SetBreakpointsArray) {
    DapDebugger *dbg = make_dap_with_input(
        "{\"seq\":2,\"type\":\"request\",\"command\":\"setBreakpoints\",\"arguments\":{\"source\":{\"path\":\"test.pny\"},\"breakpoints\":[{\"line\":10},{\"line\":20}]}}");
    ASSERT_NE(dbg, nullptr);
    dap_process_message(dbg);
    EXPECT_GE(dap_get_breakpoint_count(dbg), 1);
    dap_free(dbg);
}

/* stackTrace DAP 请求 */
TEST(DebuggerCov6, StackTraceRequest) {
    DapDebugger *dbg = make_dap_with_input(
        "{\"seq\":4,\"type\":\"request\",\"command\":\"stackTrace\",\"arguments\":{\"threadId\":1}}");
    ASSERT_NE(dbg, nullptr);
    int r = dap_process_message(dbg);
    EXPECT_NE(r, -1);
    dap_free(dbg);
}

/* variables DAP 请求 */
TEST(DebuggerCov6, VariablesRequest) {
    DapDebugger *dbg = make_dap_with_input(
        "{\"seq\":5,\"type\":\"request\",\"command\":\"variables\",\"arguments\":{\"variablesReference\":0}}");
    ASSERT_NE(dbg, nullptr);
    int r = dap_process_message(dbg);
    EXPECT_NE(r, -1);
    dap_free(dbg);
}

/* dap_get_actors with actors */
TEST(DebuggerCov6, GetActorsWithActors) {
    DapDebugger *dbg = make_dap();
    dap_notify_actor_created(dbg, 1, "main");
    dap_notify_actor_created(dbg, 2, "worker");
    int count = 0;
    DapActorInfo *actors = dap_get_actors(dbg, &count);
    EXPECT_EQ(count, 2);
    EXPECT_NE(actors, nullptr);
    dap_free(dbg);
}

/* dap_get_messages with messages */
TEST(DebuggerCov6, GetMessagesWithMsgs) {
    DapDebugger *dbg = make_dap();
    dap_notify_message(dbg, 1, 2, "greet");
    dap_notify_message(dbg, 2, 1, "reply");
    int count = 0;
    DapMessageInfo *msgs = dap_get_messages(dbg, -1, &count);
    EXPECT_GE(count, 1);
    EXPECT_NE(msgs, nullptr);
    dap_free(dbg);
}

/* dap_check_breakpoint 触发暂停 */
TEST(DebuggerCov6, CheckBreakpointPause) {
    DapDebugger *dbg = make_dap();
    dap_set_breakpoint(dbg, "test.pny", 10);
    int hit = dap_check_breakpoint(dbg, "test.pny", 10);
    EXPECT_EQ(hit, 1);
    dap_free(dbg);
}

/* dap_check_breakpoint 未命中 */
TEST(DebuggerCov6, CheckBreakpointMiss) {
    DapDebugger *dbg = make_dap();
    dap_set_breakpoint(dbg, "test.pny", 10);
    int hit = dap_check_breakpoint(dbg, "test.pny", 20);
    EXPECT_EQ(hit, 0);
    EXPECT_NE(dap_get_state(dbg), DAP_STATE_STOPPED);
    dap_free(dbg);
}

/* dap_notify_actor_created 触发回调 */
static bool actor_created_called = false;
static void on_actor_created(DapDebugger *dbg, int id, const char *name) {
    (void)dbg; (void)id; (void)name;
    actor_created_called = true;
}

TEST(DebuggerCov6, NotifyActorCreatedCallback) {
    actor_created_called = false;
    DapDebugger *dbg = make_dap();
    dap_set_on_actor_created(dbg, on_actor_created);
    dap_notify_actor_created(dbg, 1, "main");
    EXPECT_TRUE(actor_created_called);
    dap_free(dbg);
}

/* dap_notify_message 触发回调 */
static bool message_called = false;
static void on_message(DapDebugger *dbg, int from, int to, const char *method) {
    (void)dbg; (void)from; (void)to; (void)method;
    message_called = true;
}

TEST(DebuggerCov6, NotifyMessageCallback) {
    message_called = false;
    DapDebugger *dbg = make_dap();
    dap_set_on_message_sent(dbg, on_message);
    dap_notify_message(dbg, 1, 2, "greet");
    EXPECT_TRUE(message_called);
    dap_free(dbg);
}

/* dap_notify_breakpoint 触发回调 */
static bool breakpoint_called = false;
static void on_breakpoint(DapDebugger *dbg, const DapBreakpoint *bp) {
    (void)dbg; (void)bp;
    breakpoint_called = true;
}

TEST(DebuggerCov6, NotifyBreakpointCallback) {
    breakpoint_called = false;
    DapDebugger *dbg = make_dap_with_input(
        "{\"seq\":1,\"type\":\"request\",\"command\":\"launch\",\"arguments\":{\"program\":\"test.pny\"}}");
    ASSERT_NE(dbg, nullptr);
    dap_process_message(dbg);
    dap_set_on_breakpoint(dbg, on_breakpoint);
    dap_set_breakpoint(dbg, "test.pny", 10);
    dap_notify_breakpoint(dbg, "test.pny", 10);
    EXPECT_TRUE(breakpoint_called);
    dap_free(dbg);
}

/* dap_step_over */
TEST(DebuggerCov6, StepOver) {
    DapDebugger *dbg = make_dap();
    dap_set_breakpoint(dbg, "test.pny", 10);
    dap_check_breakpoint(dbg, "test.pny", 10);
    dap_step_over(dbg);
    dap_free(dbg);
}

/* dap_step_into */
TEST(DebuggerCov6, StepInto) {
    DapDebugger *dbg = make_dap();
    dap_set_breakpoint(dbg, "test.pny", 10);
    dap_check_breakpoint(dbg, "test.pny", 10);
    dap_step_into(dbg);
    dap_free(dbg);
}

/* dap_step_out */
TEST(DebuggerCov6, StepOut) {
    DapDebugger *dbg = make_dap();
    dap_set_breakpoint(dbg, "test.pny", 10);
    dap_check_breakpoint(dbg, "test.pny", 10);
    dap_step_out(dbg);
    dap_free(dbg);
}

/* dap_continue */
TEST(DebuggerCov6, Continue) {
    DapDebugger *dbg = make_dap();
    dap_set_breakpoint(dbg, "test.pny", 10);
    dap_check_breakpoint(dbg, "test.pny", 10);
    dap_continue(dbg);
    dap_free(dbg);
}

/* dap_pause */
TEST(DebuggerCov6, Pause) {
    DapDebugger *dbg = make_dap();
    dap_pause(dbg);
    dap_free(dbg);
}

/* dap_remove_breakpoint */
TEST(DebuggerCov6, RemoveBreakpoint) {
    DapDebugger *dbg = make_dap();
    int id = dap_set_breakpoint(dbg, "test.pny", 10);
    EXPECT_GE(dap_get_breakpoint_count(dbg), 1);
    dap_remove_breakpoint(dbg, id);
    EXPECT_EQ(dap_get_breakpoint_count(dbg), 0);
    dap_free(dbg);
}

/* dap_set_function_breakpoint */
TEST(DebuggerCov6, FunctionBreakpoint) {
    DapDebugger *dbg = make_dap();
    int id = dap_set_function_breakpoint(dbg, "main");
    EXPECT_GE(id, 0);
    EXPECT_GE(dap_get_breakpoint_count(dbg), 1);
    dap_free(dbg);
}

/* dap_set_actor_breakpoint */
TEST(DebuggerCov6, ActorBreakpoint) {
    DapDebugger *dbg = make_dap();
    int id = dap_set_actor_breakpoint(dbg, 1);
    EXPECT_GE(id, 0);
    EXPECT_GE(dap_get_breakpoint_count(dbg), 1);
    dap_free(dbg);
}

/* dap_set_message_breakpoint */
TEST(DebuggerCov6, MessageBreakpoint) {
    DapDebugger *dbg = make_dap();
    int id = dap_set_message_breakpoint(dbg, "greet");
    EXPECT_GE(id, 0);
    EXPECT_GE(dap_get_breakpoint_count(dbg), 1);
    dap_free(dbg);
}

/* dap_enable_breakpoint */
TEST(DebuggerCov6, EnableBreakpoint) {
    DapDebugger *dbg = make_dap();
    int id = dap_set_breakpoint(dbg, "test.pny", 10);
    dap_enable_breakpoint(dbg, id, 0);
    int hit = dap_check_breakpoint(dbg, "test.pny", 10);
    EXPECT_EQ(hit, 0);
    dap_enable_breakpoint(dbg, id, 1);
    hit = dap_check_breakpoint(dbg, "test.pny", 10);
    EXPECT_EQ(hit, 1);
    dap_free(dbg);
}

/* dap_check_actor_breakpoint */
TEST(DebuggerCov6, CheckActorBreakpoint) {
    DapDebugger *dbg = make_dap();
    dap_set_actor_breakpoint(dbg, 1);
    int hit = dap_check_actor_breakpoint(dbg, 1);
    EXPECT_EQ(hit, 1);
    dap_free(dbg);
}

/* dap_check_message_breakpoint */
TEST(DebuggerCov6, CheckMessageBreakpoint) {
    DapDebugger *dbg = make_dap();
    dap_set_message_breakpoint(dbg, "greet");
    int hit = dap_check_message_breakpoint(dbg, "greet");
    EXPECT_EQ(hit, 1);
    dap_free(dbg);
}

/* dap_list_breakpoints */
TEST(DebuggerCov6, ListBreakpoints) {
    DapDebugger *dbg = make_dap();
    dap_set_breakpoint(dbg, "test.pny", 10);
    dap_set_breakpoint(dbg, "test.pny", 20);
    DapBreakpoint *bp = dap_list_breakpoints(dbg);
    EXPECT_NE(bp, nullptr);
    dap_free(dbg);
}

/* dap_get_stack_trace */
TEST(DebuggerCov6, GetStackTrace) {
    DapDebugger *dbg = make_dap();
    int count = 0;
    DapStackFrame *frames = dap_get_stack_trace(dbg, &count);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(frames, nullptr);
    dap_free(dbg);
}

/* dap_get_variables */
TEST(DebuggerCov6, GetVariables) {
    DapDebugger *dbg = make_dap();
    int count = 0;
    DapVariable *vars = dap_get_variables(dbg, 0, &count);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(vars, nullptr);
    dap_free(dbg);
}

/* dap_get_state initial */
TEST(DebuggerCov6, GetStateInitial) {
    DapDebugger *dbg = make_dap();
    DapState state = dap_get_state(dbg);
    EXPECT_EQ(state, DAP_STATE_IDLE);
    dap_free(dbg);
}

/* dap_get_breakpoint_count initial */
TEST(DebuggerCov6, GetBreakpointCountInitial) {
    DapDebugger *dbg = make_dap();
    EXPECT_EQ(dap_get_breakpoint_count(dbg), 0);
    dap_free(dbg);
}

/* dap_run with empty input */
TEST(DebuggerCov6, RunEmpty) {
    DapDebugger *dbg = make_dap();
    int r = dap_run(dbg);
    EXPECT_NE(r, -1);
    dap_free(dbg);
}

/* dap_process_message with no input */
TEST(DebuggerCov6, ProcessNoInput) {
    DapDebugger *dbg = make_dap();
    int r = dap_process_message(dbg);
    EXPECT_EQ(r, -1);
    dap_free(dbg);
}

/* dap_notify_breakpoint with null file */
TEST(DebuggerCov6, NotifyBreakpointNullFile) {
    DapDebugger *dbg = make_dap();
    dap_notify_breakpoint(dbg, NULL, 10);
    dap_free(dbg);
}

/* dap_notify_actor_created with null name */
TEST(DebuggerCov6, NotifyActorCreatedNullName) {
    DapDebugger *dbg = make_dap();
    dap_notify_actor_created(dbg, 1, NULL);
    dap_free(dbg);
}

/* dap_notify_message with null method */
TEST(DebuggerCov6, NotifyMessageNullMethod) {
    DapDebugger *dbg = make_dap();
    dap_notify_message(dbg, 1, 2, NULL);
    dap_free(dbg);
}

/* 多次 setBreakpoint 同一文件不同行 */
TEST(DebuggerCov6, MultipleBreakpointsSameFile) {
    DapDebugger *dbg = make_dap();
    for (int i = 1; i <= 10; i++) {
        dap_set_breakpoint(dbg, "test.pny", i);
    }
    EXPECT_EQ(dap_get_breakpoint_count(dbg), 10);
    dap_free(dbg);
}

/* 多个文件的断点 */
TEST(DebuggerCov6, MultipleFilesBreakpoints) {
    DapDebugger *dbg = make_dap();
    dap_set_breakpoint(dbg, "a.pny", 1);
    dap_set_breakpoint(dbg, "b.pny", 2);
    dap_set_breakpoint(dbg, "c.pny", 3);
    EXPECT_EQ(dap_get_breakpoint_count(dbg), 3);
    EXPECT_EQ(dap_check_breakpoint(dbg, "a.pny", 1), 1);
    dap_free(dbg);
}

/* dap_notify_actor_created 多次 */
TEST(DebuggerCov6, NotifyActorCreatedMultiple) {
    DapDebugger *dbg = make_dap();
    for (int i = 1; i <= 5; i++) {
        char name[32];
        snprintf(name, sizeof(name), "actor_%d", i);
        dap_notify_actor_created(dbg, i, name);
    }
    int count = 0;
    DapActorInfo *actors = dap_get_actors(dbg, &count);
    EXPECT_EQ(count, 5);
    EXPECT_NE(actors, nullptr);
    dap_free(dbg);
}

/* dap_notify_message 多次 */
TEST(DebuggerCov6, NotifyMessageMultiple) {
    DapDebugger *dbg = make_dap();
    for (int i = 1; i <= 5; i++) {
        char method[32];
        snprintf(method, sizeof(method), "method_%d", i);
        dap_notify_message(dbg, i, i+1, method);
    }
    int count = 0;
    DapMessageInfo *msgs = dap_get_messages(dbg, -1, &count);
    EXPECT_GE(count, 5);
    EXPECT_NE(msgs, nullptr);
    dap_free(dbg);
}
