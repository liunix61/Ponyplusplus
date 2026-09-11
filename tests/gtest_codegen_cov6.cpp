#include <gtest/gtest.h>
#include <ponypp/codegen.h>
#include <ponypp/ast.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

static bool gen_ast(ASTNode *ast) {
    char tmpl[] = "/tmp/ponypp_cg6_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) return false;
    close(fd);
    FILE *f = fopen(tmpl, "w");
    if (!f) { unlink(tmpl); return false; }
    Codegen *cg = codegen_new(f);
    if (!cg) { fclose(f); unlink(tmpl); return false; }
    codegen_set_source_file(cg, "test.pny");
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    unlink(tmpl);
    return true;
}

/* print(NODE_FLOAT) */
TEST(CodegenCov6, PrintFloat) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *print_stmt = ast_node_new(NODE_PRINT, 3, 1);
    ASTNode *val = ast_node_new(NODE_FLOAT, 3, 7);
    val->data = strdup("3.14");
    
    ast_node_add_child(print_stmt, val);
    ast_node_add_child(create, print_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* print(NODE_IDENT) */
TEST(CodegenCov6, PrintIdent) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *print_stmt = ast_node_new(NODE_PRINT, 3, 1);
    ASTNode *val = ast_node_new(NODE_IDENT, 3, 7);
    val->data = strdup("x");
    
    ast_node_add_child(print_stmt, val);
    ast_node_add_child(create, print_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* print(NODE_INT) */
TEST(CodegenCov6, PrintInt) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *print_stmt = ast_node_new(NODE_PRINT, 3, 1);
    ASTNode *val = ast_node_new(NODE_INT, 3, 7);
    val->data = strdup("42");
    
    ast_node_add_child(print_stmt, val);
    ast_node_add_child(create, print_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* print(NODE_STRING) */
TEST(CodegenCov6, PrintString) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *print_stmt = ast_node_new(NODE_PRINT, 3, 1);
    ASTNode *val = ast_node_new(NODE_STRING, 3, 7);
    val->data = strdup("hello");
    
    ast_node_add_child(print_stmt, val);
    ast_node_add_child(create, print_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* print(NODE_CALL) */
TEST(CodegenCov6, PrintCall) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *print_stmt = ast_node_new(NODE_PRINT, 3, 1);
    ASTNode *call = ast_node_new(NODE_CALL, 3, 7);
    call->data = strdup("foo");
    ASTNode *arg = ast_node_new(NODE_INT, 3, 12);
    arg->data = strdup("1");
    ast_node_add_child(call, arg);
    
    ast_node_add_child(print_stmt, call);
    ast_node_add_child(create, print_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* NODE_SEND 消息发送 */
TEST(CodegenCov6, SendMsg) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *send = ast_node_new(NODE_SEND, 3, 1);
    send->data = strdup("greet");
    ASTNode *target = ast_node_new(NODE_IDENT, 3, 1);
    target->data = strdup("other");
    ast_node_add_child(send, target);
    
    ast_node_add_child(create, send);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* NODE_MSG_CALL 同步消息 */
TEST(CodegenCov6, MsgCall) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *msg = ast_node_new(NODE_MSG_CALL, 3, 1);
    msg->data = strdup("compute");
    ASTNode *target = ast_node_new(NODE_IDENT, 3, 1);
    target->data = strdup("worker");
    ast_node_add_child(msg, target);
    
    ast_node_add_child(create, msg);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* NODE_BOOL 赋值 */
TEST(CodegenCov6, Assign) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *assign = ast_node_new(NODE_BOOL, 3, 1);
    ASTNode *lhs = ast_node_new(NODE_IDENT, 3, 1);
    lhs->data = strdup("x");
    ASTNode *rhs = ast_node_new(NODE_INT, 3, 5);
    rhs->data = strdup("42");
    ast_node_add_child(assign, lhs);
    ast_node_add_child(assign, rhs);
    
    ast_node_add_child(create, assign);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* NODE_RETURN */
TEST(CodegenCov6, Return) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *ret = ast_node_new(NODE_RETURN, 3, 1);
    ASTNode *val = ast_node_new(NODE_INT, 3, 8);
    val->data = strdup("0");
    ast_node_add_child(ret, val);
    
    ast_node_add_child(create, ret);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* NODE_IF */
TEST(CodegenCov6, IfStmt) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *if_stmt = ast_node_new(NODE_IF, 3, 1);
    ASTNode *cond = ast_node_new(NODE_INT, 3, 4);
    cond->data = strdup("1");
    ast_node_add_child(if_stmt, cond);
    
    ast_node_add_child(create, if_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* NODE_WHILE */
TEST(CodegenCov6, WhileStmt) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *while_stmt = ast_node_new(NODE_WHILE, 3, 1);
    ASTNode *cond = ast_node_new(NODE_INT, 3, 7);
    cond->data = strdup("0");
    ast_node_add_child(while_stmt, cond);
    
    ast_node_add_child(create, while_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* NODE_LET */
TEST(CodegenCov6, LetStmt) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *let = ast_node_new(NODE_LET, 3, 1);
    let->data = strdup("x");
    ASTNode *val = ast_node_new(NODE_INT, 3, 9);
    val->data = strdup("1");
    ast_node_add_child(let, val);
    
    ast_node_add_child(create, let);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* NODE_VAR */
TEST(CodegenCov6, VarStmt) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *var = ast_node_new(NODE_VAR, 3, 1);
    var->data = strdup("y");
    ASTNode *val = ast_node_new(NODE_INT, 3, 9);
    val->data = strdup("2");
    ast_node_add_child(var, val);
    
    ast_node_add_child(create, var);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* NODE_CHAR */
TEST(CodegenCov6, BinOp) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *print_stmt = ast_node_new(NODE_PRINT, 3, 1);
    ASTNode *binop = ast_node_new(NODE_CHAR, 3, 7);
    binop->data = strdup("+");
    ASTNode *lhs = ast_node_new(NODE_INT, 3, 7);
    lhs->data = strdup("1");
    ASTNode *rhs = ast_node_new(NODE_INT, 3, 11);
    rhs->data = strdup("2");
    ast_node_add_child(binop, lhs);
    ast_node_add_child(binop, rhs);
    
    ast_node_add_child(print_stmt, binop);
    ast_node_add_child(create, print_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* NODE_CHAR 减法 */
TEST(CodegenCov6, BinOpSub) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *print_stmt = ast_node_new(NODE_PRINT, 3, 1);
    ASTNode *binop = ast_node_new(NODE_CHAR, 3, 7);
    binop->data = strdup("-");
    ASTNode *lhs = ast_node_new(NODE_INT, 3, 7);
    lhs->data = strdup("5");
    ASTNode *rhs = ast_node_new(NODE_INT, 3, 11);
    rhs->data = strdup("3");
    ast_node_add_child(binop, lhs);
    ast_node_add_child(binop, rhs);
    
    ast_node_add_child(print_stmt, binop);
    ast_node_add_child(create, print_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* NODE_CHAR 乘法 */
TEST(CodegenCov6, BinOpMul) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ASTNode *print_stmt = ast_node_new(NODE_PRINT, 3, 1);
    ASTNode *binop = ast_node_new(NODE_CHAR, 3, 7);
    binop->data = strdup("*");
    ASTNode *lhs = ast_node_new(NODE_INT, 3, 7);
    lhs->data = strdup("2");
    ASTNode *rhs = ast_node_new(NODE_INT, 3, 11);
    rhs->data = strdup("3");
    ast_node_add_child(binop, lhs);
    ast_node_add_child(binop, rhs);
    
    ast_node_add_child(print_stmt, binop);
    ast_node_add_child(create, print_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* NODE_CAP */
TEST(CodegenCov6, CapNode) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    ASTNode *cap = ast_node_new(NODE_CAP, 1, 7);
    cap->data = strdup("iso");
    ast_node_add_child(actor, cap);
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* 空程序 */
TEST(CodegenCov6, EmptyProgram) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    EXPECT_TRUE(gen_ast(prog));
}

/* 只有 actor 没有 body */
TEST(CodegenCov6, ActorNoBody) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(gen_ast(prog));
}

/* sourcemap 写入/查询 */
TEST(CodegenCov6, SourcemapWrite) {
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);
    sourcemap_add(sm, 1, "test.pny", 1, 0);
    sourcemap_add(sm, 2, "test.pny", 2, 0);
    sourcemap_add(sm, 3, "test.pny", 3, 0);
    
    char tmpl[] = "/tmp/ponypp_sm6_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    sourcemap_save_json(sm, tmpl);
    
    /* 查询 */
    SourceMapEntry entry;
    int r = sourcemap_lookup(sm, 1, &entry);
    EXPECT_EQ(r, 0);
    
    sourcemap_free(sm);
    unlink(tmpl);
}

/* sourcemap 行号范围 */
TEST(CodegenCov6, SourcemapLineRange) {
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);
    for (int i = 1; i <= 100; i++) {
        sourcemap_add(sm, i, "test.pny", i, 0);
    }
    
    SourceMapEntry entry;
    sourcemap_lookup(sm, 50, &entry);
    EXPECT_EQ(entry.source_line, 50);
    
    sourcemap_free(sm);
}

/* sourcemap 大列号 */
TEST(CodegenCov6, SourcemapLargeColumn) {
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);
    sourcemap_add(sm, 1, "test.pny", 1, 1000);
    
    SourceMapEntry entry;
    sourcemap_lookup(sm, 1, &entry);
    EXPECT_EQ(entry.source_col, 1000);
    
    sourcemap_free(sm);
}

/* codegen_new_with_map */
TEST(CodegenCov6, NewWithMap) {
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);
    
    char tmpl[] = "/tmp/ponypp_cg6m_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    
    Codegen *cg = codegen_new_with_map(f, sm);
    ASSERT_NE(cg, nullptr);
    codegen_free(cg);
    
    fclose(f);
    unlink(tmpl);
    sourcemap_free(sm);
}
