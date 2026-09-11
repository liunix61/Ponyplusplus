#include <gtest/gtest.h>
#include <ponypp/wasm.h>
#include <ponypp/ast.h>
#include <ponypp.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

static bool wasm_gen(ASTNode *ast, TargetKind target) {
    char tmpl[] = "/tmp/ponypp_wasm6_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) return false;
    close(fd);
    int r = wasm_write_program(ast, tmpl, target);
    unlink(tmpl);
    return r == 0;
}

/* NODE_IF in constructor */
TEST(WasmCov6, IfStmt) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ASTNode *if_stmt = ast_node_new(NODE_IF, 3, 1);
    ASTNode *cond = ast_node_new(NODE_INT, 3, 4);
    cond->data = strdup("1");
    ast_node_add_child(if_stmt, cond);
    ast_node_add_child(create, if_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_PRINT with DROP */
TEST(WasmCov6, PrintWithDrop) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ASTNode *print_stmt = ast_node_new(NODE_PRINT, 3, 1);
    ASTNode *val = ast_node_new(NODE_STRING, 3, 7);
    val->data = strdup("hello");
    ast_node_add_child(print_stmt, val);
    ast_node_add_child(create, print_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_WHILE */
TEST(WasmCov6, WhileStmt) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ASTNode *while_stmt = ast_node_new(NODE_WHILE, 3, 1);
    ASTNode *cond = ast_node_new(NODE_INT, 3, 7);
    cond->data = strdup("0");
    ast_node_add_child(while_stmt, cond);
    ast_node_add_child(create, while_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_RETURN */
TEST(WasmCov6, ReturnStmt) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ASTNode *ret = ast_node_new(NODE_RETURN, 3, 1);
    ASTNode *val = ast_node_new(NODE_INT, 3, 8);
    val->data = strdup("0");
    ast_node_add_child(ret, val);
    ast_node_add_child(create, ret);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_LET */
TEST(WasmCov6, LetStmt) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ASTNode *let = ast_node_new(NODE_LET, 3, 1);
    let->data = strdup("x");
    ASTNode *val = ast_node_new(NODE_INT, 3, 9);
    val->data = strdup("42");
    ast_node_add_child(let, val);
    ast_node_add_child(create, let);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_VAR */
TEST(WasmCov6, VarStmt) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ASTNode *var = ast_node_new(NODE_VAR, 3, 1);
    var->data = strdup("y");
    ASTNode *val = ast_node_new(NODE_INT, 3, 9);
    val->data = strdup("2");
    ast_node_add_child(var, val);
    ast_node_add_child(create, var);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_SEND */
TEST(WasmCov6, SendMsg) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ASTNode *send = ast_node_new(NODE_SEND, 3, 1);
    send->data = strdup("greet");
    ASTNode *target = ast_node_new(NODE_IDENT, 3, 1);
    target->data = strdup("self");
    ast_node_add_child(send, target);
    ast_node_add_child(create, send);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_MSG_CALL */
TEST(WasmCov6, MsgCall) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ASTNode *msg = ast_node_new(NODE_MSG_CALL, 3, 1);
    msg->data = strdup("compute");
    ASTNode *target = ast_node_new(NODE_IDENT, 3, 1);
    target->data = strdup("self");
    ast_node_add_child(msg, target);
    ast_node_add_child(create, msg);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_ASSERT */
TEST(WasmCov6, AssertStmt) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ASTNode *assert_stmt = ast_node_new(NODE_ASSERT, 3, 1);
    ASTNode *cond = ast_node_new(NODE_INT, 3, 8);
    cond->data = strdup("1");
    ast_node_add_child(assert_stmt, cond);
    ast_node_add_child(create, assert_stmt);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* 多个语句组合 */
TEST(WasmCov6, MultipleStatements) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    
    /* let x = 1 */
    ASTNode *let = ast_node_new(NODE_LET, 3, 1);
    let->data = strdup("x");
    ASTNode *val1 = ast_node_new(NODE_INT, 3, 9);
    val1->data = strdup("1");
    ast_node_add_child(let, val1);
    ast_node_add_child(create, let);
    
    /* var y = 2 */
    ASTNode *var = ast_node_new(NODE_VAR, 4, 1);
    var->data = strdup("y");
    ASTNode *val2 = ast_node_new(NODE_INT, 4, 9);
    val2->data = strdup("2");
    ast_node_add_child(var, val2);
    ast_node_add_child(create, var);
    
    /* print("hello") */
    ASTNode *print_stmt = ast_node_new(NODE_PRINT, 5, 1);
    ASTNode *str = ast_node_new(NODE_STRING, 5, 7);
    str->data = strdup("hello");
    ast_node_add_child(print_stmt, str);
    ast_node_add_child(create, print_stmt);
    
    /* if (1) */
    ASTNode *if_stmt = ast_node_new(NODE_IF, 6, 1);
    ASTNode *cond = ast_node_new(NODE_INT, 6, 4);
    cond->data = strdup("1");
    ast_node_add_child(if_stmt, cond);
    ast_node_add_child(create, if_stmt);
    
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* 不同 target */
TEST(WasmCov6, TargetWasiP3) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P3));
}

