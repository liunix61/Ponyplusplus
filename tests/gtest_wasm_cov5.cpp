#include <gtest/gtest.h>
#include <ponypp/wasm.h>
#include <ponypp/ast.h>
#include <ponypp.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

static ASTNode *make_node(ASTNodeType type, const char *data) {
    ASTNode *n = (ASTNode*)calloc(1, sizeof(ASTNode));
    n->type = type;
    if (data) n->data = strdup(data);
    return n;
}

static void add_child(ASTNode *parent, ASTNode *child) {
    if (parent->child_count >= parent->child_cap) {
        parent->child_cap = parent->child_cap ? parent->child_cap * 2 : 4;
        parent->children = (ASTNode**)realloc(parent->children, parent->child_cap * sizeof(ASTNode*));
    }
    parent->children[parent->child_count++] = child;
}

static void free_node(ASTNode *n) {
    if (!n) return;
    for (size_t i = 0; i < n->child_count; i++) free_node(n->children[i]);
    free(n->children);
    free(n->data);
    free(n);
}

/* 构造 PROGRAM > ACTOR > NEW > BLOCK > stmts */
static ASTNode *make_program_with_stmts(ASTNode **stmts, int count) {
    ASTNode *prog = make_node(NODE_PROGRAM, NULL);
    ASTNode *actor = make_node(NODE_ACTOR, "main");
    ASTNode *newn = make_node(NODE_NEW, "create");
    ASTNode *block = make_node(NODE_EMPTY, NULL);
    for (int i = 0; i < count; i++) add_child(block, stmts[i]);
    add_child(newn, block);
    add_child(actor, newn);
    add_child(prog, actor);
    return prog;
}

