/*
 * wit.c - Pony++ WIT 接口生成器（Phase 1）
 *
 * 根据 AST 中的 Actor 和方法生成 WIT 文件。
 * 格式：
 *   package ponypp:<name>;
 *   interface <actor> {
 *     record <ctor> { <fields> }
 *     fn <be_method>(<args>) -> ?<return>
 *   }
 *   world <actor> { use ponypp:common:print; }
 */

#include "ponypp/wit.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void wit_indent(FILE *f, int depth) {
    for (int i = 0; i < depth; i++) fputs("  ", f);
}

static const char *wit_c_type_of(ASTNode *t) {
    if (!t || !t->data) return "i32";
    const char *n = (const char *)t->data;
    if (strcmp(n, "U8") == 0) return "u8";
    if (strcmp(n, "U16") == 0) return "u16";
    if (strcmp(n, "U32") == 0) return "u32";
    if (strcmp(n, "U64") == 0) return "u64";
    if (strcmp(n, "I8") == 0) return "s8";
    if (strcmp(n, "I16") == 0) return "s16";
    if (strcmp(n, "I32") == 0) return "s32";
    if (strcmp(n, "I64") == 0) return "s64";
    if (strcmp(n, "F32") == 0) return "f32";
    if (strcmp(n, "F64") == 0) return "f64";
    if (strcmp(n, "String") == 0) return "string";
    if (strcmp(n, "Bool") == 0) return "bool";
    if (strcmp(n, "None") == 0 || strcmp(n, "NoneType") == 0) return "option<nothing>";
    if (strcmp(n, "Byte") == 0) return "bytes";
    return "i32";
}

