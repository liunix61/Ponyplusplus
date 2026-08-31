#ifndef PONYPP_TYPECHECK_H
#define PONYPP_TYPECHECK_H

#include "ponypp.h"
#include "ponypp/ast.h"
#include "ponypp/types.h"

/* 检查 AST 类型 */
bool typecheck_check_ast(const ASTNode *ast, TypeContext *ctx);

/* 获取最后错误信息 */
const char *typecheck_last_error(TypeContext *ctx);

/* 获取最后错误行号 */
int typecheck_last_line(TypeContext *ctx);

#endif /* PONYPP_TYPECHECK_H */
