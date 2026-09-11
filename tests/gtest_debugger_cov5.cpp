#include <gtest/gtest.h>
#include <ponypp/debugger.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

/* 通过管道喂 DAP 协议消息测试 dap_process_message */
static void test_dap_message(const char *json_msg) {
    /* 创建管道 */
    int pipefd[2];
    if (pipe(pipefd) != 0) return;
    
    /* 写入 DAP 协议消息 */
    char header[256];
    int content_len = strlen(json_msg);
    snprintf(header, sizeof(header), "Content-Length: %d\r\n\r\n", content_len);
    
    write(pipefd[1], header, strlen(header));
    write(pipefd[1], json_msg, content_len);
    close(pipefd[1]);
    
    /* 创建调试器 */
    FILE *in = fdopen(pipefd[0], "r");
    FILE *out = fopen("/dev/null", "w");
    if (!in || !out) {
        if (in) fclose(in);
        if (out) fclose(out);
        close(pipefd[0]);
        return;
    }
    
    DapDebugger *dbg = dap_new(in, out);
    if (dbg) {
        dap_process_message(dbg);
        dap_free(dbg);
    }
    fclose(in);
    fclose(out);
}

/* ==================== DAP 协议消息 ==================== */

TEST(DebugCov5, ProcessInitialize) {
    test_dap_message("{\"seq\":1,\"type\":\"request\",\"command\":\"initialize\",\"arguments\":{\"clientID\":\"test\"}}");
}

TEST(DebugCov5, ProcessLaunch) {
    test_dap_message("{\"seq\":2,\"type\":\"request\",\"command\":\"launch\",\"arguments\":{\"program\":\"test.pny\"}}");
}

TEST(DebugCov5, ProcessSetBreakpoints) {
    test_dap_message("{\"seq\":3,\"type\":\"request\",\"command\":\"setBreakpoints\",\"arguments\":{\"source\":{\"path\":\"test.pny\"},\"lines\":[10,20]}}");
}

TEST(DebugCov5, ProcessConfigurationDone) {
    test_dap_message("{\"seq\":4,\"type\":\"request\",\"command\":\"configurationDone\"}");
}

TEST(DebugCov5, ProcessContinue) {
    test_dap_message("{\"seq\":5,\"type\":\"request\",\"command\":\"continue\",\"arguments\":{\"threadId\":1}}");
}

TEST(DebugCov5, ProcessNext) {
    test_dap_message("{\"seq\":6,\"type\":\"request\",\"command\":\"next\",\"arguments\":{\"threadId\":1}}");
}

TEST(DebugCov5, ProcessStepIn) {
    test_dap_message("{\"seq\":7,\"type\":\"request\",\"command\":\"stepIn\",\"arguments\":{\"threadId\":1}}");
}

TEST(DebugCov5, ProcessStepOut) {
    test_dap_message("{\"seq\":8,\"type\":\"request\",\"command\":\"stepOut\",\"arguments\":{\"threadId\":1}}");
}

TEST(DebugCov5, ProcessStackTrace) {
    test_dap_message("{\"seq\":9,\"type\":\"request\",\"command\":\"stackTrace\",\"arguments\":{\"threadId\":1}}");
}

TEST(DebugCov5, ProcessVariables) {
    test_dap_message("{\"seq\":10,\"type\":\"request\",\"command\":\"variables\",\"arguments\":{\"variablesReference\":1}}");
}

TEST(DebugCov5, ProcessThreads) {
    test_dap_message("{\"seq\":11,\"type\":\"request\",\"command\":\"threads\"}");
}

TEST(DebugCov5, ProcessDisconnect) {
    test_dap_message("{\"seq\":12,\"type\":\"request\",\"command\":\"disconnect\"}");
}

TEST(DebugCov5, ProcessTerminate) {
    test_dap_message("{\"seq\":13,\"type\":\"request\",\"command\":\"terminate\"}");
}

TEST(DebugCov5, ProcessActors) {
    test_dap_message("{\"seq\":14,\"type\":\"request\",\"command\":\"actors\"}");
}

TEST(DebugCov5, ProcessMessages) {
    test_dap_message("{\"seq\":15,\"type\":\"request\",\"command\":\"messages\",\"arguments\":{\"actorId\":1}}");
}

TEST(DebugCov5, ProcessUnknownCommand) {
    test_dap_message("{\"seq\":16,\"type\":\"request\",\"command\":\"unknownCommand\"}");
}

/* ==================== 空消息 ==================== */

TEST(DebugCov5, ProcessEmptyMessage) {
    int pipefd[2];
    if (pipe(pipefd) != 0) return;
    close(pipefd[1]);  /* 立即关闭写端, 读端返回 EOF */
    
    FILE *in = fdopen(pipefd[0], "r");
    FILE *out = fopen("/dev/null", "w");
    if (!in || !out) {
        if (in) fclose(in);
        if (out) fclose(out);
        close(pipefd[0]);
        return;
    }
    
    DapDebugger *dbg = dap_new(in, out);
    if (dbg) {
        int ret = dap_process_message(dbg);
        EXPECT_EQ(ret, -1);
        dap_free(dbg);
    }
    fclose(in);
    fclose(out);
}

/* ==================== 无效消息 ==================== */

TEST(DebugCov5, ProcessInvalidJson) {
    test_dap_message("not json");
}

TEST(DebugCov5, ProcessMissingCommand) {
    test_dap_message("{\"seq\":1,\"type\":\"request\"}");
}
