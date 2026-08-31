/*
 * lexer.c - Pony++ 词法分析器
 */

#include "ponypp/lexer.h"
#include "ponypp/util.h"
#include <ctype.h>

/* Lexer 结构 */
struct Lexer {
    const char *filename;
    const char *source;
    size_t length;
    size_t pos;
    int line;
    int column;
    char error[256];
    Token current;
    bool has_error;
};

/* 前向声明 */
static TokenType advance(Lexer *lex);
static char *lex_ident(Lexer *lex);
static char *lex_number(Lexer *lex);
static char *lex_string(Lexer *lex);

/* 关键字表 */
static const struct {
    const char *word;
    TokenType type;
} keywords[] = {
    {"actor", TK_KEYWORD}, {"class", TK_KEYWORD}, {"trait", TK_KEYWORD},
    {"be", TK_KEYWORD}, {"fun", TK_KEYWORD}, {"new", TK_KEYWORD},
    {"var", TK_KEYWORD}, {"let", TK_KEYWORD},
    {"if", TK_KEYWORD}, {"else", TK_KEYWORD}, {"while", TK_KEYWORD},
    {"for", TK_KEYWORD}, {"match", TK_KEYWORD}, {"return", TK_KEYWORD},
    {"supervise", TK_KEYWORD}, {"supertree", TK_KEYWORD},
    {"import", TK_KEYWORD}, {"use", TK_KEYWORD}, {"as", TK_KEYWORD},
    {"and", TK_KEYWORD}, {"or", TK_KEYWORD}, {"not", TK_KEYWORD},
    {"is", TK_KEYWORD}, {"in", TK_KEYWORD}, {"where", TK_KEYWORD},
    {"then", TK_KEYWORD}, {"try", TK_KEYWORD}, {"catch", TK_KEYWORD},
    {"finally", TK_KEYWORD}, {"throw", TK_KEYWORD},
    {"true", TK_BOOL}, {"false", TK_BOOL}, {"None", TK_TYPE},
    /* 能力 */
    {"iso", TK_CAP}, {"trn", TK_CAP}, {"ref", TK_CAP},
    {"val", TK_CAP}, {"box", TK_CAP}, {"tag", TK_CAP},
    /* 类型 */
    {"U64", TK_TYPE}, {"U32", TK_TYPE}, {"U16", TK_TYPE}, {"U8", TK_TYPE},
    {"I64", TK_TYPE}, {"I32", TK_TYPE}, {"I16", TK_TYPE}, {"I8", TK_TYPE},
    {"F64", TK_TYPE}, {"F32", TK_TYPE},
    {"String", TK_TYPE}, {"Bool", TK_TYPE}, {"Any", TK_TYPE},
    {"AnyType", TK_TYPE}, {"NoneType", TK_TYPE},
    {"Reply", TK_TYPE}, {"List", TK_TYPE}, {"Set", TK_TYPE},
    {"Array", TK_TYPE}, {"Option", TK_TYPE},
};

static TokenType lookup_keyword(const char *word, size_t len) {
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strlen(keywords[i].word) == len &&
            strncmp(keywords[i].word, word, len) == 0) {
            return keywords[i].type;
        }
    }
    return TK_IDENT;
}

Lexer *lexer_new(const char *filename, const char *source, size_t length) {
    Lexer *lex = (Lexer *)calloc(1, sizeof(Lexer));
    if (!lex) return NULL;
    lex->filename = filename;
    lex->source = source;
    lex->length = length;
    lex->pos = 0;
    lex->line = 1;
    lex->column = 1;
    lex->has_error = false;
    lex->current.type = TK_EOF;
    lex->current.value = NULL;
    lex->current.line = 1;
    lex->current.column = 1;
    lex->current.length = 0;
    strcpy(lex->error, "OK");
    return lex;
}

void lexer_free(Lexer *lex) {
    if (!lex) return;
    if (lex->current.value) free(lex->current.value);
    free(lex);
}

int lexer_line(const Lexer *lex) { return lex->line; }
int lexer_column(const Lexer *lex) { return lex->column; }
const char *lexer_error(Lexer *lex) { return lex->error; }

static void set_error(Lexer *lex, const char *msg) {
    snprintf(lex->error, sizeof(lex->error),
             "%s (第 %d 行, 第 %d 列)", msg, lex->line, lex->column);
    lex->has_error = true;
}

static char peek(Lexer *lex) {
    if (lex->pos >= lex->length) return '\0';
    return lex->source[lex->pos];
}

static char peek_next(Lexer *lex) {
    if (lex->pos + 1 >= lex->length) return '\0';
    return lex->source[lex->pos + 1];
}

static void advance_char(Lexer *lex) {
    if (lex->pos < lex->length) {
        if (lex->source[lex->pos] == '\n') {
            lex->line++;
            lex->column = 1;
        } else {
            lex->column++;
        }
        lex->pos++;
    }
}