TEST(WasmCov6, TargetComponent) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_COMPONENT));
}

TEST(WasmCov6, TargetBrowser) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_BROWSER));
}

TEST(WasmCov6, TargetMcuWasm) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_MCU_WASM));
}

/* 多个 actor */
TEST(WasmCov6, MultipleActors) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    
    ASTNode *actor1 = ast_node_new(NODE_ACTOR, 1, 1);
    actor1->data = strdup("main");
    ASTNode *create1 = ast_node_new(NODE_NEW, 2, 1);
    create1->data = strdup("create");
    ast_node_add_child(actor1, create1);
    ast_node_add_child(prog, actor1);
    
    ASTNode *actor2 = ast_node_new(NODE_ACTOR, 5, 1);
    actor2->data = strdup("worker");
    ASTNode *create2 = ast_node_new(NODE_NEW, 6, 1);
    create2->data = strdup("create");
    ast_node_add_child(actor2, create2);
    ast_node_add_child(prog, actor2);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_BE 行为 */
TEST(WasmCov6, BehaviorDef) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ast_node_add_child(actor, create);
    
    ASTNode *be = ast_node_new(NODE_BE, 5, 1);
    be->data = strdup("greet");
    ast_node_add_child(actor, be);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_FUN 函数 */
TEST(WasmCov6, FunctionDef) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ast_node_add_child(actor, create);
    
    ASTNode *fun = ast_node_new(NODE_FUN, 5, 1);
    fun->data = strdup("add");
    ast_node_add_child(actor, fun);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* 空程序 */
TEST(WasmCov6, EmptyProgram) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* null ast */
TEST(WasmCov6, NullAst) {
    int r = wasm_write_program(nullptr, "/tmp/test.wasm", TARGET_WASI_P2);
    EXPECT_EQ(r, 0);
}

/* null output */
TEST(WasmCov6, NullOutput) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    int r = wasm_write_program(prog, nullptr, TARGET_WASI_P2);
    EXPECT_NE(r, 0);
}

/* NODE_IMPORT */
TEST(WasmCov6, ImportStmt) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *import = ast_node_new(NODE_IMPORT, 1, 1);
    import->data = strdup("std.io");
    ast_node_add_child(prog, import);
    
    ASTNode *actor = ast_node_new(NODE_ACTOR, 2, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 3, 1);
    create->data = strdup("create");
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_LAMBDA */
TEST(WasmCov6, LambdaExpr) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ASTNode *let = ast_node_new(NODE_LET, 3, 1);
    let->data = strdup("f");
    ASTNode *lambda = ast_node_new(NODE_LAMBDA, 3, 9);
    ast_node_add_child(let, lambda);
    ast_node_add_child(create, let);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_LIST */
TEST(WasmCov6, ListLiteral) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ASTNode *let = ast_node_new(NODE_LET, 3, 1);
    let->data = strdup("list");
    ASTNode *list = ast_node_new(NODE_LIST, 3, 9);
    ASTNode *item1 = ast_node_new(NODE_INT, 3, 10);
    item1->data = strdup("1");
    ASTNode *item2 = ast_node_new(NODE_INT, 3, 13);
    item2->data = strdup("2");
    ast_node_add_child(list, item1);
    ast_node_add_child(list, item2);
    ast_node_add_child(let, list);
    ast_node_add_child(create, let);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_MAP */
TEST(WasmCov6, MapLiteral) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ASTNode *let = ast_node_new(NODE_LET, 3, 1);
    let->data = strdup("map");
    ASTNode *map = ast_node_new(NODE_MAP, 3, 9);
    ast_node_add_child(let, map);
    ast_node_add_child(create, let);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}

/* NODE_INDEX_ACCESS */
TEST(WasmCov6, IndexAccess) {
    ASTNode *prog = ast_node_new(NODE_PROGRAM, 1, 1);
    ASTNode *actor = ast_node_new(NODE_ACTOR, 1, 1);
    actor->data = strdup("main");
    ASTNode *create = ast_node_new(NODE_NEW, 2, 1);
    create->data = strdup("create");
    ASTNode *let = ast_node_new(NODE_LET, 3, 1);
    let->data = strdup("item");
    ASTNode *idx = ast_node_new(NODE_INDEX_ACCESS, 3, 9);
    ASTNode *arr = ast_node_new(NODE_IDENT, 3, 9);
    arr->data = strdup("list");
    ASTNode *index = ast_node_new(NODE_INT, 3, 14);
    index->data = strdup("0");
    ast_node_add_child(idx, arr);
    ast_node_add_child(idx, index);
    ast_node_add_child(let, idx);
    ast_node_add_child(create, let);
    ast_node_add_child(actor, create);
    ast_node_add_child(prog, actor);
    
    EXPECT_TRUE(wasm_gen(prog, TARGET_WASI_P2));
}
