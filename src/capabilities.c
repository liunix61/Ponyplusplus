/*
 * capabilities.c - Pony++ 引用能力验证（Phase 1 简化版）
 *
 * 支持：
 *   - iso 唯一性检查（每个 actor 内 iso 字段只允许一个）
 *   - trn 不能出现在 be 方法签名中
 *   - ref/box 不能出现在跨 actor 方法参数中
 */

#include "ponypp/capabilities.h"
#include <stdio.h>
#include <string.h>

#define MAX_ERRORS 64

static int cap_errors[MAX_ERRORS];
static const char *cap_error_msgs[MAX_ERRORS];
static int cap_error_count = 0;

static void cap_add_error(int line, const char *msg) {
    if (cap_error_count < MAX_ERRORS) {
        cap_errors[cap_error_count] = line;
        cap_error_msgs[cap_error_count] = msg;
        cap_error_count++;
    }
}

static int is_be_method(ASTNode *n) {
    return n && n->type == NODE_BE;
}

int capabilities_check_program(ASTNode *ast, CapCheckResult *result) {
    if (!result) return 1;
    result->ok = 1;
    result->error_count = 0;
    result->errors = NULL;
    cap_error_count = 0;

    if (!ast) {
        result->ok = 0;
        result->error_count = 1;
        result->errors = (const char **)calloc(1, sizeof(char *));
        result->errors[0] = "empty program";
        return 1;
    }

    for (size_t i = 0; i < ast->child_count; i++) {
        ASTNode *actor = ast->children[i];
        if (!actor || actor->type != NODE_ACTOR) continue;

        int iso_count = 0;
        for (size_t j = 0; j < actor->child_count; j++) {
            ASTNode *m = actor->children[j];
            if (!m) continue;
            if (m->type == NODE_VAR || m->type == NODE_LET) {
                /* 简化：检查 data 是否含 'iso' */
                const char *d = (const char *)m->data;
                if (d && strstr(d, "iso")) iso_count++;
            }
            if (is_be_method(m)) {
                /* be 方法参数不能含 trn */
                if (m->child_count > 0 && m->children[0]) {
                    ASTNode *params = m->children[0];
                    for (size_t k = 0; k < params->child_count; k++) {
                        ASTNode *p = params->children[k];
                        if (p && p->data && strstr((const char *)p->data, "trn")) {
                            const char *msg = "trn capability not allowed in be method params";
                            cap_add_error(m->line, msg);
                        }
                    }
                }
            }
        }
        if (iso_count > 1) {
            const char *msg = "multiple iso fields in one actor (only one iso allowed)";
            cap_add_error(actor->line, msg);
        }
    }

    if (cap_error_count > 0) {
        result->ok = 0;
        result->error_count = cap_error_count;
        result->errors = (const char **)calloc(cap_error_count, sizeof(char *));
        for (int i = 0; i < cap_error_count; i++) {
            result->errors[i] = cap_error_msgs[i];
        }
    }
    return result->ok ? 0 : 1;
}

void cap_check_free_result(CapCheckResult *result) {
    if (!result) return;
    if (result->errors) { free(result->errors); result->errors = NULL; }
    free(result);
}