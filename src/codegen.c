#include "ponypp/codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

struct Codegen {
    FILE *out;
    int indent;
};

Codegen *codegen_new(FILE *out) {
    Codegen *cg = (Codegen *)calloc(1, sizeof(Codegen));
    if (!cg) return NULL;
    cg->out = out;
    cg->indent = 0;
    return cg;
}

void codegen_free(Codegen *cg) {
    if (cg) free(cg);
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

static char *cg_cstr_escape(const char *s, char *buf, size_t buf_size) {
    size_t j = 0;
    for (size_t i = 0; s && s[i] && j < buf_size - 3; i++) {
        char c = s[i];
        if (c == '\\') { if (j + 2 < buf_size) { buf[j++] = '\\'; buf[j++] = '\\'; } }
        else if (c == '"') { if (j + 2 < buf_size) { buf[j++] = '\\'; buf[j++] = '"'; } }
        else if (c == '\n') { if (j + 2 < buf_size) { buf[j++] = '\\'; buf[j++] = 'n'; } }
        else if (c == '\t') { if (j + 2 < buf_size) { buf[j++] = '\\'; buf[j++] = 't'; } }
        else buf[j++] = c;
    }
    buf[j] = '\0';
    return buf;
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
        case NODE_BOOL:
            cg_emit_raw(cg, "%s", strcmp(n->data, "true") == 0 ? "1" : "0");
            break;
        case NODE_CALL: {
            const char *func = (const char *)n->data;
            ASTNode *args = (n->child_count > 1) ? n->children[1] : NULL;
            if (func && strcmp(func, "print") == 0 && args && args->child_count > 0 && args->children[0]) {
                ASTNode *a0 = args->children[0];
                if (a0->type == NODE_STRING) {
                    const char *s = (const char *)a0->data;
                    char buf[512];
                    cg_emit_raw(cg, "printf(\"%s\\n\")", cg_cstr_escape(s ? s : "", buf, sizeof(buf)));
                    break;
                } else if (a0->type == NODE_INT) {
                    const char *d = (const char *)a0->data;
                    cg_emit_raw(cg, "printf(\"%u\\n\", %s)", d ? d : "0", d ? d : "0");
                    break;
                } else {
                    cg_emit_raw(cg, "printf(\"%u\\n\", ");
                    cg_expr(cg, a0);
                    cg_emit_raw(cg, ")");
                    break;
                }
            }
            cg_emit_raw(cg, "%s(", func ? func : "");
            if (args && args->child_count > 0) {
                for (size_t i = 0; i < args->child_count; i++) {
                    if (i) cg_emit_raw(cg, ", ");
                    cg_expr(cg, args->children[i]);
                }
            }
            cg_emit_raw(cg, ")");
            break;
        }
        case NODE_EMPTY:
            if (n->data) cg_emit_raw(cg, "%s", (const char *)n->data);
            else if (n->child_count > 0 && n->children[0]) cg_expr(cg, n->children[0]);
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
        case NODE_VAR:
        case NODE_LET: {
            const char *name = (const char *)n->data;
            const char *type = (n->child_count > 0 && n->children[0]) ? cg_type_of(n->children[0]) : "int";
            cg_emit(cg, "%s %s;", type, name ? name : "v");
            if (n->child_count > 1 && n->children[1]) {
                cg_emit_raw(cg, " = ");
                cg_expr(cg, n->children[1]);
                cg_emit_raw(cg, ";\n");
            }
            break;
        }
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
        case NODE_RETURN: {
            cg_emit(cg, "return");
            if (n->child_count > 0) { cg_emit_raw(cg, " "); cg_expr(cg, n->children[0]); }
            cg_emit_raw(cg, ";\n");
            break;
        }
        default: {
            cg_emit(cg, "/* stmt */");
            cg_expr(cg, n);
            cg_emit_raw(cg, ";\n");
            break;
        }
    }
}

