#ifndef PONYPP_PARSER_H
#define PONYPP_PARSER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "ponypp.h"
#include "ponypp/ast.h"

/* 初始化语法分析器 */
Parser *parser_new(const char *filename, Token *tokens, size_t token_count);

/* 释放语法分析器 */
void parser_free(Parser *p);

/* 解析整个程序 */
ASTNode *parser_parse_program(Parser *p);

/* 获取当前行号 */
int parser_line(Parser *p);

/* 获取错误信息 */
const char *parser_error(Parser *p);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_PARSER_H */
