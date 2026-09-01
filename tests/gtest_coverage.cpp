#include "ponypp.h"
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/codegen.h"
#include "ponypp/ast.h"
#include "gtest_helpers.h"
#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---- ast.c: ast_method_new / ast_field_new / ast_int_new / ast_call_new / ast_actor_free ---- */
TEST(AstExtra, MethodNew) {
    ASTNode *m = ast_method_new("run", true, 1, 1);   /* is_be */
    ASSERT_TRUE(m != nullptr);
    ASSERT_EQ(m->type, NODE_BE);
    ast_node_free(m);
    ASTNode *f = ast_method_new("getX", false, 2, 1);
    ASSERT_TRUE(f != nullptr);
    ASSERT_EQ(f->type, NODE_FUN);
    ast_node_free(f);
}

TEST(AstExtra, FieldNew) {
    ASTNode *v = ast_field_new("count", true, 1, 1);
    ASSERT_EQ(v->type, NODE_VAR);
    ast_node_free(v);
    ASTNode *l = ast_field_new("const", false, 1, 1);
    ASSERT_EQ(l->type, NODE_LET);
    ast_node_free(l);
}

TEST(AstExtra, IntNew) {
    ASTNode *n = ast_int_new(42, 1, 1);
    ASSERT_TRUE(n != nullptr);
    uint64_t *p = (uint64_t*)n->data;
    ASSERT_EQ(*p, 42u);
    ast_node_free(n);
}

TEST(AstExtra, CallNew) {
    ASTNode *callee = ast_node_new(NODE_IDENT, 1, 1);
    callee->data = s_strdup("actor");
    ASTNode *call = ast_call_new(callee, "send", true, 1, 1);
    ASSERT_TRUE(call != nullptr);
    ASSERT_EQ(call->type, NODE_CALL);
    ASSERT_STREQ((const char*)call->data, "send");
    ast_node_free(call);
}

TEST(AstExtra, ActorFree) {
    Actor *a = (Actor*)s_malloc(sizeof(Actor));
    a->name = s_strdup("Main");
    a->fields = (ASTNode**)s_malloc(sizeof(ASTNode*) * 2);
    a->field_count = 2;
    a->methods = (ASTNode**)s_malloc(sizeof(ASTNode*) * 1);
    a->method_count = 1;
    a->constructors = nullptr;
    ast_actor_free(a);
    s_free(a);
}

TEST(AstExtra, NodeRelease) {
    ASTNode *n = ast_node_new(NODE_IDENT, 1, 1);
    n->data = s_strdup("x");
    n->ref_count = 2;
    ast_node_release(n);
    ASSERT_EQ(n->ref_count, 1);
    ast_node_release(n);
    /* ref_count 0 triggers ast_node_free internally, n is freed */
}

TEST(AstExtra, TokenPrint) {
    Token t;
    t.type = TK_IDENT;
    t.value = (char*)"hello";
    t.line = 1;
    t.column = 5;
    FILE *f = fopen("/tmp/ponypp_tok.txt", "w");
    ASSERT_TRUE(f != nullptr);
    token_print(&t, f);
    fclose(f);
    char *buf = s_file_read("/tmp/ponypp_tok.txt");
    ASSERT_TRUE(buf != nullptr);
    ASSERT_NE(strstr(buf, "IDENT"), nullptr);
    s_free(buf);
    remove("/tmp/ponypp_tok.txt");
}

TEST(AstExtra, SStrcpy) {
    char dst[16];
    char *r = s_strcpy(dst, "hello");
    ASSERT_STREQ(dst, "hello");
    ASSERT_EQ(r, dst);
}

/* ---- codegen.c: cover more expr branches ---- */
TEST(CodegenExtra4, AllExprTypes) {
    const char *src =
        "actor A {\n"
        "  var count: U64 = 42\n"
        "  var rate: F64 = 3.14\n"
        "  var active: Bool = true\n"
        "  var label: String = \"hello\"\n"
        "  new create() => {}\n"
        "  be run() => {\n"
        "    print(\"msg\")\n"
        "    print(100)\n"
        "    print(2.71)\n"
        "    print(true)\n"
        "    count = 5\n"
        "    count = count + 1\n"
        "    { count = 99 }\n"
        "    if count == 0 => { print(\"zero\") } else => { print(\"nz\") }\n"
        "    while count < 10 => { count = count + 1 }\n"
        "  }\n"
        "  fun get() => { return count }\n"
        "  fun r() => { return }\n"
        "}";
    ASTNode *ast = parse_to_ast(src);
    ASSERT_TRUE(ast != nullptr);
    const char *path = "/tmp/ponypp_cge4.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    remove(path);
    ast_node_free(ast);
}

TEST(CodegenExtra4, StringEscapes) {
    const char *src =
        "actor A { new create() => {}\n"
        "  be run() => {\n"
        "    print(\"a\\\\b\")\n"
        "    print(\"line1\\nline2\")\n"
        "    print(\"tab\\there\")\n"
        "  }\n"
        "}";
    ASTNode *ast = parse_to_ast(src);
    const char *path = "/tmp/ponypp_cge5.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    remove(path);
    ast_node_free(ast);
}

TEST(CodegenExtra4, ConstructorWithParams) {
    const char *src =
        "actor A(val initial: U64 = 0, val name: String = \"default\") {\n"
        "  var v: U64 = initial\n"
        "  var n: String = name\n"
        "  new create(initial: U64, name: String) => {}\n"
        "  be run() => { print(v) }\n"
        "}";
    ASTNode *ast = parse_to_ast(src);
    const char *path = "/tmp/ponypp_cge6.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    remove(path);
    ast_node_free(ast);
}

TEST(CodegenExtra4, MultiActorSupervise) {
    const char *src =
        "actor A { var x: U64 = 0; new create() => {} }\n"
        "actor B { var y: U64 = 1; new create() => {} }\n"
        "actor C { var z: U64 = 2; new create() => {} }\n"
        "supervise A one_for_one\n"
        "supervise B restart\n"
        "supervise C none\n";
    ASTNode *ast = parse_to_ast(src);
    const char *path = "/tmp/ponypp_cge7.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    remove(path);
    ast_node_free(ast);
}

TEST(CodegenExtra4, ComplexTypes) {
    const char *src =
        "actor A {\n"
        "  var a: I32 = -1\n"
        "  var b: U8 = 0xFF\n"
        "  var c: F32 = 2.5\n"
        "  var d: I16 = 100\n"
        "  new create() => {}\n"
        "}";
    ASTNode *ast = parse_to_ast(src);
    const char *path = "/tmp/ponypp_cge8.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    remove(path);
    ast_node_free(ast);
}
