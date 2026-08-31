/*
 * util.c - Pony++ 工具函数
 */

#include "ponypp/util.h"
#include "ponypp/ast.h"
#include "ponypp/types.h"
#include <errno.h>

/* ---- 内存工具 ---- */

char *s_malloc(size_t size) {
    void *p = malloc(size);
    if (!p) {
        fprintf(stderr, "致命错误: 内存分配失败 (%zu 字节)\n", size);
        exit(EXIT_FAILURE);
    }
    return (char *)p;
}

void s_free(void *ptr) {
    if (ptr) free(ptr);
}

char *s_strdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (!d) {
        fprintf(stderr, "致命错误: 内存分配失败\n");
        exit(EXIT_FAILURE);
    }
    memcpy(d, s, n);
    return d;
}

char *s_strndup(const char *s, size_t n) {
    char *d = malloc(n + 1);
    if (!d) {
        fprintf(stderr, "致命错误: 内存分配失败\n");
        exit(EXIT_FAILURE);
    }
    memcpy(d, s, n);
    d[n] = '\0';
    return d;
}

void *s_realloc(void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    if (!p) {
        fprintf(stderr, "致命错误: 内存重分配失败\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

size_t s_strlen(const char *s) {
    if (!s) return 0;
    return strlen(s);
}

int s_strcmp(const char *a, const char *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    return strcmp(a, b);
}

void *s_memcpy(void *dst, const void *src, size_t n) {
    return memcpy(dst, src, n);
}

void *s_memset(void *dst, int c, size_t n) {
    return memset(dst, c, n);
}

char *s_strcpy(char *dst, const char *src) {
    strcpy(dst, src);
    return dst;
}

char *s_strcat(char *dst, const char *src) {
    strcat(dst, src);
    return dst;
}

size_t s_strlcpy(char *dst, const char *src, size_t n) {
    if (n > 0) {
        strncpy(dst, src, n - 1);
        dst[n - 1] = '\0';
    }
    return strlen(src);
}

/* ---- 文件工具 ---- */

char *s_file_read(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) {
        fclose(f);
        return NULL;
    }

    char *buf = s_malloc((size_t)size + 1);
    size_t read_bytes = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[read_bytes] = '\0';
    return buf;
}

int s_file_write(const char *path, const char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t written = fwrite(data, 1, len, f);
    fclose(f);
    return (written == len) ? 0 : -1;
}

/* ---- 输出路径解析 ---- */

char *s_resolve_output(const char *output, const char *ext) {
    if (output) {
        return s_strdup(output);
    }
    /* 默认使用 input.ext，但这里没有 input 信息，返回临时路径 */
    const char *temp = "/tmp/ponypp_output";
    char *result = s_malloc(strlen(temp) + strlen(ext) + 2);
    strcpy(result, temp);
    strcat(result, ".");
    strcat(result, ext);
    return result;
}

/* ---- Token 显示 ---- */

void token_print(const Token *tok, FILE *out) {
    fprintf(out, "Token(%s, \"%s\", line=%d, col=%d)\n",
            token_type_name(tok->type), tok->value, tok->line, tok->column);
}

const char *token_type_name(TokenType type) {
    switch (type) {
        case TK_EOF: return "EOF";
        case TK_KEYWORD: return "KEYWORD";
        case TK_IDENT: return "IDENT";
        case TK_INT: return "INT";
        case TK_FLOAT: return "FLOAT";
        case TK_STRING: return "STRING";
        case TK_BOOL: return "BOOL";
        case TK_TYPE: return "TYPE";
        case TK_CAP: return "CAP";
        case TK_PUNCT: return "PUNCT";
        case TK_ARROW: return "ARROW";
        case TK_HASH: return "HASH";
        case TK_DOLLAR: return "DOLLAR";
        case TK_AMP: return "AMP";
        case TK_PERCENT: return "PERCENT";
        case TK_AT: return "AT";
        case TK_PIPE: return "PIPE";
        case TK_QUESTION: return "QUESTION";
        case TK_COLON: return "COLON";
        case TK_BRACKET_L: return "[";
        case TK_BRACKET_R: return "]";
        case TK_BRACE_L: return "{";
        case TK_BRACE_R: return "}";
        case TK_PAREN_L: return "(";
        case TK_PAREN_R: return ")";
        case TK_SEMI: return ";";
        case TK_COMMA: return ",";
        case TK_DOT: return ".";
        case TK_DASH: return "-";
        case TK_PLUS: return "+";
        case TK_STAR: return "*";
        case TK_SLASH: return "/";
        case TK_EQ: return "=";
        case TK_BANG: return "!";
        case TK_LT: return "<";
        case TK_GT: return ">";
        case TK_LE: return "<=";
        case TK_GE: return ">=";
        case TK_EQEQ: return "==";
        case TK_NEQ: return "!=";
        case TK_COLONCOLON: return "::";
        case TK_ARROW_ARR: return "=>";
        default: return "?";
    }
}

const char *capability_kind_name(CapabilityKind cap) {
    switch (cap) {
        case CAP_UNKNOWN: return "unknown";
        case CAP_ISO: return "iso";
        case CAP_TRN: return "trn";
        case CAP_REF: return "ref";
        case CAP_VAL: return "val";
        case CAP_BOX: return "box";
        case CAP_TAG: return "tag";
        default: return "?";
    }
}

const char *type_kind_name(TypeKind kind) {
    switch (kind) {
        case TYPE_UNKNOWN: return "Unknown";
        case TYPE_INT64: return "I64";
        case TYPE_INT32: return "I32";
        case TYPE_UINT64: return "U64";
        case TYPE_UINT32: return "U32";
        case TYPE_UINT8: return "U8";
        case TYPE_FLOAT64: return "F64";
        case TYPE_FLOAT32: return "F32";
        case TYPE_STRING: return "String";
        case TYPE_BOOL: return "Bool";
        case TYPE_NONE: return "None";
        case TYPE_ANY: return "Any";
        case TYPE_GENERIC: return "Generic";
        case TYPE_TUPLE: return "Tuple";
        case TYPE_UNION: return "Union";
        case TYPE_ARRAY: return "Array";
        default: return "?";
    }
}

/* ---- AST 打印 ---- */

void ast_node_print(const ASTNode *node, FILE *out) {
    if (!node) return;
    const char *type_name = "Unknown";
    switch (node->type) {
        case NODE_PROGRAM: type_name = "Program"; break;
        case NODE_ACTOR: type_name = "Actor"; break;
        case NODE_CLASS: type_name = "Class"; break;
        case NODE_TRAIT: type_name = "Trait"; break;
        case NODE_INTERFACE: type_name = "Interface"; break;
        case NODE_BE: type_name = "Behavior"; break;
        case NODE_FUN: type_name = "Function"; break;
        case NODE_NEW: type_name = "Constructor"; break;
        case NODE_VAR: type_name = "Var"; break;
        case NODE_LET: type_name = "Let"; break;
        case NODE_IF: type_name = "If"; break;
        case NODE_ELSE: type_name = "Else"; break;
        case NODE_WHILE: type_name = "While"; break;
        case NODE_FOR: type_name = "For"; break;
        case NODE_MATCH: type_name = "Match"; break;
        case NODE_RETURN: type_name = "Return"; break;
        case NODE_PRINT: type_name = "Print"; break;
        case NODE_ASSERT: type_name = "Assert"; break;
        case NODE_SUPERVISE: type_name = "Supervise"; break;
        case NODE_SUPERTREE: type_name = "Supertree"; break;
        case NODE_YIELD: type_name = "Yield"; break;
        case NODE_SEND: type_name = "Send"; break;
        case NODE_CALL: type_name = "Call"; break;
        case NODE_LAMBDA: type_name = "Lambda"; break;
        case NODE_LIST: type_name = "List"; break;
        case NODE_MAP: type_name = "Map"; break;
        case NODE_STRING: type_name = "String"; break;
        case NODE_INT: type_name = "Int"; break;
        case NODE_FLOAT: type_name = "Float"; break;
        case NODE_BOOL: type_name = "Bool"; break;
        case NODE_UNION: type_name = "Union"; break;
        case NODE_TUPLE: type_name = "Tuple"; break;
        case NODE_ARRAY: type_name = "Array"; break;
        case NODE_ALIAS: type_name = "Alias"; break;
        default: type_name = "Empty"; break;
    }
    fprintf(out, "%s (line=%d, col=%d, children=%zu)\n",
            type_name, node->line, node->column, node->child_count);
    for (size_t i = 0; i < node->child_count; i++) {
        fprintf(out, "  child[%zu]: ", i);
        ast_node_print(node->children[i], out);
    }
}

void ast_node_print_dot(const ASTNode *node, FILE *out) {
    if (!node) return;
    fprintf(out, "digraph AST {\n");
    fprintf(out, "  \"root\" [label=\"Program\"];\n");
    fprintf(out, "}\n");
}