static void skip_whitespace(Lexer *lex) {
    while (lex->pos < lex->length) {
        char c = lex->source[lex->pos];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance_char(lex);
        } else if (c == '/' && peek_next(lex) == '/') {
            /* 行注释 */
            while (lex->pos < lex->length && lex->source[lex->pos] != '\n') {
                advance_char(lex);
            }
        } else if (c == '/' && peek_next(lex) == '*') {
            /* 块注释 */
            advance_char(lex); advance_char(lex);
            while (lex->pos < lex->length &&
                   !(lex->source[lex->pos] == '*' && peek_next(lex) == '/')) {
                advance_char(lex);
            }
            if (lex->pos < lex->length) {
                advance_char(lex); advance_char(lex);
            }
        } else if (c == '#') {
            /* 预处理指令行 */
            while (lex->pos < lex->length && lex->source[lex->pos] != '\n') {
                advance_char(lex);
            }
        } else {
            break;
        }
    }
}

static char *lex_ident(Lexer *lex) {
    size_t start = lex->pos;
    while (lex->pos < lex->length && (isalnum((unsigned char)lex->source[lex->pos]) ||
           lex->source[lex->pos] == '_')) {
        advance_char(lex);
    }
    size_t len = lex->pos - start;
    return s_strndup(lex->source + start, len);
}

static char *lex_number(Lexer *lex) {
    size_t start = lex->pos;
    bool is_float = false;
    while (lex->pos < lex->length) {
        char c = lex->source[lex->pos];
        if (isdigit((unsigned char)c)) {
            advance_char(lex);
        } else if (c == '.' && !is_float) {
            is_float = true;
            advance_char(lex);
        } else if (c == 'x' || c == 'X') {
            advance_char(lex);
            while (lex->pos < lex->length && isxdigit((unsigned char)lex->source[lex->pos])) {
                advance_char(lex);
            }
        } else if (c == '_' && start < lex->pos - 1) {
            advance_char(lex);
        } else {
            break;
        }
    }
    size_t len = lex->pos - start;
    return s_strndup(lex->source + start, len);
}

static char *lex_string(Lexer *lex) {
    char quote = lex->source[lex->pos];
    advance_char(lex); /* 跳过引号 */
    char buf[4096];
    size_t i = 0;
    while (lex->pos < lex->length && lex->source[lex->pos] != quote) {
        char c = lex->source[lex->pos];
        if (c == '\\' && lex->pos + 1 < lex->length) {
            advance_char(lex);
            char esc = lex->source[lex->pos];
            switch (esc) {
                case 'n': buf[i++] = '\n'; break;
                case 't': buf[i++] = '\t'; break;
                case 'r': buf[i++] = '\r'; break;
                case '\\': buf[i++] = '\\'; break;
                case '"': buf[i++] = '"'; break;
                case '\'': buf[i++] = '\''; break;
                default: buf[i++] = esc; break;
            }
        } else {
            buf[i++] = c;
        }
        if (i >= sizeof(buf) - 1) break;
        advance_char(lex);
    }
    if (lex->pos < lex->length) {
        advance_char(lex); /* 跳过结束引号 */
    }
    buf[i] = '\0';
    return s_strdup(buf);
}

