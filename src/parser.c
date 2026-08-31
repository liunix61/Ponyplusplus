/*
 * parser.c - Pony++ 递归下降语法分析器
 */

#include "ponypp/parser.h"
#include "ponypp/util.h"

/* Parser 结构 */
struct Parser {
    const char *filename;
    Token *tokens;
    size_t token_count;
    size_t pos;
    char error[512];
    bool has_error;
};

static bool is_keyword(TokenType t) {
    return t == TK_KEYWORD;
}

static const char *kw_value(const Token *t) {
    if (t->type == TK_KEYWORD || t->type == TK_CAP || t->type == TK_TYPE ||
        t->type == TK_BOOL) {
        return t->value;
    }
    return NULL;
}

static bool is_keyword_token(const Token *t, const char *word) {
    if (!t->value) return false;
    return strcmp(t->value, word) == 0;
}

static bool match(Parser *p, TokenType type) {
    if (p->pos >= p->token_count) return false;
    return p->tokens[p->pos].type == type;
}

static bool match_keyword(Parser *p, const char *word) {
    if (p->pos >= p->token_count) return false;
    return is_keyword_token(&p->tokens[p->pos], word);
}

static Token *cur(Parser *p) {
    if (p->pos >= p->token_count) {
        static Token eof = { TK_EOF, NULL, 0, 0, 0 };
        return &eof;
    }
    return &p->tokens[p->pos];
}

static Token *advance(Parser *p) {
    Token *t = cur(p);
    p->pos++;
    return t;
}

static bool consume(Parser *p, TokenType type, const char *msg) {
    if (match(p, type)) {
        advance(p);
        return true;
    }
    snprintf(p->error, sizeof(p->error),
             "%s (期望 %s, 实际 %s, 第 %d 行)",
             msg, token_type_name(type),
             token_type_name(cur(p)->type), cur(p)->line);
    p->has_error = true;
    return false;
}

static void set_error(Parser *p, const char *msg) {
    snprintf(p->error, sizeof(p->error),
             "%s (第 %d 行)", msg, cur(p)->line);
    p->has_error = true;
}

Parser *parser_new(const char *filename, Token *tokens, size_t token_count) {
    Parser *p = (Parser *)calloc(1, sizeof(Parser));
    if (!p) return NULL;
    p->filename = filename;
    p->tokens = tokens;
    p->token_count = token_count;
    p->pos = 0;
    strcpy(p->error, "OK");
    return p;
}

void parser_free(Parser *p) { free(p); }
int parser_line(Parser *p) { return (p->pos < p->token_count) ? p->tokens[p->pos].line : 1; }
const char *parser_error(Parser *p) { return p->error; }

/* ---- 辅助解析 ---- */

static CapabilityKind parse_capability(Parser *p) {
    Token *t = cur(p);
    if (t->type == TK_CAP) {
        advance(p);
        if (strcmp(t->value, "iso") == 0) return CAP_ISO;
        if (strcmp(t->value, "trn") == 0) return CAP_TRN;
        if (strcmp(t->value, "ref") == 0) return CAP_REF;
        if (strcmp(t->value, "val") == 0) return CAP_VAL;
        if (strcmp(t->value, "box") == 0) return CAP_BOX;
        if (strcmp(t->value, "tag") == 0) return CAP_TAG;
    }
    return CAP_UNKNOWN;
}

/* 解析类型表达式（简化版） */
static ASTNode *parse_type(Parser *p) {
    Token *t = cur(p);
    if (t->type == TK_TYPE || t->type == TK_IDENT || t->type == TK_CAP) {
        advance(p);
        ASTNode *node = ast_node_new(NODE_EMPTY, t->line, t->column);
        /* 简单类型，数据存名字 */
        if (node && node->data == NULL) {
            node->data = s_strdup(t->value);
        }
        /* 泛型类型 List[U64] */
        if (match(p, TK_BRACKET_L)) {
            advance(p);
            ASTNode *args = ast_node_new(NODE_EMPTY, t->line, t->column);
            if (args) args->data = s_strdup("");
            node->child_count = 1;
            node->children = (ASTNode **)malloc(sizeof(ASTNode *));
            if (node->children) node->children[0] = args;
            while (!match(p, TK_BRACKET_R) && p->pos < p->token_count) {
                ASTNode *arg = parse_type(p);
                if (arg) {
                    ast_node_add_child(args, arg);
                }
                if (match(p, TK_COMMA)) advance(p);
            }
            if (match(p, TK_BRACKET_R)) advance(p);
        }
        return node;
    }
    return NULL;
}

