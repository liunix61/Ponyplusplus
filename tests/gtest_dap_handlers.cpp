#include <gtest/gtest.h>
#include <ponypp/debugger.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

class DapHandlerTest : public ::testing::Test {
protected:
    FILE *in_file;
    FILE *out_file;
    DapDebugger *dbg;
    
    void SetUp() override {
        in_file = tmpfile();
        out_file = tmpfile();
        dbg = dap_new(in_file, out_file);
    }
    
    void TearDown() override {
        if (dbg) dap_free(dbg);
        if (in_file) fclose(in_file);
        if (out_file) fclose(out_file);
    }
    
    /* 发送 DAP 消息并处理 */
    void send_dap_message(const char *command, int seq = 1) {
        char msg[1024];
        snprintf(msg, sizeof(msg),
            "Content-Length: %zu\r\n\r\n"
            "{\"seq\":%d,\"type\":\"request\",\"command\":\"%s\"}",
            strlen(command) + 30, seq, command);
        
        rewind(in_file);
        fprintf(in_file, "%s", msg);
        rewind(in_file);
        
        dap_process_message(dbg);
    }
};

/* ==================== 断点管理 ==================== */

TEST_F(DapHandlerTest, SetBreakpoint) {
    int id = dap_set_breakpoint(dbg, "test.pony", 10);
    EXPECT_GT(id, 0);
}

TEST_F(DapHandlerTest, SetMultipleBreakpoints) {
    int id1 = dap_set_breakpoint(dbg, "test.pony", 10);
    int id2 = dap_set_breakpoint(dbg, "test.pony", 20);
    int id3 = dap_set_breakpoint(dbg, "other.pony", 5);
    
    EXPECT_GT(id1, 0);
    EXPECT_GT(id2, 0);
    EXPECT_GT(id3, 0);
    EXPECT_NE(id1, id2);
}

TEST_F(DapHandlerTest, SetBreakpointNullArgs) {
    EXPECT_LE(dap_set_breakpoint(dbg, nullptr, 10), 0);
    EXPECT_LE(dap_set_breakpoint(nullptr, "test.pony", 10), 0);
}

TEST_F(DapHandlerTest, SetFunctionBreakpoint) {
    int id = dap_set_function_breakpoint(dbg, "my_function");
    EXPECT_GT(id, 0);
    
    EXPECT_LE(dap_set_function_breakpoint(dbg, nullptr), 0);
    EXPECT_LE(dap_set_function_breakpoint(nullptr, "func"), 0);
}

TEST_F(DapHandlerTest, SetActorBreakpoint) {
    int id = dap_set_actor_breakpoint(dbg, 1);
    EXPECT_GT(id, 0);
    
    EXPECT_LE(dap_set_actor_breakpoint(nullptr, 1), 0);
}

TEST_F(DapHandlerTest, SetMessageBreakpoint) {
    int id = dap_set_message_breakpoint(dbg, "do_work");
    EXPECT_GT(id, 0);
    
    EXPECT_LE(dap_set_message_breakpoint(dbg, nullptr), 0);
    EXPECT_LE(dap_set_message_breakpoint(nullptr, "method"), 0);
}

TEST_F(DapHandlerTest, RemoveBreakpoint) {
    int id = dap_set_breakpoint(dbg, "test.pony", 10);
    ASSERT_GT(id, 0);
    
    EXPECT_EQ(dap_remove_breakpoint(dbg, id), 0);
    EXPECT_NE(dap_remove_breakpoint(dbg, 9999), 0);
    EXPECT_NE(dap_remove_breakpoint(nullptr, id), 0);
}

TEST_F(DapHandlerTest, EnableDisableBreakpoint) {
    int id = dap_set_breakpoint(dbg, "test.pony", 10);
    ASSERT_GT(id, 0);
    
    EXPECT_EQ(dap_enable_breakpoint(dbg, id, 0), 0);  /* 禁用 */
    EXPECT_EQ(dap_enable_breakpoint(dbg, id, 1), 0);  /* 启用 */
    
    EXPECT_NE(dap_enable_breakpoint(nullptr, id, 1), 0);
}

/* ==================== 断点检查 ==================== */

