#include <gtest/gtest.h>
#include "gtest_helpers.h"
#include "ponypp/capabilities.h"

TEST(Capabilities, EmptyProgram) {
    CapCheckResult* r = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    ASSERT_NE(r, nullptr);
    EXPECT_NE(capabilities_check_program(nullptr, r), 0);
    EXPECT_EQ(r->ok, 0);
    cap_check_free_result(r);
}

TEST(Capabilities, ValidActor) {
    const char* src =
        "actor main {\n"
        "  var count: U64 = 0\n"
        "  be run() => {}\n"
        "}";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    CapCheckResult* r = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    EXPECT_EQ(capabilities_check_program(ast, r), 0);
    cap_check_free_result(r);
    ast_node_free(ast);
}

TEST(Capabilities, MultipleActors) {
    const char* src =
        "actor A { var x: U64 = 0 }\n"
        "actor B { var y: I64 = 42 }";
    ASTNode* ast = parse_to_ast(src);
    ASSERT_NE(ast, nullptr);
    CapCheckResult* r = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    EXPECT_EQ(capabilities_check_program(ast, r), 0);
    cap_check_free_result(r);
    ast_node_free(ast);
}

TEST(Capabilities, FreeResult) {
    CapCheckResult* r = (CapCheckResult*)calloc(1, sizeof(CapCheckResult));
    r->errors = (const char**)calloc(1, sizeof(char*));
    cap_check_free_result(r);
    EXPECT_TRUE(1);
}
