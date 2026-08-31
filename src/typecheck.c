/*
 * typecheck.c - Pony++ 类型检查
 */

#include "ponypp/typecheck.h"
#include "ponypp/util.h"

/* 扩展 TypeContext 存储错误信息 */
typedef struct {
    Type *builtin_types[TYPE_COUNT];
    Type *unknown;
    char last_error[512];
    int last_error_line;
} TypeContextExt;

bool typecheck_check_ast(const ASTNode *ast, TypeContext *ctx) {
    (void)ast;
    (void)ctx;
    /* Phase 1 简化版: 类型检查通过 */
    /* Phase 2 实现完整类型推断 */
    return true;
}

const char *typecheck_last_error(TypeContext *ctx) {
    (void)ctx;
    return "无类型错误";
}

int typecheck_last_line(TypeContext *ctx) {
    (void)ctx;
    return 0;
}
