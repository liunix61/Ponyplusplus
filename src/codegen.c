/*
 * codegen.c - Pony++ Native backend C 代码生成器
 */

#include "ponypp/codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "ponypp/util.h"
#include "ponypp/runtime.h"

struct Codegen {
    FILE *out;
    int indent;
    size_t field_count;
    char **fields;
    char **field_types; /* 字段类型: "String"/"U32"/"U64" 等 */
    int in_constructor; /* 1=self 值类型 self.field, 0=指针 self->field */
    char actor_name[64]; /* 当前 Actor 名称, 用于方法调用 */
    char **params;      /* 当前方法的参数名 */
    size_t param_count; /* 当前方法的参数数量 */
    char known_actors[32][64]; /* 程序中所有 actor 类型名（构造调用识别用） */
    size_t known_actor_count;
    char local_vars[256][64]; /* Bug#35: 32 上限静默丢弃致 String 变量被误判 int */   /* 当前 actor 内局部变量名 */
    char local_types[256][64];  /* 对应 actor 类型名（方法调用分派用） */
    size_t local_var_count;
    char str_ret_methods[256][64]; /* 返回类型为 String 的方法名(扁平) */
    size_t str_ret_count;
    char type_names[32][64];   /* 所有 actor/class 类型名 */
    char type_fields[32][32][64]; /* 每个类型的字段名 */
    char type_field_types[32][32][64]; /* 每个类型的字段类型 (Bug#25/26 链式解析) */
    size_t type_field_counts[32];
    size_t type_count;
    char extern_funcs[64][64];  /* extern fun FFI 名 (0.2.16) */
    char extern_rets[64][64];   /* 对应返回类型名 */
    size_t extern_count;
};

Codegen *codegen_new(FILE *out) {
    Codegen *cg = (Codegen *)calloc(1, sizeof(Codegen));
    if (!cg) return NULL;
    cg->out = out;
    cg->indent = 0;
    cg->in_constructor = 0;
    return cg;
}

void codegen_free(Codegen *cg) {
    if (!cg) return;
    for (size_t i = 0; i < cg->field_count; i++) {
        free(cg->fields[i]);
        if (cg->field_types) free(cg->field_types[i]);
    }
    free(cg->fields);
    free(cg->field_types);
    free(cg);
}

static void cg_push(Codegen *cg) { cg->indent += 2; }
static void cg_pop(Codegen *cg) { cg->indent -= 2; if (cg->indent < 0) cg->indent = 0; }

static void cg_write_indent(Codegen *cg) {
    for (int i = 0; i < cg->indent; i++) fputc(' ', cg->out);
}

static void cg_emit(Codegen *cg, const char *fmt, ...) {
    cg_write_indent(cg);
    va_list ap; va_start(ap, fmt);
    vfprintf(cg->out, fmt, ap);
    va_end(ap);
}

static void cg_emit_raw(Codegen *cg, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vfprintf(cg->out, fmt, ap);
    va_end(ap);
}

static int cg_has_field(Codegen *cg, const char *name) {
    for (size_t i = 0; i < cg->field_count; i++) {
        if (strcmp(cg->fields[i], name) == 0) return 1;
    }
    return 0;
}

static void cg_set_fields(Codegen *cg, size_t fc, char **fields) {
    for (size_t i = 0; i < cg->field_count; i++) free(cg->fields[i]);
    free(cg->fields);
    cg->field_count = fc;
    cg->fields = fields;
}

static void cg_set_fields_with_types(Codegen *cg, size_t fc, char **fields, char **types) {
    /* 先释放旧的 field_types（用旧的 field_count） */
    if (cg->field_types) {
        for (size_t i = 0; i < cg->field_count; i++) free(cg->field_types[i]);
        free(cg->field_types);
        cg->field_types = NULL;
    }
    cg_set_fields(cg, fc, fields);
    cg->field_types = types;
}

static const char *cg_field_type(Codegen *cg, const char *name) {
    for (size_t i = 0; i < cg->field_count; i++) {
        if (strcmp(cg->fields[i], name) == 0 && cg->field_types) return cg->field_types[i];
    }
    return NULL;
}

static void cg_set_ctor(Codegen *cg, int in_ctor) { cg->in_constructor = in_ctor; }

static void cg_set_params(Codegen *cg, size_t pc, char **params) {
    for (size_t i = 0; i < cg->param_count; i++) free(cg->params[i]);
    free(cg->params);
    cg->params = params;
    cg->param_count = pc;
}

static int cg_is_param(Codegen *cg, const char *name) {
    for (size_t i = 0; i < cg->param_count; i++) {
        if (cg->params[i] && strcmp(cg->params[i], name) == 0) return 1;
    }
    return 0;
}

static bool cg_has_local(Codegen *cg, const char *name) {
    if (!name) return false;
    for (size_t i = 0; i < cg->local_var_count; i++)
        if (strcmp(cg->local_vars[i], name) == 0) return true;
    for (size_t i = 0; i < cg->param_count; i++)
        if (cg->params[i] && strcmp(cg->params[i], name) == 0) return true;
    return false;
}

static void cg_emit_field_access(Codegen *cg, const char *name) {
    if (name && name[0] == 't' && name[1] == 'h' && name[2] == 'i' && name[3] == 's' && name[4] == '.') {
        cg_emit_raw(cg, "self->%s", name + 5);
    } else if (name && !cg_has_local(cg, name) && cg_has_field(cg, name)) {
        cg_emit_raw(cg, "self->%s", name);
    } else {
        cg_emit_raw(cg, "%s", name ? name : "0");
    }
}

static const char *cg_type_of(ASTNode *n, const char **actor_types, size_t atc) {
    if (!n || !n->data) return "int";
    const char *name = (const char *)n->data;
    /* 泛型类型 List[T] / Set[T] / Map[K,V] 等: C 中用对应指针类型 */
    if (n->child_count > 0 && n->children[0]) {
        /* NODE_EMPTY data="typeargs" 表示泛型 */
        if (n->children[0]->type == NODE_EMPTY && n->children[0]->data &&
            strcmp((const char *)n->children[0]->data, "typeargs") == 0) {
            if (strcmp(name, "List") == 0) return "PnyList *";
            if (strcmp(name, "Set") == 0) return "PnySet *";
            if (strcmp(name, "Map") == 0) return "PnyMap *";
            return "void *";
        }
    }
    /* Actor 类型 → _t 后缀 */
    for (size_t i = 0; i < atc; i++) {
        if (actor_types && actor_types[i] && strcmp(actor_types[i], name) == 0) {
            return actor_types[i]; /* caller handles _t suffix */
        }
    }
    if (strcmp(name, "JSON") == 0) return "PnyJson *";
    if (strcmp(name, "U8") == 0) return "unsigned char";
    if (strcmp(name, "U16") == 0) return "unsigned short";
    if (strcmp(name, "U32") == 0) return "unsigned int";
    if (strcmp(name, "U64") == 0) return "unsigned long long";
    if (strcmp(name, "I8") == 0) return "signed char";
    if (strcmp(name, "I16") == 0) return "signed short";
    if (strcmp(name, "I32") == 0) return "signed int";
    if (strcmp(name, "I64") == 0) return "signed long long";
    if (strcmp(name, "F32") == 0) return "float";
    if (strcmp(name, "F64") == 0) return "double";
    if (strcmp(name, "USize") == 0) return "size_t";
    if (strcmp(name, "ISize") == 0) return "ptrdiff_t";
    if (strcmp(name, "ActorRef") == 0) return "void *";
    if (strcmp(name, "String") == 0) return "const char *";
    if (strcmp(name, "Char") == 0) return "char";
    if (strcmp(name, "Bool") == 0) return "int";
    if (strcmp(name, "Int") == 0) return "long long";
    if (strcmp(name, "None") == 0 || strcmp(name, "NoneType") == 0) return "void";
    if (strcmp(name, "JSON") == 0) return "PnyJson *";
    return name;
}

/* 内置类型名 → C 类型名（不依赖 actor_types，用于 var/let 声明） */
static const char *cg_builtin_type(const char *name) {
    if (!name) return "int";
    if (strcmp(name, "JSON") == 0) return "PnyJson *";
    if (strcmp(name, "U8") == 0) return "unsigned char";
    if (strcmp(name, "U16") == 0) return "unsigned short";
    if (strcmp(name, "U32") == 0) return "unsigned int";
    if (strcmp(name, "U64") == 0) return "unsigned long long";
    if (strcmp(name, "I8") == 0) return "signed char";
    if (strcmp(name, "I16") == 0) return "signed short";
    if (strcmp(name, "I32") == 0) return "signed int";
    if (strcmp(name, "I64") == 0) return "signed long long";
    if (strcmp(name, "F32") == 0) return "float";
    if (strcmp(name, "F64") == 0) return "double";
    if (strcmp(name, "USize") == 0) return "size_t";
    if (strcmp(name, "ISize") == 0) return "ptrdiff_t";
    if (strcmp(name, "String") == 0) return "const char *";
    if (strcmp(name, "Char") == 0) return "char";
    if (strcmp(name, "Bool") == 0) return "int";
    if (strcmp(name, "Int") == 0) return "long long";
    if (strcmp(name, "None") == 0 || strcmp(name, "NoneType") == 0) return "void";
    if (strcmp(name, "JSON") == 0) return "PnyJson *";
    if (strcmp(name, "List") == 0) return "PnyList *";
    if (strcmp(name, "Set") == 0) return "PnySet *";
    if (strcmp(name, "Map") == 0) return "PnyMap *";
    return name;
}

static bool cg_is_known_actor(const Codegen *cg, const char *name) {
    if (!name) return false;
    for (size_t i = 0; i < cg->known_actor_count; i++)
        if (strcmp(cg->known_actors[i], name) == 0) return true;
    return false;
}

static const char *cg_local_actor_type(const Codegen *cg, const char *var) {
    if (!var) return NULL;
    for (size_t i = 0; i < cg->local_var_count; i++)
        if (strcmp(cg->local_vars[i], var) == 0) return cg->local_types[i];
    return NULL;
}

static const char *cg_type_field_base(const Codegen *cg, const char *type) {
    /* 返回类型名本身若已登记, 否则 NULL */
    if (!type) return NULL;
    for (size_t i = 0; i < cg->type_count; i++)
        if (strcmp(cg->type_names[i], type) == 0) return cg->type_names[i];
    return NULL;
}

static bool cg_is_str_ret_method(const Codegen *cg, const char *m) {
    if (!m) return false;
    for (size_t i = 0; i < cg->str_ret_count; i++)
        if (strcmp(cg->str_ret_methods[i], m) == 0) return true;
    return false;
}

static bool cg_type_has_field(const Codegen *cg, const char *type, const char *field) {
    if (!type || !field) return false;
    for (size_t i = 0; i < cg->type_count; i++) {
        if (strcmp(cg->type_names[i], type) != 0) continue;
        for (size_t j = 0; j < cg->type_field_counts[i]; j++)
            if (strcmp(cg->type_fields[i][j], field) == 0) return true;
        return false;
    }
    return false;
}

static void cg_local_add(Codegen *cg, const char *var, const char *type) {
    if (cg->local_var_count >= 256) return;
    snprintf(cg->local_vars[cg->local_var_count], 64, "%s", var ? var : "");
    snprintf(cg->local_types[cg->local_var_count], 64, "%s", type ? type : "");
    cg->local_var_count++;
}

static char *cg_cstr_escape(const char *s, char *buf, size_t sz) {
    size_t j = 0;
    for (size_t i = 0; s && s[i] && j < sz - 3; i++) {
        char c = s[i];
        if (c == '\\') { if (j + 2 < sz) { buf[j++] = '\\'; buf[j++] = '\\'; } }
        else if (c == '"') { if (j + 2 < sz) { buf[j++] = '\\'; buf[j++] = '"'; } }
        else if (c == '\n') { if (j + 2 < sz) { buf[j++] = '\\'; buf[j++] = 'n'; } }
        else if (c == '\r') { if (j + 2 < sz) { buf[j++] = '\\'; buf[j++] = 'r'; } }
        else if (c == '\t') { if (j + 2 < sz) { buf[j++] = '\\'; buf[j++] = 't'; } }
        else buf[j++] = c;
    }
    buf[j] = '\0';
    return buf;
}

static void cg_expr(Codegen *cg, ASTNode *n);
static void cg_stmt(Codegen *cg, ASTNode *n);
static void cg_emit_create_call(Codegen *cg, const char *name, ASTNode *actor);
static void cg_emit_main(Codegen *cg, ASTNode *ast);
static void cg_emit_runtime(Codegen *cg);
static void cg_emit_create_call(Codegen *cg, const char *name, ASTNode *actor);
static void cg_emit_main(Codegen *cg, ASTNode *ast);
static void cg_actor(Codegen *cg, ASTNode *actor,
                     const char **actor_types, size_t atc);

/* 链式字段解析 (Bug#25/26): "a.b.c" → recv_expr + 末段类型
 * 首段查局部变量/参数, 逐段查 class 字段类型; 成功返回末段类型(静态buf), 失败 NULL */
static const char *cg_chain_resolve(Codegen *cg, const char *name, char *recv_out, size_t recv_sz) {
    if (!name) return NULL;
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", name);
    char *first = buf;
    char *dot = strchr(buf, '.');
    if (!dot) return NULL;
    *dot = 0;
    const char *ct = NULL;
    if (strcmp(first, "this") == 0 && cg->actor_name[0]) {
        ct = cg->actor_name;
        snprintf(recv_out, recv_sz, "self");
    } else {
        ct = cg_local_actor_type(cg, first);
        if (!ct) {
            ct = cg_field_type(cg, first);
            if (!ct) return NULL;
            snprintf(recv_out, recv_sz, "self->%s", first);
        } else {
            snprintf(recv_out, recv_sz, "%s", first);
        }
    }
    /* ct 是首段类型(class 名或内置); 逐段深入 */
    char curtype[128];
    snprintf(curtype, sizeof(curtype), "%s", ct);
    char *seg = dot + 1;
    for (;;) {
        char *next = strchr(seg, '.');
        if (!next) {
            /* 末段: recv 拼接 + 返回其字段类型 */
            size_t rl = strlen(recv_out);
            snprintf(recv_out + rl, recv_sz - rl, "->%s", seg);
            const char *ft = NULL;
            for (size_t i = 0; i < cg->type_count; i++)
                if (cg->type_names[i] && strcmp(cg->type_names[i], curtype) == 0) {
                    for (size_t j = 0; j < cg->type_field_counts[i]; j++)
                        if (strcmp(cg->type_fields[i][j], seg) == 0) { ft = cg->type_field_types[i][j]; break; }
                    break;
                }
            return ft;
        }
        *next = 0;
        /* 中间段: 类型推进 + recv 拼接 (Bug#26) */
        size_t rl = strlen(recv_out);
        snprintf(recv_out + rl, recv_sz - rl, "->%s", seg);
        const char *nt = NULL;
        for (size_t i = 0; i < cg->type_count; i++)
            if (cg->type_names[i] && strcmp(cg->type_names[i], curtype) == 0) {
                for (size_t j = 0; j < cg->type_field_counts[i]; j++)
                    if (strcmp(cg->type_fields[i][j], seg) == 0) { nt = cg->type_field_types[i][j]; break; }
                break;
            }
        if (!nt) return NULL;
        snprintf(curtype, sizeof(curtype), "%s", nt);
        seg = next + 1;
    }
}

/* Bug#61: 点链 receiver 发射 — a.b.c 形态用链解析(recv->seg), 替代裸点 fallback */
static void cg_emit_receiver(Codegen *cg, const char *receiver) {
    if (receiver && strchr(receiver, '.')) {
        char _r[192];
        _r[0] = 0;
        cg_chain_resolve(cg, receiver, _r, sizeof(_r));
        if (_r[0]) {
            cg_emit_raw(cg, "%s", _r);
            return;
        }
    }
    cg_emit_field_access(cg, receiver);
}

static bool cg_expr_is_string(Codegen *cg, ASTNode *n) {
    if (!n) return false;
    if (n->type == NODE_STRING) return true;
    if (n->type == NODE_IDENT && n->data) {
        const char *d = (const char *)n->data;
        if (strncmp(d, "this.", 5) == 0) {
            const char *f = d + 5;
            const char *dot = strchr(f, '.');
            if (dot) {
                /* 多段链 this.a.b (Bug#25): 链式解析末段类型 */
                char thisbuf[288];
                snprintf(thisbuf, sizeof(thisbuf), "this.%s", f);
                char scratch[64];
                const char *ft = cg_chain_resolve(cg, thisbuf, scratch, sizeof(scratch));
                return ft && strcmp(ft, "String") == 0;
            }
            const char *ft = cg_field_type(cg, f);
            return ft && strcmp(ft, "String") == 0;
        }
        /* 局部变量多段链 a.b.c (Bug#25) */
        if (strchr(d, '.') != NULL) {
            char scratch[64];
            const char *ft = cg_chain_resolve(cg, d, scratch, sizeof(scratch));
            if (ft && strcmp(ft, "String") == 0) return true;
        }
        if (strchr(d, '.') == NULL) {
            const char *lt = cg_local_actor_type(cg, d);
            if (lt && (strcmp(lt, "String") == 0 || strcmp(lt, "const char *") == 0)) return true;
            const char *ft = cg_field_type(cg, d);
            if (ft && strcmp(ft, "String") == 0) return true;
        }
        return false;
    }
    if (n->type == NODE_CALL && n->data) {
        /* 内置函数返回 String (Bug#24) */
        static const char *str_fns[] = {
            "str_field", "file_read", "env_get", "sandbox_exec", "str_hash", "slice",
            "stdin_line", "json_raw_get", "http_accept", "http_accept_unix", "http_post_unix", "http_respond_unix", "sys_exec",
            "field_get", "str_from_char", "str_replace_all", NULL
        };
        const char *f = (const char *)n->data;
        for (int i = 0; str_fns[i]; i++)
            if (strcmp(f, str_fns[i]) == 0) return true;
        /* Bug#41: 链式 .slice/.find/.contains 返回 String */
        if (f[0] == '.') {
            if (strcmp(f, ".slice") == 0 || strcmp(f, ".find") == 0 || strcmp(f, ".contains") == 0) return true;
        }
        /* Bug#39: recv.slice(...) 点号形态 — 方法名(最后一个 . 后)也查内建 String 返回表 */
        {
            const char *dot = strrchr(f, '.');
            if (dot) {
                for (int i = 0; str_fns[i]; i++)
                    if (strcmp(dot + 1, str_fns[i]) == 0) return true;
            }
        }
        /* Bug#36: 方法调用 recv.method() 查 String 返回注册表 */
        {
            const char *mdot = strrchr(f, '.');
            const char *mname = mdot ? mdot + 1 : f;
            for (size_t i = 0; i < cg->str_ret_count; i++)
                if (strcmp(cg->str_ret_methods[i], mname) == 0) return true;
            /* 内建转换 .string()/to_string() 返回 String */
            if (strcmp(mname, "string") == 0 || strcmp(mname, "to_string") == 0) return true;
        }
        return false;
    }
    if (n->type == NODE_EMPTY && n->data && strcmp((const char *)n->data, "+") == 0) {
        return cg_expr_is_string(cg, n->child_count > 0 ? n->children[0] : NULL) ||
               cg_expr_is_string(cg, n->child_count > 1 ? n->children[1] : NULL);
    }
    return false;
}