static TokenType advance(Lexer *lex) {
    skip_whitespace(lex);
    if (lex->pos >= lex->length) {
        lex->current.type = TK_EOF;
        lex->current.line = lex->line;
        lex->current.column = lex->column;
        if (lex->current.value) { free(lex->current.value); lex->current.value = NULL; }
        return TK_EOF;
    }

    int start_line = lex->line;
    int start_col = lex->column;
    char c = lex->source[lex->pos];

    if (isalpha((unsigned char)c) || c == '_') {
        char *word = lex_ident(lex);
        TokenType kw_type = lookup_keyword(word, strlen(word));
        if (kw_type == TK_IDENT) {
            lex->current.type = TK_IDENT;
        } else {
            lex->current.type = kw_type;
        }
        lex->current.value = word;
        lex->current.line = start_line;
        lex->current.column = start_col;
        lex->current.length = (int)strlen(word);
        return lex->current.type;
    }

    if (isdigit((unsigned char)c)) {
        char *num = lex_number(lex);
        lex->current.type = TK_INT;
        lex->current.value = num;
        lex->current.line = start_line;
        lex->current.column = start_col;
        lex->current.length = (int)strlen(num);
        return TK_INT;
    }

    if (c == '"' || c == '\'') {
        char *str = lex_string(lex);
        lex->current.type = TK_STRING;
        lex->current.value = str;
        lex->current.line = start_line;
        lex->current.column = start_col;
        lex->current.length = (int)strlen(str);
        return TK_STRING;
    }

    /* 多字符运算符 */
    if (c == '=' && peek_next(lex) == '>') {
        advance_char(lex); advance_char(lex);
        lex->current.type = TK_ARROW_ARR;
        lex->current.value = s_strdup("=>");
        lex->current.line = start_line;
        lex->current.column = start_col;
        lex->current.length = 2;
        return TK_ARROW_ARR;
    }

    if (c == '=' && peek_next(lex) == '=') {
        advance_char(lex); advance_char(lex);
        lex->current.type = TK_EQEQ;
        lex->current.value = s_strdup("==");
        lex->current.line = start_line;
        lex->current.column = start_col;
        lex->current.length = 2;
        return TK_EQEQ;
    }

    if (c == '!' && peek_next(lex) == '=') {
        advance_char(lex); advance_char(lex);
        lex->current.type = TK_NEQ;
        lex->current.value = s_strdup("!=");
        lex->current.line = start_line;
        lex->current.column = start_col;
        lex->current.length = 2;
        return TK_NEQ;
    }

    if (c == '<' && peek_next(lex) == '=') {
        advance_char(lex); advance_char(lex);
        lex->current.type = TK_LE;
        lex->current.value = s_strdup("<=");
        lex->current.line = start_line;
        lex->current.column = start_col;
        lex->current.length = 2;
        return TK_LE;
    }

    if (c == '>' && peek_next(lex) == '=') {
        advance_char(lex); advance_char(lex);
        lex->current.type = TK_GE;
        lex->current.value = s_strdup(">=");
        lex->current.line = start_line;
        lex->current.column = start_col;
        lex->current.length = 2;
        return TK_GE;
    }

    if (c == ':' && peek_next(lex) == ':') {
        advance_char(lex); advance_char(lex);
        lex->current.type = TK_COLONCOLON;
        lex->current.value = s_strdup("::");
        lex->current.line = start_line;
        lex->current.column = start_col;
        lex->current.length = 2;
        return TK_COLONCOLON;
    }

    /* 单字符 */
    advance_char(lex);
    lex->current.value = s_malloc(2);
    lex->current.value[0] = c;
    lex->current.value[1] = '\0';
    lex->current.line = start_line;
    lex->current.column = start_col;
    lex->current.length = 1;

    switch (c) {
        case '(': lex->current.type = TK_PAREN_L; return TK_PAREN_L;
        case ')': lex->current.type = TK_PAREN_R; return TK_PAREN_R;
        case '{': lex->current.type = TK_BRACE_L; return TK_BRACE_L;
        case '}': lex->current.type = TK_BRACE_R; return TK_BRACE_R;
        case '[': lex->current.type = TK_BRACKET_L; return TK_BRACKET_L;
        case ']': lex->current.type = TK_BRACKET_R; return TK_BRACKET_R;
        case '.': lex->current.type = TK_DOT; return TK_DOT;
        case ',': lex->current.type = TK_COMMA; return TK_COMMA;
        case ';': lex->current.type = TK_SEMI; return TK_SEMI;
        case ':': lex->current.type = TK_COLON; return TK_COLON;
        case '+': lex->current.type = TK_PLUS; return TK_PLUS;
        case '-': lex->current.type = TK_DASH; return TK_DASH;
        case '*': lex->current.type = TK_STAR; return TK_STAR;
        case '/': lex->current.type = TK_SLASH; return TK_SLASH;
        case '=': lex->current.type = TK_EQ; return TK_EQ;
        case '!': lex->current.type = TK_BANG; return TK_BANG;
        case '<': lex->current.type = TK_LT; return TK_LT;
        case '>': lex->current.type = TK_GT; return TK_GT;
        case '@': lex->current.type = TK_AT; return TK_AT;
        case '|': lex->current.type = TK_PIPE; return TK_PIPE;
        case '?': lex->current.type = TK_QUESTION; return TK_QUESTION;
        case '&': lex->current.type = TK_AMP; return TK_AMP;
        case '%': lex->current.type = TK_PERCENT; return TK_PERCENT;
        case '$': lex->current.type = TK_DOLLAR; return TK_DOLLAR;
        case '#': lex->current.type = TK_HASH; return TK_HASH;
        default:
            set_error(lex, "未知字符");
            lex->current.type = TK_PUNCT;
            return TK_PUNCT;
    }
}

TokenType lexer_next(Lexer *lex) {
    return advance(lex);
}

Token *lexer_current(Lexer *lex) {
    return &lex->current;
}

bool lexer_lex_all(Lexer *lex, Token **tokens, size_t *token_count) {
    size_t cap = 256;
    size_t count = 0;
    *tokens = (Token *)malloc(cap * sizeof(Token));
    if (!*tokens) return false;
    *token_count = 0;

    TokenType type;
    while ((type = advance(lex)) != TK_EOF) {
        if (lex->has_error) {
            /* 继续尝试解析 */
        }
        if (count >= cap) {
            cap *= 2;
            *tokens = (Token *)realloc(*tokens, cap * sizeof(Token));
            if (!*tokens) return false;
        }
        (*tokens)[count].type = lex->current.type;
        (*tokens)[count].value = lex->current.value;
        (*tokens)[count].line = lex->current.line;
        (*tokens)[count].column = lex->current.column;
        (*tokens)[count].length = lex->current.length;
        lex->current.value = NULL; /* 所有权转移 */
        count++;
    }
    *token_count = count;
    return !lex->has_error;
}
