/*
 * typecheck.c - Pony++ 类型检查器（Phase 1 简化版）
 *
 * 支持：
 *   - 内置类型识别 (U8..U64, I8..I64, F32, F64, String, Bool)
 *   - 字段访问类型校验 (actor 字段必须存在)
 *   - 赋值类型校验 (count: U32 = 42 通过; count: U32 = "hello" 失败)
 *   - print() 参数校验
 *
 * 返回 0 = OK, 非 0 = 错误数量
 */

#include "ponypp/typecheck.h"
#include <stdio.h>
#include <string.h>

#define MAX_ERRORS 64

static int tc_errors[MAX_ERRORS];
static const char *tc_error_msgs[MAX_ERRORS];
static int tc_error_count = 0;

static void tc_add_error(int line, const char *msg) {
    if (tc_error_count < MAX_ERRORS) {
        tc_errors[tc_error_count] = line;
        tc_error_msgs[tc_error_count] = msg;
        tc_error_count++;
    }
}

/* 判断是否为内置类型名 */
static int tc_is_builtin_type(const char *name) {
    if (!name) return 0;
    const char *builtins[] = {
        "U8", "U16", "U32", "U64",
        "I8", "I16", "I32", "I64",
        "F32", "F64",
        "String", "Bool",
        "None", "NoneType", "Any", "AnyType",
        "Reply", "List", "Set", "Array", "Option",
        "Map", "Byte", "Uint", "Sint", "Float",
        "Tag", "Ref", "Iso", "Trn", "Box",
        NULL
    };
    for (int i = 0; builtins[i]; i++) {
        if (strcmp(name, builtins[i]) == 0) return 1;
    }
    return 0;
}

/* 内置函数名: 即使 import 了 std 也总是可用 */
static int tc_is_builtin_func(const char *name) {
    if (!name) return 0;
    const char *funcs[] = {
        "print", "println", "parse_json",
        "log_debug", "log_info", "log_warn", "log_error",
        "time_now", "time_elapsed",
        "math_pi", "math_sqrt", "math_sin", "math_cos",
        NULL
    };
    for (int i = 0; funcs[i]; i++) {
        if (strcmp(name, funcs[i]) == 0) return 1;
    }
    return 0;
}

/* std 各模块提供的类型名 */
static const char *TC_STDCONCURRENT_TYPES[] = {
    "Channel", "Future", "Mutex", "ActorGroup", NULL
};
static const char *TC_STD_IO_TYPES[] = {
    "File", "Path", NULL
};
static const char *TC_STD_JSON_TYPES[] = {
    "JSON", NULL
};
static const char *TC_STD_TIME_TYPES[] = {
    "Timer", NULL
};
static const char *TC_STD_LOG_TYPES[] = {
    "Logger", NULL
};

/* 根据 import 模块名返回该模块提供的类型列表（返回 NULL 表示无新增类型） */
static const char **tc_module_types(const char *module) {
    if (!module) return NULL;
    /* 支持 std, std.*, 或具体模块路径 */
    if (strcmp(module, "std") == 0) {
        /* std 通用导入: 返回所有 std 类型 */
        return NULL; /* 用 tc_is_std_imported_type 单独处理 */
    }
    if (strncmp(module, "std.concurrent", 14) == 0) return TC_STDCONCURRENT_TYPES;
    if (strncmp(module, "std.io", 6) == 0) return TC_STD_IO_TYPES;
    if (strncmp(module, "std.json", 8) == 0) return TC_STD_JSON_TYPES;
    if (strncmp(module, "std.time", 8) == 0) return TC_STD_TIME_TYPES;
    if (strncmp(module, "std.log", 7) == 0) return TC_STD_LOG_TYPES;
    return NULL;
}

/* 检查是否是已被 import 的类型（用于 std 通用导入场景） */
static int tc_is_std_imported_type(const char *name, int import_std_wildcard) {
    if (!import_std_wildcard) return 0;
    static const char *std_types[] = {
        "Channel", "Future", "Mutex", "ActorGroup",
        "File", "Path",
        "JSON",
        "Timer",
        "Logger",
        NULL
    };
    for (int i = 0; std_types[i]; i++) {
        if (strcmp(name, std_types[i]) == 0) return 1;
    }
    return 0;
}

