#ifndef PONYPP_LEXER_H
#define PONYPP_LEXER_H

#include "ponypp.h"
#include "ast.h"

/**
 * 初始化词法分析器
 */
Lexer *lexer_new(const char *filename, const char *source, size_t length);

/**
 * 释放词法分析器
 */
void lexer_free(Lexer *lex);

/**
 * 获取下一个 Token
 * @return Token 类型，EOF 时返回 TK_EOF
 */
TokenType lexer_next(Lexer *lex);

/**
 * 获取当前 Token 的值
 */
Token *lexer_current(Lexer *lex);

/**
 * 获取当前行号
 */
int lexer_line(const Lexer *lex);

/**
 * 获取当前列号
 */
int lexer_column(const Lexer *lex);

/**
 * 错误处理
 */
const char *lexer_error(Lexer *lex);

/**
 * 一次性词法分析，返回所有 Token
 * @return true 成功，false 失败
 */
bool lexer_lex_all(Lexer *lex, Token **tokens, size_t *token_count);

#endif /* PONYPP_LEXER_H */