/* NODE_INT 作为语句 (default case) */
TEST(WasmCov4, IntAsStmt) {
    ASTNode *stmts[] = { make_node(NODE_INT, "42") };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    const char *out = "/tmp/wasm_int_stmt.wasm";
    int r = wasm_write_program(prog, out, TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink(out);
}

/* NODE_FLOAT */
TEST(WasmCov4, FloatExpr) {
    ASTNode *stmts[] = { make_node(NODE_FLOAT, "3.14") };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_float.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_float.wasm");
}

/* NODE_STRING (直接作为语句) */
TEST(WasmCov4, StringExpr) {
    ASTNode *stmts[] = { make_node(NODE_STRING, "hello") };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_str.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_str.wasm");
}

/* NODE_BOOL true/false */
TEST(WasmCov4, BoolExpr) {
    ASTNode *s1 = make_node(NODE_BOOL, "true");
    ASTNode *s2 = make_node(NODE_BOOL, "false");
    ASTNode *stmts[] = { s1, s2 };
    ASTNode *prog = make_program_with_stmts(stmts, 2);
    int r = wasm_write_program(prog, "/tmp/wasm_bool.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_bool.wasm");
}

/* NODE_IDENT */
TEST(WasmCov4, IdentExpr) {
    ASTNode *stmts[] = { make_node(NODE_IDENT, "x") };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_ident.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_ident.wasm");
}

/* NODE_SEND (消息发送) */
TEST(WasmCov4, SendExpr) {
    ASTNode *send = make_node(NODE_SEND, NULL);
    add_child(send, make_node(NODE_IDENT, "receiver"));
    add_child(send, make_node(NODE_INT, "42"));
    ASTNode *stmts[] = { send };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_send.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_send.wasm");
}

/* NODE_MSG_CALL (同步消息调用) */
TEST(WasmCov4, MsgCallExpr) {
    ASTNode *msg = make_node(NODE_MSG_CALL, NULL);
    add_child(msg, make_node(NODE_IDENT, "receiver"));
    add_child(msg, make_node(NODE_INT, "42"));
    ASTNode *stmts[] = { msg };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_msg.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_msg.wasm");
}

/* NODE_IF (作为表达式) */
TEST(WasmCov4, IfExpr) {
    ASTNode *ifn = make_node(NODE_IF, NULL);
    add_child(ifn, make_node(NODE_BOOL, "true"));
    ASTNode *stmts[] = { ifn };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_if.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_if.wasm");
}

/* NODE_WHILE */
TEST(WasmCov4, WhileExpr) {
    ASTNode *wh = make_node(NODE_WHILE, NULL);
    ASTNode *stmts[] = { wh };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_while.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_while.wasm");
}

/* NODE_FOR */
TEST(WasmCov4, ForExpr) {
    ASTNode *fr = make_node(NODE_FOR, NULL);
    ASTNode *stmts[] = { fr };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_for.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_for.wasm");
}

/* NODE_RETURN with child */
TEST(WasmCov4, ReturnWithChild) {
    ASTNode *ret = make_node(NODE_RETURN, NULL);
    add_child(ret, make_node(NODE_INT, "42"));
    ASTNode *stmts[] = { ret };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_ret.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_ret.wasm");
}

/* NODE_RETURN without child */
TEST(WasmCov4, ReturnNoChild) {
    ASTNode *ret = make_node(NODE_RETURN, NULL);
    ASTNode *stmts[] = { ret };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_ret2.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_ret2.wasm");
}

/* NODE_CAP with child */
TEST(WasmCov4, CapWithChild) {
    ASTNode *cap = make_node(NODE_CAP, "ref");
    add_child(cap, make_node(NODE_INT, "1"));
    ASTNode *stmts[] = { cap };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_cap.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_cap.wasm");
}

/* NODE_VAR with child */
TEST(WasmCov4, VarWithChild) {
    ASTNode *var = make_node(NODE_VAR, "x");
    add_child(var, make_node(NODE_INT, "42"));
    ASTNode *stmts[] = { var };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_var.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_var.wasm");
}

/* NODE_LET with child */
TEST(WasmCov4, LetWithChild) {
    ASTNode *let = make_node(NODE_LET, "y");
    add_child(let, make_node(NODE_STRING, "test"));
    ASTNode *stmts[] = { let };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_let.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_let.wasm");
}

/* NODE_CALL non-print */
TEST(WasmCov4, CallNonPrint) {
    ASTNode *call = make_node(NODE_CALL, "foo");
    add_child(call, make_node(NODE_INT, "1"));
    ASTNode *stmts[] = { call };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_call.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_call.wasm");
}

/* NODE_CALL print with IDENT arg */
TEST(WasmCov4, PrintIdentArg) {
    ASTNode *call = make_node(NODE_CALL, "print");
    ASTNode *args = make_node(NODE_EMPTY, "args");
    add_child(args, make_node(NODE_IDENT, "x"));
    add_child(call, args);
    ASTNode *stmts[] = { call };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_printid.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_printid.wasm");
}

/* NODE_CALL print with no args */
TEST(WasmCov4, PrintNoArgs) {
    ASTNode *call = make_node(NODE_CALL, "print");
    ASTNode *stmts[] = { call };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_printno.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_printno.wasm");
}

/* NODE_IF with multiple children (if/else) */
TEST(WasmCov4, IfElse) {
    ASTNode *ifn = make_node(NODE_IF, NULL);
    add_child(ifn, make_node(NODE_BOOL, "true"));
    add_child(ifn, make_node(NODE_INT, "1"));
    ASTNode *stmts[] = { ifn };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_ifelse.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_ifelse.wasm");
}

/* WASI P3 target */
TEST(WasmCov4, WasiP3) {
    ASTNode *call = make_node(NODE_CALL, "print");
    ASTNode *args = make_node(NODE_EMPTY, "args");
    add_child(args, make_node(NODE_STRING, "p3"));
    add_child(call, args);
    ASTNode *stmts[] = { call };
    ASTNode *prog = make_program_with_stmts(stmts, 1);
    int r = wasm_write_program(prog, "/tmp/wasm_p3.wasm", TARGET_WASI_P3);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_p3.wasm");
}

/* 多个语句混合 */
TEST(WasmCov4, MixedStmts) {
    ASTNode *call = make_node(NODE_CALL, "print");
    ASTNode *args = make_node(NODE_EMPTY, "args");
    add_child(args, make_node(NODE_INT, "42"));
    add_child(call, args);
    ASTNode *var = make_node(NODE_VAR, "x");
    add_child(var, make_node(NODE_INT, "1"));
    ASTNode *ret = make_node(NODE_RETURN, NULL);
    add_child(ret, make_node(NODE_INT, "0"));
    ASTNode *stmts[] = { call, var, ret };
    ASTNode *prog = make_program_with_stmts(stmts, 3);
    int r = wasm_write_program(prog, "/tmp/wasm_mixed.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_mixed.wasm");
}

/* 多个 actor */
TEST(WasmCov4, MultipleActors) {
    ASTNode *prog = make_node(NODE_PROGRAM, NULL);
    for (int i = 0; i < 3; i++) {
        ASTNode *actor = make_node(NODE_ACTOR, "actor");
        ASTNode *newn = make_node(NODE_NEW, "create");
        ASTNode *block = make_node(NODE_EMPTY, NULL);
        add_child(block, make_node(NODE_INT, "1"));
        add_child(newn, block);
        add_child(actor, newn);
        add_child(prog, actor);
    }
    int r = wasm_write_program(prog, "/tmp/wasm_multi.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_multi.wasm");
}

/* actor 无 NEW */
TEST(WasmCov4, ActorNoNew) {
    ASTNode *prog = make_node(NODE_PROGRAM, NULL);
    ASTNode *actor = make_node(NODE_ACTOR, "main");
    add_child(prog, actor);
    int r = wasm_write_program(prog, "/tmp/wasm_an.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_an.wasm");
}

/* NEW 无 block */
TEST(WasmCov4, NewNoBlock) {
    ASTNode *prog = make_node(NODE_PROGRAM, NULL);
    ASTNode *actor = make_node(NODE_ACTOR, "main");
    ASTNode *newn = make_node(NODE_NEW, "create");
    add_child(actor, newn);
    add_child(prog, actor);
    int r = wasm_write_program(prog, "/tmp/wasm_nb.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_nb.wasm");
}

/* null children */
TEST(WasmCov4, NullChildren) {
    ASTNode *prog = make_node(NODE_PROGRAM, NULL);
    ASTNode *actor = make_node(NODE_ACTOR, "main");
    ASTNode *newn = make_node(NODE_NEW, "create");
    ASTNode *block = make_node(NODE_EMPTY, NULL);
    add_child(block, NULL);
    add_child(newn, block);
    add_child(actor, newn);
    add_child(prog, actor);
    int r = wasm_write_program(prog, "/tmp/wasm_null.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_null.wasm");
}

/* 字符串收集: 多个字符串 */
TEST(WasmCov4, StringCollection) {
    ASTNode *call1 = make_node(NODE_CALL, "print");
    ASTNode *args1 = make_node(NODE_EMPTY, "args");
    add_child(args1, make_node(NODE_STRING, "hello"));
    add_child(call1, args1);
    ASTNode *call2 = make_node(NODE_CALL, "print");
    ASTNode *args2 = make_node(NODE_EMPTY, "args");
    add_child(args2, make_node(NODE_STRING, "world"));
    add_child(call2, args2);
    ASTNode *stmts[] = { call1, call2 };
    ASTNode *prog = make_program_with_stmts(stmts, 2);
    int r = wasm_write_program(prog, "/tmp/wasm_strcol.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
    free_node(prog);
    unlink("/tmp/wasm_strcol.wasm");
}
