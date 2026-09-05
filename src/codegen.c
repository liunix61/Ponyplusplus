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
    cg_set_fields(cg, fc, fields);
    if (cg->field_types) {
        for (size_t i = 0; i < cg->field_count; i++) free(cg->field_types[i]);
        free(cg->field_types);
    }
    cg->field_types = types;
}

static const char *cg_field_type(Codegen *cg, const char *name) {
    for (size_t i = 0; i < cg->field_count; i++) {
        if (strcmp(cg->fields[i], name) == 0 && cg->field_types) return cg->field_types[i];
    }
    return NULL;
}

static void cg_set_ctor(Codegen *cg, int in_ctor) { cg->in_constructor = in_ctor; }

static void cg_emit_field_access(Codegen *cg, const char *name) {
    if (name && cg_has_field(cg, name)) {
        cg_emit_raw(cg, "self%s%s", cg->in_constructor ? "." : "->", name);
    } else {
        cg_emit_raw(cg, "%s", name ? name : "0");
    }
}

static const char *cg_type_of(ASTNode *t, const char **actor_types, size_t atc) {
    if (!t || !t->data) return "int";
    const char *n = (const char *)t->data;
    /* Actor 类型 → _t 后缀 */
    for (size_t i = 0; i < atc; i++) {
        if (actor_types && actor_types[i] && strcmp(actor_types[i], n) == 0) {
            return actor_types[i]; /* caller handles _t suffix */
        }
    }
    if (strcmp(n, "U8") == 0) return "unsigned char";
    if (strcmp(n, "U16") == 0) return "unsigned short";
    if (strcmp(n, "U32") == 0) return "unsigned int";
    if (strcmp(n, "U64") == 0) return "unsigned long long";
    if (strcmp(n, "I8") == 0) return "signed char";
    if (strcmp(n, "I16") == 0) return "signed short";
    if (strcmp(n, "I32") == 0) return "signed int";
    if (strcmp(n, "I64") == 0) return "signed long long";
    if (strcmp(n, "F32") == 0) return "float";
    if (strcmp(n, "F64") == 0) return "double";
    if (strcmp(n, "String") == 0) return "const char *";
    if (strcmp(n, "Bool") == 0) return "int";
    if (strcmp(n, "None") == 0 || strcmp(n, "NoneType") == 0) return "void";
    return n;
}

static char *cg_cstr_escape(const char *s, char *buf, size_t sz) {
    size_t j = 0;
    for (size_t i = 0; s && s[i] && j < sz - 3; i++) {
        char c = s[i];
        if (c == '\\') { if (j + 2 < sz) { buf[j++] = '\\'; buf[j++] = '\\'; } }
        else if (c == '"') { if (j + 2 < sz) { buf[j++] = '\\'; buf[j++] = '"'; } }
        else if (c == '\n') { if (j + 2 < sz) { buf[j++] = '\\'; buf[j++] = 'n'; } }
        else if (c == '\t') { if (j + 2 < sz) { buf[j++] = '\\'; buf[j++] = 't'; } }
        else buf[j++] = c;
    }
    buf[j] = '\0';
    return buf;
}