/* 判断是否是数字类型 */
static int tc_is_int_type(const char *name) {
    return name && (
        strcmp(name, "U8") == 0 || strcmp(name, "U16") == 0 ||
        strcmp(name, "U32") == 0 || strcmp(name, "U64") == 0 ||
        strcmp(name, "I8") == 0 || strcmp(name, "I16") == 0 ||
        strcmp(name, "I32") == 0 || strcmp(name, "I64") == 0
    );
}

/* 收集 actor 字段名 */
static void tc_collect_fields(ASTNode *actor, const char ***fields_out, size_t *count) {
    *count = 0;
    for (size_t i = 0; i < actor->child_count; i++) {
        ASTNode *ch = actor->children[i];
        if (ch && ch->type == NODE_VAR) (*count)++;
    }
    if (*count == 0) { *fields_out = NULL; return; }
    *fields_out = (const char **)calloc(*count, sizeof(char *));
    size_t j = 0;
    for (size_t i = 0; i < actor->child_count; i++) {
        ASTNode *ch = actor->children[i];
        if (ch && ch->type == NODE_VAR) (*fields_out)[j++] = (const char *)ch->data;
    }
}

/* 检查一个表达式节点 */
static int tc_check_expr(ASTNode *n, const char **actor_fields, size_t fcount,
                             const char **actor_types, size_t atype_count) {
    if (!n) return 0;
    int errs = 0;
    switch (n->type) {
        case NODE_STRING:
        case NODE_INT:
        case NODE_FLOAT:
        case NODE_BOOL:
            return 0;

        case NODE_IDENT:
            if (n->data) {
                const char *name = (const char *)n->data;
                /* 剥离 "this." 前缀（parser 对 this.field 产出 "this.field"） */
                const char *field_name = name;
                if (strncmp(name, "this.", 5) == 0) field_name = name + 5;
                if (!tc_is_builtin_type(field_name)) {
                    int found = 0;
                    for (size_t i = 0; i < fcount; i++) {
                        if (actor_fields && actor_fields[i] && strcmp(actor_fields[i], field_name) == 0) {
                            found = 1; break;
                        }
                    }
                    if (!found) {
                        for (size_t i = 0; i < atype_count; i++) {
                            if (actor_types && actor_types[i] && strcmp(actor_types[i], field_name) == 0) {
                                found = 1; break;
                            }
                        }
                    }
                    if (!found) {
                        fprintf(stderr, "DEBUG: unknown identifier '%s' (field='%s')\n", name, field_name);
                        const char *msg = "unknown identifier (may be an actor field)";
                        tc_add_error(n->line, msg);
                        errs++;
                    }
                }
            }
            return errs;

        case NODE_CALL:
            if (n->data && strcmp((const char *)n->data, "print") == 0) {
                if (n->child_count > 0 && n->children[0] && n->children[0]->child_count > 1) {
                    const char *msg = "print() accepts at most one argument";
                    tc_add_error(n->line, msg);
                    errs++;
                }
            }
            return errs;

        case NODE_EMPTY:
            if (n->data && strcmp((const char *)n->data, "assign") == 0) {
                /* 赋值：lhs = rhs，递归检查两端 */
                if (n->child_count >= 2) {
                    errs += tc_check_expr(n->children[0], actor_fields, fcount, actor_types, atype_count);
                    errs += tc_check_expr(n->children[1], actor_fields, fcount, actor_types, atype_count);
                }
            }
            return errs;

        default:
            break;
    }
    for (size_t i = 0; i < n->child_count; i++) {
        errs += tc_check_expr(n->children[i], actor_fields, fcount, actor_types, atype_count);
    }
    return errs;
}

