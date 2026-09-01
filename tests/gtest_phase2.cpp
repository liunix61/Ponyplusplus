#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/codegen.h"
#include "gtest_helpers.h"
#include <gtest/gtest.h>
#include <string.h>
#include <assert.h>

/* Phase 2: Actor messaging syntax (! and @ operators, supervise) */

TEST(Phase2Syntax, ActorSendBang) {
    /* target ! msg  — send and forget */
    ASTNode *ast = parse_to_ast(
        "actor Sender { var target: ActorRef\n"
        "  new create() => {}\n"
        "  be send(msg: String) => { target ! msg }\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}

TEST(Phase2Syntax, ActorCallAt) {
    /* val reply = target @ query() — request-response */
    ASTNode *ast = parse_to_ast(
        "actor Sender { var target: ActorRef\n"
        "  new create() => {}\n"
        "  be call() => { val reply = target @ query() }\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}

TEST(Phase2Syntax, SuperviseOneForOne) {
    ASTNode *ast = parse_to_ast(
        "actor Worker { new create() => {} }\n"
        "actor Supervisor { new create() => {}\n"
        "  be run() => { }\n"
        "}\n"
        "supervise Worker one_for_one\n");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}

TEST(Phase2Syntax, SuperviseOneForAll) {
    ASTNode *ast = parse_to_ast(
        "actor Worker { new create() => {} }\n"
        "actor Supervisor { new create() => {} }\n"
        "supervise Worker one_for_all\n");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}

TEST(Phase2Syntax, SuperviseRestart) {
    ASTNode *ast = parse_to_ast(
        "actor Worker { new create() => {} }\n"
        "actor Supervisor { new create() => {} }\n"
        "supervise Worker restart\n");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}

TEST(Phase2Syntax, SuperviseNone) {
    ASTNode *ast = parse_to_ast(
        "actor Worker { new create() => {} }\n"
        "actor Supervisor { new create() => {} }\n"
        "supervise Worker none\n");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}

TEST(Phase2Syntax, MultiActorMailbox) {
    ASTNode *ast = parse_to_ast(
        "actor A { var msg: String = \"\"\n"
        "  new create() => {}\n"
        "  be handle(m: String) => { msg = m }\n"
        "}\n"
        "actor B { var target: ActorRef\n"
        "  new create() => {}\n"
        "  be run() => { target ! \"hello\" }\n"
        "}\n"
        "actor C { var a: ActorRef; var b: ActorRef\n"
        "  new create() => {}\n"
        "  be orchestrate() => { a ! \"x\"; b ! \"y\" }\n"
        "}\n");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}

TEST(Phase2Syntax, CodegenSupervise) {
    const char *src =
        "actor Worker { var count: U64 = 0\n"
        "  new create() => {}\n"
        "  be handle() => { count = count + 1 }\n"
        "}\n"
        "actor Supervisor { var child: ActorRef\n"
        "  new create() => {}\n"
        "  be run() => { child ! \"start\" }\n"
        "}\n"
        "supervise Worker one_for_one\n";
    ASTNode *ast = parse_to_ast(src);
    ASSERT_TRUE(ast != nullptr);
    const char *path = "/tmp/ponypp_p2.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    codegen_program(codegen_new(f), ast);
    fclose(f);
    char *out = s_file_read(path);
    ASSERT_TRUE(out != nullptr);
    s_free(out);
    remove(path);
    ast_node_free(ast);
}

TEST(Phase2Syntax, ActorRefType) {
    ASTNode *ast = parse_to_ast(
        "actor A { var ref: ActorRef\n"
        "  new create() => {}\n"
        "  be run() => { ref ! \"ping\" }\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}

TEST(Phase2Syntax, SendMultiple) {
    ASTNode *ast = parse_to_ast(
        "actor Router { var a: ActorRef; var b: ActorRef; var c: ActorRef\n"
        "  new create() => {}\n"
        "  be fanout(msg: String) => { a ! msg; b ! msg; c ! msg }\n"
        "}");
    ASSERT_TRUE(ast != nullptr);
    ast_node_free(ast);
}