int wit_write_program(ASTNode *ast, const char *output, TargetKind target) {
    if (!ast || !output) return 1;
    FILE *f = fopen(output, "w");
    if (!f) return 1;

    const char *pkg_prefix = "ponypp";
    if (target == TARGET_MCU_WASM) pkg_prefix = "ponypp:mcu";
    else if (target == TARGET_BROWSER) pkg_prefix = "ponypp:browser";
    else if (target == TARGET_WASI_P3) pkg_prefix = "ponypp:wasi-p3";

    if (ast->child_count == 0) {
        fprintf(f, "package %s:default;\n", pkg_prefix);
        fclose(f);
        return 0;
    }

    for (size_t i = 0; i < ast->child_count; i++) {
        ASTNode *actor = ast->children[i];
        if (!actor || actor->type != NODE_ACTOR) continue;
        const char *name = (const char *)actor->data;
        if (!name) name = "default";

        /* 检查泛型类型参数 */
        char tparams_str[512] = "";
        for (size_t j = 0; j < actor->child_count; j++) {
            ASTNode *c = actor->children[j];
            if (c && c->data && strcmp((const char *)c->data, "typeparams") == 0) {
                char buf[512] = "";
                for (size_t k = 0; k < c->child_count; k++) {
                    ASTNode *tp = c->children[k];
                    if (!tp || !tp->data) continue;
                    if (buf[0]) strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
                    strncat(buf, (const char *)tp->data, sizeof(buf) - strlen(buf) - 1);
                }
                if (buf[0]) {
                    snprintf(tparams_str, sizeof(tparams_str), "<%s>", buf);
                }
                break;
            }
        }

        fprintf(f, "package %s:%s;\n", pkg_prefix, name);
        fprintf(f, "interface %s%s {\n", name, tparams_str);

        /* Constructor record */
        for (size_t j = 0; j < actor->child_count; j++) {
            ASTNode *m = actor->children[j];
            if (!m || m->type != NODE_NEW) continue;
            const char *ctor = (const char *)m->data;
            if (!ctor) ctor = "new";
            wit_indent(f, 1);
            fprintf(f, "record %s {\n", ctor);
            if (m->child_count > 0 && m->children[0] &&
                m->children[0]->data &&
                strcmp((const char *)m->children[0]->data, "params") == 0) {
                for (size_t k = 0; k < m->children[0]->child_count; k++) {
                    ASTNode *p = m->children[0]->children[k];
                    if (!p) continue;
                    const char *pname = (const char *)p->data;
                    const char *ptype = (p->child_count > 0 && p->children[0])
                                        ? wit_c_type_of(p->children[0]) : "i32";
                    wit_indent(f, 2);
                    fprintf(f, "%s: %s,\n", pname ? pname : "arg", ptype);
                }
            }
            wit_indent(f, 1);
            fprintf(f, "}\n");
        }

        /* Fields as record */
        size_t fcount = 0;
        for (size_t j = 0; j < actor->child_count; j++) {
            ASTNode *ch = actor->children[j];
            if (ch && ch->type == NODE_VAR) fcount++;
        }
        if (fcount > 0) {
            wit_indent(f, 1);
            fprintf(f, "record state {\n");
            for (size_t j = 0; j < actor->child_count; j++) {
                ASTNode *ch = actor->children[j];
                if (!ch || ch->type != NODE_VAR) continue;
                const char *fname = (const char *)ch->data;
                const char *ftype = (ch->child_count > 0 && ch->children[0])
                                    ? wit_c_type_of(ch->children[0]) : "i32";
                wit_indent(f, 2);
                fprintf(f, "%s: %s,\n", fname ? fname : "field", ftype);
            }
            wit_indent(f, 1);
            fprintf(f, "}\n");
        }

        /* Methods */
        for (size_t j = 0; j < actor->child_count; j++) {
            ASTNode *m = actor->children[j];
            if (!m) continue;
            if (m->type != NODE_BE && m->type != NODE_FUN) continue;
            const char *mname = (const char *)m->data;
            if (!mname) continue;
            wit_indent(f, 1);
            fprintf(f, "fn %s(", mname);
            /* params */
            ASTNode *params = NULL;
            for (size_t k = 0; k < m->child_count; k++) {
                ASTNode *c2 = m->children[k];
                if (c2 && c2->type == NODE_EMPTY && params == NULL) params = c2;
                break;
            }
            if (params && params->data && strcmp((const char *)params->data, "params") == 0) {
                size_t first = 1;
                for (size_t k = 0; k < params->child_count; k++) {
                    ASTNode *p = params->children[k];
                    if (!p) continue;
                    if (!first) fprintf(f, ", ");
                    first = 0;
                    const char *pname = (const char *)p->data;
                    const char *ptype = (p->child_count > 0 && p->children[0])
                                        ? wit_c_type_of(p->children[0]) : "i32";
                    fprintf(f, "%s: %s", pname ? pname : "arg", ptype);
                }
            }
            fprintf(f, ")");
            /* return type */
            if (m->type == NODE_FUN) {
                fprintf(f, " -> i32");
            }
            fprintf(f, "\n");
        }

        fprintf(f, "}\n\n");

        /* world — target 感知 */
        if (target == TARGET_MCU_WASM) {
            /* MCU world: 硬件外设 imports */
            const char *mcu_platform = "generic";
            /* 简化: 默认 generic 外设 */
            fprintf(f, "world %s {\n", name);
            wit_indent(f, 1);
            fprintf(f, "use self:%s;\n", name);
            wit_indent(f, 1);
            fprintf(f, "import gpio: ponypp:mcu/gpio;\n");
            wit_indent(f, 1);
            fprintf(f, "import uart: ponypp:mcu/uart;\n");
            wit_indent(f, 1);
            fprintf(f, "import timer: ponypp:mcu/timer;\n");
            fprintf(f, "}\n");
        } else if (target == TARGET_BROWSER) {
            /* Browser world: JS 胶水 + 定时器 */
            fprintf(f, "world %s {\n", name);
            wit_indent(f, 1);
            fprintf(f, "use self:%s;\n", name);
            wit_indent(f, 1);
            fprintf(f, "import js: ponypp:js/glue;\n");
            wit_indent(f, 1);
            fprintf(f, "import timer: ponypp:timer;\n");
            fprintf(f, "}\n");
        } else if (target == TARGET_WASI_P3) {
            /* WASI P3 world: 文件系统 + 时钟 + 终端 */
            fprintf(f, "world %s {\n", name);
            wit_indent(f, 1);
            fprintf(f, "use self:%s;\n", name);
            wit_indent(f, 1);
            fprintf(f, "import console: wasi:cli/terminal;\n");
            wit_indent(f, 1);
            fprintf(f, "import clock: wasi:clocks;\n");
            wit_indent(f, 1);
            fprintf(f, "import preopens: wasi:filesystem/preopens;\n");
            fprintf(f, "}\n");
        } else {
            /* 默认 WASI P2 world */
            fprintf(f, "world %s {\n", name);
            wit_indent(f, 1);
            fprintf(f, "use self:%s;\n", name);
            fprintf(f, "}\n");
        }
    }

    fclose(f);
    return 0;
}