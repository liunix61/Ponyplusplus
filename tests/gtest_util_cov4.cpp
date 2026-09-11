#include <gtest/gtest.h>
#include <ponypp.h>
#include <ponypp/util.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

static ASTNode *parse_code(const char *code) {
    Lexer *lx = lexer_new("test.pny", code, strlen(code));
    if (!lx) return nullptr;
    Token *tokens = nullptr;
    size_t count = 0;
    lexer_lex_all(lx, &tokens, &count);
    lexer_free(lx);
    Parser *ps = parser_new("test.pny", tokens, count);
    if (!ps) return nullptr;
    ASTNode *ast = parser_parse_program(ps);
    parser_free(ps);
    return ast;
}

/* ==================== token_print 更多情况 ==================== */

TEST(UtilCov4, TokenPrintValid) {
    Lexer *lx = lexer_new("test.pny", "fn main() { }", 13);
    if (!lx) return;
    Token *tokens = nullptr;
    size_t count = 0;
    lexer_lex_all(lx, &tokens, &count);
    lexer_free(lx);
    if (tokens && count > 0) {
        token_print(&tokens[0], stdout);
    }
    free(tokens);
}

/* ==================== ast_node_print 更多程序 ==================== */

TEST(UtilCov4, AstNodePrintEmptyProgram) {
    ASTNode *ast = parse_code("");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintEmptyFunc) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintFuncWithLet) {
    ASTNode *ast = parse_code("fn main() { let x: i32 = 1; }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintFuncWithReturn) {
    ASTNode *ast = parse_code("fn main() -> i32 { return 42; }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintFuncWithPrint) {
    ASTNode *ast = parse_code("fn main() { print(42); }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintFuncWithIf) {
    ASTNode *ast = parse_code("fn main() { if true { print(1); } }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintFuncWithWhile) {
    ASTNode *ast = parse_code("fn main() { while false { print(1); } }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintFuncWithParams) {
    ASTNode *ast = parse_code("fn add(a: i32, b: i32) -> i32 { return a + b; }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintMultipleFuncs) {
    ASTNode *ast = parse_code("fn main() { } fn helper() -> i32 { return 1; }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

/* ==================== ast_node_print_dot 更多程序 ==================== */

TEST(UtilCov4, AstNodePrintDotEmptyProgram) {
    ASTNode *ast = parse_code("");
    if (ast) {
        ast_node_print_dot(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintDotEmptyFunc) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        ast_node_print_dot(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintDotFuncWithLet) {
    ASTNode *ast = parse_code("fn main() { let x: i32 = 1; }");
    if (ast) {
        ast_node_print_dot(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintDotFuncWithReturn) {
    ASTNode *ast = parse_code("fn main() -> i32 { return 42; }");
    if (ast) {
        ast_node_print_dot(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintDotFuncWithIf) {
    ASTNode *ast = parse_code("fn main() { if true { print(1); } }");
    if (ast) {
        ast_node_print_dot(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintDotFuncWithWhile) {
    ASTNode *ast = parse_code("fn main() { while false { print(1); } }");
    if (ast) {
        ast_node_print_dot(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov4, AstNodePrintDotMultipleFuncs) {
    ASTNode *ast = parse_code("fn main() { } fn helper() -> i32 { return 1; }");
    if (ast) {
        ast_node_print_dot(ast, stdout);
        ast_node_free(ast);
    }
}

/* ==================== token_type_name 更多类型 ==================== */

TEST(UtilCov4, TokenTypeNamesMore) {
    EXPECT_NE(token_type_name(TK_EOF), nullptr);
    EXPECT_NE(token_type_name(TK_IDENT), nullptr);
    EXPECT_NE(token_type_name(TK_INT), nullptr);
}

/* ==================== capability_kind_name 更多能力 ==================== */

TEST(UtilCov4, CapabilityKindNamesMore) {
    EXPECT_NE(capability_kind_name(CAP_ISO), nullptr);
    EXPECT_NE(capability_kind_name(CAP_TRN), nullptr);
    EXPECT_NE(capability_kind_name(CAP_REF), nullptr);
    EXPECT_NE(capability_kind_name(CAP_VAL), nullptr);
    EXPECT_NE(capability_kind_name(CAP_BOX), nullptr);
    EXPECT_NE(capability_kind_name(CAP_TAG), nullptr);
}

/* ==================== type_kind_name 更多类型 ==================== */

TEST(UtilCov4, TypeKindNamesMore) {
    EXPECT_NE(type_kind_name(TYPE_UNKNOWN), nullptr);
    EXPECT_NE(type_kind_name(TYPE_INT64), nullptr);
    EXPECT_NE(type_kind_name(TYPE_INT32), nullptr);
    EXPECT_NE(type_kind_name(TYPE_UINT64), nullptr);
    EXPECT_NE(type_kind_name(TYPE_UINT32), nullptr);
    EXPECT_NE(type_kind_name(TYPE_UINT8), nullptr);
    EXPECT_NE(type_kind_name(TYPE_FLOAT64), nullptr);
    EXPECT_NE(type_kind_name(TYPE_FLOAT32), nullptr);
    EXPECT_NE(type_kind_name(TYPE_STRING), nullptr);
    EXPECT_NE(type_kind_name(TYPE_BOOL), nullptr);
    EXPECT_NE(type_kind_name(TYPE_NONE), nullptr);
    EXPECT_NE(type_kind_name(TYPE_ANY), nullptr);
}

/* ==================== s_resolve_output 更多情况 ==================== */

TEST(UtilCov4, ResolveOutputWithExt) {
    char *result = s_resolve_output("test", ".wasm");
    EXPECT_NE(result, nullptr);
    s_free(result);
}

TEST(UtilCov4, ResolveOutputNullExt) {
    char *result = s_resolve_output("test", nullptr);
    EXPECT_NE(result, nullptr);
    s_free(result);
}

/* ==================== s_file_read/write 更多情况 ==================== */

TEST(UtilCov4, FileWriteReadEmpty) {
    const char *path = "/tmp/test_util_cov4_empty.txt";
    int ret = s_file_write(path, "", 0);
    EXPECT_EQ(ret, 0);
    char *read = s_file_read(path);
    EXPECT_NE(read, nullptr);
    s_free(read);
    unlink(path);
}

TEST(UtilCov4, FileWriteReadLarge) {
    const char *path = "/tmp/test_util_cov4_large.txt";
    char data[1024];
    memset(data, 'A', sizeof(data));
    int ret = s_file_write(path, data, sizeof(data));
    EXPECT_EQ(ret, 0);
    char *read = s_file_read(path);
    EXPECT_NE(read, nullptr);
    s_free(read);
    unlink(path);
}
