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

/* 解析类型表达式（含引用能力前缀） */
static ASTNode *parse_type(Parser *p) {
    Token *t = cur(p);
    /* 检查引用能力前缀: iso/trn/ref/val/box foo() */
    if (t->type == TK_CAP) {
        advance(p);
        ASTNode *cap = ast_node_new(NODE_CAP, t->line, t->column);
        if (cap) cap->data = s_strdup(t->value);
        ASTNode *type = parse_type(p);
        if (cap && type) ast_node_add_child(cap, type);
        return cap;
    }
    if (t->type == TK_TYPE || t->type == TK_IDENT) {
        advance(p);
        ASTNode *node = ast_node_new(NODE_IDENT, t->line, t->column);
        if (node) node->data = s_strdup(t->value);
        if (match(p, TK_BRACKET_L)) {
            advance(p);
            ASTNode *args = ast_node_new(NODE_EMPTY, t->line, t->column);
            if (args) args->data = s_strdup("typeargs");
            node->child_count = 1;
            node->children = (ASTNode **)malloc(sizeof(ASTNode *));
            if (node->children) node->children[0] = args;
            while (!match(p, TK_BRACKET_R) && p->pos < p->token_count) {
                ASTNode *arg = parse_type(p);
                if (arg) ast_node_add_child(args, arg);
                if (match(p, TK_COMMA)) advance(p);
            }
            if (match(p, TK_BRACKET_R)) advance(p);
        }
        return node;
    }
    return NULL;
}

/* 前向声明 */
static ASTNode *parse_statement(Parser *p);
static ASTNode *parse_expression(Parser *p);