/* 解析参数列表 */
static ASTNode *parse_params(Parser *p) {
    ASTNode *params = ast_node_new(NODE_EMPTY, cur(p)->line, cur(p)->column);
    if (!params) return NULL;

    while (!match(p, TK_PAREN_R) && p->pos < p->token_count) {
        /* 解析参数: [cap] name: Type [= default] */
        parse_capability(p);
        if (match(p, TK_IDENT)) {
            Token *name_tok = advance(p);
            ASTNode *param = ast_node_new(NODE_EMPTY, name_tok->line, name_tok->column);
            if (param) {
                param->data = s_strdup(name_tok->value);
                ast_node_add_child(params, param);
            }
            if (match(p, TK_COLON)) {
                advance(p);
                ASTNode *type = parse_type(p);
                if (type && param) {
                    ast_node_add_child(param, type);
                }
            }
            if (match(p, TK_EQ)) {
                advance(p);
                /* 默认值解析 (简化: 跳过表达式) */
            }
        }
        if (match(p, TK_COMMA)) advance(p);
    }
    return params;
}

/* 前向声明 */
static ASTNode *parse_statement(Parser *p);
static ASTNode *parse_expression(Parser *p);

/* 解析块 { ... } */
static ASTNode *parse_block(Parser *p) {
    if (!match(p, TK_BRACE_L)) {
        set_error(p, "期望 '{'");
        return NULL;
    }
    advance(p);

    ASTNode *block = ast_node_new(NODE_EMPTY, cur(p)->line, cur(p)->column);
    if (!block) return NULL;

    while (!match(p, TK_BRACE_R) && p->pos < p->token_count) {
        if (match(p, TK_SEMI)) { advance(p); continue; }
        ASTNode *stmt = parse_statement(p);
        if (stmt) {
            ast_node_add_child(block, stmt);
        } else {
            if (!match(p, TK_BRACE_R) && p->pos < p->token_count) {
                advance(p); /* 跳过未知 token */
            }
        }
    }
    if (match(p, TK_BRACE_R)) advance(p);
    return block;
}