TEST_F(DapHandlerTest, CheckBreakpoint) {
    dap_set_breakpoint(dbg, "test.pony", 10);
    
    EXPECT_TRUE(dap_check_breakpoint(dbg, "test.pony", 10));
    EXPECT_FALSE(dap_check_breakpoint(dbg, "test.pony", 20));
    EXPECT_FALSE(dap_check_breakpoint(dbg, "other.pony", 10));
    
    EXPECT_FALSE(dap_check_breakpoint(nullptr, "test.pony", 10));
}

TEST_F(DapHandlerTest, CheckActorBreakpoint) {
    dap_set_actor_breakpoint(dbg, 1);
    
    EXPECT_TRUE(dap_check_actor_breakpoint(dbg, 1));
    EXPECT_FALSE(dap_check_actor_breakpoint(dbg, 2));
    
    EXPECT_FALSE(dap_check_actor_breakpoint(nullptr, 1));
}

TEST_F(DapHandlerTest, CheckMessageBreakpoint) {
    dap_set_message_breakpoint(dbg, "do_work");
    
    EXPECT_TRUE(dap_check_message_breakpoint(dbg, "do_work"));
    EXPECT_FALSE(dap_check_message_breakpoint(dbg, "other_method"));
    
    EXPECT_FALSE(dap_check_message_breakpoint(nullptr, "do_work"));
}

/* ==================== 执行控制 ==================== */

TEST_F(DapHandlerTest, Continue) {
    EXPECT_EQ(dap_continue(dbg), 0);
    EXPECT_NE(dap_continue(nullptr), 0);
}

TEST_F(DapHandlerTest, StepOver) {
    EXPECT_EQ(dap_step_over(dbg), 0);
    EXPECT_NE(dap_step_over(nullptr), 0);
}

TEST_F(DapHandlerTest, StepInto) {
    EXPECT_EQ(dap_step_into(dbg), 0);
    EXPECT_NE(dap_step_into(nullptr), 0);
}

TEST_F(DapHandlerTest, StepOut) {
    EXPECT_EQ(dap_step_out(dbg), 0);
    EXPECT_NE(dap_step_out(nullptr), 0);
}

TEST_F(DapHandlerTest, Pause) {
    EXPECT_EQ(dap_pause(dbg), 0);
    EXPECT_NE(dap_pause(nullptr), 0);
}

/* ==================== 堆栈/变量 ==================== */

TEST_F(DapHandlerTest, GetStackTrace) {
    int count = 0;
    DapStackFrame *frames = dap_get_stack_trace(dbg, &count);
    /* 可能返回 NULL 或空 */
    (void)frames;
    
    EXPECT_EQ(dap_get_stack_trace(nullptr, &count), nullptr);
}

TEST_F(DapHandlerTest, GetVariables) {
    int count = 0;
    DapVariable *vars = dap_get_variables(dbg, 1, &count);
    (void)vars;
    
    EXPECT_EQ(dap_get_variables(nullptr, 1, &count), nullptr);
}

/* ==================== 空值安全 ==================== */

TEST(DapNullSafety, AllNull) {
    dap_free(nullptr);
    
    EXPECT_LE(dap_set_breakpoint(nullptr, "f", 1), 0);
    EXPECT_LE(dap_set_function_breakpoint(nullptr, "f"), 0);
    EXPECT_LE(dap_set_actor_breakpoint(nullptr, 1), 0);
    EXPECT_LE(dap_set_message_breakpoint(nullptr, "m"), 0);
    EXPECT_NE(dap_remove_breakpoint(nullptr, 1), 0);
    EXPECT_NE(dap_enable_breakpoint(nullptr, 1, 1), 0);
    EXPECT_FALSE(dap_check_breakpoint(nullptr, "f", 1));
    EXPECT_FALSE(dap_check_actor_breakpoint(nullptr, 1));
    EXPECT_FALSE(dap_check_message_breakpoint(nullptr, "m"));
    EXPECT_NE(dap_continue(nullptr), 0);
    EXPECT_NE(dap_step_over(nullptr), 0);
    EXPECT_NE(dap_step_into(nullptr), 0);
    EXPECT_NE(dap_step_out(nullptr), 0);
    EXPECT_NE(dap_pause(nullptr), 0);
}