/* 解析参数列表 */
static ASTNode *parse_params(Parser *p) {
Token *t = cur(p);
ASTNode *params = ast_node_new(NODE_EMPTY, t->line, t->column);
if (!params) return NULL;
params->data = s_strdup("params");
if (!match(p, TK_PAREN_L)) {
    ast_node_free(params);
    return NULL;
}
advance(p);

while (!match(p, TK_PAREN_R) && p->pos < p->token_count) {
        parse_capability(p);
        if (match(p, TK_IDENT)) {
            Token *name_tok = cur(p);
            advance(p);
            ASTNode *param = ast_node_new(NODE_EMPTY, name_tok->line, name_tok->column);
            if (param) {
                param->data = s_strdup(name_tok->value);
                ast_node_add_child(params, param);
                if (match(p, TK_COLON)) { advance(p); ASTNode *type = parse_type(p); if (type) ast_node_add_child(param, type); }
            }
            if (match(p, TK_EQ)) { advance(p); (void)parse_expression(p); }
        }
        if (match(p, TK_COMMA)) advance(p);
    }
    /* 消费闭合括号 */
    if (match(p, TK_PAREN_R)) advance(p);
    return params;
}

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

    /* for 语句: for i in 0..10 do { body } */
    if (is_keyword_token(t, "for")) {
        advance(p);
        ASTNode *node = ast_node_new(NODE_FOR, line, col);
        if (!node) return NULL;
        /* 解析迭代变量 */
        if (cur_type(p) == TK_IDENT || match_keyword(p, "it") || match_keyword(p, "_")) {
            Token *var_t = cur(p);
            advance(p);
            ASTNode *var_node = ast_node_new(NODE_IDENT, var_t->line, var_t->column);
            if (var_node && var_t->value) var_node->ident = var_t->value;
            if (var_node) ast_node_add_child(node, var_node);
        }
        /* in 关键字 */
        match_keyword(p, "in");
        /* 解析 range 表达式 (start..end) */
        ASTNode *range_expr = parse_expression(p);
        if (range_expr) ast_node_add_child(node, range_expr);
        /* do 关键字 (可选) */
        match_keyword(p, "do");
        /* 循环体 */
        ASTNode *body = parse_block(p);
        if (body) ast_node_add_child(node, body);
        return node;
    }

    /* match 表达式 */
    if (is_keyword_token(t, "match")) {
        advance(p);
        ASTNode *node = ast_node_new(NODE_MATCH, line, col);
        if (!node) return NULL;
        ASTNode *expr = parse_expression(p);
        if (expr) ast_node_add_child(node, expr);
        /* 兼容: match x => { ... } 和 match x { ... } */
        if (match_keyword(p, "=>")) advance(p);
        if (match(p, TK_BRACE_L)) advance(p);
        /* match arms: pattern => body */
        while (!match(p, TK_BRACE_R) && p->pos < p->token_count) {
            ASTNode *arm = ast_node_new(NODE_MATCH_ARM, line, col);
            if (arm) {
                ASTNode *pat = parse_expression(p);
                if (pat) ast_node_add_child(arm, pat);
                if (match_keyword(p, "=>")) advance(p);
                ASTNode *body = parse_expression(p);
                if (body) ast_node_add_child(arm, body);
                ast_node_add_child(node, arm);
            } else {
                if (match(p, TK_SEMI)) advance(p);
                continue;
            }
            if (match(p, TK_SEMI)) advance(p);
        }
        if (match(p, TK_BRACE_R)) advance(p);
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
        ASTNode *start_node = ast_node_new(NODE_INT, line, col);
        if (start_node) {
            start_node->data = s_strdup(t->value);
            start_node->value_int = atoi(t->value);
        }
        /* 范围: start..end */
        if (match(p, TK_RANGE)) {
            advance(p);
            Token *end_t = cur(p);
            if (end_t->type == TK_INT) {
                advance(p);
                ASTNode *end_node = ast_node_new(NODE_INT, end_t->line, end_t->column);
                if (end_node) {
                    end_node->data = s_strdup(end_t->value);
                    end_node->value_int = atoi(end_t->value);
                }
                ASTNode *range_node = ast_node_new(NODE_EMPTY, line, col);
                range_node->data = s_strdup("range");
                if (start_node) ast_node_add_child(range_node, start_node);
                if (end_node) ast_node_add_child(range_node, end_node);
                return range_node;
            }
        }
        return start_node;
    }

    /* 布尔 */
    if (t->type == TK_BOOL) {
        advance(p);
        ASTNode *node = ast_bool_new(strcmp(t->value, "true") == 0, line, col);
        return node;
    }

    /* 标识符 */
    if (t->type == TK_IDENT) {
        advance(p);
        if (match(p, TK_PAREN_L)) {
            advance(p);
            ASTNode *call = ast_node_new(NODE_CALL, line, col);
            if (call) call->data = s_strdup(t->value);
            ASTNode *args = ast_node_new(NODE_EMPTY, line, col);
            if (args) { args->data = s_strdup("args"); ast_node_add_child(call, args); }
            while (!match(p, TK_PAREN_R) && p->pos < p->token_count) {
                ASTNode *arg = parse_expression(p);
                if (arg) ast_node_add_child(args, arg);
                if (match(p, TK_COMMA)) advance(p);
            }
            if (match(p, TK_PAREN_R)) advance(p);
            return call;
        }
        if (match(p, TK_EQ)) {
            advance(p);
            ASTNode *assign = ast_node_new(NODE_EMPTY, line, col);
            if (assign) {
                assign->data = s_strdup("assign");
                ASTNode *ident = ast_node_new(NODE_IDENT, line, col);
                if (ident) ident->data = s_strdup(t->value);
                ast_node_add_child(assign, ident);
                ASTNode *rhs = parse_expression(p);
                if (rhs) ast_node_add_child(assign, rhs);
            }
            return assign;
        }
        ASTNode *node = ast_node_new(NODE_IDENT, line, col);
        if (node) node->data = s_strdup(t->value);

        /* 检查 ! (异步发送) 和 @ (同步调用) 后缀操作 */
        if (match(p, TK_BANG)) {
            advance(p);
            ASTNode *msg = ast_node_new(NODE_SEND, line, col);
            if (msg) {
                msg->data = s_strdup("send");
                if (node) ast_node_add_child(msg, node);
                ASTNode *payload = parse_expression(p);
                if (payload) ast_node_add_child(msg, payload);
            }
            return msg;
        }
        if (match(p, TK_AT)) {
            advance(p);
            ASTNode *sync = ast_node_new(NODE_MSG_CALL, line, col);
            if (sync) {
                sync->data = s_strdup("call");
                if (node) ast_node_add_child(sync, node);
                ASTNode *payload = parse_expression(p);
                if (payload) ast_node_add_child(sync, payload);
            }
            return sync;
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
    ASTNode *params = parse_params(p);
    if (params) { if (params->child_count > 0) ast_node_add_child(node, params); else ast_node_free(params); }

    /* 返回类型 */
    if (match(p, TK_COLON)) {
        advance(p);
        ASTNode *ret_type = parse_type(p);
        if (ret_type) ast_node_add_child(node, ret_type);
    }

    /* 方法体 */
    if (match(p, TK_BRACE_L)) {
        ASTNode *body = parse_block(p);
        if (body) ast_node_add_child(node, body);
    } else if (match(p, TK_ARROW_ARR)) {
        advance(p);
        if (match(p, TK_BRACE_L)) {
            ASTNode *body = parse_block(p);
            if (body) ast_node_add_child(node, body);
        } else {
            ASTNode *expr = parse_expression(p);
            if (expr) ast_node_add_child(node, expr);
        }
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
    ASTNode *params = parse_params(p);
    if (params) { if (params->child_count > 0) ast_node_add_child(node, params); else ast_node_free(params); }

    /* 构造体 */
    if (match(p, TK_BRACE_L)) {
        ASTNode *body = parse_block(p);
        if (body) ast_node_add_child(node, body);
    } else if (match(p, TK_ARROW_ARR)) {
        advance(p);
        if (match(p, TK_BRACE_L)) {
            ASTNode *body = parse_block(p);
            if (body) ast_node_add_child(node, body);
        } else {
            ASTNode *expr = parse_expression(p);
            if (expr) ast_node_add_child(node, expr);
        }
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
    ASTNode *params = parse_params(p);
    if (params) { if (params->child_count > 0) ast_node_add_child(node, params); else ast_node_free(params); }

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
            /* import "module" or import Module */
            advance(p);
            ASTNode *imp = ast_node_new(NODE_IMPORT, t->line, t->column);
            if (imp && match(p, TK_IDENT)) {
                Token *m = advance(p);
                imp->data = s_strdup(m->value);
                ast_node_add_child(program, imp);
            } else if (imp) {
                ast_node_add_child(program, imp);
            }
            if (match(p, TK_SEMI)) advance(p);
        } else if (is_keyword_token(t, "use")) {
            /* use Module or use "path/to/module" */
            advance(p);
            ASTNode *imp = ast_node_new(NODE_IMPORT, t->line, t->column);
            if (imp && match(p, TK_IDENT)) {
                Token *m = advance(p);
                imp->data = s_strdup(m->value);
                ast_node_add_child(program, imp);
            } else if (imp) {
                ast_node_add_child(program, imp);
            }
            if (match(p, TK_SEMI)) advance(p);
        } else {
            /* 跳过未知内容 */
            advance(p);
        }
    }

    return program;
}