/* 解析语句 */
static ASTNode *parse_statement(Parser *p) {
    Token *t = cur(p);
    int line = t->line;
    int col = t->column;

    /* var 声明 */
    if (is_keyword_token(t, "var")) {
        advance(p);
        if (match(p, TK_IDENT)) {
            Token *name = advance(p);
            ASTNode *node = ast_node_new(NODE_VAR, line, col);
            if (node) node->data = s_strdup(name->value);
            if (match(p, TK_COLON)) {
                advance(p);
                ASTNode *type = parse_type(p);
                if (type && node) ast_node_add_child(node, type);
            }
            if (match(p, TK_EQ)) {
                advance(p);
                ASTNode *expr = parse_expression(p);
                if (expr && node) ast_node_add_child(node, expr);
            }
            if (match(p, TK_SEMI)) advance(p);
            return node;
        }
        return NULL;
    }

    /* let 声明 */
    if (is_keyword_token(t, "let")) {
        advance(p);
        if (match(p, TK_IDENT)) {
            Token *name = advance(p);
            ASTNode *node = ast_node_new(NODE_LET, line, col);
            if (node) node->data = s_strdup(name->value);
            if (match(p, TK_COLON)) {
                advance(p);
                ASTNode *type = parse_type(p);
                if (type && node) ast_node_add_child(node, type);
            }
            if (match(p, TK_EQ)) {
                advance(p);
                ASTNode *expr = parse_expression(p);
                if (expr && node) ast_node_add_child(node, expr);
            }
            if (match(p, TK_SEMI)) advance(p);
            return node;
        }
        return NULL;
    }

    /* if 语句 */
    if (is_keyword_token(t, "if")) {
        advance(p);
        ASTNode *node = ast_node_new(NODE_IF, line, col);
        if (!node) return NULL;
        ASTNode *cond = parse_expression(p);
        if (cond) ast_node_add_child(node, cond);
        ASTNode *then_block = parse_block(p);
        if (then_block) ast_node_add_child(node, then_block);
        if (match_keyword(p, "else")) {
            advance(p);
            ASTNode *else_node = ast_node_new(NODE_ELSE, cur(p)->line, cur(p)->column);
            if (else_node) {
                if (match_keyword(p, "if")) {
                    ast_node_add_child(else_node, parse_statement(p));
                } else {
                    ast_node_add_child(else_node, parse_block(p));
                }
                ast_node_add_child(node, else_node);
            }
        }
        return node;
    }

    /* while 语句 */
    if (is_keyword_token(t, "while")) {
        advance(p);
        ASTNode *node = ast_node_new(NODE_WHILE, line, col);
        if (!node) return NULL;
        ASTNode *cond = parse_expression(p);
        if (cond) ast_node_add_child(node, cond);
        ASTNode *body = parse_block(p);
        if (body) ast_node_add_child(node, body);
        return node;
    }

    /* return 语句 */
    if (is_keyword_token(t, "return")) {
        advance(p);
        ASTNode *node = ast_node_new(NODE_RETURN, line, col);
        if (!node) return NULL;
        if (!match(p, TK_SEMI) && !match(p, TK_BRACE_R)) {
            ASTNode *expr = parse_expression(p);
            if (expr) ast_node_add_child(node, expr);
        }
        if (match(p, TK_SEMI)) advance(p);
        return node;
    }

    /* 表达式语句 */
    ASTNode *expr = parse_expression(p);
    if (match(p, TK_SEMI)) advance(p);
    return expr;
}

/* 解析表达式（简化：支持基本字面量和变量引用） */
static ASTNode *parse_expression(Parser *p) {
    Token *t = cur(p);
    int line = t->line;
    int col = t->column;

    /* 字符串 */
    if (t->type == TK_STRING) {
        advance(p);
        ASTNode *node = ast_string_new(t->value, line, col);
        return node;
    }

    /* 数字 */
    if (t->type == TK_INT) {
        advance(p);
        ASTNode *node = ast_int_new(0, line, col);
        if (node && node->data) {
            /* 存储为字符串以便后续解析 */
            node->data = s_strdup(t->value);
        }
        return node;
    }

    /* 布尔 */
    if (t->type == TK_BOOL) {
        advance(p);
        ASTNode *node = ast_bool_new(strcmp(t->value, "true") == 0, line, col);
        return node;
    }

    /* 标识符或方法调用 */
    if (t->type == TK_IDENT) {
        advance(p);
        ASTNode *node = ast_node_new(NODE_EMPTY, line, col);
        if (node) node->data = s_strdup(t->value);
        /* 方法调用 */
        if (match(p, TK_PAREN_L)) {
            advance(p);
            ASTNode *call = ast_node_new(NODE_CALL, line, col);
            if (call) call->data = s_strdup(t->value);
            if (node) {
                ast_node_add_child(call, node);
                /* 参数列表 */
                ASTNode *args = ast_node_new(NODE_EMPTY, line, col);
                if (args) args->data = s_strdup("");
                ast_node_add_child(call, args);
                while (!match(p, TK_PAREN_R) && p->pos < p->token_count) {
                    ASTNode *arg = parse_expression(p);
                    if (arg) ast_node_add_child(args, arg);
                    if (match(p, TK_COMMA)) advance(p);
                }
                if (match(p, TK_PAREN_R)) advance(p);
                return call;
            }
        }
        return node;
    }

    return NULL;
}