static void cg_expr(Codegen *cg, ASTNode *n) {
    if (!n) return;
    switch (n->type) {
        case NODE_STRING: {
            const char *s = (const char *)n->data;
            char buf[512];
            cg_emit_raw(cg, "\"%s\"", cg_cstr_escape(s ? s : "", buf, sizeof(buf)));
            break;
        }
        case NODE_INT:
            cg_emit_raw(cg, "%s", n->data ? (const char *)n->data : "0");
            break;
        case NODE_FLOAT:
            cg_emit_raw(cg, "%s", n->data ? (const char *)n->data : "0.0");
            break;
        case NODE_BOOL: {
            /* ast_bool_new 把值存在 data(bool*); value_int 兜底 */
            bool bv = false;
            if (n->data) bv = *(bool *)n->data;
            else bv = n->value_int != 0;
            cg_emit_raw(cg, "%d", bv ? 1 : 0);
            break;
        }
        case NODE_CHAR: {
            /* data = 实际字符值 (e.g. 'a'=0x61, '\n'=0x0A) */
            char c = ' ';
            const char *ds = (const char *)n->data;
            if (ds && ds[0]) c = ds[0];
            if (c == '\n') cg_emit_raw(cg, "'\\n'");
            else if (c == '\t') cg_emit_raw(cg, "'\\t'");
            else if (c == '\r') cg_emit_raw(cg, "'\\r'");
            else if (c == '\\') cg_emit_raw(cg, "'\\\\'");
            else if (c == '\'') cg_emit_raw(cg, "'\\''");
            else if (c == '\0') cg_emit_raw(cg, "0");
            else cg_emit_raw(cg, "'%c'", (unsigned char)c);
            break;
        }
        case NODE_CALL: {
            const char *func = (const char *)n->data;
            /* Bug#41: 链式方法 f(...).method(args) — func=".method",
             * child0=receiver 表达式, child1=args */
            if (func && func[0] == '.' && n->child_count >= 2) {
                ASTNode *recv = n->children[0];
                ASTNode *margs = n->children[1];
                const char *m = func + 1;
                if (strcmp(m, "len") == 0) {
                    cg_emit_raw(cg, "(long long)strlen(");
                    cg_expr(cg, recv);
                    cg_emit_raw(cg, ")");
                } else if (strcmp(m, "slice") == 0) {
                    cg_emit_raw(cg, "pny_str_slice(");
                    cg_expr(cg, recv);
                    cg_emit_raw(cg, ", ");
                    if (margs && margs->child_count > 0) cg_expr(cg, margs->children[0]);
                    else cg_emit_raw(cg, "0");
                    cg_emit_raw(cg, ", ");
                    if (margs && margs->child_count > 1) cg_expr(cg, margs->children[1]);
                    else cg_emit_raw(cg, "-1");
                    cg_emit_raw(cg, ")");
                } else if (strcmp(m, "find") == 0) {
                    cg_emit_raw(cg, "pny_str_find(");
                    cg_expr(cg, recv);
                    cg_emit_raw(cg, ", ");
                    if (margs && margs->child_count > 0) cg_expr(cg, margs->children[0]);
                    cg_emit_raw(cg, ")");
                } else if (strcmp(m, "find_from") == 0) {
                    cg_emit_raw(cg, "pny_str_find_from(");
                    cg_expr(cg, recv);
                    cg_emit_raw(cg, ", ");
                    if (margs && margs->child_count > 0) cg_expr(cg, margs->children[0]);
                    else cg_emit_raw(cg, "\"\"");
                    cg_emit_raw(cg, ", ");
                    if (margs && margs->child_count > 1) cg_expr(cg, margs->children[1]);
                    else cg_emit_raw(cg, "0");
                    cg_emit_raw(cg, ")");
                } else if (strcmp(m, "contains") == 0) {
                    cg_emit_raw(cg, "pny_str_contains(");
                    cg_expr(cg, recv);
                    cg_emit_raw(cg, ", ");
                    if (margs && margs->child_count > 0) cg_expr(cg, margs->children[0]);
                    cg_emit_raw(cg, ")");
                } else {
                    fprintf(stderr, "错误: 不支持的链式方法 .%s (Bug#41 仅支持 len/slice/find/find_from/contains)\n", m);
                    cg_emit_raw(cg, "0");
                }
                break;
            }
            ASTNode *args = (n->child_count > 0 && n->children[0] &&
                             n->children[0]->data &&
                             strcmp((const char *)n->children[0]->data, "args") == 0)
                             ? n->children[0] : NULL;
            /* extern fun FFI 调用 (0.2.16): 命中注册表直发 C 调用 */
            if (func) {
                int ext_idx = -1;
                for (size_t ei = 0; ei < cg->extern_count; ei++) {
                    if (strcmp(cg->extern_funcs[ei], func) == 0) { ext_idx = (int)ei; break; }
                }
                if (ext_idx >= 0) {
                    cg_emit_raw(cg, "%s(", func);
                    if (args) {
                        for (size_t k = 0; k < args->child_count; k++) {
                            if (k) cg_emit_raw(cg, ", ");
                            if (args->children[k]) cg_expr(cg, args->children[k]);
                        }
                    }
                    cg_emit_raw(cg, ")");
                    break;
                }
            }
            if (func && strcmp(func, "print") == 0) {
                if (args && args->child_count == 1 && args->children[0]) {
                    ASTNode *a0 = args->children[0];
                    if (a0->type == NODE_STRING) {
                        char buf[512];
                        cg_emit_raw(cg, "printf(\"%s\\n\")", cg_cstr_escape(a0->data ? (const char *)a0->data : "", buf, sizeof(buf)));
                        break;
                    } else if (a0->type == NODE_INT) {
                        const char *_i = a0->data ? (const char *)a0->data : "0";
                        cg_emit_raw(cg, "printf(\"%u\\n\", %s)", _i, _i);
                        break;
                    } else if (a0->type == NODE_IDENT) {
                        const char *_id = a0->data ? (const char *)a0->data : "?";
                        if (strchr(_id, '.')) {
                            /* 字段访问表达式 (b.v / this.b.v): 交由 cg_expr 按类型字段表生成 */
                            cg_emit_raw(cg, "printf(\"%%d\\n\", (int)(");
                            cg_expr(cg, a0);
                            cg_emit_raw(cg, "))");
                            break;
                        }
                        const char *_ftype = cg_field_type(cg, _id);
                        if (!_ftype) _ftype = cg_local_actor_type(cg, _id); /* 局部 String 变量 */
                        if (_ftype && strcmp(_ftype, "String") == 0) {
                            cg_emit_raw(cg, "printf(\"%%s\\n\", (");
                            cg_emit_field_access(cg, _id);
                            cg_emit_raw(cg, ") ? (const char *)(");
                            cg_emit_field_access(cg, _id);
                            cg_emit_raw(cg, ") : \"\")");
                        } else {
                            cg_emit_raw(cg, "printf(\"%%d\\n\", (int)(");
                            cg_emit_field_access(cg, _id);
                            cg_emit_raw(cg, "))");
                        }
                        break;
                    } else if (a0->type == NODE_CALL) {
                        /* 嵌套方法调用: String 返回方法用 %s */
                        const char *cf = a0->data ? (const char *)a0->data : "";
                        const char *cm = strrchr(cf, '.');
                        cm = cm ? cm + 1 : cf;
                        if (cg_is_str_ret_method(cg, cm)) {
                            cg_emit_raw(cg, "printf(\"%%s\\n\", ");
                        } else {
                            cg_emit_raw(cg, "printf(\"%%d\\n\", ");
                        }
                        cg_expr(cg, a0);
                        cg_emit_raw(cg, ")");
                        break;
                    } else if (cg_expr_is_string(cg, a0)) {
                        /* print("..." + x) — 字符串表达式用 %s */
                        cg_emit_raw(cg, "printf(\"%%s\\n\", ");
                        cg_expr(cg, a0);
                        cg_emit_raw(cg, ")");
                        break;
                    } else if (a0->type == NODE_EMPTY || a0->type == NODE_CHAR || a0->type == NODE_BOOL || a0->type == NODE_INDEX_ACCESS) {
                        /* print(1+2), print('a'), print(true), print(arr[0]) — 复杂表达式 */
                        cg_emit_raw(cg, "printf(\"%%d\\n\", (int)");
                        cg_expr(cg, a0);
                        cg_emit_raw(cg, ")");
                        break;
                    } else {
                        const char *_d = a0->data ? (const char *)a0->data : "??";
                        cg_emit_raw(cg, "printf(\"%s\\n\", ", _d);
                        cg_emit_raw(cg, ")");
                        break;
                    }
                }
                cg_emit_raw(cg, "printf(\"\\n\")");
                break;
            }
            /* 方法调用: receiver.method() */
            if (func && strchr(func, '.')) {
                char method_name[256];
                const char *dot = strrchr(func, '.'); /* 最后一个点: 多段 receiver (Bug#26) */
                size_t recv_len = (size_t)(dot - func);
                char receiver[128];
                memcpy(receiver, func, recv_len);
                receiver[recv_len] = 0;
                strcpy(method_name, dot + 1);

                /* Bug#34: 内建特判(String/List)只对非类 receiver 生效 —
                   this.append()/this.field.append() 曾被劫持为 pny_list_append */
                int recv_not_this = (strcmp(receiver, "this") != 0);
                if (recv_not_this) {
                    const char *rt = NULL;
                    char _crecv[192];
                    if (strchr(receiver, '.') != NULL) {
                        rt = cg_chain_resolve(cg, receiver, _crecv, sizeof(_crecv));
                    } else {
                        rt = cg_local_actor_type(cg, receiver);
                        if (!rt) rt = cg_field_type(cg, receiver); /* 裸字段名 receiver (this. 已剥离) */
                    }
                    if (rt) {
                        for (size_t i = 0; i < cg->type_count; i++) {
                            if (cg->type_names[i] && strcmp(cg->type_names[i], rt) == 0) { recv_not_this = 0; break; }
                        }
                    }
                }

                /* String.len() */
                if (recv_not_this && strcmp(method_name, "len") == 0) {
                    cg_emit_raw(cg, "(int)strlen(");
                    cg_emit_receiver(cg, receiver);
                    cg_emit_raw(cg, ")");
                    break;
                }
                /* String.charAt(i) */
                if (recv_not_this && strcmp(method_name, "charAt") == 0) {
                    cg_emit_raw(cg, "(int)((");
                    cg_emit_receiver(cg, receiver);
                    cg_emit_raw(cg, ")[");
                    if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                    else cg_emit_raw(cg, "0");
                    cg_emit_raw(cg, "])");
                    break;
                }
                /* String.to_string() — 直接用原字符串 */
                if (recv_not_this && strcmp(method_name, "to_string") == 0) {
                    cg_emit_receiver(cg, receiver);
                    break;
                }
                /* String.startsWith(s) */
                if (recv_not_this && strcmp(method_name, "startsWith") == 0) {
                    cg_emit_raw(cg, "((");
                    cg_emit_receiver(cg, receiver);
                    cg_emit_raw(cg, ") && (");
                    if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                    else cg_emit_raw(cg, "\"\"");
                    cg_emit_raw(cg, " && 1)"); /* 简化: 检查非空 */
                    break;
                }
                /* String.find(sub) — 未找到返回 4294967295 (Bug#37) */
                if (recv_not_this && strcmp(method_name, "find") == 0) {
                    cg_emit_raw(cg, "pny_str_find(");
                    cg_emit_receiver(cg, receiver);
                    cg_emit_raw(cg, ", ");
                    if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                    else cg_emit_raw(cg, "\"\"");
                    cg_emit_raw(cg, ")");
                    break;
                }
                /* String.find_from(sub, off) — P1 单趟增量扫描 */
                if (recv_not_this && strcmp(method_name, "find_from") == 0) {
                    cg_emit_raw(cg, "pny_str_find_from(");
                    cg_emit_receiver(cg, receiver);
                    cg_emit_raw(cg, ", ");
                    if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                    else cg_emit_raw(cg, "\"\"");
                    cg_emit_raw(cg, ", ");
                    if (args && args->child_count > 1) cg_expr(cg, args->children[1]);
                    else cg_emit_raw(cg, "0");
                    cg_emit_raw(cg, ")");
                    break;
                }
                /* String.contains(sub) (Bug#37) */
                if (recv_not_this && strcmp(method_name, "contains") == 0) {
                    cg_emit_raw(cg, "pny_str_contains(");
                    cg_emit_receiver(cg, receiver);
                    cg_emit_raw(cg, ", ");
                    if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                    else cg_emit_raw(cg, "\"\"");
                    cg_emit_raw(cg, ")");
                    break;
                }
                /* String.slice(start[, end]) — end 缺省到尾; 越界钳位 (Bug#38) */
                if (recv_not_this && strcmp(method_name, "slice") == 0) {
                    cg_emit_raw(cg, "pny_str_slice(");
                    cg_emit_receiver(cg, receiver);
                    cg_emit_raw(cg, ", ");
                    if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                    else cg_emit_raw(cg, "0");
                    if (args && args->child_count > 1) {
                        cg_emit_raw(cg, ", ");
                        cg_expr(cg, args->children[1]);
                    } else {
                        cg_emit_raw(cg, ", -1");
                    }
                    cg_emit_raw(cg, ")");
                    break;
                }
                /* String.toUpperCase() */
                if (recv_not_this && strcmp(method_name, "toUpperCase") == 0) {
                    cg_emit_raw(cg, "((");
                    cg_emit_receiver(cg, receiver);
                    cg_emit_raw(cg, ") ? 1 : 1)"); /* placeholder — toUpperCase stub */
                    break;
                }
                /* List.append(item) → pny_list_append(self->field, item) */
                if (recv_not_this && strcmp(method_name, "append") == 0) {
                    cg_emit_raw(cg, "pny_list_append(");
                    cg_emit_receiver(cg, receiver);
                    cg_emit_raw(cg, ", ");
                    if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                    else cg_emit_raw(cg, "NULL");
                    cg_emit_raw(cg, ")");
                    break;
                }
                /* List.length → pny_list_len(self->field) */
                if (recv_not_this && strcmp(method_name, "length") == 0) {
                    cg_emit_raw(cg, "pny_list_len(");
                    cg_emit_receiver(cg, receiver);
                    cg_emit_raw(cg, ")");
                    break;
                }
                /* 字段访问: b.v → b->v; this.b.v → self->b->v (method_name 是 receiver 类型的字段) */
                {
                    const char *base_type = NULL;
                    char self_prefix[256];
                    self_prefix[0] = 0;
                    if (strcmp(receiver, "this") == 0) {
                        base_type = cg->actor_name[0] ? cg->actor_name : NULL;
                        snprintf(self_prefix, sizeof(self_prefix), "self");
                    } else if (strncmp(receiver, "this.", 5) == 0) {
                        const char *f = receiver + 5;
                        for (size_t i = 0; i < cg->field_count; i++) {
                            if (cg->fields[i] && strcmp(cg->fields[i], f) == 0) {
                                base_type = cg->field_types[i];
                                break;
                            }
                        }
                        snprintf(self_prefix, sizeof(self_prefix), "self->%s", f);
                    } else {
                        base_type = cg_local_actor_type(cg, receiver);
                        if (base_type) snprintf(self_prefix, sizeof(self_prefix), "%s", receiver);
                    }
                    if (base_type && self_prefix[0] && cg_type_has_field(cg, base_type, method_name)) {
                        cg_emit_raw(cg, "%s->%s", self_prefix, method_name);
                        break;
                    }
                }
                /* 方法调用统一分派: 局部变量 c.m() / 字段(本actor) f.m() → {T}_{m}(recv, ...) */
                {
                    const char *at = cg_local_actor_type(cg, receiver);
                    const char *ft = NULL;
                    char recv_expr[160];
                    recv_expr[0] = 0;
                    if (at) {
                        snprintf(recv_expr, sizeof(recv_expr), "%s", receiver);
                    } else if (strcmp(receiver, "this") == 0) {
                        /* this.method() → {Actor}_{method}(self) (Bug#20) */
                        if (cg->actor_name[0]) at = cg->actor_name;
                        snprintf(recv_expr, sizeof(recv_expr), "self");
                    } else if (strchr(receiver, '.') != NULL) {
                        /* 多段 receiver a.b.m() (Bug#26): 链式解析 recv_expr + 类型 */
                        char chain_recv[192];
                        const char *ct = cg_chain_resolve(cg, receiver, chain_recv, sizeof(chain_recv));
                        if (ct) {
                            at = ct;
                            snprintf(recv_expr, sizeof(recv_expr), "%s", chain_recv);
                        }
                    } else {
                        for (size_t i = 0; i < cg->field_count; i++)
                            if (cg->fields[i] && strcmp(cg->fields[i], receiver) == 0) { ft = cg->field_types[i]; break; }
                        if (ft && !cg_type_field_base(cg, ft)) ft = NULL;
                        if (ft) snprintf(recv_expr, sizeof(recv_expr), "self->%s", receiver);
                    }
                    const char *disp = at ? at : ft;
                    int disp_is_json = disp && (strcmp(disp, "JSON") == 0 || strcmp(disp, "PnyJson *") == 0);
                    if (disp && recv_expr[0] && !disp_is_json && !cg_type_field_base(cg, disp)) {
                        disp = NULL; /* 非复合类型(内置标量)不参与方法分派 */
                    }
                    if (disp && recv_expr[0] && disp_is_json) {
                        if (strcmp(method_name, "get") == 0) {
                            cg_emit_raw(cg, "pny_json_get(%s, ", recv_expr);
                            if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                            cg_emit_raw(cg, ")");
                            break;
                        }
                        if (strcmp(method_name, "set") == 0) {
                            cg_emit_raw(cg, "pny_json_set(%s, ", recv_expr);
                            if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                            cg_emit_raw(cg, ", ");
                            if (args && args->child_count > 1) cg_expr(cg, args->children[1]);
                            else cg_emit_raw(cg, "\"\"");
                            cg_emit_raw(cg, ")");
                            break;
                        }
                        if (strcmp(method_name, "to_string") == 0) {
                            cg_emit_raw(cg, "pny_json_stringify(%s)", recv_expr);
                            break;
                        }
                    }
                    if (disp && recv_expr[0]) {
                        cg_emit_raw(cg, "%s_%s(%s", disp, method_name, recv_expr);
                        if (args) {
                            for (size_t i = 0; i < args->child_count; i++) {
                                cg_emit_raw(cg, ", ");
                                cg_expr(cg, args->children[i]);
                            }
                        }
                        cg_emit_raw(cg, ")");
                        break;
                    }
                }
                /* primitive/class 静态方法: Receiver.method() → {Receiver}_{method}(NULL, args) */
                {
                    int recv_is_type = 0;
                    for (size_t i = 0; i < cg->known_actor_count; i++) {
                        if (strcmp(cg->known_actors[i], receiver) == 0) { recv_is_type = 1; break; }
                    }
                    if (recv_is_type) {
                        cg_emit_raw(cg, "%s_%s(NULL", receiver, method_name);
                        if (args) {
                            for (size_t k = 0; k < args->child_count; k++) {
                                cg_emit_raw(cg, ", ");
                                if (args->children[k]) cg_expr(cg, args->children[k]);
                            }
                        }
                        cg_emit_raw(cg, ")");
                        break;
                    }
                }
                /* 内建转换 .string(): 数值→pny_itoa(recv), String→原值 */
                if (method_name && strcmp(method_name, "string") == 0) {
                    const char *rlt = receiver ? cg_local_actor_type(cg, receiver) : NULL;
                    int r_is_str = rlt && (strcmp(rlt, "String") == 0 || strcmp(rlt, "const char *") == 0);
                    int r_is_u64 = 0;
                    if (!r_is_str && receiver && receiver[0]) {
                        const char *flt = cg_field_type(cg, receiver);
                        if (flt && (strcmp(flt, "String") == 0 || strcmp(flt, "const char *") == 0)) r_is_str = 1;
                        if (flt && strcmp(flt, "U64") == 0) r_is_u64 = 1;
                        if (rlt && strcmp(rlt, "U64") == 0) r_is_u64 = 1;
                    }
                    /* receiver 表达式输出 (receiver 在本作用域, 618行定义) */
                    if (!r_is_str) cg_emit_raw(cg, r_is_u64 ? "pny_utoa(" : "pny_itoa(");
                    if (receiver && strcmp(receiver, "this") == 0) {
                        cg_emit_raw(cg, "self");
                    } else if (receiver && strncmp(receiver, "this.", 5) == 0) {
                        cg_emit_raw(cg, "self->%s", receiver + 5);
                    } else if (receiver && strchr(receiver, '.')) {
                        cg_emit_field_access(cg, receiver);
                    } else if (receiver && (cg_is_param(cg, receiver) || cg_has_local(cg, receiver))) {
                        cg_emit_raw(cg, "%s", receiver);
                    } else if (receiver && receiver[0]) {
                        cg_emit_raw(cg, "self->%s", receiver);
                    } else {
                        if (!r_is_str) cg_emit_raw(cg, "0)");
                        break;
                    }
                    if (!r_is_str) cg_emit_raw(cg, ")");
                    break;
                }
                /* Generic method: receiver->method(...) — stub for now */
                cg_emit_raw(cg, "0"); /* stub: unknown method returns 0 */
                break;
            }
            /* 已登记 Actor 类型构造（含零参）: Counter() → Counter_create() */
            if (func && cg_is_known_actor(cg, func)) {
                cg_emit_raw(cg, "%s_create(", func);
                if (args) {
                    for (size_t i = 0; i < args->child_count; i++) {
                        if (i > 0) cg_emit_raw(cg, ", ");
                        cg_expr(cg, args->children[i]);
                    }
                }
                cg_emit_raw(cg, ")");
                break;
            }
            /* 类型构造函数: List(), Set(), Map(), ActorRef() 等 */
            if (func) {
                /* List() → pny_list_new() */
                if (strcmp(func, "List") == 0) {
                    cg_emit_raw(cg, "pny_list_new()");
                    break;
                }
                /* Set() → pny_set_new() */
                if (strcmp(func, "Set") == 0) {
                    cg_emit_raw(cg, "pny_set_new()");
                    break;
                }
                /* JSON() → pny_json_new() */
                if (strcmp(func, "JSON") == 0) {
                    cg_emit_raw(cg, "pny_json_new()");
                    break;
                }
                /* Map() → pny_map_new() */
                if (strcmp(func, "Map") == 0) {
                    cg_emit_raw(cg, "pny_map_new()");
                    break;
                }
                /* ActorRef() → NULL (无参) 或构造调用 */
                if (strcmp(func, "ActorRef") == 0) {
                    if (args && args->child_count > 0) {
                        cg_emit_raw(cg, "pny_actor_send(NULL, \"%s\", ", func);
                        cg_expr(cg, args->children[0]);
                        cg_emit_raw(cg, ")");
                    } else {
                        cg_emit_raw(cg, "NULL");
                    }
                    break;
                }
                /* String() → strdup() */
                if (strcmp(func, "String") == 0) {
                    if (args && args->child_count > 0) {
                        cg_emit_raw(cg, "strdup(");
                        cg_expr(cg, args->children[0]);
                        cg_emit_raw(cg, ")");
                    } else {
                        cg_emit_raw(cg, "strdup(\"\")");
                    }
                    break;
                }
                /* 跨 actor 构造: Actor(args) → Actor_create(args) */
                /* 如果 func 是当前 actor 名且参数匹配构造函数，改为 Actor_create(args) */
                if (strcmp(func, cg->actor_name) == 0 && cg->actor_name[0]) {
                    cg_emit_raw(cg, "%s_create(", func);
                    if (args) {
                        for (size_t i = 0; i < args->child_count; i++) {
                            if (i > 0) cg_emit_raw(cg, ", ");
                            cg_expr(cg, args->children[i]);
                        }
                    }
                    cg_emit_raw(cg, ")");
                    break;
                }
                /* 如果是其他 actor 名 (Actor, NotTheActor)，生成 Xxx_create(args) */
                /* 检查是否是已知 actor 类型 */
                int is_actor_type = 0;
                if (args && args->child_count > 0) {
                    is_actor_type = 1; /* 有参数的调用可能是 actor 构造 */
                }
                if (is_actor_type && func[0] >= 'A' && func[0] <= 'Z') {
                    /* 首字母大写，可能是 actor 类型 */
                    cg_emit_raw(cg, "%s_create(", func);
                    if (args) {
                        for (size_t i = 0; i < args->child_count; i++) {
                            if (i > 0) cg_emit_raw(cg, ", ");
                            cg_expr(cg, args->children[i]);
                        }
                    }
                    cg_emit_raw(cg, ")");
                    break;
                }
            }
            /* 内置函数: http_accept_unix(path) / http_post_unix(path, body) */
            if (func && strcmp(func, "http_accept_unix") == 0) {
                cg_emit_raw(cg, "pny_http_accept_unix(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ")");
                break;
            }
            if (func && strcmp(func, "http_post_unix") == 0) {
                cg_emit_raw(cg, "pny_http_post_unix(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ", ");
                if (args && args->child_count > 1) cg_expr(cg, args->children[1]);
                cg_emit_raw(cg, ")");
                break;
            }
            if (func && strcmp(func, "http_respond_unix") == 0) {
                cg_emit_raw(cg, "((int)pny_http_respond_unix(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, "))");
                break;
            }
            /* 内置函数: http_accept/http_respond/stdin_line/json_raw_get */
            if (func && strcmp(func, "http_accept") == 0) {
                cg_emit_raw(cg, "pny_http_accept(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ")");
                break;
            }
            if (func && strcmp(func, "http_respond") == 0) {
                cg_emit_raw(cg, "((int)pny_http_respond(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, "))");
                break;
            }
            if (func && strcmp(func, "stdin_line") == 0) {
                cg_emit_raw(cg, "pny_stdin_line()");
                break;
            }
            if (func && strcmp(func, "json_raw_get") == 0) {
                cg_emit_raw(cg, "pny_json_raw_get(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ", ");
                if (args && args->child_count > 1) cg_expr(cg, args->children[1]);
                cg_emit_raw(cg, ")");
                break;
            }
            /* 内置函数: str_hash/arg/file_append/str_field/str_to_int */
            if (func && strcmp(func, "str_hash") == 0) {
                cg_emit_raw(cg, "pny_str_hash(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ")");
                break;
            }
            if (func && strcmp(func, "arg") == 0) {
                cg_emit_raw(cg, "pny_arg(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ")");
                break;
            }
            if (func && strcmp(func, "file_append") == 0) {
                cg_emit_raw(cg, "((int)pny_file_append(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ", ");
                if (args && args->child_count > 1) cg_expr(cg, args->children[1]);
                cg_emit_raw(cg, "))");
                break;
            }
            if (func && strcmp(func, "str_field") == 0) {
                cg_emit_raw(cg, "pny_str_field(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ", ");
                if (args && args->child_count > 1) cg_expr(cg, args->children[1]);
                cg_emit_raw(cg, ", ");
                if (args && args->child_count > 2) cg_expr(cg, args->children[2]);
                cg_emit_raw(cg, ")");
                break;
            }
            if (func && strcmp(func, "str_from_char") == 0) {
                cg_emit_raw(cg, "pny_str_from_char(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ")");
                break;
            }
            /* Bug#40: str_replace_all 内建分派 (此前缺表被误分派成类方法) */
            if (func && strcmp(func, "str_replace_all") == 0) {
                cg_emit_raw(cg, "pny_str_replace_all(");
                for (size_t ai = 0; ai < args->child_count; ai++) {
                    if (ai) cg_emit_raw(cg, ", ");
                    cg_expr(cg, args->children[ai]);
                }
                cg_emit_raw(cg, ")");
                break;
            }
            if (func && strcmp(func, "str_to_int") == 0) {
                cg_emit_raw(cg, "pny_str_to_int(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ")");
                break;
            }
            /* 内置函数: http_post(url, body) / http_post_h(url, body, extra_headers) */
            if (func && (strcmp(func, "http_post") == 0 || strcmp(func, "http_post_h") == 0)) {
                cg_emit_raw(cg, "pny_http_post_h(");
                for (size_t ai = 0; ai < args->child_count; ai++) {
                    if (ai) cg_emit_raw(cg, ", ");
                    cg_expr(cg, args->children[ai]);
                }
                if (args->child_count < 3) cg_emit_raw(cg, ", NULL");
                cg_emit_raw(cg, ")");
                break;
            }
            /* 内置函数: sandbox_exec(wasm_path, fuel) → pny_sandbox_exec (需 -include wasm_exec.h) */
            if (func && strcmp(func, "sandbox_exec") == 0) {
                cg_emit_raw(cg, "pny_sandbox_exec(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ", ");
                if (args && args->child_count > 1) cg_expr(cg, args->children[1]);
                cg_emit_raw(cg, ")");
                break;
            }
            /* 内置函数: sys_exec(cmd) → pny_exec_capture(cmd) */
            if (func && strcmp(func, "sys_exec") == 0) {
                cg_emit_raw(cg, "pny_exec_capture(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ")");
                break;
            }
            /* 内置函数: file_read/file_write/file_exists */
            if (func && strcmp(func, "file_read") == 0) {
                cg_emit_raw(cg, "pny_file_read(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ")");
                break;
            }
            if (func && strcmp(func, "file_write") == 0) {
                cg_emit_raw(cg, "((int)pny_file_write(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ", ");
                if (args && args->child_count > 1) cg_expr(cg, args->children[1]);
                cg_emit_raw(cg, "))");
                break;
            }
            if (func && strcmp(func, "file_size") == 0) {
                cg_emit_raw(cg, "((int)pny_file_size(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, "))");
                break;
            }
            if (func && strcmp(func, "env_get") == 0) {
                /* M3: 环境变量读取 (native/wasi-libc getenv) */
                cg_emit_raw(cg, "pny_env_get(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ")");
                break;
            }
            if (func && strcmp(func, "char_code") == 0) {
                /* 首字节 ASCII 码 (校验和用); 空串 = 0 */
                cg_emit_raw(cg, "((int)pny_char_code(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, "))");
                break;
            }
            if (func && strcmp(func, "file_exists") == 0) {
                cg_emit_raw(cg, "((int)pny_file_exists(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, "))");
                break;
            }
            /* 内置函数: parse_json(s) → pny_json_parse(s) */
            if (func && strcmp(func, "parse_json") == 0) {
                cg_emit_raw(cg, "pny_json_parse(");
                if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                cg_emit_raw(cg, ")");
                break;
            }
            cg_emit_raw(cg, "%s_%s(", cg->actor_name[0] ? cg->actor_name : "main", func ? func : "?");
            if (args) {
                cg_emit_raw(cg, "self");
                for (size_t i = 0; i < args->child_count; i++) {
                    cg_emit_raw(cg, ", ");
                    cg_expr(cg, args->children[i]);
                }
            } else {
                cg_emit_raw(cg, "self");
            }
            cg_emit_raw(cg, ")");
            break;
        }
        case NODE_SEND: {
            /* receiver ! payload -> pny_actor_send(receiver_self, "method", payload) */
            if (n->child_count >= 1 && n->children[0]->type == NODE_IDENT) {
                const char *recv = n->children[0]->data;
                const char *recv_name = recv ? (const char*)recv : "?";
                cg_emit_raw(cg, "pny_actor_send(&%s_self, \"%s\", ", recv_name,
                            n->data && strcmp((const char*)n->data, "send") == 0 ? "handle" : (const char*)n->data);
                if (n->child_count >= 2) cg_expr(cg, n->children[1]);
                else cg_emit_raw(cg, "NULL");
                cg_emit_raw(cg, ")");
            }
            break;
        }
        case NODE_MSG_CALL: {
            /* receiver @ payload -> pny_actor_send_sync(receiver_self, "method", payload) */
            if (n->child_count >= 1 && n->children[0]->type == NODE_IDENT) {
                const char *recv = n->children[0]->data;
                const char *recv_name = recv ? (const char*)recv : "?";
                cg_emit_raw(cg, "pny_actor_send_sync(&%s_self, \"%s\", ", recv_name,
                            n->data && strcmp((const char*)n->data, "call") == 0 ? "handle" : (const char*)n->data);
                if (n->child_count >= 2) cg_expr(cg, n->children[1]);
                else cg_emit_raw(cg, "NULL");
                cg_emit_raw(cg, ")");
            }
            break;
        }
        case NODE_IDENT: {
            const char *name = (const char *)n->data;
            if (name && strncmp(name, "this.", 5) == 0) {
                /* "this.field" → self->field; "this.f.g" → self->f->g (类型字段表) */
                const char *rest = name + 5;
                const char *dot = strchr(rest, '.');
                if (dot) {
                    char seg[64];
                    size_t fl = (size_t)(dot - rest);
                    if (fl >= 64) fl = 63;
                    memcpy(seg, rest, fl);
                    seg[fl] = 0;
                    const char *ftype = NULL;
                    for (size_t i = 0; i < cg->field_count; i++)
                        if (cg->fields[i] && strcmp(cg->fields[i], seg) == 0) { ftype = cg->field_types[i]; break; }
                    if (ftype && cg_type_has_field(cg, ftype, dot + 1)) {
                        cg_emit_raw(cg, "self->%s->%s", seg, dot + 1);
                    } else {
                        cg_emit_raw(cg, "self->%s.%s", seg, dot + 1);
                    }
                } else {
                    cg_emit_raw(cg, "self->%s", rest);
                }
            } else if (name && strcmp(name, "this") == 0) {
                cg_emit_raw(cg, "self");
            } else if (name && strcmp(name, "nil") == 0) {
                cg_emit_raw(cg, "NULL");
            } else if (name && cg_is_param(cg, name)) {
                /* 参数名: 直接输出变量名, 不加 self-> */
                cg_emit_raw(cg, "%s", name);
            } else if (name && cg_has_local(cg, name)) {
                /* 局部变量 (var r: I32 = f()): 直接输出变量名 */
                cg_emit_raw(cg, "%s", name);
            } else if (name && strchr(name, '.')) {
                /* 局部变量字段访问: b.v → b->v (类型字段表分派) */
                char recv[128];
                const char *dot = strchr(name, '.');
                size_t rl = (size_t)(dot - name);
                if (rl >= 128) rl = 127;
                memcpy(recv, name, rl);
                recv[rl] = 0;
                const char *rt = cg_local_actor_type(cg, recv);
                if (rt && strchr(dot + 1, '.') != NULL) {
                    /* 多段链 a.b.c (Bug#22): class 字段皆指针, 全段 -> */
                    cg_emit_raw(cg, "%s->", recv);
                    for (const char *q = dot + 1; *q; q++) {
                        if (*q == '.') cg_emit_raw(cg, "->");
                        else cg_emit_raw(cg, "%c", *q);
                    }
                } else if (rt && cg_type_has_field(cg, rt, dot + 1)) {
                    cg_emit_raw(cg, "%s->%s", recv, dot + 1);
                } else {
                    /* Bug#63: 首段非局部变量 — 检查是否 self 字段 */
                    int is_self_field = 0;
                    for (size_t i = 0; i < cg->field_count; i++) {
                        if (cg->fields[i] && strcmp(cg->fields[i], recv) == 0) {
                            is_self_field = 1;
                            break;
                        }
                    }
                    if (is_self_field) {
                        /* self.pt.peers → self->pt->peers */
                        cg_emit_raw(cg, "self->%s->", recv);
                        for (const char *q = dot + 1; *q; q++) {
                            if (*q == '.') cg_emit_raw(cg, "->");
                            else cg_emit_raw(cg, "%c", *q);
                        }
                    } else {
                        cg_emit_field_access(cg, name);
                    }
                }
            } else if (name && cg_is_known_actor(cg, name)) {
                /* primitive 单例值引用: var f: Fmt = Fmt → NULL (无实例) */
                cg_emit_raw(cg, "NULL");
            } else if (name) {
                cg_emit_field_access(cg, name);
            }
            break;
        }
        /* 引用能力: 类型修饰, 代码生成只输出子类型 */
        case NODE_CAP:
            if (n->child_count > 0) cg_expr(cg, n->children[0]);
            else if (n->data) cg_emit_raw(cg, "%s", (const char *)n->data);
            break;
        /* import/use: 生成预处理 include 或注释 */
        case NODE_IMPORT:
            if (n->data) {
                cg_emit_raw(cg, "// import %s\n", (const char *)n->data);
            }
            break;
        /* match 表达式: 生成 switch/if-else 链 */
        case NODE_MATCH: {
            if (n->child_count >= 2 && n->children[0]) {
                ASTNode *mexpr = n->children[0];
                int first = 1;
                /* 具体分支(通配符延后) */
                for (size_t i = 1; i < n->child_count; i++) {
                    ASTNode *arm = n->children[i];
                    if (!arm || arm->child_count < 2) continue;
                    ASTNode *pat = arm->children[0];
                    if (pat && pat->type == NODE_IDENT && pat->data &&
                        strcmp((const char *)pat->data, "_") == 0) continue;
                    if (first) { cg_emit_raw(cg, "if ("); first = 0; }
                    else cg_emit_raw(cg, "else if (");
                    if (pat && (pat->type == NODE_INT || pat->type == NODE_CHAR)) {
                        cg_expr(cg, mexpr);
                        cg_emit_raw(cg, " == ");
                        cg_expr(cg, pat);
                    } else {
                        cg_emit_raw(cg, "strcmp(");
                        cg_expr(cg, mexpr);
                        cg_emit_raw(cg, ", ");
                        cg_expr(cg, pat);
                        cg_emit_raw(cg, ") == 0");
                    }
                    cg_emit_raw(cg, ") {\n");
                    cg_push(cg);
                    cg_stmt(cg, arm->children[1]);
                    cg_pop(cg);
                    cg_emit(cg, "}\n");
                }
                /* 通配分支: else 收尾 */
                for (size_t i = 1; i < n->child_count; i++) {
                    ASTNode *arm = n->children[i];
                    if (!arm || arm->child_count < 2) continue;
                    ASTNode *pat = arm->children[0];
                    if (!(pat && pat->type == NODE_IDENT && pat->data &&
                          strcmp((const char *)pat->data, "_") == 0)) continue;
                    if (first) { cg_emit_raw(cg, "{\n"); first = 0; }
                    else cg_emit_raw(cg, "else {\n");
                    cg_push(cg);
                    cg_stmt(cg, arm->children[1]);
                    cg_pop(cg);
                    cg_emit(cg, "}\n");
                }
                cg_emit_raw(cg, "\n");
            }
            break;
        }
        /* 索引访问: arr[index] */
        case NODE_INDEX_ACCESS: {
            if (n->child_count == 2 && n->children[0] && n->children[1]) {
                cg_expr(cg, n->children[0]);
                cg_emit_raw(cg, "[");
                cg_expr(cg, n->children[1]);
                cg_emit_raw(cg, "]");
            }
            break;
        }
        case NODE_EMPTY:
            if (n->data) {
                const char *d = (const char *)n->data;
                if (strcmp(d, "assign") == 0) {
                    if (n->child_count > 0) cg_expr(cg, n->children[0]);
                    cg_emit_raw(cg, " = ");
                    if (n->child_count > 1) cg_expr(cg, n->children[1]);
                } else if (strcmp(d, "or") == 0 || strcmp(d, "and") == 0) {
                    /* 逻辑运算符 */
                    cg_emit_raw(cg, "(");
                    if (n->child_count > 0) cg_expr(cg, n->children[0]);
                    cg_emit_raw(cg, " %s ", strcmp(d, "and") == 0 ? "&&" : "||");
                    if (n->child_count > 1) cg_expr(cg, n->children[1]);
                    cg_emit_raw(cg, ")");
                } else if (strcmp(d, "==") == 0 || strcmp(d, "!=") == 0 ||
                           strcmp(d, "<") == 0 || strcmp(d, ">") == 0 ||
                           strcmp(d, "<=") == 0 || strcmp(d, ">=") == 0) {
                    /* 比较运算符; 任一侧是字符串表达式 → strcmp 内容比较
                       (Bug#30: 字面量判定不够 — var/concat/builtin调用同样可能是 String)
                       Bug#43: 此前仅 ==/!= 走 strcmp, < > <= >= 落到裸指针比较
                       (字面量/同缓冲区偶然正确, 分配字符串必错) — 全部比较统一 strcmp */
                    int str_cmp = 0;
                    if (n->child_count >= 2 && n->children[0] && n->children[1] &&
                        (n->children[0]->type == NODE_STRING || n->children[1]->type == NODE_STRING ||
                         cg_expr_is_string(cg, n->children[0]) || cg_expr_is_string(cg, n->children[1]))) {
                        str_cmp = 1;
                    }
                    if (str_cmp) {
                        cg_emit_raw(cg, "(strcmp(");
                        cg_expr(cg, n->children[0]);
                        cg_emit_raw(cg, ", ");
                        cg_expr(cg, n->children[1]);
                        cg_emit_raw(cg, ") %s 0)", d);
                    } else {
                        cg_emit_raw(cg, "(");
                        if (n->child_count > 0) cg_expr(cg, n->children[0]);
                        cg_emit_raw(cg, " %s ", d);
                        if (n->child_count > 1) cg_expr(cg, n->children[1]);
                        cg_emit_raw(cg, ")");
                    }
                } else if (strcmp(d, "+") == 0 && cg_expr_is_string(cg, n)) {
                    /* String 拼接 → pny_str_concat (非字符串侧自动 pny_itoa, Bug#23) */
                    cg_emit_raw(cg, "pny_str_concat(");
                    if (n->child_count > 0) {
                        if (cg_expr_is_string(cg, n->children[0])) cg_expr(cg, n->children[0]);
                        else { cg_emit_raw(cg, "pny_itoa("); cg_expr(cg, n->children[0]); cg_emit_raw(cg, ")"); }
                    }
                    cg_emit_raw(cg, ", ");
                    if (n->child_count > 1) {
                        if (cg_expr_is_string(cg, n->children[1])) cg_expr(cg, n->children[1]);
                        else { cg_emit_raw(cg, "pny_itoa("); cg_expr(cg, n->children[1]); cg_emit_raw(cg, ")"); }
                    }
                    cg_emit_raw(cg, ")");
                } else if (strcmp(d, "+") == 0) {
                    /* 加法: 需要处理 char 和 int */
                    cg_emit_raw(cg, "((int)(");
                    if (n->child_count > 0) cg_expr(cg, n->children[0]);
                    cg_emit_raw(cg, ") + (int)(");
                    if (n->child_count > 1) cg_expr(cg, n->children[1]);
                    cg_emit_raw(cg, "))");
                } else if (strcmp(d, "bitor") == 0 || strcmp(d, "bitxor") == 0 ||
                           strcmp(d, "bitand") == 0 || strcmp(d, "shl") == 0 ||
                           strcmp(d, "shr") == 0) {
                    /* 位运算/移位 (TLS ChaCha20): 裸 C 运算符 — U32 位级语义 */
                    const char *bop = strcmp(d, "bitor") == 0 ? "|" :
                                      strcmp(d, "bitxor") == 0 ? "^" :
                                      strcmp(d, "bitand") == 0 ? "&" :
                                      strcmp(d, "shl") == 0 ? "<<" : ">>";
                    cg_emit_raw(cg, "(");
                    if (n->child_count > 0) cg_expr(cg, n->children[0]);
                    cg_emit_raw(cg, " %s ", bop);
                    if (n->child_count > 1) cg_expr(cg, n->children[1]);
                    cg_emit_raw(cg, ")");
                } else if (strcmp(d, "-") == 0 || strcmp(d, "*") == 0 || strcmp(d, "/") == 0 || strcmp(d, "%") == 0) {
                    /* 算术运算符 */
                    cg_emit_raw(cg, "((int)(");
                    if (n->child_count > 0) cg_expr(cg, n->children[0]);
                    cg_emit_raw(cg, ") %s (int)(", d);
                    if (n->child_count > 1) cg_expr(cg, n->children[1]);
                    cg_emit_raw(cg, "))");
                } else if (strcmp(d, "not") == 0) {
                    /* 一元取反 */
                    cg_emit_raw(cg, "!(");
                    if (n->child_count > 0) cg_expr(cg, n->children[0]);
                    cg_emit_raw(cg, ")");
                } else if (strcmp(d, "neg") == 0) {
                    /* 一元负号 */
                    cg_emit_raw(cg, "-(");
                    if (n->child_count > 0) cg_expr(cg, n->children[0]);
                    cg_emit_raw(cg, ")");
                } else if (strcmp(d, "bitnot") == 0) {
                    /* 位非 ~ (U32 位级补码语义) */
                    cg_emit_raw(cg, "(unsigned int)~(");
                    if (n->child_count > 0) cg_expr(cg, n->children[0]);
                    cg_emit_raw(cg, ")");
                } else {
                    cg_emit_raw(cg, "%s", d);
                }
            } else if (n->child_count > 0 && n->children[0]) {
                cg_expr(cg, n->children[0]);
            }
            break;
        default:
            if (n->data) cg_emit_raw(cg, "%s", (const char *)n->data);
            break;
    }
}

static void cg_stmt(Codegen *cg, ASTNode *n) {
    if (!n) return;
    if (n->type == NODE_EMPTY) {
        /* return 语句: return expr; */
        if (n->data && strcmp((const char *)n->data, "return") == 0) {
            cg_emit_raw(cg, "return ");
            if (n->child_count > 0) cg_expr(cg, n->children[0]);
            cg_emit_raw(cg, ";\n");
            return;
        }
        if (n->data && strcmp((const char *)n->data, "break") == 0) {
            cg_emit(cg, "break;\n");
            return;
        }
        if (n->data && strcmp((const char *)n->data, "continue") == 0) {
            cg_emit(cg, "continue;\n");
            return;
        }
        if (n->data && strcmp((const char *)n->data, "error") == 0) {
            cg_emit(cg, "/* error */ return 0;\n");
            return;
        }
        /* 赋值: assign(ident, rhs) 及复合赋值 add/sub/mul/div-assign → lhs = lhs op rhs */
        if (n->data && n->child_count >= 2) {
            const char *dstr = (const char *)n->data;
            const char *compound_op = NULL;
            if (strcmp(dstr, "assign") != 0) {
                if (strcmp(dstr, "add-assign") == 0) compound_op = "+";
                else if (strcmp(dstr, "sub-assign") == 0) compound_op = "-";
                else if (strcmp(dstr, "mul-assign") == 0) compound_op = "*";
                else if (strcmp(dstr, "div-assign") == 0) compound_op = "/";
            }
            if (strcmp(dstr, "assign") == 0 || compound_op) {
            /* 左值 */
            if (n->children[0]->type == NODE_IDENT && n->children[0]->data) {
                const char *lhs = (const char *)n->children[0]->data;
                if (lhs[0] == 't' && lhs[1] == 'h' && lhs[2] == 'i' && lhs[3] == 's' && lhs[4] == '.') {
                    /* "this.field" → self->field */
                    cg_emit_raw(cg, "/* stmt */self->%s = ", lhs + 5);
                    if (compound_op) cg_emit_raw(cg, "self->%s %s ", lhs + 5, compound_op);
                } else if (cg_is_param(cg, lhs)) {
                    /* 参数赋值: 直接输出 */
                    cg_emit_raw(cg, "/* stmt */%s = ", lhs);
                    if (compound_op) cg_emit_raw(cg, "%s %s ", lhs, compound_op);
                } else if (cg_has_field(cg, lhs)) {
                    /* 裸字段名 (如 pos = 0) → self->pos = 0 */
                    cg_emit_raw(cg, "/* stmt */self->%s = ", lhs);
                    if (compound_op) cg_emit_raw(cg, "self->%s %s ", lhs, compound_op);
                } else if (strchr(lhs, '.')) {
                    /* 点链左值 (g.signature / a.b.c): 经 cg_chain_resolve → recv->field (nanonode S3) */
                    char chain_recv[256];
                    chain_recv[0] = 0;
                    cg_chain_resolve(cg, lhs, chain_recv, sizeof(chain_recv));
                    cg_emit_raw(cg, "/* stmt */%s = ", chain_recv[0] ? chain_recv : lhs);
                    if (compound_op) cg_emit_raw(cg, "%s %s ", chain_recv[0] ? chain_recv : lhs, compound_op);
                } else {
                    /* 普通标识符 (局部变量) */
                    cg_emit_raw(cg, "/* stmt */%s = ", lhs);
                    if (compound_op) cg_emit_raw(cg, "%s %s ", lhs, compound_op);
                }
                cg_expr(cg, n->children[1]);
                cg_emit_raw(cg, ";");
                return;
            }
            }
        }
        /* 块节点: 遍历子节点 */
        for (size_t i = 0; i < n->child_count; i++) cg_stmt(cg, n->children[i]);
        return;
    }
    switch (n->type) {
        case NODE_IF: {
            cg_emit(cg, "if (");
            if (n->child_count > 0) cg_expr(cg, n->children[0]);
            cg_emit_raw(cg, ") {\n");
            cg_push(cg);
            if (n->child_count > 1) {
                for (size_t i = 0; i < n->children[1]->child_count; i++) cg_stmt(cg, n->children[1]->children[i]);
            }
            cg_pop(cg);
            cg_emit(cg, "}");
            if (n->child_count > 2 && n->children[2]) {
                cg_emit_raw(cg, " else {\n"); cg_push(cg);
                for (size_t i = 0; i < n->children[2]->child_count; i++) cg_stmt(cg, n->children[2]->children[i]);
                cg_pop(cg); cg_emit(cg, "}");
            }
            cg_emit_raw(cg, "\n");
            break;
        }
        case NODE_WHILE: {
            cg_emit(cg, "while (");
            if (n->child_count > 0) cg_expr(cg, n->children[0]);
            cg_emit_raw(cg, ") {\n");
            cg_push(cg);
            if (n->child_count > 1) {
                for (size_t i = 0; i < n->children[1]->child_count; i++) cg_stmt(cg, n->children[1]->children[i]);
            }
            cg_pop(cg);
            cg_emit(cg, "}\n");
            break;
        }
        case NODE_FOR: {
            /* children[0]=var, [1]=range_expr, [2]=body */
            char *var_name = n->child_count > 0 ? n->children[0]->ident : "_i";
            if (!var_name) var_name = "_i";
            int start = 0, end = 10;
            if (n->child_count > 1) {
                ASTNode *re = n->children[1];
                if (re && re->child_count >= 2) {
                    start = re->children[0]->value_int;
                    end = re->children[1]->value_int;
                } else if (re && re->value_int) {
                    end = re->value_int;
                }
            }
            cg_emit_raw(cg, "for (unsigned long long ");
            cg_emit(cg, var_name);
            cg_emit_raw(cg, " = ");
            cg_emit(cg, "%d", start);
            cg_emit_raw(cg, "; ");
            cg_emit(cg, var_name);
            cg_emit_raw(cg, " < ");
            cg_emit(cg, "%d", end);
            cg_emit_raw(cg, "; ");
            cg_emit(cg, var_name);
            cg_emit_raw(cg, "++) {\n");
            cg_push(cg);
            if (n->child_count > 2) {
                for (size_t i = 0; i < n->children[2]->child_count; i++) cg_stmt(cg, n->children[2]->children[i]);
            }
            cg_pop(cg);
            cg_emit(cg, "}\n");
            break;
        }
        case NODE_MATCH:
            cg_expr(cg, n); /* match 语句: 生成 if/else 链 */
            break;
        case NODE_RETURN:
            cg_emit(cg, "return");
            if (n->child_count > 0) { cg_emit_raw(cg, " "); cg_expr(cg, n->children[0]); }
            cg_emit_raw(cg, ";\n");
            break;
        case NODE_LET: {
            if (!n->data) break;
            if (n->child_count > 0 && n->children[0]->type == NODE_CAP && n->children[0]->data && strcmp((const char *)n->children[0]->data, "type") == 0) {
                /* val y: Type = expr → Ctype y = expr; */
                const char *ptype = cg_builtin_type((const char *)n->children[0]->children[0]->data);
                cg_emit_raw(cg, "%s %s", ptype, n->data);
                if (n->child_count > 1) {
                    cg_emit_raw(cg, " = ");
                    cg_expr(cg, n->children[1]);
                }
            } else {
                /* val y = expr → void *y = expr; */
                /* 登记局部变量类型: 从构造函数调用推断类型 */
                if (n->child_count > 0 && n->children[0]->type == NODE_CALL && n->children[0]->data) {
                    const char *ctor = (const char *)n->children[0]->data;
                    /* Counter() → Counter */
                    for (size_t i = 0; i < cg->known_actor_count; i++) {
                        if (strcmp(cg->known_actors[i], ctor) == 0) {
                            cg_local_add(cg, n->data, ctor);
                            break;
                        }
                    }
                }
                cg_emit_raw(cg, "void *%s", n->data);
                if (n->child_count > 0) {
                    cg_emit_raw(cg, " = ");
                    cg_expr(cg, n->children[0]);
                } else {
                    cg_emit_raw(cg, " = NULL");
                }
            }
            cg_emit_raw(cg, ";\n");
            break;
        }
        case NODE_VAR: {
            if (!n->data) break;
            if (n->child_count > 0 && n->children[0]->type == NODE_CAP && n->children[0]->data && strcmp((const char *)n->children[0]->data, "type") == 0) {
                /* var x: Type = expr → Ctype x = expr; */
                const char *vtype_name = (const char *)n->children[0]->children[0]->data;
                if (cg_is_known_actor(cg, vtype_name)) {
                    /* 局部 actor 变量: 指针类型 + 登记供方法调用分派 */
                    cg_emit_raw(cg, "%s_t *%s", vtype_name, n->data);
                    cg_local_add(cg, n->data, vtype_name);
                } else if (vtype_name && strcmp(vtype_name, "JSON") == 0) {
                    cg_emit_raw(cg, "PnyJson *%s", n->data);
                    cg_local_add(cg, n->data, "JSON");
                } else {
                    const char *ptype = cg_builtin_type(vtype_name);
                    cg_emit_raw(cg, "%s %s", ptype, n->data);
                    cg_local_add(cg, n->data, vtype_name); /* 登记供打印/分派 */
                }
                if (n->child_count > 1) {
                    cg_emit_raw(cg, " = ");
                    cg_expr(cg, n->children[1]);
                }
            } else {
                /* var x = expr → void *x = expr; */
                cg_emit_raw(cg, "void *%s", n->data);
                if (n->child_count > 0) {
                    cg_emit_raw(cg, " = ");
                    cg_expr(cg, n->children[0]);
                } else {
                    cg_emit_raw(cg, " = NULL");
                }
            }
            cg_emit_raw(cg, ";\n");
            break;
        }
        default:
            cg_emit(cg, "/* stmt */");
            cg_expr(cg, n);
            cg_emit_raw(cg, ";\n");
            break;
    }
}

static void cg_actor(Codegen *cg, ASTNode *actor,
                         const char **actor_types, size_t atc) {
    const char *name = (const char *)actor->data;
    if (!name) return;
    snprintf(cg->actor_name, sizeof(cg->actor_name), "%s", name);

    char **field_names = NULL;
    char **field_type_names = NULL;
    size_t fc = 0;
    for (size_t i = 0; i < actor->child_count; i++) {
        ASTNode *ch = actor->children[i];
        if (ch && ch->type == NODE_VAR && ch->data) {
            char **tmp = (char **)realloc(field_names, (fc + 1) * sizeof(char *));
            if (!tmp) {
                for (size_t j = 0; j < fc; j++) free(field_names[j]);
                free(field_names);
                return;
            }
            field_names = tmp;
            field_names[fc] = s_strdup((const char *)ch->data);

            /* 记录字段类型 */
            char **tmp_types = (char **)realloc(field_type_names, (fc + 1) * sizeof(char *));
            if (!tmp_types) {
                for (size_t j = 0; j < fc; j++) { free(field_names[j]); free(field_type_names[j]); }
                free(field_names);
                free(field_type_names);
                return;
            }
            field_type_names = tmp_types;
            /* 从类型节点获取类型名 */
            const char *ft = NULL;
            if (ch->child_count > 0 && ch->children[0] && ch->children[0]->data) {
                ft = (const char *)ch->children[0]->data;
            }
            field_type_names[fc] = s_strdup(ft ? ft : "int");
            fc++;
        }
    }
    cg_set_fields_with_types(cg, fc, field_names, field_type_names);

    /* Actor 结构体 */
    cg_emit(cg, "typedef struct {\n");
    cg_push(cg);
    for (size_t i = 0; i < actor->child_count; i++) {
        ASTNode *ch = actor->children[i];
        if (ch && ch->type == NODE_VAR) {
            const char *fn = (const char *)ch->data;
            const char *ft = NULL;
            if (ch->child_count > 0 && ch->children[0]) {
                ft = cg_type_of(ch->children[0], actor_types, atc);
                /* 检测是否为 Actor 类型名（需在末尾加 _t） */
                int is_actor = 0;
                for (size_t ai = 0; ai < atc; ai++) {
                    if (actor_types[ai] && strcmp(actor_types[ai], ft) == 0) { is_actor = 1; break; }
                }
                if (is_actor) {
                    static char buf[64];
                    snprintf(buf, sizeof(buf), "%s_t *", ft);
                    ft = buf;
                }
            } else ft = "int";
            cg_emit(cg, "%s %s;", ft, fn ? fn : "f");
        }
    }
    cg_pop(cg);
    cg_emit(cg, "} %s_t;\n\n", name);

    /* 如果没有显式构造函数, 生成默认构造器 */
    int has_ctor = 0;
    for (size_t i = 0; i < actor->child_count; i++) {
        if (actor->children[i] && actor->children[i]->type == NODE_NEW) { has_ctor = 1; break; }
    }
    if (!has_ctor) {
        cg_emit(cg, "static %s_t *%s_create() {\n", name, name);
        cg_push(cg);
        cg_emit(cg, "%s_t *self = (%s_t *)calloc(1, sizeof(%s_t));\n", name, name, name);
        cg_emit(cg, "memset(self, 0, sizeof(*self));\n");
        cg_emit(cg, "return self;\n");
        cg_pop(cg);
        cg_emit(cg, "}\n\n");
    }

    /* 前向声明: 允许方法在定义前被调用 (Bug: implicit int 冲突) */
    for (size_t i = 0; i < actor->child_count; i++) {
        ASTNode *ch = actor->children[i];
        if (!ch) continue;
        if (ch->type == NODE_NEW) {
            const char *ctor = (const char *)ch->data;
            cg_emit(cg, "static %s_t * %s_%s(", name, name, ctor ? ctor : "new");
            if (ch->child_count > 0 && ch->children[0] && ch->children[0]->data &&
                strcmp((const char *)ch->children[0]->data, "params") == 0) {
                for (size_t j = 0; j < ch->children[0]->child_count; j++) {
                    ASTNode *p = ch->children[0]->children[j];
                    if (j) cg_emit_raw(cg, ", ");
                    const char *pt_raw = (p->child_count > 0 && p->children[0]) ? cg_type_of(p->children[0], actor_types, atc) : "int";
                    const char *pt = pt_raw;
                    {
                        static char pbuf[80];
                        int pt_actor = 0;
                        for (size_t ai = 0; ai < atc; ai++) {
                            if (actor_types && actor_types[ai] && strcmp(actor_types[ai], pt_raw) == 0) { pt_actor = 1; break; }
                        }
                        if (pt_actor) { snprintf(pbuf, sizeof(pbuf), "%s_t *", pt_raw); pt = pbuf; }
                    }
                    cg_emit_raw(cg, "%s %s", pt, p->data ? (const char *)p->data : "a");
                }
            }
            cg_emit_raw(cg, ");\n");
        } else if (ch->type == NODE_BE || ch->type == NODE_FUN) {
            const char *fn = (const char *)ch->data;
            const char *rtype = (ch->type == NODE_FUN) ? "int" : "void";
            ASTNode *params = NULL;
            ASTNode *body = NULL;
            int rtype_locked = 0; /* 返回类型标注识别后锁定, body内ident不再覆盖 */
            for (size_t j = 0; j < ch->child_count; j++) {
                ASTNode *c2 = ch->children[j];
                if (c2->type == NODE_EMPTY && params == NULL &&
                    c2->data && strcmp((const char *)c2->data, "params") == 0) {
                    params = c2;
                } else if (c2->type == NODE_STRING && body == NULL && !rtype_locked) {
                    rtype = "const char *";
                } else if (c2->type == NODE_IDENT && c2->data && body == NULL && !rtype_locked) {
                    const char *tn = (const char *)c2->data;
                    if (strcmp(tn, "Bool") == 0) { rtype = "int"; rtype_locked = 1; }
                    else if (strcmp(tn, "String") == 0) { rtype = "const char *"; rtype_locked = 1; }
                    else if (strcmp(tn, "I64") == 0) { rtype = "signed long long"; rtype_locked = 1; }
                    else if (strcmp(tn, "I32") == 0) { rtype = "signed int"; rtype_locked = 1; }
                    else if (strcmp(tn, "U64") == 0) { rtype = "unsigned long long"; rtype_locked = 1; }
                    else if (strcmp(tn, "U32") == 0) { rtype = "unsigned int"; rtype_locked = 1; }
                    else if (strcmp(tn, "F64") == 0) { rtype = "double"; rtype_locked = 1; }
                    else if (strcmp(tn, "F32") == 0) { rtype = "float"; rtype_locked = 1; }
                    else if (strcmp(tn, "ActorRef") == 0) { rtype = "void *"; rtype_locked = 1; }
                    else if (strcmp(tn, "Char") == 0) { rtype = "char"; rtype_locked = 1; }
                    else {
                        int is_actor = 0;
                        for (size_t k = 0; k < atc; k++) {
                            if (actor_types && actor_types[k] && strcmp(actor_types[k], tn) == 0) {
                                is_actor = 1; break;
                            }
                        }
                        rtype = is_actor ? "void *" : cg_type_of(c2, actor_types, atc);
                    }
                } else if (c2->type == NODE_TYPE_PARAM && body == NULL) {
                    rtype = cg_type_of(c2, actor_types, atc);
                } else if (c2->type == NODE_EMPTY && !c2->data && body == NULL) {
                    body = c2;
                }
            }
            cg_emit(cg, "static %s %s_%s(%s_t *self", rtype, name, fn ? fn : "m", name);
            if (params && params->data && strcmp((const char *)params->data, "params") == 0 && params->child_count > 0) {
                for (size_t j = 0; j < params->child_count; j++) {
                    ASTNode *p = params->children[j];
                    cg_emit_raw(cg, ", ");
                    const char *pt_raw = (p->child_count > 0 && p->children[0]) ? cg_type_of(p->children[0], actor_types, atc) : "int";
                    const char *pt = pt_raw;
                    {
                        static char pbuf[80];
                        int pt_actor = 0;
                        for (size_t ai = 0; ai < atc; ai++) {
                            if (actor_types && actor_types[ai] && strcmp(actor_types[ai], pt_raw) == 0) { pt_actor = 1; break; }
                        }
                        if (pt_actor) { snprintf(pbuf, sizeof(pbuf), "%s_t *", pt_raw); pt = pbuf; }
                    }
                    cg_emit_raw(cg, "%s %s", pt, p->data ? (const char *)p->data : "a");
                }
            }
            cg_emit_raw(cg, ");\n");
        }
    }
    cg_emit(cg, "\n");

    for (size_t i = 0; i < actor->child_count; i++) {
        ASTNode *ch = actor->children[i];
        if (!ch) continue;
        if (ch->type == NODE_NEW) {
            cg->local_var_count = 0; /* 构造函数作用域: 重置局部/参数表 */
            const char *ctor = (const char *)ch->data;
            cg_emit(cg, "static %s_t *%s_%s(", name, name, ctor ? ctor : "new");
            if (ch->child_count > 0 && ch->children[0] && ch->children[0]->data &&
                strcmp((const char *)ch->children[0]->data, "params") == 0) {
                for (size_t j = 0; j < ch->children[0]->child_count; j++) {
                    ASTNode *p = ch->children[0]->children[j];
                    if (j) cg_emit_raw(cg, ", ");
                    const char *pt_raw = (p->child_count > 0 && p->children[0]) ? cg_type_of(p->children[0], actor_types, atc) : "int";
                    const char *pt = pt_raw;
                    {
                        static char pbuf[80];
                        int pt_actor = 0;
                        for (size_t ai = 0; ai < atc; ai++) {
                            if (actor_types[ai] && strcmp(actor_types[ai], pt_raw) == 0) { pt_actor = 1; break; }
                        }
                        if (pt_actor) { snprintf(pbuf, sizeof(pbuf), "%s_t *", pt_raw); pt = pbuf; }
                    }
                    const char *pn = (const char *)p->data;
                    cg_emit_raw(cg, "%s %s", pt, pn ? pn : "a");
                    cg_local_add(cg, pn, pt_raw); /* 参数登记: 供字段/方法分派 */
                }
            }
            cg_emit_raw(cg, ") {\n");
            cg_push(cg);
            /* 收集参数名, 用于参数 vs 字段区分 */
            {
                size_t pc = 0;
                char **param_names = NULL;
                if (ch->child_count > 0 && ch->children[0] && ch->children[0]->data &&
                    strcmp((const char *)ch->children[0]->data, "params") == 0) {
                    ASTNode *pnode = ch->children[0];
                    param_names = (char **)calloc(pnode->child_count, sizeof(char *));
                    for (size_t j = 0; j < pnode->child_count; j++) {
                        ASTNode *p = pnode->children[j];
                        param_names[j] = s_strdup(p->data ? (const char *)p->data : "a");
                        pc++;
                    }
                }
                cg_set_params(cg, pc, param_names);
            }
            cg_emit(cg, "%s_t *self = (%s_t *)calloc(1, sizeof(%s_t));\n", name, name, name);
            cg_emit(cg, "memset(self, 0, sizeof(*self));\n");
            cg_set_ctor(cg, 1);
            /* 找到构造体 (跳过 params 节点) */
            ASTNode *body = NULL;
            if (getenv("PONYPP_DEBUG_MAIN")) {
                fprintf(stderr, "[DEBUG-CTOR] name=%s child_count=%zu\n", name, ch->child_count);
                for (size_t bi = 0; bi < ch->child_count; bi++) {
                    ASTNode *bc = ch->children[bi];
                    fprintf(stderr, "[DEBUG-CTOR] child[%zu] type=%d data=%s cc=%zu\n", bi, bc ? bc->type : -1, bc && bc->data ? (const char*)bc->data : "(null)", bc ? bc->child_count : 0);
                }
            }
            for (size_t bi = 0; bi < ch->child_count; bi++) {
                if (ch->children[bi]->type == NODE_EMPTY && ch->children[bi]->child_count > 0 &&
                    !(ch->children[bi]->data && strcmp((const char *)ch->children[bi]->data, "params") == 0)) {
                    body = ch->children[bi];
                }
            }
            if (body) {
                for (size_t j = 0; j < body->child_count; j++) cg_stmt(cg, body->children[j]);
            }
            cg_emit(cg, "return self;\n");
            cg_pop(cg);
            cg_emit(cg, "}\n\n");
            cg_set_ctor(cg, 0);
        } else if (ch->type == NODE_BE || ch->type == NODE_FUN) {
            cg->local_var_count = 0; /* 方法作用域: 重置局部/参数表 */
            const char *fn = (const char *)ch->data;
            const char *rtype = (ch->type == NODE_FUN) ? "int" : "void";
            ASTNode *params = NULL;
            ASTNode *body = NULL;
            for (size_t j = 0; j < ch->child_count; j++) {
                ASTNode *c2 = ch->children[j];
                if (c2->type == NODE_EMPTY && params == NULL &&
                    c2->data && strcmp((const char *)c2->data, "params") == 0) {
                    params = c2;
                } else if (c2->type == NODE_EMPTY && !c2->data) {
                    /* NODE_EMPTY 无 data = 方法体 */
                    if (body == NULL) body = c2;
                } else if (c2->type == NODE_STRING) {
                    rtype = "const char *";
                } else if (c2->type == NODE_IDENT && c2->data && body == NULL) {
                    /* NODE_IDENT 且不是 body = 返回类型 */
                    const char *tn = (const char *)c2->data;
                    if (strcmp(tn, "Bool") == 0) rtype = "int";
                    else if (strcmp(tn, "String") == 0) rtype = "const char *";
                    else if (strcmp(tn, "I64") == 0) rtype = "signed long long";
                    else if (strcmp(tn, "I32") == 0) rtype = "signed int";
                    else if (strcmp(tn, "U64") == 0) rtype = "unsigned long long";
                    else if (strcmp(tn, "U32") == 0) rtype = "unsigned int";
                    else if (strcmp(tn, "F64") == 0) rtype = "double";
                    else if (strcmp(tn, "F32") == 0) rtype = "float";
                    else if (strcmp(tn, "ActorRef") == 0) rtype = "void *";
                    else if (strcmp(tn, "Char") == 0) rtype = "char";
                    else {
                        /* 检查是否为 actor 类型 → void * */
                        int is_actor = 0;
                        for (size_t k = 0; k < atc; k++) {
                            if (actor_types && actor_types[k] && strcmp(actor_types[k], tn) == 0) {
                                is_actor = 1; break;
                            }
                        }
                        rtype = is_actor ? "void *" : cg_type_of(c2, actor_types, atc);
                    }
                } else if (c2->type == NODE_TYPE_PARAM && body == NULL) {
                    /* NODE_TYPE_PARAM = 泛型返回类型如 List[Actor] */
                    rtype = cg_type_of(c2, actor_types, atc);
                } else if (body == NULL) {
                    body = c2;
                }
            }
            cg_emit(cg, "static %s %s_%s(%s_t *self", rtype, name, fn ? fn : "m", name);
            if (params && params->data && strcmp((const char *)params->data, "params") == 0 && params->child_count > 0) {
                for (size_t j = 0; j < params->child_count; j++) {
                    ASTNode *p = params->children[j];
                    cg_emit_raw(cg, ", ");
                    const char *pt_raw = (p->child_count > 0 && p->children[0]) ? cg_type_of(p->children[0], actor_types, atc) : "int";
                    const char *pt = pt_raw;
                    {
                        static char pbuf[80];
                        int pt_actor = 0;
                        for (size_t ai = 0; ai < atc; ai++) {
                            if (actor_types[ai] && strcmp(actor_types[ai], pt_raw) == 0) { pt_actor = 1; break; }
                        }
                        if (pt_actor) { snprintf(pbuf, sizeof(pbuf), "%s_t *", pt_raw); pt = pbuf; }
                    }
                    const char *pn = (const char *)p->data;
                    cg_emit_raw(cg, "%s %s", pt, pn ? pn : "a");
                    cg_local_add(cg, pn, pt_raw); /* 参数登记: 供字段/方法分派 */
                }
            }
            cg_emit_raw(cg, ") {\n");
            cg_push(cg);
            /* 收集方法参数名 */
            {
                size_t pc = 0;
                char **param_names = NULL;
                if (params && params->child_count > 0) {
                    param_names = (char **)calloc(params->child_count, sizeof(char *));
                    for (size_t j = 0; j < params->child_count; j++) {
                        ASTNode *p = params->children[j];
                        param_names[j] = s_strdup(p->data ? (const char *)p->data : "a");
                        pc++;
                    }
                }
                cg_set_params(cg, pc, param_names);
            }
            cg_set_ctor(cg, 0);
            int _fun_ret_handled = 0;
            if (body) {
                /* fun 返回值: 如果 body 只有一个子节点且是表达式类型, 生成 return */
                if (ch->type == NODE_FUN && body->child_count == 1 && rtype && strcmp(rtype, "void") != 0) {
                    ASTNode *last = body->children[0];
                    int last_is_return = (last && last->type == NODE_RETURN);
                    if (last && last->type == NODE_EMPTY && last->data && strcmp((const char *)last->data, "return") == 0) last_is_return = 1;
                    if (last && !last_is_return && last->type != NODE_IF && last->type != NODE_WHILE && last->type != NODE_FOR && last->type != NODE_MATCH && last->type != NODE_PRINT && last->type != NODE_ASSERT) {
                        cg_emit(cg, "return ");
                        cg_expr(cg, last);
                        cg_emit(cg, ";\n");
                        _fun_ret_handled = 1;
                    }
                }
                if (!_fun_ret_handled) {
                    for (size_t j = 0; j < body->child_count; j++) cg_stmt(cg, body->children[j]);
                }
            }
            if (!_fun_ret_handled && strcmp(rtype, "void") != 0) {
                /* 检查 body 最后一个子节点是否是 NODE_RETURN */
                int has_return = 0;
                if (body && body->child_count > 0) {
                    ASTNode *last_stmt = body->children[body->child_count - 1];
                    if (last_stmt && last_stmt->type == NODE_RETURN) has_return = 1;
                    if (last_stmt && last_stmt->type == NODE_EMPTY && last_stmt->data && strcmp((const char *)last_stmt->data, "return") == 0) has_return = 1;
                }
                if (!has_return) {
                    if (strcmp(rtype, "const char *") == 0) cg_emit(cg, "return NULL;\n");
                    else cg_emit(cg, "return 0;\n");
                }
            }
            cg_pop(cg);
            cg_emit(cg, "}\n\n");
        }
    }
}

static void cg_emit_runtime(Codegen *cg) {
    cg_emit_raw(cg,
        "typedef struct PnyList {\n"
        "    void **items;\n"
        "    size_t len, cap;\n"
        "} PnyList;\n"
        "static PnyList *pny_list_new(void) {\n"
        "    PnyList *l = (PnyList *)calloc(1, sizeof(PnyList));\n"
        "    return l;\n}\n"
        "static void pny_list_append(PnyList *l, void *data) {\n"
        "    if (!l) return;\n"
        "    if (l->len >= l->cap) {\n"
        "        l->cap = l->cap ? l->cap * 2 : 8;\n"
        "        l->items = (void **)realloc(l->items, l->cap * sizeof(void *));\n"
        "    }\n"
        "    l->items[l->len++] = data;\n}\n"
        "static size_t pny_list_len(const PnyList *l) { return l ? l->len : 0; }\n"
        "typedef struct PnySet {\n"
        "    void **items;\n"
        "    size_t len, cap;\n"
        "} PnySet;\n"
        "static PnySet *pny_set_new(void) { return (PnySet *)calloc(1, sizeof(PnySet)); }\n"
        "typedef struct PnyMap {\n"
        "    void **items;\n"
        "    size_t len, cap;\n"
        "} PnyMap;\n"
        "static PnyMap *pny_map_new(void) { return (PnyMap *)calloc(1, sizeof(PnyMap)); }\n"
        "typedef struct PnyActor {\n"
        "    const char *name;\n"
        "    void *state;\n"
        "    size_t state_size;\n"
        "    struct PnyActor *next;\n"
        "    struct PnyMessage *messages;\n"
        "} PnyActor;\n\n"
        "typedef struct PnyMessage {\n"
        "    char *method;\n"
        "    void *arg;\n"
        "    struct PnyMessage *next;\n"
        "} PnyMessage;\n\n"
        "typedef struct PnyRuntime {\n"
        "    PnyActor *actors;\n"
        "    size_t actor_count;\n"
        "} PnyRuntime;\n\n"
        "static PnyRuntime *pny_runtime_global = NULL;\n\n"
        "static PnyRuntime *pny_runtime_new(void) {\n"
        "    PnyRuntime *r = (PnyRuntime *)calloc(1, sizeof(PnyRuntime));\n"
        "    if (!r) return NULL;\n"
        "    pny_runtime_global = r;\n"
        "    return r;\n}\n\n"
        "static void pny_actor_register(PnyRuntime *r, PnyActor *a) {\n"
        "    if (!r || !a) return;\n"
        "    a->next = r->actors;\n"
        "    r->actors = a;\n}\n\n"
        "static PnyMessage *pny_msg_new(const char *m, void *arg) {\n"
        "    PnyMessage *msg = (PnyMessage *)malloc(sizeof(PnyMessage));\n"
        "    if (!msg) return NULL;\n"
        "    if (m) { msg->method = (char *)malloc(strlen(m) + 1); strcpy(msg->method, m); }\n        else msg->method = NULL;\n"
        "    msg->arg = arg;\n"
        "    msg->next = NULL;\n"
        "    return msg;\n}\n\n"
        "static void pny_actor_send(PnyActor *a, const char *m, void *arg) {\n"
        "    if (!a) return;\n"
        "    PnyMessage *msg = pny_msg_new(m, arg);\n"
        "    if (!msg) return;\n"
        "    if (a->messages) {\n"
        "        PnyMessage *tail = a->messages;\n"
        "        while (tail->next) tail = tail->next;\n"
        "        tail->next = msg;\n"
        "    } else { a->messages = msg; }\n}\n\n"
        "static void pny_runtime_free(PnyRuntime *r) {\n"
        "    if (!r) return;\n"
        "    PnyActor *a = r->actors;\n"
        "    while (a) { PnyActor *n = a->next;\n"
        "        PnyMessage *m = a->messages; while (m) { PnyMessage *mn = m->next; free(m->method); free(m); m = mn; }\n"
        "        free(a->state); free(a); a = n;\n"
        "    }\n"
        "    free(r);\n}\n\n"
        "static PnyActor *pny_actor_new(const char *nm, size_t sz) {\n"
        "    PnyActor *a = (PnyActor *)malloc(sizeof(PnyActor));\n"
        "    if (!a) return NULL;\n"
        "    a->name = nm;\n"
        "    a->state = sz > 0 ? calloc(1, sz) : NULL;\n"
        "    a->state_size = sz;\n"
        "    a->next = NULL;\n"
        "    a->messages = NULL;\n"
        "    return a;\n}\n\n");
}

static int cg_ast_uses_json(ASTNode *n) {
    if (!n) return 0;
    if (n->data) {
        const char *d = (const char *)n->data;
        if (strcmp(d, "JSON") == 0 || strcmp(d, "parse_json") == 0) return 1;
        if (strstr(d, "json")) return 1;
    }
    for (size_t i = 0; i < n->child_count; i++)
        if (cg_ast_uses_json(n->children[i])) return 1;
    return 0;
}

static const char *PNY_PEX_RUNTIME =
"\n/* ===== ponyexecution 内联运行时 ===== */\nstatic char *pny_str_hash(const char *s) {\n    unsigned long h = 5381;\n    if (s) for (const char *p = s; *p; p++) h = h * 33 + (unsigned char)*p;\n    char *out = (char *)pny_xmalloc(24);\n    if (out) snprintf(out, 24, \"%016lx\", h);\n    return out;\n}\n/* Bug#40: str_replace_all 运行时 — 全量替换 */\nstatic char *pny_str_replace_all(const char *s, const char *old_, const char *nw) {\n    if (!s) return (char *)\"\";\n    if (!old_ || !*old_) return strdup(s);\n    if (!nw) nw = \"\";\n    size_t ol = strlen(old_), nl = strlen(nw);\n    size_t cap = strlen(s) + 1; char *out = (char *)malloc(cap);\n    if (!out) return (char *)\"\";\n    size_t olen = 0; const char *p = s;\n    const char *hit;\n    while ((hit = strstr(p, old_)) != NULL) {\n        size_t seg = (size_t)(hit - p);\n        while (olen + seg + nl + 1 > cap) { cap *= 2; out = (char *)realloc(out, cap); }\n        memcpy(out + olen, p, seg); olen += seg;\n        memcpy(out + olen, nw, nl); olen += nl;\n        p = hit + ol;\n    }\n    size_t rest = strlen(p);\n    while (olen + rest + 1 > cap) { cap *= 2; out = (char *)realloc(out, cap); }\n    memcpy(out + olen, p, rest); olen += rest;\n    out[olen] = 0;\n    return out;\n}\nstatic char *pny_arg(int i) {\n    FILE *f = fopen(\"/proc/self/cmdline\", \"rb\");\n    if (!f) return (char *)\"\";\n    static char buf[8192];\n    size_t n = fread(buf, 1, sizeof(buf) - 1, f);\n    fclose(f);\n    buf[n] = 0;\n    int idx = 0;\n    char *p = buf;\n    while (idx < i) {\n        while ((size_t)(p - buf) < n && *p) p++;\n        if ((size_t)(p - buf) >= n) return (char *)\"\";\n        p++;\n        idx++;\n    }\n    return p;\n}\nstatic int pny_file_append(const char *path, const char *content) {\n    if (!path) return 0;\n    FILE *f = fopen(path, \"ab\");\n    if (!f) return 0;\n    size_t len = content ? strlen(content) : 0;\n    size_t wr = len ? fwrite(content, 1, len, f) : 0;\n    fclose(f);\n    return wr == len;\n}\nstatic char *pny_str_field(const char *s, int idx, const char *sep) {\n    if (!s || !sep) return (char *)\"\";\n    const char *p = s;\n    int cur = 0;\n    size_t seplen = strlen(sep);\n    while (cur < idx) {\n        const char *hit = strstr(p, sep);\n        if (!hit) return (char *)\"\";\n        p = hit + seplen;\n        cur++;\n    }\n    const char *end = strstr(p, sep);\n    size_t len = end ? (size_t)(end - p) : strlen(p);\n    char *out = (char *)pny_xmalloc(len + 1);\n    if (!out) return (char *)\"\";\n    memcpy(out, p, len);\n    out[len] = 0;\n    return out;\n}\nstatic int pny_str_to_int(const char *s) {\n    return s ? atoi(s) : 0;\n}\n";

static const char *PNY_PEX2_RUNTIME =
"\n/* ===== ponyexecution M1b 内联: HTTP/stdio/JSON原始提取 ===== */\n#include <sys/socket.h>\n#include <netinet/in.h>\n#include <arpa/inet.h>\n#include <unistd.h>\n#include <poll.h>\nstatic int pny_http_fd = -1;\nstatic int pny_http_conn = -1;\nstatic int pny_http_keep = 0;\nstatic char *pny_http_accept(int port) {\n    if (port <= 0) return (char *)\"\";\n    if (pny_http_fd < 0) {\n        pny_http_fd = socket(AF_INET, SOCK_STREAM, 0);\n        if (pny_http_fd < 0) return (char *)\"\";\n        int yes = 1;\n        setsockopt(pny_http_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));\n        struct sockaddr_in addr;\n        memset(&addr, 0, sizeof(addr));\n        addr.sin_family = AF_INET;\n        addr.sin_addr.s_addr = htonl(INADDR_ANY);\n        addr.sin_port = htons((uint16_t)port);\n        if (bind(pny_http_fd, (struct sockaddr *)&addr, (socklen_t)sizeof(addr)) < 0) { close(pny_http_fd); pny_http_fd = -1; return (char *)\"\"; }\n        listen(pny_http_fd, 128);\n    }\n    /* keep-alive: 复用已有连接(poll 300ms 无数据/已关→回退新连接) */\n    if (pny_http_conn >= 0) {\n        struct pollfd pf;\n        pf.fd = pny_http_conn; pf.events = POLLIN;\n        int pr = poll(&pf, 1, 300);\n        if (pr <= 0) { close(pny_http_conn); pny_http_conn = -1; }\n        else { char pk; ssize_t rr = recv(pny_http_conn, &pk, 1, MSG_PEEK); if (rr <= 0) { close(pny_http_conn); pny_http_conn = -1; } }\n    }\n    if (pny_http_conn < 0) {\n        pny_http_conn = accept(pny_http_fd, NULL, NULL);\n        if (pny_http_conn < 0) return (char *)\"\";\n    }\n    /* 块读请求头(1024B/次, 替代逐字节 read) */\n    static char hdr[65536];\n    size_t hlen = 0;\n    char *sep = NULL;\n    while (hlen < sizeof(hdr) - 1) {\n        ssize_t r = read(pny_http_conn, hdr + hlen, 1024);\n        if (r <= 0) { close(pny_http_conn); pny_http_conn = -1; return (char *)\"\"; }\n        hlen += (size_t)r;\n        hdr[hlen] = 0;\n        sep = strstr(hdr, \"\\r\\n\\r\\n\");\n        if (sep) break;\n    }\n    if (!sep) { close(pny_http_conn); pny_http_conn = -1; return (char *)\"\"; }\n    pny_http_keep = (strstr(hdr, \"Connection: close\") == NULL) ? 1 : 0;\n    const char *clp = strstr(hdr, \"Content-Length:\");\n    long cl = clp ? strtol(clp + 15, NULL, 10) : 0;\n    if (cl < 0 || cl > 65535) { close(pny_http_conn); pny_http_conn = -1; return (char *)\"\"; }\n    size_t body_start = (size_t)(sep - hdr) + 4;\n    static char body[65536];\n    size_t got = 0;\n    if (hlen > body_start) {\n        got = hlen - body_start;\n        if (got > (size_t)cl) got = (size_t)cl;\n        memcpy(body, hdr + body_start, got);\n    }\n    while (got < (size_t)cl) {\n        ssize_t r = read(pny_http_conn, body + got, (size_t)cl - got);\n        if (r <= 0) { close(pny_http_conn); pny_http_conn = -1; return (char *)\"\"; }\n        got += (size_t)r;\n    }\n    body[cl] = 0;\n    return body;\n}\nstatic int pny_http_respond(const char *json) {\n    if (pny_http_conn < 0) return 0;\n    size_t len = json ? strlen(json) : 0;\n    char hdr[256];\n    int hl = snprintf(hdr, sizeof(hdr),\n        \"HTTP/1.1 200 OK\\r\\nContent-Type: application/json\\r\\nContent-Length: %zu\\r\\nConnection: %s\\r\\n\\r\\n\",\n        len, pny_http_keep ? \"keep-alive\" : \"close\");\n    if (write(pny_http_conn, hdr, (size_t)hl) < 0) { /* ignore */ }\n    if (len) { if (write(pny_http_conn, json, len) < 0) { /* ignore */ } }\n    if (!pny_http_keep) { close(pny_http_conn); pny_http_conn = -1; }\n    return 1;\n}\nstatic char *pny_stdin_line(void) {\n    static char buf[65536];\n    if (!fgets(buf, (int)sizeof(buf), stdin)) return (char *)\"\";\n    size_t n = strlen(buf);\n    while (n && (buf[n-1]=='\\n' || buf[n-1]=='\\r')) buf[--n] = 0;\n    return buf;\n}\nstatic char *pny_json_raw_get(const char *s, const char *key) {\n    if (!s || !key) return (char *)\"\";\n    char pat[256];\n    snprintf(pat, sizeof(pat), \"\\\"%s\\\"\", key);\n    const char *p = strstr(s, pat);\n    /* Bug#29: key-name collision (value-of-type precedes key-text) => colon-follow check */\n    while (p) {\n        const char *after = p + strlen(pat);\n        while (*after == ' ') after++;\n        if (*after == ':') break;\n        p = strstr(p + 1, pat);\n    }\n    if (!p) return (char *)\"\";\n    p += strlen(pat);\n    while (*p == ' ' || *p == ':') p++;\n    if (*p == '{' || *p == '[') {\n        char open = *p, close = (open == '{') ? '}' : ']';\n        int depth = 0;\n        const char *q = p;\n        int in_str = 0;\n        for (; *q; q++) {\n            if (*q == '\"' && q > p && q[-1] != '\\\\') in_str = !in_str;\n            if (in_str) continue;\n            if (*q == open) depth++;\n            else if (*q == close) { depth--; if (!depth) { q++; break; } }\n        }\n        size_t len = (size_t)(q - p);\n        char *out = (char *)malloc(len + 1);\n        if (!out) return (char *)\"\";\n        memcpy(out, p, len); out[len] = 0;\n        return out;\n    }\n    if (*p == '\"') {\n        p++;\n        const char *e = p;\n        while (*e && !(*e == '\"' && e[-1] != '\\\\')) e++;\n        size_t len = (size_t)(e - p);\n        char *out = (char *)malloc(len + 1);\n        if (!out) return (char *)\"\";\n        memcpy(out, p, len); out[len] = 0;\n        return out;\n    }\n    const char *e = p;\n    while (*e && *e != ',' && *e != '}' && *e != ']') e++;\n    size_t len = (size_t)(e - p);\n    while (len && (e[-1] == ' ' || e[-1] == '\\n')) { e--; len--; }\n    char *out = (char *)malloc(len + 1);\n    if (!out) return (char *)\"\";\n    memcpy(out, p, len); out[len] = 0;\n    return out;\n}\n";

static const char *PNY_EXEC_RUNTIME =
"\n/* ===== exec 内联运行时 (M2: 真实命令执行, 输出截断8KB) ===== */\nstatic char *pny_exec_capture(const char *cmd) {\n    if (!cmd) return (char *)\"\";\n    FILE *p = popen(cmd, \"r\");\n    if (!p) return (char *)\"EXEC: popen failed\";\n    char *buf = (char *)malloc(8192);\n    if (!buf) { pclose(p); return (char *)\"EXEC: oom\"; }\n    size_t n = fread(buf, 1, 8191, p);\n    buf[n] = 0;\n    int rc = pclose(p);\n    if (n == 0 && rc != 0) {\n        snprintf(buf, 8192, \"EXEC: exit=%d\", rc);\n    }\n    return buf;\n}\n";

static const char *PNY_FILE_RUNTIME =
"\n/* ===== File IO 内联运行时 ===== */\nstatic char *pny_file_read(const char *path) {\n    if (!path) return (char *)\"\";\n    FILE *f = fopen(path, \"rb\");\n    if (!f) return (char *)\"\";\n    fseek(f, 0, SEEK_END);\n    long n = ftell(f);\n    fseek(f, 0, SEEK_SET);\n    if (n < 0) { fclose(f); return (char *)\"\"; }\n    char *buf = (char *)malloc((size_t)n + 1);\n    if (!buf) { fclose(f); return (char *)\"\"; }\n    size_t rd = fread(buf, 1, (size_t)n, f);\n    buf[rd] = 0;\n    fclose(f);\n    return buf;\n}\nstatic int pny_file_write(const char *path, const char *content) {\n    if (!path) return 0;\n    FILE *f = fopen(path, \"wb\");\n    if (!f) return 0;\n    size_t len = content ? strlen(content) : 0;\n    size_t wr = len ? fwrite(content, 1, len, f) : 0;\n    fclose(f);\n    return wr == len;\n}\nstatic int pny_file_exists(const char *path) {\n    if (!path) return 0;\n    FILE *f = fopen(path, \"rb\");\n    if (!f) return 0;\n    fclose(f);\n    return 1;\n}\nstatic int pny_file_size(const char *path) {\n    if (!path) return 0;\n    FILE *f = fopen(path, \"rb\");\n    if (!f) return 0;\n    fseek(f, 0, SEEK_END);\n    long n = ftell(f);\n    fclose(f);\n    return (int)(n < 0 ? 0 : n);\n}\nstatic int pny_char_code(const char *s) {\n    if (!s || !*s) return 0;\n    return (int)(unsigned char)s[0];\n}\nstatic const char *pny_env_get(const char *name) {\n    if (!name) return \"\";\n    const char *v = getenv(name);\n    return v ? v : \"\";\n}\n";

static const char *PNY_STR_RUNTIME =
"\n/* ===== String concat 内联运行时 ===== */\nstatic char *pny_str_concat(const char *a, const char *b) {\n    if (!a) a = \"\"; if (!b) b = \"\";\n    size_t na = strlen(a), nb = strlen(b);\n    char *r = (char *)pny_xmalloc(na + nb + 1);\n    if (!r) return (char *)\"\";\n    memcpy(r, a, na); memcpy(r + na, b, nb + 1);\n    return r;\n}\n/* ===== 长度感知字符串 API (M3: 显式长度免 strlen; 签名对齐 extern fun 生成原型) ===== */\nconst char *pny_str_concat_n(const char *a, unsigned long long na, const char *b, unsigned long long nb) {\n    if (!a) { a = \"\"; na = 0; }\n    if (!b) { b = \"\"; nb = 0; }\n    char *r = (char *)pny_xmalloc((size_t)(na + nb + 1));\n    if (!r) return \"\";\n    memcpy(r, a, (size_t)na); memcpy(r + na, b, (size_t)nb + 1);\n    return r;\n}\nconst char *pny_str_slice_n(const char *s, unsigned int n, unsigned int start, unsigned int end) {\n    if (!s) return \"\";\n    if (start > n) start = n;\n    if (end > n) end = n;\n    if (end < start) end = start;\n    unsigned int len = end - start;\n    char *out = (char *)pny_xmalloc((size_t)len + 1);\n    if (!out) return \"\";\n    memcpy(out, s + start, (size_t)len);\n    out[len] = 0;\n    return out;\n}\nstatic char *pny_itoa(long long v) {\n    char *out = (char *)pny_xmalloc(24);\n    if (out) snprintf(out, 24, \"%lld\", v);\n    return out;\n}\nstatic char *pny_utoa(unsigned long long v) {\n    char *out = (char *)pny_xmalloc(24);\n    if (out) snprintf(out, 24, \"%llu\", v);\n    return out;\n}\nstatic char *pny_str_from_char(int c) {\n    char *out = (char *)pny_xmalloc(2);\n    if (out) { out[0] = (char)c; out[1] = 0; }\n    return out;\n}\nstatic long long pny_str_find(const char *s, const char *sub) {\n    if (!s || !sub) return 4294967295LL;\n    const char *p = strstr(s, sub);\n    return p ? (long long)(p - s) : 4294967295LL;\n}\n/* P1: find_from — 从偏移起查找, 单趟增量扫描替代 O(n^2) 重复 str_field */\nstatic long long pny_str_find_from(const char *s, const char *sub, long long from) {\n    if (!s || !sub) return 4294967295LL;\n    size_t n2 = strlen(s);\n    if (from < 0) from = 0;\n    if ((size_t)from > n2) return 4294967295LL;\n    const char *p2 = strstr(s + from, sub);\n    return p2 ? (long long)(p2 - s) : 4294967295LL;\n}\nstatic int pny_str_contains(const char *s, const char *sub) {\n    if (!s || !sub) return 0;\n    return strstr(s, sub) != NULL;\n}\nstatic char *pny_str_slice(const char *s, long long start, long long end) {\n    if (!s) return (char *)\"\";\n    size_t n = strlen(s);\n    if (start < 0) start = 0;\n    if ((size_t)start > n) start = (long long)n;\n    if (end < 0 || (size_t)end > n) end = (long long)n;\n    if (end < start) end = start;\n    size_t len = (size_t)(end - start);\n    char *out = (char *)pny_xmalloc(len + 1);\n    if (!out) return (char *)\"\";\n    memcpy(out, s + start, len);\n    out[len] = 0;\n    return out;\n}\n";

static const char *PNY_ARENA_RUNTIME =
"\n/* ===== Arena: integer-offset page pool (M3) - internal malloc, process-lifetime ===== */\n"
"typedef struct PnyArena { char *data; unsigned int len; unsigned int cap; } PnyArena;\n"
"static PnyArena *pny_arena_tab[256];\n"
"unsigned int pny_arena_new(unsigned int cap) {\n"
"    unsigned int h = 1;\n"
"    while (h < 256 && pny_arena_tab[h]) h++;\n"
"    if (h >= 256) return 0;\n"
"    PnyArena *a = (PnyArena *)malloc(sizeof(PnyArena));\n"
"    if (!a) return 0;\n"
"    if (cap < 65536) cap = 65536;\n"
"    a->data = (char *)malloc(cap);\n"
"    if (!a->data) { free(a); return 0; }\n"
"    a->len = 0; a->cap = cap;\n"
"    pny_arena_tab[h] = a;\n"
"    return h;\n"
"}\n"
"unsigned int pny_arena_append(unsigned int h, const char *s, unsigned int n) {\n"
"    if (h == 0 || h >= 256 || !pny_arena_tab[h] || !s) return 0;\n"
"    PnyArena *a = pny_arena_tab[h];\n"
"    if (a->len + n > a->cap) {\n"
"        unsigned int ncap = a->cap ? a->cap * 2 : 65536;\n"
"        while (ncap < a->len + n) ncap *= 2;\n"
"        char *nd = (char *)malloc(ncap);\n"
"        if (!nd) return 0;\n"
"        memcpy(nd, a->data, a->len);\n"
"        free(a->data);\n"
"        a->data = nd; a->cap = ncap;\n"
"    }\n"
"    unsigned int off = a->len;\n"
"    memcpy(a->data + a->len, s, n);\n"
"    a->len += n;\n"
"    return off;\n"
"}\n"
"const char *pny_arena_get(unsigned int h, unsigned int off, unsigned int n) {\n"
"    if (h == 0 || h >= 256 || !pny_arena_tab[h]) return \"\";\n"
"    PnyArena *a = pny_arena_tab[h];\n"
"    if (off >= a->len) return \"\";\n"
"    if (off + n > a->len) n = a->len - off;\n"
"    char *out = (char *)pny_xmalloc((size_t)n + 1);\n"
"    if (!out) return \"\";\n"
"    memcpy(out, a->data + off, n);\n"
"    out[n] = 0;\n"
"    return out;\n"
"}\n"
"unsigned int pny_arena_len(unsigned int h) {\n"
"    if (h == 0 || h >= 256 || !pny_arena_tab[h]) return 0;\n"
"    return pny_arena_tab[h]->len;\n"
"}\n"
"const char *pny_bytes32(unsigned int b0, unsigned int b1, unsigned int b2, unsigned int b3, unsigned int b4, unsigned int b5, unsigned int b6, unsigned int b7, unsigned int b8, unsigned int b9, unsigned int b10, unsigned int b11, unsigned int b12, unsigned int b13, unsigned int b14, unsigned int b15, unsigned int b16, unsigned int b17, unsigned int b18, unsigned int b19, unsigned int b20, unsigned int b21, unsigned int b22, unsigned int b23, unsigned int b24, unsigned int b25, unsigned int b26, unsigned int b27, unsigned int b28, unsigned int b29, unsigned int b30, unsigned int b31) {\n"
"    char *out = (char *)pny_xmalloc(33);\n"
"    out[0]=(char)b0;out[1]=(char)b1;out[2]=(char)b2;out[3]=(char)b3;out[4]=(char)b4;out[5]=(char)b5;out[6]=(char)b6;out[7]=(char)b7;out[8]=(char)b8;out[9]=(char)b9;out[10]=(char)b10;out[11]=(char)b11;out[12]=(char)b12;out[13]=(char)b13;out[14]=(char)b14;out[15]=(char)b15;out[16]=(char)b16;out[17]=(char)b17;out[18]=(char)b18;out[19]=(char)b19;out[20]=(char)b20;out[21]=(char)b21;out[22]=(char)b22;out[23]=(char)b23;out[24]=(char)b24;out[25]=(char)b25;out[26]=(char)b26;out[27]=(char)b27;out[28]=(char)b28;out[29]=(char)b29;out[30]=(char)b30;out[31]=(char)b31;out[32]=0;\n"
"    return out;\n"
"}\n"
"unsigned long long pny_u64_add(unsigned long long a, unsigned long long b) { return a + b; }\n"
"unsigned long long pny_u64_sub(unsigned long long a, unsigned long long b) { return a - b; }\n"
"unsigned long long pny_u64_mul(unsigned long long a, unsigned long long b) { __uint128_t p = (__uint128_t)a * (__uint128_t)b; return (unsigned long long)p; }\n"
"unsigned long long pny_u64_mulh(unsigned long long a, unsigned long long b) { __uint128_t p = (__uint128_t)a * (__uint128_t)b; return (unsigned long long)(p >> 64); }\n"
"unsigned long long pny_u64_not(unsigned long long a) { return ~a; }\n"
"unsigned int pny_str_u8(const char *s, unsigned int off) {\n"
"    if (!s) return 0;\n"
"    return (unsigned int)(unsigned char)s[off];\n"
"}\n"
"unsigned int pny_str_region_cmp(const char *s, unsigned int off, unsigned int len, const char *key) {\n"
"    if (!s || !key) return 1;\n"
"    size_t kl = strlen(key);\n"
"    size_t ml = (kl < (size_t)len) ? kl : (size_t)len;\n"
"    int c = memcmp(s + off, key, ml);\n"
"    if (c == 0) {\n"
"        if ((size_t)len == kl) return 0;\n"
"        return ((size_t)len > kl) ? 1 : 2;\n"
"    }\n"
"    return c > 0 ? 1 : 2;\n"
"}\n"
"unsigned int pny_arena_reset(unsigned int h) {\n"
"    if (h == 0 || h >= 256 || !pny_arena_tab[h]) return 0;\n"
"    pny_arena_tab[h]->len = 0;\n"
"    return 1;\n"
"}\n";

static const char *PNY_JSON_RUNTIME =
"""\n/* ===== inline JSON runtime (flat string objects, Ponypi M0) ===== */\ntypedef struct PnyJsonPair { char *key; char *val; } PnyJsonPair;\ntypedef struct PnyJson { PnyJsonPair *pairs; int count; int cap; } PnyJson;\n\nstatic char *pj_strdup(const char *s) {\n    if (!s) s = \"\";\n    size_t n = strlen(s) + 1;\n    char *d = (char *)malloc(n);\n    memcpy(d, s, n);\n    return d;\n}\nstatic char *pj_unescape(const char *s, const char **end) {\n    size_t cap = 64, len = 0;\n    char *out = (char *)malloc(cap);\n    while (*s && *s != '\"') {\n        char c = *s++;\n        if (c == '\\\\' && *s) {\n            char e = *s++;\n            if (e == 'n') c = '\\n';\n            else if (e == 't') c = '\\t';\n            else c = e;\n        }\n        if (len + 2 > cap) { cap *= 2; out = (char *)realloc(out, cap); }\n        out[len++] = c;\n    }\n    out[len] = 0;\n    if (*s == '\"') s++;\n    if (end) *end = s;\n    return out;\n}\nstatic void pj_skip_ws(const char **s) {\n    while (**s == ' ' || **s == '\\t' || **s == '\\n' || **s == '\\r') (*s)++;\n}\nstatic PnyJson *pny_json_new(void) { return (PnyJson *)calloc(1, sizeof(PnyJson)); }\nstatic PnyJson *pny_json_parse(const char *s) {\n    PnyJson *j = pny_json_new();\n    const char *p = s;\n    if (!p) return j;\n    pj_skip_ws(&p);\n    if (*p != '{') return j;\n    p++;\n    pj_skip_ws(&p);\n    while (*p && *p != '}') {\n        if (*p != '\"') break;\n        p++;\n        const char *e = NULL;\n        char *k = pj_unescape(p, &e);\n        p = e;\n        pj_skip_ws(&p);\n        if (*p != ':') { free(k); break; }\n        p++;\n        pj_skip_ws(&p);\n        char *v = NULL;\n        if (*p == '\"') { p++; v = pj_unescape(p, &e); p = e; }\n        else {\n            const char *st = p;\n            while (*p && *p != ',' && *p != '}') p++;\n            v = (char *)malloc((size_t)(p - st) + 1);\n            memcpy(v, st, (size_t)(p - st));\n            v[p - st] = 0;\n        }\n        if (j->count == j->cap) {\n            j->cap = j->cap ? j->cap * 2 : 8;\n            j->pairs = (PnyJsonPair *)realloc(j->pairs, (size_t)j->cap * sizeof(PnyJsonPair));\n        }\n        j->pairs[j->count].key = k;\n        j->pairs[j->count].val = v;\n        j->count++;\n        pj_skip_ws(&p);\n        if (*p == ',') { p++; pj_skip_ws(&p); }\n    }\n    return j;\n}\nstatic const char *pny_json_get(PnyJson *j, const char *k) {\n    if (!j || !k) return \"\";\n    for (int i = 0; i < j->count; i++)\n        if (strcmp(j->pairs[i].key, k) == 0) return j->pairs[i].val ? j->pairs[i].val : \"\";\n    return \"\";\n}\nstatic void pny_json_set(PnyJson *j, const char *k, const char *v) {\n    if (!j || !k) return;\n    for (int i = 0; i < j->count; i++) {\n        if (strcmp(j->pairs[i].key, k) == 0) {\n            free(j->pairs[i].val);\n            j->pairs[i].val = pj_strdup(v);\n            return;\n        }\n    }\n    if (j->count == j->cap) {\n        j->cap = j->cap ? j->cap * 2 : 8;\n        j->pairs = (PnyJsonPair *)realloc(j->pairs, (size_t)j->cap * sizeof(PnyJsonPair));\n    }\n    j->pairs[j->count].key = pj_strdup(k);\n    j->pairs[j->count].val = pj_strdup(v);\n    j->count++;\n}\nstatic char *pj_escape(const char *s) {\n    size_t cap = 64, len = 0;\n    char *out = (char *)malloc(cap);\n    for (; s && *s; s++) {\n        char c = *s;\n        char buf[2];\n        int n = 1;\n        buf[0] = c;\n        if (c == '\"' || c == '\\\\') { buf[0] = c; n = 2; }\n        if (len + (size_t)n + 3 > cap) { cap = (len + (size_t)n + 3) * 2; out = (char *)realloc(out, cap); }\n        if (n == 2) { out[len++] = '\\\\'; out[len++] = buf[0]; }\n        else if (c == '\\n') { out[len++] = '\\\\'; out[len++] = 'n'; }\n        else if (c == '\\t') { out[len++] = '\\\\'; out[len++] = 't'; }\n        else out[len++] = c;\n    }\n    out[len] = 0;\n    return out;\n}\nstatic const char *pny_json_stringify(PnyJson *j) {\n    if (!j) return \"null\";\n    size_t cap = 128, len = 0;\n    char *out = (char *)malloc(cap);\n    out[len++] = '{';\n    for (int i = 0; i < j->count; i++) {\n        char *k = pj_escape(j->pairs[i].key);\n        char *v = pj_escape(j->pairs[i].val);\n        size_t need = strlen(k) + strlen(v) + 8;\n        if (len + need + 2 > cap) { cap = (len + need + 2) * 2; out = (char *)realloc(out, cap); }\n        if (i) out[len++] = ',';\n        len += (size_t)snprintf(out + len, cap - len, \"\\\"%s\\\":\\\"%s\\\"\", k, v);\n        free(k);\n        free(v);\n    }\n    out[len++] = '}';\n    out[len] = 0;\n    return out;\n}\n/* ===== end inline JSON runtime ===== */\n""";

static const char *PNY_HTTP_RUNTIME =
"\n/* ===== http_post 内联运行时 (极简 HTTP/1.1 客户端, 无 https/chunked) ===== */\n"
"#include <sys/socket.h>\n#include <netinet/in.h>\n#include <arpa/inet.h>\n#include <netdb.h>\n#include <sys/un.h>\n#include <poll.h>\n#include <unistd.h>\n"
"static int pny_cli_fd = -1;\nstatic char pny_cli_key[300];\nstatic char *pny_http_post_once(const char *url, const char *body, int retry);\nstatic char *pny_http_post(const char *url, const char *body) {\n    return pny_http_post_once(url, body, 1);\n}\nstatic char *pny_http_post_once(const char *url, const char *body, int retry) {\n    if (!url || strncmp(url, \"http://\", 7) != 0) return (char *)\"\";\n    const char *hp = url + 7;\n    const char *slash = strchr(hp, '/');\n    char host[256];\n    size_t hl = slash ? (size_t)(slash - hp) : strlen(hp);\n    if (hl >= sizeof(host)) hl = sizeof(host) - 1;\n    memcpy(host, hp, hl); host[hl] = 0;\n    const char *path = slash ? slash : \"/\";\n    int port = 80;\n    char *colon = strchr(host, ':');\n    if (colon) { *colon = 0; port = atoi(colon + 1); }\n    char key[300]; snprintf(key, sizeof(key), \"%s:%d\", host, port);\n    int fd = -1;\n    if (pny_cli_fd >= 0) {\n        if (strcmp(pny_cli_key, key) == 0) fd = pny_cli_fd;\n        else { close(pny_cli_fd); pny_cli_fd = -1; }\n    }\n    if (fd < 0) {\n        fd = socket(AF_INET, SOCK_STREAM, 0);\n        if (fd < 0) return (char *)\"\";\n        struct hostent *he = gethostbyname(host);\n        if (!he) { close(fd); return (char *)\"\"; }\n        struct sockaddr_in addr;\n        memset(&addr, 0, sizeof(addr));\n        addr.sin_family = AF_INET;\n        addr.sin_port = htons((uint16_t)port);\n        memcpy(&addr.sin_addr, he->h_addr_list[0], (size_t)he->h_length);\n        if (connect(fd, (struct sockaddr *)&addr, (socklen_t)sizeof(addr)) < 0) { close(fd); return (char *)\"\"; }\n    }\n    size_t blen = body ? strlen(body) : 0;\n    char hdr[512];\n    int hln = snprintf(hdr, sizeof(hdr),\n        \"POST %s HTTP/1.1\\r\\nHost: %s\\r\\nContent-Type: application/json\\r\\nContent-Length: %zu\\r\\nConnection: keep-alive\\r\\n\\r\\n\",\n        path, host, blen);\n    if (write(fd, hdr, (size_t)hln) < 0 || (blen && write(fd, body, blen) < 0)) {\n        close(fd);\n        if (fd == pny_cli_fd) pny_cli_fd = -1;\n        if (retry) return pny_http_post_once(url, body, 0);\n        return (char *)\"\";\n    }\n    /* 帧读取: 头到 \\r\\n\\r\\n, 按 Content-Length 收满 body (keep-alive 不能读到 EOF) */\n    char *resp = (char *)malloc(67584);\n    if (!resp) { close(fd); return (char *)\"\"; }\n    size_t total = 0, body_start = 0;\n    int hdr_done = 0;\n    long cl = -1;\n    while (total < 67583) {\n        ssize_t r = read(fd, resp + total, 2048);\n        if (r <= 0) break;\n        total += (size_t)r;\n        resp[total] = 0;\n        if (!hdr_done) {\n            char *sep = strstr(resp, \"\\r\\n\\r\\n\");\n            if (sep) {\n                hdr_done = 1;\n                body_start = (size_t)(sep - resp) + 4;\n                const char *clp = strstr(resp, \"Content-Length:\");\n                cl = clp ? strtol(clp + 15, NULL, 10) : 0;\n            }\n        }\n        if (hdr_done && cl >= 0 && (long)(total - body_start) >= cl) break;\n    }\n    if (!hdr_done || cl < 0 || (long)(total - body_start) < cl) {\n        close(fd);\n        if (fd == pny_cli_fd) pny_cli_fd = -1;\n        free(resp);\n        if (retry) return pny_http_post_once(url, body, 0);\n        return (char *)\"\";\n    }\n    pny_cli_fd = fd;\n    snprintf(pny_cli_key, sizeof(pny_cli_key), \"%s\", key);\n    char *out = (char *)malloc((size_t)cl + 1);\n    if (!out) { free(resp); return (char *)\"\"; }\n    memcpy(out, resp + body_start, (size_t)cl);\n    out[cl] = 0;\n    free(resp);\n    return out;\n}\nstatic int pny_unix_fd = -1;\nstatic int pny_unix_conn = -1;\nstatic int pny_unix_keep = 0;\nstatic char *pny_http_accept_unix(const char *path) {\n    if (!path || !*path) return (char *)\"\";\n    if (pny_unix_fd < 0) {\n        pny_unix_fd = socket(AF_UNIX, SOCK_STREAM, 0);\n        if (pny_unix_fd < 0) return (char *)\"\";\n        struct sockaddr_un ua;\n        memset(&ua, 0, sizeof(ua));\n        ua.sun_family = AF_UNIX;\n        if (strlen(path) >= sizeof(ua.sun_path)) { close(pny_unix_fd); pny_unix_fd = -1; return (char *)\"\"; }\n        strncpy(ua.sun_path, path, sizeof(ua.sun_path) - 1);\n        unlink(path);\n        if (bind(pny_unix_fd, (struct sockaddr *)&ua, (socklen_t)sizeof(ua)) < 0) { close(pny_unix_fd); pny_unix_fd = -1; return (char *)\"\"; }\n        listen(pny_unix_fd, 128);\n    }\n    /* keep-alive 复用 (与 TCP 版同语义) */\n    if (pny_unix_conn >= 0) {\n        struct pollfd pf;\n        pf.fd = pny_unix_conn; pf.events = POLLIN;\n        int pr = poll(&pf, 1, 300);\n        if (pr <= 0) { close(pny_unix_conn); pny_unix_conn = -1; }\n        else { char pk; ssize_t rr = recv(pny_unix_conn, &pk, 1, MSG_PEEK); if (rr <= 0) { close(pny_unix_conn); pny_unix_conn = -1; } }\n    }\n    if (pny_unix_conn < 0) {\n        pny_unix_conn = accept(pny_unix_fd, NULL, NULL);\n        if (pny_unix_conn < 0) return (char *)\"\";\n    }\n    static char uhdr[65536];\n    size_t hlen = 0;\n    char *sep = NULL;\n    while (hlen < sizeof(uhdr) - 1) {\n        ssize_t r = read(pny_unix_conn, uhdr + hlen, 1024);\n        if (r <= 0) { close(pny_unix_conn); pny_unix_conn = -1; return (char *)\"\"; }\n        hlen += (size_t)r;\n        uhdr[hlen] = 0;\n        sep = strstr(uhdr, \"\\r\\n\\r\\n\");\n        if (sep) break;\n    }\n    if (!sep) { close(pny_unix_conn); pny_unix_conn = -1; return (char *)\"\"; }\n    pny_unix_keep = (strstr(uhdr, \"Connection: close\") == NULL) ? 1 : 0;\n    const char *clp = strstr(uhdr, \"Content-Length:\");\n    long cl = clp ? strtol(clp + 15, NULL, 10) : 0;\n    if (cl < 0 || cl > 65535) { close(pny_unix_conn); pny_unix_conn = -1; return (char *)\"\"; }\n    size_t body_start = (size_t)(sep - uhdr) + 4;\n    static char ubody[65536];\n    size_t got = 0;\n    if (hlen > body_start) {\n        got = hlen - body_start;\n        if (got > (size_t)cl) got = (size_t)cl;\n        memcpy(ubody, uhdr + body_start, got);\n    }\n    while (got < (size_t)cl) {\n        ssize_t r = read(pny_unix_conn, ubody + got, (size_t)cl - got);\n        if (r <= 0) { close(pny_unix_conn); pny_unix_conn = -1; return (char *)\"\"; }\n        got += (size_t)r;\n    }\n    ubody[cl] = 0;\n    return ubody;\n}\nstatic int pny_http_respond_unix(const char *json) {\n    if (pny_unix_conn < 0) return 0;\n    size_t len = json ? strlen(json) : 0;\n    char hdr[256];\n    int hl = snprintf(hdr, sizeof(hdr),\n        \"HTTP/1.1 200 OK\\r\\nContent-Type: application/json\\r\\nContent-Length: %zu\\r\\nConnection: %s\\r\\n\\r\\n\",\n        len, pny_unix_keep ? \"keep-alive\" : \"close\");\n    if (write(pny_unix_conn, hdr, (size_t)hl) < 0) { /* ignore */ }\n    if (len) { if (write(pny_unix_conn, json, len) < 0) { /* ignore */ } }\n    if (!pny_unix_keep) { close(pny_unix_conn); pny_unix_conn = -1; }\n    return 1;\n}\nstatic char *pny_http_post_unix(const char *path, const char *body) {\n    if (!path || !*path) return (char *)\"\";\n    int fd = socket(AF_UNIX, SOCK_STREAM, 0);\n    if (fd < 0) return (char *)\"\";\n    struct sockaddr_un ua;\n    memset(&ua, 0, sizeof(ua));\n    ua.sun_family = AF_UNIX;\n    if (strlen(path) >= sizeof(ua.sun_path)) { close(fd); return (char *)\"\"; }\n    strncpy(ua.sun_path, path, sizeof(ua.sun_path) - 1);\n    if (connect(fd, (struct sockaddr *)&ua, (socklen_t)sizeof(ua)) < 0) { close(fd); return (char *)\"\"; }\n    size_t blen = body ? strlen(body) : 0;\n    char hdr[512];\n    int hln = snprintf(hdr, sizeof(hdr),\n        \"POST / HTTP/1.1\\r\\nHost: localhost\\r\\nContent-Type: application/json\\r\\nContent-Length: %zu\\r\\nConnection: close\\r\\n\\r\\n\",\n        blen);\n    if (write(fd, hdr, (size_t)hln) < 0 || (blen && write(fd, body, blen) < 0)) { close(fd); return (char *)\"\"; }\n    char *resp = (char *)malloc(65536 + 2048);\n    if (!resp) { close(fd); return (char *)\"\"; }\n    size_t total = 0;\n    int hdr_done = 0;\n    size_t body_start = 0;\n    long cl = -1;\n    while (total < 65536 + 2048 - 1) {\n        ssize_t r = read(fd, resp + total, 2048);\n        if (r <= 0) break;\n        total += (size_t)r;\n        resp[total] = 0;\n        if (!hdr_done) {\n            char *sep = strstr(resp, \"\\r\\n\\r\\n\");\n            if (sep) {\n                hdr_done = 1;\n                body_start = (size_t)(sep - resp) + 4;\n                const char *clp = strstr(resp, \"Content-Length:\");\n                cl = clp ? strtol(clp + 15, NULL, 10) : 0;\n            }\n        }\n        if (hdr_done && cl >= 0 && (long)(total - body_start) >= cl) break;\n    }\n    close(fd);\n    if (!hdr_done || cl < 0 || (long)(total - body_start) < cl) { free(resp); return (char *)\"\"; }\n    char *out = (char *)malloc((size_t)cl + 1);\n    if (!out) { free(resp); return (char *)\"\"; }\n    memcpy(out, resp + body_start, (size_t)cl);\n    out[cl] = 0;\n    free(resp);\n    return out;\n}\nstatic char *pny_http_post_h(const char *url, const char *body, const char *extra_headers) {\n"
"    if (!url || strncmp(url, \"http://\", 7) != 0) return (char *)\"\";\n"
"    const char *hp = url + 7;\n"
"    const char *slash = strchr(hp, '/');\n"
"    char host[256];\n"
"    size_t hl = slash ? (size_t)(slash - hp) : strlen(hp);\n"
"    if (hl >= sizeof(host)) hl = sizeof(host) - 1;\n"
"    memcpy(host, hp, hl); host[hl] = 0;\n"
"    const char *path = slash ? slash : \"/\";\n"
"    int port = 80;\n"
"    char *colon = strchr(host, ':');\n"
"    if (colon) { *colon = 0; port = atoi(colon + 1); }\n"
"    int fd = socket(AF_INET, SOCK_STREAM, 0);\n"
"    if (fd < 0) return (char *)\"\";\n"
"    struct hostent *he = gethostbyname(host);\n"
"    if (!he) { close(fd); return (char *)\"\"; }\n"
"    struct sockaddr_in addr;\n"
"    memset(&addr, 0, sizeof(addr));\n"
"    addr.sin_family = AF_INET;\n"
"    addr.sin_port = htons((uint16_t)port);\n"
"    memcpy(&addr.sin_addr, he->h_addr_list[0], (size_t)he->h_length);\n"
"    if (connect(fd, (struct sockaddr *)&addr, (socklen_t)sizeof(addr)) < 0) { close(fd); return (char *)\"\"; }\n"
"    size_t blen = body ? strlen(body) : 0;\n"
"    char hdr[1024];\n"
"    int hln = snprintf(hdr, sizeof(hdr),\n"
"        \"POST %s HTTP/1.1\\r\\nHost: %s\\r\\nContent-Type: application/json\\r\\n%sContent-Length: %zu\\r\\nConnection: close\\r\\n\\r\\n\",\n"
"        path, host, extra_headers ? extra_headers : \"\", blen);\n"
"    if (write(fd, hdr, (size_t)hln) < 0) { close(fd); return (char *)\"\"; }\n"
"    if (blen && write(fd, body, blen) < 0) { close(fd); return (char *)\"\"; }\n"
"    size_t cap = 65536, got = 0;\n"
"    char *resp = (char *)malloc(cap);\n"
"    if (!resp) { close(fd); return (char *)\"\"; }\n"
"    ssize_t r;\n"
"    while (got < cap - 1 && (r = read(fd, resp + got, cap - 1 - got)) > 0) got += (size_t)r;\n"
"    resp[got] = 0;\n"
"    close(fd);\n"
"    char *b = strstr(resp, \"\\r\\n\\r\\n\");\n"
"    if (b) return b + 4;\n"
"    return resp;\n"
"}\n"
"#define pny_http_post(u, b) pny_http_post_h(u, b, NULL)\n";

void codegen_program(Codegen *cg, ASTNode *ast) {
    if (getenv("PONYPP_DEBUG_MAIN")) {
        fprintf(stderr, "[DEBUG-PROG] ast=%p type=%d child_count=%zu\n", (void*)ast, ast ? ast->type : -1, ast ? ast->child_count : 0);
        for (size_t di = 0; ast && di < ast->child_count && di < 10; di++) {
            ASTNode *dch = ast->children[di];
            fprintf(stderr, "[DEBUG-PROG] child[%zu] type=%d data=%s child_count=%zu\n", di, dch ? dch->type : -1, dch && dch->data ? (const char*)dch->data : "(null)", dch ? dch->child_count : 0);
        }
    }
    cg_emit_raw(cg, "/* Pony++ native backend generated code */\n");
    cg_emit_raw(cg, "#define _DEFAULT_SOURCE\n#define _POSIX_C_SOURCE 200809L\n");
    cg_emit_raw(cg, "#include <stdio.h>\n");
    cg_emit_raw(cg, "#include <string.h>\n");
    cg_emit_raw(cg, "#include <stdlib.h>\n");
    cg_emit_raw(cg, "#include <stdint.h>\n\n");
    /* PONYPP_GC=1: Boehm GC 模式 — 运行时全部分配走 GC_malloc (泄漏修复, App-as-test ponydb 实证) */
    if (getenv("PONYPP_GC")) {
        cg_emit_raw(cg, "#include <gc/gc.h>\n");
        cg_emit_raw(cg, "#define malloc(n) GC_malloc(n)\n");
        cg_emit_raw(cg, "#define calloc(c, n) GC_malloc((size_t)(c) * (size_t)(n))\n");
        cg_emit_raw(cg, "#define realloc(p, n) GC_realloc(p, n)\n");
        cg_emit_raw(cg, "#define free(p) GC_free(p)\n");
        cg_emit_raw(cg, "#define strdup(s) GC_strdup(s)\n");
        /* P1-4: 纯字符缓冲走 GC_malloc_atomic (免保守扫描, 降 GC churn) */
        cg_emit_raw(cg, "#define pny_xmalloc(n) GC_malloc_atomic(n)\n\n");
    } else {
        cg_emit_raw(cg, "#define pny_xmalloc(n) malloc(n)\n\n");
    }
    cg_emit_raw(cg, "%s", PNY_HTTP_RUNTIME);
    cg_emit_runtime(cg);
    cg_emit_raw(cg, "%s", PNY_STR_RUNTIME);
    cg_emit_raw(cg, "%s", PNY_ARENA_RUNTIME);
    cg_emit_raw(cg, "%s", PNY_FILE_RUNTIME);
    cg_emit_raw(cg, "%s", PNY_PEX_RUNTIME);
    cg_emit_raw(cg, "%s", PNY_PEX2_RUNTIME);
    cg_emit_raw(cg, "%s", PNY_EXEC_RUNTIME);
    if (cg_ast_uses_json(ast)) {
        cg_emit_raw(cg, "%s", PNY_JSON_RUNTIME);
        cg_emit_raw(cg, "\n");
    }

    /* 处理 import/use 声明 */
    for (size_t i = 0; ast && i < ast->child_count; i++) {
        if (ast->children[i] && ast->children[i]->type == NODE_IMPORT) {
            const char *mod = (const char *)ast->children[i]->data;
            cg_emit_raw(cg, "// import %s\n", mod ? mod : "?");
        }
    }

    /* extern fun FFI 声明 (0.2.16): 发 C 原型 + 注册调用表 */
    for (size_t i = 0; ast && i < ast->child_count; i++) {
        ASTNode *ex = ast->children[i];
        if (!ex || ex->type != NODE_EXTERN || !ex->data) continue;
        const char *ret_c = "long long";
        ASTNode *params = NULL;
        for (size_t k = 0; k < ex->child_count; k++) {
            ASTNode *ch = ex->children[k];
            if (ch && ch->data && strcmp((const char *)ch->data, "params") == 0) params = ch;
        }
        /* 子节点序: [params容器?] [返回类型?] — 返回类型是最后一个非 params 子节点 */
        for (size_t k = ex->child_count; k > 0; k--) {
            ASTNode *ch = ex->children[k - 1];
            if (ch && ch->data && strcmp((const char *)ch->data, "params") != 0) {
                ret_c = cg_builtin_type((const char *)ch->data);
                break;
            }
        }
        cg_emit_raw(cg, "extern %s %s(", ret_c, (const char *)ex->data);
        if (params) {
            for (size_t k = 0; k < params->child_count; k++) {
                ASTNode *pm = params->children[k];
                if (!pm) continue;
                const char *pt = "long long";
                for (size_t j = 0; j < pm->child_count; j++) {
                    if (pm->children[j] && pm->children[j]->data) {
                        pt = cg_builtin_type((const char *)pm->children[j]->data);
                        break;
                    }
                }
                cg_emit_raw(cg, "%s%s", k ? ", " : "", pt);
            }
        } else {
            cg_emit_raw(cg, "void");
        }
        cg_emit_raw(cg, ");\n");
        if (cg->extern_count < 64) {
            snprintf(cg->extern_funcs[cg->extern_count], 64, "%s", (const char *)ex->data);
            /* 返回类型名登记（String 感知用） */
            const char *ret_src = "";
            for (size_t k = ex->child_count; k > 0; k--) {
                ASTNode *ch = ex->children[k - 1];
                if (ch && ch->data && strcmp((const char *)ch->data, "params") != 0) {
                    ret_src = (const char *)ch->data;
                    break;
                }
            }
            snprintf(cg->extern_rets[cg->extern_count], 64, "%s", ret_src);
            cg->extern_count++;
        }
    }
    if (ast) {
        for (size_t i = 0; i < ast->child_count; i++) {
            if (ast->children[i] && ast->children[i]->type == NODE_EXTERN) {
                cg_emit_raw(cg, "\n");
                break;
            }
        }
    }

    /* 收集所有 Actor 类型名（用于字段类型推断） */
    const char **actor_type_names = NULL;
    size_t atn_count = 0;
    for (size_t i = 0; ast && i < ast->child_count; i++) {
        if (ast->children[i] && (ast->children[i]->type == NODE_ACTOR || ast->children[i]->type == NODE_CLASS || ast->children[i]->type == NODE_TRAIT || ast->children[i]->type == NODE_INTERFACE)) {
            const char *nm = (const char *)ast->children[i]->data;
            if (nm) {
                atn_count++;
                actor_type_names = (const char **)realloc(actor_type_names, atn_count * sizeof(char *));
                actor_type_names[atn_count - 1] = nm;
                if (cg->known_actor_count < 32) {
                    snprintf(cg->known_actors[cg->known_actor_count], 64, "%s", nm);
                    cg->known_actor_count++;
                }
                /* String 返回方法注册: fun xxx(...): String */
                for (size_t k = 0; k < ast->children[i]->child_count; k++) {
                    ASTNode *mch = ast->children[i]->children[k];
                    if (mch && (mch->type == NODE_FUN || mch->type == NODE_BE) && mch->data &&
                        cg->str_ret_count < 256) {
                        for (size_t ci = 0; ci < mch->child_count; ci++) {
                            ASTNode *cch = mch->children[ci];
                            if (cch && cch->data && cch->type != NODE_EMPTY &&
                                strcmp((const char *)cch->data, "String") == 0) {
                                snprintf(cg->str_ret_methods[cg->str_ret_count], 64, "%s",
                                         (const char *)mch->data);
                                cg->str_ret_count++;
                                break;
                            }
                        }
                    }
                }
                /* 类型字段注册表 */
                if (cg->type_count < 32) {
                    size_t ti = cg->type_count++;
                    snprintf(cg->type_names[ti], 64, "%s", nm);
                    cg->type_field_counts[ti] = 0;
                    for (size_t k = 0; k < ast->children[i]->child_count; k++) {
                        ASTNode *fch = ast->children[i]->children[k];
                        if (fch && fch->type == NODE_VAR && fch->data && cg->type_field_counts[ti] < 32) {
                            size_t fi = cg->type_field_counts[ti];
                            snprintf(cg->type_fields[ti][fi], 64, "%s", (const char *)fch->data);
                            const char *ftt = (fch->child_count > 0 && fch->children[0] && fch->children[0]->data)
                                ? (const char *)fch->children[0]->data : "int";
                            snprintf(cg->type_field_types[ti][fi], 64, "%s", ftt);
                            cg->type_field_counts[ti]++;
                        }
                    }
                }
            }
        }
    }

    for (size_t i = 0; ast && i < ast->child_count; i++) {
        if (ast->children[i] && (ast->children[i]->type == NODE_ACTOR || ast->children[i]->type == NODE_CLASS || ast->children[i]->type == NODE_TRAIT || ast->children[i]->type == NODE_INTERFACE)) {
            cg->local_var_count = 0; /* 新类型作用域: 重置局部变量表 */
            cg_actor(cg, ast->children[i], actor_type_names, atn_count);
        }
    }
    free(actor_type_names);

    /* 生成监督树注册代码 */
    for (size_t i = 0; ast && i < ast->child_count; i++) {
        if (ast->children[i] && ast->children[i]->type == NODE_SUPERVISE) {
            ASTNode *sup = ast->children[i];
            const char *child_name = sup->data ? (const char*)sup->data : "Worker";
            int strategy = SUPERVISE_ONE_FOR_ONE;
            int max_restarts = 3;
            if (sup->child_count > 0 && sup->children[0] && sup->children[0]->data) {
                const char *s = (const char*)sup->children[0]->data;
                if (strcmp(s, "one_for_all") == 0) strategy = SUPERVISE_ONE_FOR_ALL;
                else if (strcmp(s, "restart") == 0) strategy = SUPERVISE_RESTART;
                else if (strcmp(s, "none") == 0) strategy = SUPERVISE_NONE;
                else strategy = SUPERVISE_ONE_FOR_ONE;
            }
            cg_emit(cg, "/* supervise %s %s */\n", child_name, sup->child_count > 0 ? (const char*)sup->children[0]->data : "one_for_one");
            cg_emit(cg, "pny_supervise_register(&__supervisor_self, &%s_self, %d, %d);\n", child_name, strategy, max_restarts);
        }
    }

    /* 生成 main() */
    cg_emit_main(cg, ast);
}

static void cg_emit_create_call(Codegen *cg, const char *name, ASTNode *actor) {
    /* 查找构造函数，检查参数 */
    int has_params = 0;
    ASTNode *ctor_params = NULL;
    for (size_t j = 0; j < actor->child_count; j++) {
        ASTNode *m = actor->children[j];
        if (m && m->type == NODE_NEW && m->child_count > 0) {
            /* children[0] 仅当带参时是 params; 零参时 children[0]=body */
            if (m->child_count >= 2) {
                ctor_params = m->children[0];
                if (ctor_params->child_count > 0) has_params = 1;
            }
            break;
        }
    }
    if (has_params && ctor_params && ctor_params->child_count > 0) {
        cg_emit(cg, "%s_t *__main_obj = %s_create(", name, name);
        for (size_t k = 0; k < ctor_params->child_count; k++) {
            if (k > 0) cg_emit_raw(cg, ", ");
            ASTNode *param = ctor_params->children[k];
            if (param->child_count >= 2) {
                cg_expr(cg, param->children[1]);
            } else {
                cg_emit_raw(cg, "0");
            }
        }
        cg_emit_raw(cg, ");\n");
    } else {
        cg_emit(cg, "%s_t *__main_obj = %s_create();\n", name, name);
    }
}

static void cg_emit_main(Codegen *cg, ASTNode *ast) {
    cg_emit_raw(cg, "int main(int argc, char *argv[]) {\n");
    cg_push(cg);
    /* 入口 actor 选择: 优先名为 "main" 且带 create/run, 再任意带 create/run 的 actor;
       全部不匹配才输出 Hello (修复: 旧逻辑第一个无构造 actor 直接截胡输出 Hello) */
    ASTNode *entry_actor = NULL;
    const char *entry_name = NULL;
    int entry_has_run = 0;
    if (getenv("PONYPP_DEBUG_MAIN")) {
        fprintf(stderr, "[DEBUG-MAIN] ast=%p child_count=%zu\n", (void*)ast, ast ? ast->child_count : 0);
        for (size_t di = 0; ast && di < ast->child_count; di++) {
            ASTNode *dch = ast->children[di];
            fprintf(stderr, "[DEBUG-MAIN] child[%zu] type=%d data=%s\n", di, dch ? dch->type : -1, dch && dch->data ? (const char*)dch->data : "(null)");
        }
    }
    for (int pass = 0; pass < 2 && !entry_actor; pass++) {
        for (size_t i = 0; ast && i < ast->child_count; i++) {
            ASTNode *ch = ast->children[i];
            if (!ch || ch->type != NODE_ACTOR) continue;
            const char *nm = (const char *)ch->data;
            if (!nm) continue;
            if (pass == 0 && strcmp(nm, "main") != 0) continue;
            int has_ctor = 0, has_run = 0;
            for (size_t j = 0; j < ch->child_count; j++) {
                ASTNode *m = ch->children[j];
                if (!m) continue;
                if (m->type == NODE_NEW) has_ctor = 1;
                if ((m->type == NODE_BE || m->type == NODE_FUN) && m->data &&
                    strcmp((const char *)m->data, "run") == 0) has_run = 1;
            }
            if (has_ctor || has_run) {
                entry_actor = ch; entry_name = nm; entry_has_run = has_run;
                break;
            }
        }
    }
    if (entry_actor) {
        cg_emit_create_call(cg, entry_name, entry_actor);
        if (entry_has_run) cg_emit(cg, "%s_%s(__main_obj);\n", entry_name, "run");
        cg_emit(cg, "PnyRuntime *r = pny_runtime_new();\n");
        cg_emit(cg, "PnyActor *__actor = pny_actor_new(\"%s\", sizeof(%s_t));\n", entry_name, entry_name);
        cg_emit(cg, "if (__actor) { memcpy(__actor->state, __main_obj, sizeof(%s_t)); pny_actor_register(r, __actor); }\n", entry_name);
        cg_emit(cg, "(void)r; (void)__actor;\n");
    } else {
        cg_emit(cg, "printf(\"Hello from Pony++ native (real backend)\\n\");\n");
    }
    skip_main_body:
    cg_pop(cg);
    cg_emit(cg, "    return 0;\n");
    cg_emit(cg, "}\n");
}

/* ==================== Source Map ==================== */

SourceMap *sourcemap_new(void) {
    SourceMap *sm = (SourceMap *)calloc(1, sizeof(SourceMap));
    if (!sm) return NULL;
    sm->cap = 256;
    sm->entries = (SourceMapEntry *)calloc(sm->cap, sizeof(SourceMapEntry));
    if (!sm->entries) { free(sm); return NULL; }
    return sm;
}

void sourcemap_free(SourceMap *sm) {
    if (!sm) return;
    free(sm->entries);
    free(sm);
}

int sourcemap_add(SourceMap *sm, int gen_line, const char *file, int src_line, int src_col) {
    if (!sm || !file) return -1;
    if (sm->count >= sm->cap) {
        size_t nc = sm->cap * 2;
        SourceMapEntry *ne = (SourceMapEntry *)realloc(sm->entries, nc * sizeof(SourceMapEntry));
        if (!ne) return -2;
        memset(ne + sm->cap, 0, (nc - sm->cap) * sizeof(SourceMapEntry));
        sm->entries = ne;
        sm->cap = nc;
    }
    SourceMapEntry *e = &sm->entries[sm->count++];
    e->generated_line = gen_line;
    e->source_line = src_line;
    e->source_col = src_col;
    strncpy(e->source_file, file, sizeof(e->source_file) - 1);
    return 0;
}

int sourcemap_lookup(const SourceMap *sm, int gen_line, SourceMapEntry *out) {
    if (!sm || !out) return -1;
    /* 二分查找最近的<=gen_line的条目 */
    size_t lo = 0, hi = sm->count;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (sm->entries[mid].generated_line <= gen_line)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == 0) return -2;
    *out = sm->entries[lo - 1];
    return 0;
}

