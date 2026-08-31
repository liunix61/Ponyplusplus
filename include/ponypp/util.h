#ifndef PONYPP_UTIL_H
#define PONYPP_UTIL_H

#include "ponypp.h"

/* 内存工具 */
char *s_malloc(size_t size);
void s_free(void *ptr);
char *s_strdup(const char *s);
char *s_strndup(const char *s, size_t n);
void *s_realloc(void *ptr, size_t size);
size_t s_strlen(const char *s);
int s_strcmp(const char *a, const char *b);
void *s_memcpy(void *dst, const void *src, size_t n);
void *s_memset(void *dst, int c, size_t n);
char *s_strcpy(char *dst, const char *src);
char *s_strcat(char *dst, const char *src);
size_t s_strlcpy(char *dst, const char *src, size_t n);

/* 文件工具 */
char *s_file_read(const char *path);
int s_file_write(const char *path, const char *data, size_t len);

/* 输出路径解析 */
char *s_resolve_output(const char *output, const char *ext);

/* Token 显示 */
void token_print(const Token *tok, FILE *out);
const char *token_type_name(TokenType type);
const char *capability_kind_name(CapabilityKind cap);

/* 类型工具 */
const char *type_kind_name(TypeKind kind);

/* AST 工具 */
void ast_node_print(const ASTNode *node, FILE *out);
void ast_node_print_dot(const ASTNode *node, FILE *out);

#endif /* PONYPP_UTIL_H */