/* 解析 Actor 方法 */
static ASTNode *parse_method(Parser *p, bool is_be) {
    advance(p); /* 跳过 be/fun */
    if (!match(p, TK_IDENT)) {
        set_error(p, "期望方法名");
        return NULL;
    }
    Token *name_tok = advance(p);
    ASTNode *node = ast_node_new(is_be ? NODE_BE : NODE_FUN,
                                  name_tok->line, name_tok->column);
    if (!node) return NULL;
    node->data = s_strdup(name_tok->value);

    /* 参数列表 */
    if (match(p, TK_PAREN_L)) {
        advance(p);
        ASTNode *params = parse_params(p);
        if (params) ast_node_add_child(node, params);
        if (match(p, TK_PAREN_R)) advance(p);
    }

    /* 返回类型 */
    if (match(p, TK_COLON)) {
        advance(p);
        ASTNode *ret_type = parse_type(p);
        if (ret_type) ast_node_add_child(node, ret_type);
    }

    /* 方法体 */
    if (match(p, TK_ARROW_ARR) || (match(p, TK_COLON) && match_keyword(p, "=>"))) {
        /* Pony 风格: => */
    }
    if (match(p, TK_BRACE_L)) {
        ASTNode *body = parse_block(p);
        if (body) ast_node_add_child(node, body);
    } else if (match(p, TK_ARROW_ARR)) {
        advance(p);
        ASTNode *expr = parse_expression(p);
        if (expr) ast_node_add_child(node, expr);
    } else if (match(p, TK_COLON)) {
        /* Pony 风格: : 开头的方法体 */
        advance(p);
        /* 简化: 解析块或表达式 */
        if (match(p, TK_BRACE_L)) {
            ASTNode *body = parse_block(p);
            if (body) ast_node_add_child(node, body);
        } else {
            ASTNode *expr = parse_expression(p);
            if (expr) ast_node_add_child(node, expr);
        }
    }

    return node;
}

/* 解析 Actor 构造函数 */
static ASTNode *parse_constructor(Parser *p) {
    advance(p); /* 跳过 new */
    if (!match(p, TK_IDENT)) {
        set_error(p, "期望构造函数名");
        return NULL;
    }
    Token *name_tok = advance(p);
    ASTNode *node = ast_node_new(NODE_NEW, name_tok->line, name_tok->column);
    if (!node) return NULL;
    node->data = s_strdup(name_tok->value);

    /* 参数列表 */
    if (match(p, TK_PAREN_L)) {
        advance(p);
        ASTNode *params = parse_params(p);
        if (params) ast_node_add_child(node, params);
        if (match(p, TK_PAREN_R)) advance(p);
    }

    /* 构造体 */
    if (match(p, TK_BRACE_L)) {
        ASTNode *body = parse_block(p);
        if (body) ast_node_add_child(node, body);
    } else if (match(p, TK_ARROW_ARR)) {
        advance(p);
        ASTNode *expr = parse_expression(p);
        if (expr) ast_node_add_child(node, expr);
    } else if (match(p, TK_COLON)) {
        advance(p);
        if (match(p, TK_BRACE_L)) {
            ASTNode *body = parse_block(p);
            if (body) ast_node_add_child(node, body);
        } else {
            ASTNode *expr = parse_expression(p);
            if (expr) ast_node_add_child(node, expr);
        }
    }

    return node;
}

/* 解析 Actor 字段 */
static ASTNode *parse_field(Parser *p, bool is_var) {
    advance(p); /* 跳过 var/let */
    if (!match(p, TK_IDENT)) {
        set_error(p, "期望字段名");
        return NULL;
    }
    Token *name_tok = advance(p);
    ASTNode *node = ast_node_new(is_var ? NODE_VAR : NODE_LET,
                                  name_tok->line, name_tok->column);
    if (!node) return NULL;
    node->data = s_strdup(name_tok->value);

    if (match(p, TK_COLON)) {
        advance(p);
        ASTNode *type = parse_type(p);
        if (type) ast_node_add_child(node, type);
    }
    if (match(p, TK_EQ)) {
        advance(p);
        ASTNode *init = parse_expression(p);
        if (init) ast_node_add_child(node, init);
    }
    if (match(p, TK_SEMI)) advance(p);
    return node;
}

