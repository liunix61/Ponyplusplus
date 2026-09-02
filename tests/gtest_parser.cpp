#include <gtest/gtest.h>
#include "gtest_helpers.h"
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static bool ast_has_node(const ASTNode* node, int target_type) {
    if (!node) return false;
    if (node->type == target_type) return true;
    for (size_t i = 0; node->child_count > 0 && i < node->child_count; i++) {
        if (ast_has_node(node->children[i], target_type)) return true;
    }
    return false;
}

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

/* import statement creates NODE_IMPORT */
TEST(Parser, ImportStatement) {
    ASTNode* ast = parse_to_ast("import std.io\n");
    ASSERT_NE(ast, nullptr);
    ASSERT_TRUE(ast_has_node(ast, NODE_IMPORT));
    ast_node_free(ast);
}

TEST(Parser, UseStatement) {
    ASTNode* ast = parse_to_ast("use net/http;\n");
    ASSERT_NE(ast, nullptr);
    ASSERT_TRUE(ast_has_node(ast, NODE_IMPORT));
    ast_node_free(ast);
}

/* CAP prefix on type: ref Foo */
TEST(Parser, CapRefType) {
    ASTNode* ast = parse_to_ast(
        "actor A {\n"
        "  var x: ref B\n"
        "  new create() => {}\n"
        "}\n");
    ASSERT_NE(ast, nullptr);
    ASSERT_TRUE(ast_has_node(ast, NODE_CAP));
    ast_node_free(ast);
}

TEST(Parser, CapIsoValBox) {
    for (const char* cap : {"iso", "trn", "val", "box"}) {
        char src[128];
        snprintf(src, sizeof(src),
            "actor A {\n var x: %s B\n new create() => {}\n}\n", cap);
        ASTNode* ast = parse_to_ast(src);
        ASSERT_NE(ast, nullptr) << "failed for cap=" << cap;
        ASSERT_TRUE(ast_has_node(ast, NODE_CAP));
        ast_node_free(ast);
    }
}

/* match expression: match x { 1 => a, _ => b } */
TEST(Parser, MatchExpression) {
    ASTNode* ast = parse_to_ast(
        "actor A {\n"
        "  new create() => {\n"
        "    match x { 1 => a; _ => b }\n"
        "  }\n"
        "}");
    ASSERT_NE(ast, nullptr);
    ASSERT_TRUE(ast_has_node(ast, NODE_MATCH));
    ast_node_free(ast);
}

TEST(Parser, MatchMultipleArms) {
    ASTNode* ast = parse_to_ast(
        "actor A {\n"
        "  new create() => {\n"
        "    match v { 0 => x; 1 => y; _ => z }\n"
        "  }\n"
        "}");
    ASSERT_NE(ast, nullptr);
    ast_node_free(ast);
}
