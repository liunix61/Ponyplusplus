#ifndef PONYPP_CAPABILITIES_H
#define PONYPP_CAPABILITIES_H

#include "ponypp.h"
#include "ponypp/ast.h"

/* 验证 AST 的引用能力 */
bool cap_verify_ast(const ASTNode *ast);

/* 获取最后错误信息 */
const char *cap_last_error(void);

/* 获取最后错误行号 */
int cap_last_line(void);

#endif /* PONYPP_CAPABILITIES_H */