static void cg_expr(Codegen *cg, ASTNode *n);
static void cg_stmt(Codegen *cg, ASTNode *n);
static void cg_actor(Codegen *cg, ASTNode *actor,
                        const char **actor_types, size_t atc);

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
        case NODE_BOOL:
            cg_emit_raw(cg, "%s", n->data && strcmp(n->data, "true") == 0 ? "1" : "0");
            break;
        case NODE_CALL: {
            const char *func = (const char *)n->data;
            ASTNode *args = (n->child_count > 0 && n->children[0] &&
                             n->children[0]->data &&
                             strcmp((const char *)n->children[0]->data, "args") == 0)
                             ? n->children[0] : NULL;
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
                        const char *_ftype = cg_field_type(cg, _id);
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
                        /* print(s.len()) — 嵌套方法调用 */
                        cg_emit_raw(cg, "printf(\"%%d\\n\", ");
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
                const char *dot = strchr(func, '.');
                size_t recv_len = (size_t)(dot - func);
                char receiver[128];
                memcpy(receiver, func, recv_len);
                receiver[recv_len] = 0;
                strcpy(method_name, dot + 1);

                /* String.len() */
                if (strcmp(method_name, "len") == 0) {
                    cg_emit_raw(cg, "(int)strlen(");
                    cg_emit_field_access(cg, receiver);
                    cg_emit_raw(cg, ")");
                    break;
                }
                /* String.charAt(i) */
                if (strcmp(method_name, "charAt") == 0) {
                    cg_emit_raw(cg, "(int)((");
                    cg_emit_field_access(cg, receiver);
                    cg_emit_raw(cg, ")[");
                    if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                    else cg_emit_raw(cg, "0");
                    cg_emit_raw(cg, "])");
                    break;
                }
                /* String.to_string() — 直接用原字符串 */
                if (strcmp(method_name, "to_string") == 0) {
                    cg_emit_field_access(cg, receiver);
                    break;
                }
                /* String.startsWith(s) */
                if (strcmp(method_name, "startsWith") == 0) {
                    cg_emit_raw(cg, "((");
                    cg_emit_field_access(cg, receiver);
                    cg_emit_raw(cg, ") && (");
                    if (args && args->child_count > 0) cg_expr(cg, args->children[0]);
                    else cg_emit_raw(cg, "\"\"");
                    cg_emit_raw(cg, " && 1)"); /* 简化: 检查非空 */
                    break;
                }
                /* String.toUpperCase() */
                if (strcmp(method_name, "toUpperCase") == 0) {
                    cg_emit_raw(cg, "((");
                    cg_emit_field_access(cg, receiver);
                    cg_emit_raw(cg, ") ? 1 : 1)"); /* placeholder — toUpperCase stub */
                    break;
                }
                /* Generic method: receiver->method(...) — stub for now */
                cg_emit_raw(cg, "0"); /* stub: unknown method returns 0 */
                break;
            }
            cg_emit_raw(cg, "%s(", func ? func : "?");
            if (args) {
                for (size_t i = 0; i < args->child_count; i++) {
                    if (i) cg_emit_raw(cg, ", ");
                    cg_expr(cg, args->children[i]);
                }
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
        case NODE_IDENT:
            cg_emit_field_access(cg, (const char *)n->data);
            break;
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
                cg_emit_raw(cg, "int _match_expr = ");
                cg_expr(cg, n->children[0]);
                cg_emit_raw(cg, ";\n");
                for (size_t i = 1; i < n->child_count; i++) {
                    ASTNode *arm = n->children[i];
                    if (!arm || arm->child_count < 2) continue;
                    cg_emit_raw(cg, "if (");
                    cg_expr(cg, arm->children[0]);
                    cg_emit_raw(cg, " == _match_expr) {\n");
                    cg_expr(cg, arm->children[1]);
                    cg_emit_raw(cg, "; } else ");
                }
                cg_emit_raw(cg, "{ }\n");
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
        /* 赋值: assign(ident, rhs) */
        if (n->data && strcmp((const char *)n->data, "assign") == 0 && n->child_count >= 2) {
            /* 左值 */
            if (n->children[0]->type == NODE_IDENT && n->children[0]->data) {
                const char *lhs = (const char *)n->children[0]->data;
                cg_emit_raw(cg, "/* stmt */self%s%s = ", cg->in_constructor ? "." : "->", lhs);
                cg_expr(cg, n->children[1]);
                cg_emit_raw(cg, ";");
                return;
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
        case NODE_RETURN:
            cg_emit(cg, "return");
            if (n->child_count > 0) { cg_emit_raw(cg, " "); cg_expr(cg, n->children[0]); }
            cg_emit_raw(cg, ";\n");
            break;
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
                    snprintf(buf, sizeof(buf), "%s_t", ft);
                    ft = buf;
                }
            } else ft = "int";
            cg_emit(cg, "%s %s;", ft, fn ? fn : "f");
        }
    }
    cg_pop(cg);
    cg_emit(cg, "} %s_t;\n\n", name);

    for (size_t i = 0; i < actor->child_count; i++) {
        ASTNode *ch = actor->children[i];
        if (!ch) continue;
        if (ch->type == NODE_NEW) {
            const char *ctor = (const char *)ch->data;
            cg_emit(cg, "static %s_t %s_%s(", name, name, ctor ? ctor : "new");
            if (ch->child_count > 0 && ch->children[0] && ch->children[0]->data &&
                strcmp((const char *)ch->children[0]->data, "params") == 0) {
                for (size_t j = 0; j < ch->children[0]->child_count; j++) {
                    ASTNode *p = ch->children[0]->children[j];
                    if (j) cg_emit_raw(cg, ", ");
                    const char *pt = (p->child_count > 0 && p->children[0]) ? cg_type_of(p->children[0], actor_types, atc) : "int";
                    const char *pn = (const char *)p->data;
                    cg_emit_raw(cg, "%s %s", pt, pn ? pn : "a");
                }
            }
            cg_emit_raw(cg, ") {\n");
            cg_push(cg);
            cg_emit(cg, "%s_t self;\n", name);
            cg_emit(cg, "memset(&self, 0, sizeof(self));\n");
            cg_set_ctor(cg, 1);
            /* 找到构造体 (跳过 params 节点) */
            ASTNode *body = NULL;
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
            const char *fn = (const char *)ch->data;
            const char *rtype = "void";
            ASTNode *params = NULL;
            ASTNode *body = NULL;
            for (size_t j = 0; j < ch->child_count; j++) {
                ASTNode *c2 = ch->children[j];
                if (c2->type == NODE_EMPTY && params == NULL &&
                    c2->data && strcmp((const char *)c2->data, "params") == 0) {
                    params = c2;
                } else if (c2->type == NODE_STRING) {
                    rtype = "const char *";
                } else if (body == NULL) {
                    body = c2;
                }
            }
            cg_emit(cg, "static %s %s_%s(%s_t *self", rtype, name, fn ? fn : "m", name);
            if (params && params->data && strcmp((const char *)params->data, "params") == 0 && params->child_count > 0) {
                for (size_t j = 0; j < params->child_count; j++) {
                    ASTNode *p = params->children[j];
                    cg_emit_raw(cg, ", ");
                    const char *pt = (p->child_count > 0 && p->children[0]) ? cg_type_of(p->children[0], actor_types, atc) : "int";
                    const char *pn = (const char *)p->data;
                    cg_emit_raw(cg, "%s %s", pt, pn ? pn : "a");
                }
            }
            cg_emit_raw(cg, ") {\n");
            cg_push(cg);
            cg_set_ctor(cg, 0);
            if (body) {
                for (size_t j = 0; j < body->child_count; j++) cg_stmt(cg, body->children[j]);
            }
            if (strcmp(rtype, "void") != 0) cg_emit(cg, "return NULL;\n");
            cg_pop(cg);
            cg_emit(cg, "}\n\n");
        }
    }
}