static void cg_actor(Codegen *cg, ASTNode *actor) {
    const char *name = (const char *)actor->data;
    if (!name) return;

    /* Actor 结构体 */
    cg_emit(cg, "typedef struct {\n");
    cg_push(cg);
    size_t i;
    for (i = 0; i < actor->child_count; i++) {
        ASTNode *ch = actor->children[i];
        if (ch && ch->type == NODE_VAR) {
            const char *fn = (const char *)ch->data;
            const char *ft = (ch->child_count > 0 && ch->children[0]) ? cg_type_of(ch->children[0]) : "int";
            cg_emit(cg, "%s %s;", ft, fn ? fn : "f");
        }
    }
    cg_pop(cg);
    cg_emit(cg, "} %s_t;\n\n", name);

    for (i = 0; i < actor->child_count; i++) {
        ASTNode *ch = actor->children[i];
        if (!ch) continue;
        if (ch->type == NODE_NEW) {
            const char *ctor = (const char *)ch->data;
            cg_emit(cg, "static %s_t %s_%s(", name, name, ctor ? ctor : "new");
            if (ch->child_count > 0 && ch->children[0]) {
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
            if (ch->child_count > 0 && ch->child_count > 1 && ch->children[1]) {
                for (size_t j = 0; j < ch->children[1]->child_count; j++) cg_stmt(cg, ch->children[1]->children[j]);
            }
            cg_emit(cg, "return self;\n");
            cg_pop(cg);
            cg_emit(cg, "}\n\n");
        } else if (ch->type == NODE_BE || ch->type == NODE_FUN) {
            const char *fn = (const char *)ch->data;
            const char *rtype = "void";
            ASTNode *params = NULL;
            ASTNode *body = NULL;
            for (size_t j = 0; j < ch->child_count; j++) {
                ASTNode *c2 = ch->children[j];
                if (c2->type == NODE_EMPTY && params == NULL) params = c2;
                else if (c2->type == NODE_STRING || (c2->data && strcmp((const char *)c2->data, "String") == 0)) { rtype = "const char *"; }
                else body = c2;
            }
            cg_emit(cg, "static %s %s_%s(%s_t *self", rtype, name, fn ? fn : "m", name);
            if (params && params->child_count > 0) {
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
            if (body) {
                for (size_t j = 0; j < body->child_count; j++) cg_stmt(cg, body->children[j]);
            }
            if (strcmp(rtype, "void") != 0) cg_emit(cg, "return NULL;\n");
            cg_pop(cg);
            cg_emit(cg, "}\n\n");
        }
    }
}

void codegen_program(Codegen *cg, ASTNode *ast) {
    cg_emit_raw(cg, "/* Pony++ native backend generated code */\n");
    cg_emit_raw(cg, "#include <stdio.h>\n");
    cg_emit_raw(cg, "#include <string.h>\n");
    cg_emit_raw(cg, "#include <stdlib.h>\n\n");

    for (size_t i = 0; ast && i < ast->child_count; i++) {
        if (ast->children[i] && ast->children[i]->type == NODE_ACTOR) cg_actor(cg, ast->children[i]);
    }

    cg_emit_raw(cg, "int main(int argc, char *argv[]) {\n");
    cg_push(cg);
    if (!ast) goto skip_main_body;
    for (size_t i = 0; i < ast->child_count; i++) {
        ASTNode *ch = ast->children[i];
        if (!ch || ch->type != NODE_ACTOR) continue;
        const char *name = (const char *)ch->data;
        if (!name) continue;

        /* 先找 run 方法 */
        for (size_t j = 0; j < ch->child_count; j++) {
            ASTNode *m = ch->children[j];
            if (m && (m->type == NODE_BE || m->type == NODE_FUN) && m->data && strcmp((const char *)m->data, "run") == 0) {
                cg_emit(cg, "%s_t __main_obj = %s_create();\n", name, name);
                cg_emit(cg, "%s_%s(&__main_obj);\n", name, "run");
                cg_emit(cg, "(void)__main_obj;\n");
                goto skip_main_body;
            }
        }
        for (size_t j = 0; j < ch->child_count; j++) {
            ASTNode *m = ch->children[j];
            if (m && m->type == NODE_NEW) {
                cg_emit(cg, "%s_t __main_obj = %s_create();\n", name, name);
                cg_emit(cg, "(void)__main_obj;\n");
                goto skip_main_body;
            }
        }
        /* 最后找第一个方法 */
        for (size_t j = 0; j < ch->child_count; j++) {
            ASTNode *m = ch->children[j];
            if (m && (m->type == NODE_BE || m->type == NODE_FUN) && m->data) {
                cg_emit(cg, "%s_t __main_obj = %s_create();\n", name, name);
                cg_emit(cg, "%s_%s(&__main_obj);\n", name, (const char *)m->data);
                cg_emit(cg, "(void)__main_obj;\n");
                goto skip_main_body;
            }
        }
        cg_emit(cg, "printf(\"Hello from Pony++ native (real backend)\\n\");\n");
    }
    skip_main_body:
    cg_pop(cg);
    cg_emit(cg, "    return 0;\n");
    cg_emit(cg, "}\n");
}