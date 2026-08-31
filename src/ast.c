/*
 * ast.c - Pony++ AST 节点创建与管理
 */

#include "ponypp/ast.h"
#include "ponypp/util.h"

ASTNode *ast_node_new(ASTNodeType type, int line, int column) {
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    node->type = type;
    node->line = line;
    node->column = column;
    node->ref_count = 1;
    return node;
}

void ast_node_add_child(ASTNode *parent, ASTNode *child) {
    if (!parent || !child) return;
    if (parent->child_count >= parent->child_cap) {
        parent->child_cap = parent->child_cap ? parent->child_cap * 2 : 4;
        parent->children = (ASTNode **)realloc(parent->children,
                                                 parent->child_cap * sizeof(ASTNode *));
        if (!parent->children) {
            fprintf(stderr, "致命错误: 内存分配失败\n");
            exit(EXIT_FAILURE);
        }
    }
    parent->children[parent->child_count++] = child;
}

void ast_node_release(ASTNode *node) {
    if (!node) return;
    node->ref_count--;
    if (node->ref_count > 0) return;
    ast_node_free(node);
}

static void free_node_data(ASTNode *node) {
    if (!node) return;
    /* 如果 data 是指针，释放它（简化: 假设是 char*） */
    if (node->data) {
        free(node->data);
        node->data = NULL;
    }
}

void ast_node_free(ASTNode *node) {
    if (!node) return;
    for (size_t i = 0; i < node->child_count; i++) {
        ast_node_free(node->children[i]);
    }
    if (node->children) free(node->children);
    free_node_data(node);
    free(node);
}

/* ---- 具体节点创建 ---- */

ASTNode *ast_program_new(int line, int column) {
    ASTNode *node = ast_node_new(NODE_PROGRAM, line, column);
    return node;
}

ASTNode *ast_actor_new(const char *name, int line, int column) {
    ASTNode *node = ast_node_new(NODE_ACTOR, line, column);
    if (node) node->data = s_strdup(name);
    return node;
}

ASTNode *ast_method_new(const char *name, bool is_be, int line, int column) {
    ASTNode *node = ast_node_new(is_be ? NODE_BE : NODE_FUN, line, column);
    if (node) node->data = s_strdup(name);
    return node;
}

ASTNode *ast_field_new(const char *name, bool is_var, int line, int column) {
    ASTNode *node = ast_node_new(is_var ? NODE_VAR : NODE_LET, line, column);
    if (node) node->data = s_strdup(name);
    return node;
}

ASTNode *ast_string_new(const char *value, int line, int column) {
    ASTNode *node = ast_node_new(NODE_STRING, line, column);
    if (node) node->data = s_strdup(value);
    return node;
}

ASTNode *ast_int_new(uint64_t value, int line, int column) {
    ASTNode *node = ast_node_new(NODE_INT, line, column);
    if (node) node->data = s_malloc(sizeof(uint64_t));
    if (node && node->data) *(uint64_t *)node->data = value;
    return node;
}

ASTNode *ast_bool_new(bool value, int line, int column) {
    ASTNode *node = ast_node_new(NODE_BOOL, line, column);
    if (node) node->data = s_malloc(sizeof(bool));
    if (node && node->data) *(bool *)node->data = value;
    return node;
}

ASTNode *ast_call_new(ASTNode *callee, const char *method, bool async, int line, int column) {
    (void)async;
    ASTNode *node = ast_node_new(NODE_CALL, line, column);
    if (node) {
        node->data = s_strdup(method);
        if (callee) ast_node_add_child(node, callee);
    }
    return node;
}

/* ---- 辅助释放 ---- */

void ast_actor_free(Actor *actor) {
    if (!actor) return;
    if (actor->name) free(actor->name);
    if (actor->fields) free(actor->fields);
    if (actor->methods) free(actor->methods);
    if (actor->constructors) free(actor->constructors);
}

void ast_method_free(Method *method) {
    if (!method) return;
    if (method->name) free(method->name);
}

void ast_field_free(Field *field) {
    if (!field) return;
    if (field->name) free(field->name);
}