int sourcemap_save_json(const SourceMap *sm, const char *path) {
    if (!sm || !path) return -1;
    FILE *f = fopen(path, "w");
    if (!f) return -2;
    /* 简单格式: gen_line\x01file\x01src_line */
    for (size_t i = 0; i < sm->count; i++) {
        const SourceMapEntry *e = &sm->entries[i];
        fprintf(f, "%d%c%s%c%d\n", e->generated_line, 1, e->source_file, 1, e->source_line);
    }
    fclose(f);
    return 0;
}

SourceMap *sourcemap_load_json(const char *path) {
    if (!path) return NULL;
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    SourceMap *sm = sourcemap_new();
    if (!sm) { fclose(f); return NULL; }

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        int gen_line, src_line;
        char file[256];
        /* 解析格式: gen_line|file|src_line */
        char *p1 = strchr(line, 0x01);  /* STX分隔符 */
        if (!p1) continue;
        char *p2 = strchr(p1 + 1, 0x01);
        if (!p2) continue;
        *p1 = 0; *p2 = 0;
        gen_line = atoi(line);
        strncpy(file, p1 + 1, sizeof(file) - 1);
        src_line = atoi(p2 + 1);
        sourcemap_add(sm, gen_line, file, src_line, 0);
    }
    fclose(f);
    return sm;
}

size_t sourcemap_count(const SourceMap *sm) {
    return sm ? sm->count : 0;
}

/* Codegen with source map */
typedef struct {
    FILE *out;
    SourceMap *sm;
    char source_file[256];
    int current_line;
} CodegenInternal;

/* 扩展Codegen结构(如果已有sm字段则复用) */
/* 简化实现: 通过全局变量关联 */
static SourceMap *g_current_sm = NULL;
static char g_current_file[256] = {0};
static int g_current_gen_line = 0;

Codegen *codegen_new_with_map(FILE *out, SourceMap *sm) {
    g_current_sm = sm;
    g_current_gen_line = 0;
    return codegen_new(out);
}

void codegen_set_source_file(Codegen *cg, const char *filename) {
    (void)cg;
    if (filename) strncpy(g_current_file, filename, sizeof(g_current_file) - 1);
}

void codegen_emit_line_directive(Codegen *cg, int src_line) {
    (void)cg;
    if (g_current_sm && g_current_file[0]) {
        g_current_gen_line++;
        sourcemap_add(g_current_sm, g_current_gen_line, g_current_file, src_line, 0);
    }
}
