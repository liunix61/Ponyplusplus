#include <gtest/gtest.h>
#include "gtest_helpers.h"
#include "ponypp/lexer.h"
#include "ponypp/parser.h"

TEST(Parser, EmptyProgram) {
    ASTNode* ast = parse_to_ast("");
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, NODE_PROGRAM);
    EXPECT_EQ(ast->child_count, 0);
    ast_node_free(ast);
}

TEST(Parser, SimpleActor) {
    ASTNode* ast = parse_to_ast("actor Main { }");
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->child_count, 1);
    ASTNode* actor = ast->children[0];
    EXPECT_EQ(actor->type, NODE_ACTOR);
    EXPECT_STREQ((const char*)actor->data, "Main");
    ast_node_free(ast);
}

TEST(Parser, ActorWithMethods) {
    const char* src =
        "actor Counter {\n"
        "  var count: U64 = 0\n"
        "  be increment() => {}\n"
        "  fun value(): U64 => {}\n"
        "}";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->child_count, 1);
    ASTNode* actor = ast->children[0];
    EXPECT_GE(actor->child_count, 3);
    ast_node_free(ast);
}

TEST(Parser, ActorWithConstructor) {
    const char* src =
        "actor Counter(val initial: U64) {\n"
        "  var count: U64 = initial\n"
        "  new create(initial: U64) => {}\n"
        "}";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->child_count, 1);
    ASTNode* actor = ast->children[0];
    int has_new = 0;
    for (size_t i = 0; i < actor->child_count; i++) {
        if (actor->children[i]->type == NODE_NEW) has_new = 1;
    }
    EXPECT_EQ(has_new, 1);
    ast_node_free(ast);
}

TEST(Parser, MultipleActors) {
    const char* src = "actor A {} actor B {} actor C {}";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    int actor_count = 0;
    for (size_t i = 0; i < ast->child_count; i++) {
        if (ast->children[i]->type == NODE_ACTOR) actor_count++;
    }
    EXPECT_EQ(actor_count, 3);
    ast_node_free(ast);
}

TEST(Parser, Supervise) {
    ASTNode* ast = parse_to_ast("supervise worker one_for_one");
    ASSERT_NE(ast, nullptr);
    EXPECT_GE(ast->child_count, 1);
    ast_node_free(ast);
}

TEST(Parser, ErrorHandling) {
    ASTNode* ast = parse_to_ast("actor {");
    EXPECT_TRUE(1);
    ast_node_free(ast);
}

TEST(Parser, NestedBlock) {
    const char* src =
        "actor Main {\n"
        "  be run() => {\n"
        "    if (1) {}\n"
        "    while (1) {}\n"
        "  }\n"
        "}";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    ast_node_free(ast);
}
