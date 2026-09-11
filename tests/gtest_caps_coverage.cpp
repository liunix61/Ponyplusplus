#include <gtest/gtest.h>
#include <ponypp/capabilities.h>
#include <ponypp/ast.h>
#include <cstring>
#include <cstdlib>

/* 辅助: 创建 AST 节点 */
static ASTNode *make_node(ASTNodeType type, const char *data) {
    ASTNode *n = (ASTNode *)calloc(1, sizeof(ASTNode));
    n->type = type;
    n->data = data ? strdup(data) : nullptr;
    n->child_count = 0;
    n->children = nullptr;
    n->line = 1;
    return n;
}

static void add_child(ASTNode *parent, ASTNode *child) {
    parent->child_count++;
    parent->children = (ASTNode **)realloc(parent->children, parent->child_count * sizeof(ASTNode *));
    parent->children[parent->child_count - 1] = child;
}

static void free_node(ASTNode *n) {
    if (!n) return;
    for (size_t i = 0; i < n->child_count; i++)
        free_node(n->children[i]);
    free(n->children);
    if (n->data) free((void *)n->data);
    free(n);
}

/* ==================== iso 唯一性 ==================== */

TEST(CapsFull, SingleIsoField) {
    ASTNode *prog = make_node(NODE_EMPTY, nullptr);
    ASTNode *actor = make_node(NODE_ACTOR, "Main");
    ASTNode *var_iso = make_node(NODE_VAR, "iso");
    add_child(actor, var_iso);
    add_child(prog, actor);
    
    CapCheckResult result = {};
    int ret = capabilities_check_program(prog, &result);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(result.ok, 1);
    
    free_node(prog);
}

TEST(CapsFull, MultipleIsoFields) {
    ASTNode *prog = make_node(NODE_EMPTY, nullptr);
    ASTNode *actor = make_node(NODE_ACTOR, "Main");
    ASTNode *var_iso1 = make_node(NODE_VAR, "iso");
    ASTNode *var_iso2 = make_node(NODE_LET, "iso");
    add_child(actor, var_iso1);
    add_child(actor, var_iso2);
    add_child(prog, actor);
    
    CapCheckResult result = {};
    int ret = capabilities_check_program(prog, &result);
    EXPECT_NE(ret, 0);
    EXPECT_EQ(result.ok, 0);
    EXPECT_GE(result.error_count, 1);
    
    free_node(prog);
}

/* ==================== trn 在 be 方法参数中 ==================== */

TEST(CapsFull, TrnInBeParams) {
    ASTNode *prog = make_node(NODE_EMPTY, nullptr);
    ASTNode *actor = make_node(NODE_ACTOR, "Main");
    ASTNode *be = make_node(NODE_BE, "do_work");
    ASTNode *params = make_node(NODE_EMPTY, nullptr);
    ASTNode *param = make_node(NODE_EMPTY, "x: trn Buffer");
    add_child(params, param);
    add_child(be, params);
    add_child(actor, be);
    add_child(prog, actor);
    
    CapCheckResult result = {};
    int ret = capabilities_check_program(prog, &result);
    EXPECT_NE(ret, 0);
    EXPECT_EQ(result.ok, 0);
    
    free_node(prog);
}

/* ==================== 正常 be 方法（无 trn） ==================== */

TEST(CapsFull, BeMethodNoTrn) {
    ASTNode *prog = make_node(NODE_EMPTY, nullptr);
    ASTNode *actor = make_node(NODE_ACTOR, "Main");
    ASTNode *be = make_node(NODE_BE, "do_work");
    ASTNode *params = make_node(NODE_EMPTY, nullptr);
    ASTNode *param = make_node(NODE_EMPTY, "x: U32");
    add_child(params, param);
    add_child(be, params);
    add_child(actor, be);
    add_child(prog, actor);
    
    CapCheckResult result = {};
    int ret = capabilities_check_program(prog, &result);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(result.ok, 1);
    
    free_node(prog);
}

/* ==================== 空程序 ==================== */

TEST(CapsFull, NullProgram) {
    CapCheckResult result = {};
    int ret = capabilities_check_program(nullptr, &result);
    EXPECT_NE(ret, 0);
    EXPECT_EQ(result.ok, 0);
    EXPECT_EQ(result.error_count, 1);
}

/* ==================== 非 actor 节点跳过 ==================== */

TEST(CapsFull, NonActorNodesSkipped) {
    ASTNode *prog = make_node(NODE_EMPTY, nullptr);
    ASTNode *import = make_node(NODE_IMPORT, "std/io");
    add_child(prog, import);
    
    CapCheckResult result = {};
    int ret = capabilities_check_program(prog, &result);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(result.ok, 1);
    
    free_node(prog);
}

/* ==================== 多 actor 混合 ==================== */

TEST(CapsFull, MultipleActorsMixed) {
    ASTNode *prog = make_node(NODE_EMPTY, nullptr);
    
    /* Actor 1: 正常 */
    ASTNode *actor1 = make_node(NODE_ACTOR, "Worker");
    ASTNode *var1 = make_node(NODE_VAR, "ref");
    add_child(actor1, var1);
    add_child(prog, actor1);
    
    /* Actor 2: 多 iso 错误 */
    ASTNode *actor2 = make_node(NODE_ACTOR, "Bad");
    ASTNode *iso1 = make_node(NODE_VAR, "iso");
    ASTNode *iso2 = make_node(NODE_VAR, "iso");
    add_child(actor2, iso1);
    add_child(actor2, iso2);
    add_child(prog, actor2);
    
    CapCheckResult result = {};
    int ret = capabilities_check_program(prog, &result);
    EXPECT_NE(ret, 0);
    EXPECT_EQ(result.ok, 0);
    
    free_node(prog);
}

/* ==================== 多个错误 ==================== */

TEST(CapsFull, MultipleErrors) {
    ASTNode *prog = make_node(NODE_EMPTY, nullptr);
    ASTNode *actor = make_node(NODE_ACTOR, "Bad");
    
    /* 多 iso */
    ASTNode *iso1 = make_node(NODE_VAR, "iso");
    ASTNode *iso2 = make_node(NODE_VAR, "iso");
    add_child(actor, iso1);
    add_child(actor, iso2);
    
    /* be 方法带 trn 参数 */
    ASTNode *be = make_node(NODE_BE, "bad_method");
    ASTNode *params = make_node(NODE_EMPTY, nullptr);
    ASTNode *param = make_node(NODE_EMPTY, "x: trn Data");
    add_child(params, param);
    add_child(be, params);
    add_child(actor, be);
    
    add_child(prog, actor);
    
    CapCheckResult result = {};
    int ret = capabilities_check_program(prog, &result);
    EXPECT_NE(ret, 0);
    EXPECT_GE(result.error_count, 2);
    
    free_node(prog);
}

/* ==================== be 方法无参数 ==================== */

TEST(CapsFull, BeMethodNoParams) {
    ASTNode *prog = make_node(NODE_EMPTY, nullptr);
    ASTNode *actor = make_node(NODE_ACTOR, "Main");
    ASTNode *be = make_node(NODE_BE, "do_nothing");
    add_child(actor, be);
    add_child(prog, actor);
    
    CapCheckResult result = {};
    int ret = capabilities_check_program(prog, &result);
    EXPECT_EQ(ret, 0);
    
    free_node(prog);
}

/* ==================== NULL 安全 ==================== */

TEST(CapsFull, NullResult) {
    EXPECT_NE(capabilities_check_program(nullptr, nullptr), 0);
}
