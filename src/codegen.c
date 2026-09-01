/*
 * codegen.c - Pony++ Native backend C 代码生成器
 */

#include "ponypp/codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "ponypp/util.h"

struct Codegen {
    FILE *out;
    int indent;
    size_t field_count;
    char **fields;
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
    }
    free(cg->fields);
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

static void cg_set_ctor(Codegen *cg, int in_ctor) { cg->in_constructor = in_ctor; }

static void cg_emit_field_access(Codegen *cg, const char *name) {
    if (name && cg_has_field(cg, name)) {
        cg_emit_raw(cg, "self%s%s", cg->in_constructor ? "." : "->", name);
    } else {
        cg_emit_raw(cg, "%s", name ? name : "0");
    }
}

static const char *cg_type_of(ASTNode *t) {
    if (!t || !t->data) return "int";
    const char *n = (const char *)t->data;
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
static void cg_actor(Codegen *cg, ASTNode *actor);

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
                        cg_emit_raw(cg, "printf(\"%u\\n\", ", _id);
                        cg_emit_field_access(cg, _id);
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
        case NODE_IDENT:
            cg_emit_field_access(cg, (const char *)n->data);
            break;
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
    if (n->type == NODE_EMPTY) return;
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

static void cg_actor(Codegen *cg, ASTNode *actor) {
    const char *name = (const char *)actor->data;
    if (!name) return;

    char **field_names = NULL;
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
            field_names[fc++] = s_strdup((const char *)ch->data);
        }
    }
    cg_set_fields(cg, fc, field_names);

    /* Actor 结构体 */
    cg_emit(cg, "typedef struct {\n");
    cg_push(cg);
    for (size_t i = 0; i < actor->child_count; i++) {
        ASTNode *ch = actor->children[i];
        if (ch && ch->type == NODE_VAR) {
            const char *fn = (const char *)ch->data;
            const char *ft = (ch->child_count > 0 && ch->children[0]) ? cg_type_of(ch->children[0]) : "int";
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
                    const char *pt = (p->child_count > 0 && p->children[0]) ? cg_type_of(p->children[0]) : "int";
                    const char *pn = (const char *)p->data;
                    cg_emit_raw(cg, "%s %s", pt, pn ? pn : "a");
                }
            }
            cg_emit_raw(cg, ") {\n");
            cg_push(cg);
            cg_emit(cg, "%s_t self;\n", name);
            cg_emit(cg, "memset(&self, 0, sizeof(self));\n");
            cg_set_ctor(cg, 1);
            ASTNode *body = (ch->child_count > 1) ? ch->children[1] : NULL;
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
                if (c2->type == NODE_EMPTY && params == NULL) params = c2;
                else if (c2->type == NODE_STRING) rtype = "const char *";
                else body = c2;
            }
            cg_emit(cg, "static %s %s_%s(%s_t *self", rtype, name, fn ? fn : "m", name);
            if (params && params->data && strcmp((const char *)params->data, "params") == 0 && params->child_count > 0) {
                for (size_t j = 0; j < params->child_count; j++) {
                    ASTNode *p = params->children[j];
                    if (j) cg_emit_raw(cg, ", ");
                    const char *pt = (p->child_count > 0 && p->children[0]) ? cg_type_of(p->children[0]) : "int";
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
        "    msg->method = m ? strdup(m) : NULL;\n"
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

    for (size_t i = 0; ast && i < ast->child_count; i++) {
        if (ast->children[i] && ast->children[i]->type == NODE_ACTOR) cg_actor(cg, ast->children[i]);
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