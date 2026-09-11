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

/* ==================== token_print ==================== */



TEST(UtilCov3, TokenPrintValid) {
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

/* ==================== ast_node_print ==================== */

TEST(UtilCov3, AstNodePrintNull) {
    ast_node_print(nullptr, stdout);  /* 不崩溃 */
}

TEST(UtilCov3, AstNodePrintEmptyFunc) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov3, AstNodePrintFuncWithLet) {
    ASTNode *ast = parse_code("fn main() { let x: i32 = 1; }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov3, AstNodePrintFuncWithReturn) {
    ASTNode *ast = parse_code("fn main() -> i32 { return 42; }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov3, AstNodePrintFuncWithPrint) {
    ASTNode *ast = parse_code("fn main() { print(42); }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov3, AstNodePrintFuncWithIf) {
    ASTNode *ast = parse_code("fn main() { if true { print(1); } }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov3, AstNodePrintFuncWithWhile) {
    ASTNode *ast = parse_code("fn main() { while false { print(1); } }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov3, AstNodePrintMultipleFuncs) {
    ASTNode *ast = parse_code("fn main() { } fn helper() -> i32 { return 1; }");
    if (ast) {
        ast_node_print(ast, stdout);
        ast_node_free(ast);
    }
}

/* ==================== ast_node_print_dot ==================== */

TEST(UtilCov3, AstNodePrintDotNull) {
    ast_node_print_dot(nullptr, stdout);  /* 不崩溃 */
}

TEST(UtilCov3, AstNodePrintDotEmptyFunc) {
    ASTNode *ast = parse_code("fn main() { }");
    if (ast) {
        ast_node_print_dot(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov3, AstNodePrintDotFuncWithLet) {
    ASTNode *ast = parse_code("fn main() { let x: i32 = 1; }");
    if (ast) {
        ast_node_print_dot(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov3, AstNodePrintDotFuncWithReturn) {
    ASTNode *ast = parse_code("fn main() -> i32 { return 42; }");
    if (ast) {
        ast_node_print_dot(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov3, AstNodePrintDotFuncWithIf) {
    ASTNode *ast = parse_code("fn main() { if true { print(1); } }");
    if (ast) {
        ast_node_print_dot(ast, stdout);
        ast_node_free(ast);
    }
}

TEST(UtilCov3, AstNodePrintDotFuncWithWhile) {
    ASTNode *ast = parse_code("fn main() { while false { print(1); } }");
    if (ast) {
        ast_node_print_dot(ast, stdout);
        ast_node_free(ast);
    }
}

/* ==================== token_type_name 更多类型 ==================== */

TEST(UtilCov3, TokenTypeNamesMore) {
    EXPECT_NE(token_type_name(TK_EOF), nullptr);
    EXPECT_NE(token_type_name(TK_IDENT), nullptr);
    EXPECT_NE(token_type_name(TK_INT), nullptr);
}

/* ==================== capability_kind_name 更多能力 ==================== */

TEST(UtilCov3, CapabilityKindNamesMore) {
    EXPECT_NE(capability_kind_name(CAP_ISO), nullptr);
    EXPECT_NE(capability_kind_name(CAP_TRN), nullptr);
    EXPECT_NE(capability_kind_name(CAP_REF), nullptr);
    EXPECT_NE(capability_kind_name(CAP_VAL), nullptr);
    EXPECT_NE(capability_kind_name(CAP_BOX), nullptr);
    EXPECT_NE(capability_kind_name(CAP_TAG), nullptr);
}

/* ==================== type_kind_name 更多类型 ==================== */

TEST(UtilCov3, TypeKindNamesMore) {
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

TEST(UtilCov3, ResolveOutputWithExt) {
    char *result = s_resolve_output("test", ".wasm");
    EXPECT_NE(result, nullptr);
    s_free(result);
}

TEST(UtilCov3, ResolveOutputNullExt) {
    char *result = s_resolve_output("test", nullptr);
    EXPECT_NE(result, nullptr);
    s_free(result);
}

/* ==================== s_file_read/write 更多情况 ==================== */

TEST(UtilCov3, FileWriteReadEmpty) {
    const char *path = "/tmp/test_util_cov3_empty.txt";
    int ret = s_file_write(path, "", 0);
    EXPECT_EQ(ret, 0);
    char *read = s_file_read(path);
    EXPECT_NE(read, nullptr);
    s_free(read);
    unlink(path);
}

TEST(UtilCov3, FileWriteReadLarge) {
    const char *path = "/tmp/test_util_cov3_large.txt";
    char data[1024];
    memset(data, 'A', sizeof(data));
    int ret = s_file_write(path, data, sizeof(data));
    EXPECT_EQ(ret, 0);
    char *read = s_file_read(path);
    EXPECT_NE(read, nullptr);
    s_free(read);
    unlink(path);
}
