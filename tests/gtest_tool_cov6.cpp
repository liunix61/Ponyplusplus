#include <gtest/gtest.h>
#include <ponypp/tool.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

/* 通过管道喂 stdin 测试 REPL */
static void test_repl_input(const char *input) {
    /* 创建管道 */
    int pipefd[2];
    if (pipe(pipefd) != 0) return;
    
    /* 写入输入 */
    write(pipefd[1], input, strlen(input));
    close(pipefd[1]);
    
    /* 重定向 stdin */
    int saved_stdin = dup(STDIN_FILENO);
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);
    
    /* 运行 REPL */
    tool_repl();
    
    /* 恢复 stdin */
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
}

/* ==================== REPL 命令 ==================== */

TEST(ToolCov6, ReplQuit) {
    test_repl_input(":quit\n");
}

TEST(ToolCov6, ReplShortQuit) {
    test_repl_input(":q\n");
}

TEST(ToolCov6, ReplExit) {
    test_repl_input(":exit\n");
}

TEST(ToolCov6, ReplHelp) {
    test_repl_input(":help\n:quit\n");
}

TEST(ToolCov6, ReplShortHelp) {
    test_repl_input(":h\n:quit\n");
}

TEST(ToolCov6, ReplClear) {
    test_repl_input(":clear\n:quit\n");
}

TEST(ToolCov6, ReplShortClear) {
    test_repl_input(":c\n:quit\n");
}

TEST(ToolCov6, ReplHistory) {
    test_repl_input(":history\n:quit\n");
}

TEST(ToolCov6, ReplShortHistory) {
    test_repl_input(":hist\n:quit\n");
}

TEST(ToolCov6, ReplReset) {
    test_repl_input(":reset\n:quit\n");
}

TEST(ToolCov6, ReplUnknownCommand) {
    test_repl_input(":unknown\n:quit\n");
}

TEST(ToolCov6, ReplEmptyInput) {
    test_repl_input("\n:quit\n");
}

TEST(ToolCov6, ReplCode) {
    test_repl_input("let x = 42\n:quit\n");
}

TEST(ToolCov6, ReplMultipleCommands) {
    test_repl_input(":help\n:history\n:quit\n");
}

/* ==================== tool_bootstrap ==================== */

TEST(ToolCov6, Bootstrap) {
    /* tool_bootstrap 会尝试引导编译器 */
    /* 由于需要完整的编译环境, 只测试不崩溃 */
    /* 实际测试中跳过 */
}