/* 解析 Actor */
static ASTNode *parse_actor(Parser *p) {
    advance(p); /* 跳过 actor */
    if (!match(p, TK_IDENT)) {
        set_error(p, "期望 Actor 名称");
        return NULL;
    }
    Token *name_tok = advance(p);
    ASTNode *node = ast_actor_new(name_tok->value, name_tok->line, name_tok->column);
    if (!node) return NULL;

    /* 可选参数列表: actor Name(cap param: Type) { */
    if (match(p, TK_PAREN_L)) {
        advance(p);
        ASTNode *params = parse_params(p);
        if (params) ast_node_add_child(node, params);
        if (match(p, TK_PAREN_R)) advance(p);
    }

    /* 可选引用能力 */
    if (cur(p)->type == TK_CAP) {
        advance(p);
    }

    /* Actor 体 */
    if (match(p, TK_BRACE_L)) {
        advance(p);
        while (!match(p, TK_BRACE_R) && p->pos < p->token_count) {
            if (match_keyword(p, "be")) {
                ASTNode *method = parse_method(p, true);
                if (method) ast_node_add_child(node, method);
            } else if (match_keyword(p, "fun")) {
                ASTNode *method = parse_method(p, false);
                if (method) ast_node_add_child(node, method);
            } else if (match_keyword(p, "new")) {
                ASTNode *ctor = parse_constructor(p);
                if (ctor) ast_node_add_child(node, ctor);
            } else if (match_keyword(p, "var")) {
                ASTNode *field = parse_field(p, true);
                if (field) ast_node_add_child(node, field);
            } else if (match_keyword(p, "let")) {
                ASTNode *field = parse_field(p, false);
                if (field) ast_node_add_child(node, field);
            } else if (match(p, TK_SEMI)) {
                advance(p);
            } else {
                /* 跳过未知内容 */
                advance(p);
            }
        }
        if (match(p, TK_BRACE_R)) advance(p);
    }

    return node;
}

/* 解析 supervise 声明 */
static ASTNode *parse_supervise(Parser *p) {
    Token *t = advance(p); /* 跳过 supervise */
    ASTNode *node = ast_node_new(NODE_SUPERVISE, t->line, t->column);
    if (!node) return NULL;

    /* supervise target_name strategy */
    if (match(p, TK_IDENT)) {
        Token *target = advance(p);
        node->data = s_strdup(target->value);
        if (match(p, TK_IDENT)) {
            Token *strategy = advance(p);
            (void)strategy;
        }
    }
    return node;
}

/* 解析整个程序 */
ASTNode *parser_parse_program(Parser *p) {
    ASTNode *program = ast_program_new(1, 1);
    if (!program) return NULL;

    while (p->pos < p->token_count) {
        Token *t = cur(p);

        if (is_keyword_token(t, "actor")) {
            ASTNode *actor = parse_actor(p);
            if (actor) ast_node_add_child(program, actor);
        } else if (is_keyword_token(t, "class")) {
            /* 简化: 跳过 class */
            advance(p);
        } else if (is_keyword_token(t, "trait")) {
            advance(p);
        } else if (is_keyword_token(t, "supervise")) {
            ASTNode *sup = parse_supervise(p);
            if (sup) ast_node_add_child(program, sup);
        } else if (is_keyword_token(t, "import")) {
            advance(p);
            if (match(p, TK_IDENT)) advance(p);
            if (match(p, TK_SEMI)) advance(p);
        } else if (is_keyword_token(t, "use")) {
            advance(p);
            if (match(p, TK_IDENT)) advance(p);
            if (match(p, TK_SEMI)) advance(p);
        } else {
            /* 跳过未知内容 */
            advance(p);
        }
    }

    return program;
}