static void cg_emit_runtime(Codegen *cg) {
    cg_emit_raw(cg,
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

void codegen_program(Codegen *cg, ASTNode *ast) {
    cg_emit_raw(cg, "/* Pony++ native backend generated code */\n");
    cg_emit_raw(cg, "#include <stdio.h>\n");
    cg_emit_raw(cg, "#include <string.h>\n");
    cg_emit_raw(cg, "#include <stdlib.h>\n");
    cg_emit_raw(cg, "#include <stdint.h>\n\n");
    cg_emit_runtime(cg);

    /* 处理 import/use 声明 */
    for (size_t i = 0; ast && i < ast->child_count; i++) {
        if (ast->children[i] && ast->children[i]->type == NODE_IMPORT) {
            const char *mod = (const char *)ast->children[i]->data;
            cg_emit_raw(cg, "// import %s\n", mod ? mod : "?");
        }
    }

    /* 收集所有 Actor 类型名（用于字段类型推断） */
    const char **actor_type_names = NULL;
    size_t atn_count = 0;
    for (size_t i = 0; ast && i < ast->child_count; i++) {
        if (ast->children[i] && ast->children[i]->type == NODE_ACTOR) {
            const char *nm = (const char *)ast->children[i]->data;
            if (nm) {
                atn_count++;
                actor_type_names = (const char **)realloc(actor_type_names, atn_count * sizeof(char *));
                actor_type_names[atn_count - 1] = nm;
            }
        }
    }

    for (size_t i = 0; ast && i < ast->child_count; i++) {
        if (ast->children[i] && ast->children[i]->type == NODE_ACTOR) {
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

    cg_emit_raw(cg, "int main(int argc, char *argv[]) {\n");
    cg_push(cg);
    for (size_t i = 0; ast && i < ast->child_count; i++) {
        ASTNode *ch = ast->children[i];
        if (!ch || ch->type != NODE_ACTOR) continue;
        const char *name = (const char *)ch->data;
        if (!name) continue;

        for (size_t j = 0; j < ch->child_count; j++) {
            ASTNode *m = ch->children[j];
            if (m && (m->type == NODE_BE || m->type == NODE_FUN) &&
                m->data && strcmp((const char *)m->data, "run") == 0) {
                cg_emit(cg, "%s_t __main_obj = %s_create();\n", name, name);
                cg_emit(cg, "%s_%s(&__main_obj);\n", name, "run");
                cg_emit(cg, "PnyRuntime *r = pny_runtime_new();\n");
                cg_emit(cg, "PnyActor *__actor = pny_actor_new(\"%s\", sizeof(%s_t));\n", name, name);
                cg_emit(cg, "if (__actor) { memcpy(__actor->state, &__main_obj, sizeof(%s_t)); pny_actor_register(r, __actor); }\n", name);
                cg_emit(cg, "(void)r; (void)__actor;\n");
                goto skip_main_body;
            }
        }
        for (size_t j = 0; j < ch->child_count; j++) {
            ASTNode *m = ch->children[j];
            if (m && m->type == NODE_NEW) {
                cg_emit(cg, "%s_t __main_obj = %s_create();\n", name, name);
                cg_emit(cg, "PnyRuntime *r = pny_runtime_new();\n");
                cg_emit(cg, "PnyActor *__actor = pny_actor_new(\"%s\", sizeof(%s_t));\n", name, name);
                cg_emit(cg, "if (__actor) { memcpy(__actor->state, &__main_obj, sizeof(%s_t)); pny_actor_register(r, __actor); }\n", name);
                cg_emit(cg, "(void)r; (void)__actor;\n");
                goto skip_main_body;
            }
        }
        cg_emit(cg, "printf(\"Hello from Pony++ native (real backend)\\n\");\n");
        goto skip_main_body;
    }
    skip_main_body:
    cg_pop(cg);
    cg_emit(cg, "    return 0;\n");
    cg_emit(cg, "}\n");
}