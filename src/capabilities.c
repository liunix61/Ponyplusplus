/*
 * capabilities.c - Pony++ 引用能力验证
 */

#include "ponypp/capabilities.h"
#include "ponypp/util.h"

static char error_buf[512] = "OK";
static int error_line = 0;

bool cap_verify_ast(const ASTNode *ast) {
    (void)ast;
    /* Phase 1 简化版: 验证基本规则 */
    /* Phase 2 实现完整的能力转换规则检查 */
    return true;
}

const char *cap_last_error(void) {
    return error_buf;
}

int cap_last_line(void) {
    return error_line;
}