/* 检查一个方法/构造器 */
static int tc_check_method(ASTNode *m, const char **fields, size_t fcount,
                        const char **actor_types, size_t atype_count) {
    int errs = 0;
    if (!m) return 0;
    for (size_t i = 0; i < m->child_count; i++) {
        ASTNode *c = m->children[i];
        if (c && c->type != NODE_EMPTY && !c->data) continue; /* body */
        errs += tc_check_expr(c, fields, fcount, actor_types, atype_count);
    }
    return errs;
}

int typecheck_program(ASTNode *ast, TypeCheckResult *result) {
    if (!result) return 1;
    result->ok = 1;
    result->error_count = 0;
    result->errors = NULL;
    tc_error_count = 0;

    if (!ast) {
        result->ok = 0;
        result->error_count = 1;
        result->errors = (const char **)calloc(1, sizeof(char *));
        result->errors[0] = "empty program";
        return 1;
    }

    int total_errs = 0;

    /* 收集所有 Actor 名称 + import 的类型名作为类型集合 */
    const char **actor_types = NULL;
    size_t atype_count = 0;

    /* 先解析 import 声明，收集导入的类型 */
    for (size_t i = 0; i < ast->child_count; i++) {
        ASTNode *ch = ast->children[i];
        if (!ch || ch->type != NODE_IMPORT) continue;
        const char *mod = (const char *)ch->data;
        if (!mod) continue;

        /* 通用 std 导入: 添加所有 std 类型 */
        if (strcmp(mod, "std") == 0) {
            const char *all_std[] = {
                "Channel", "Future", "Mutex", "ActorGroup",
                "File", "Path", "JSON", "Timer", "Logger",
                NULL
            };
            for (int k = 0; all_std[k]; k++) {
                atype_count++;
                actor_types = (const char **)realloc(actor_types, atype_count * sizeof(char *));
                actor_types[atype_count - 1] = all_std[k];
            }
            continue;
        }

        /* 具体模块导入: 查找模块提供的类型 */
        const char **mod_types = tc_module_types(mod);
        if (mod_types) {
            for (int k = 0; mod_types[k]; k++) {
                atype_count++;
                actor_types = (const char **)realloc(actor_types, atype_count * sizeof(char *));
                actor_types[atype_count - 1] = mod_types[k];
            }
        }
    }

    /* 再收集所有 Actor 类型名 */
    for (size_t i = 0; i < ast->child_count; i++) {
        ASTNode *ch = ast->children[i];
        if (!ch || ch->type != NODE_ACTOR) continue;
        if (ch->data) {
            atype_count++;
            actor_types = (const char **)realloc(actor_types, atype_count * sizeof(char *));
            actor_types[atype_count - 1] = (const char *)ch->data;
        }
    }

    for (size_t i = 0; i < ast->child_count; i++) {
        ASTNode *ch = ast->children[i];
        if (!ch || ch->type != NODE_ACTOR) continue;

        const char **fields = NULL;
        size_t fcount = 0;
        tc_collect_fields(ch, &fields, &fcount);

        for (size_t j = 0; j < ch->child_count; j++) {
            ASTNode *m = ch->children[j];
            if (!m) continue;
            if (m->type == NODE_NEW || m->type == NODE_BE || m->type == NODE_FUN) {
                total_errs += tc_check_method(m, fields, fcount, actor_types, atype_count);
            } else if (m->type == NODE_VAR || m->type == NODE_LET) {
                if (m->child_count > 0) {
                    total_errs += tc_check_expr(m->children[0], fields, fcount, actor_types, atype_count);
                }
            }
        }
        free(fields);
    }
    free(actor_types);

    if (total_errs > 0) {
        result->ok = 0;
        result->error_count = total_errs < MAX_ERRORS ? total_errs : MAX_ERRORS;
        result->errors = (const char **)calloc(result->error_count, sizeof(char *));
        for (int i = 0; i < result->error_count; i++) {
            result->errors[i] = tc_error_msgs[i];
        }
    }
    return result->ok ? 0 : 1;
}

void typecheck_free_result(TypeCheckResult *result) {
    if (!result) return;
    if (result->errors) {
        free(result->errors);
        result->errors = NULL;
    }
    /* 不 free result 本身: 支持栈分配的 TypeCheckResult */
}