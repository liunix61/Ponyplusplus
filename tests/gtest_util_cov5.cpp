#include <gtest/gtest.h>
#include <ponypp.h>
#include <ponypp/util.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

static ASTNode *parse_code(const char *src) {
    Lexer *lx = lexer_new("test", src, strlen(src));
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("test", toks, tc);
    return parser_parse_program(p);
}

static void test_print_node(ASTNodeType type) {
    ASTNode *n = ast_node_new(type, 0, 0);
    ast_node_print(n, stdout);
    ast_node_free(n);
}

TEST(UtilCov5, PrintActorNode) { test_print_node(NODE_ACTOR); }
TEST(UtilCov5, PrintClassNode) { test_print_node(NODE_CLASS); }
TEST(UtilCov5, PrintTraitNode) { test_print_node(NODE_TRAIT); }
TEST(UtilCov5, PrintInterfaceNode) { test_print_node(NODE_INTERFACE); }
TEST(UtilCov5, PrintBeNode) { test_print_node(NODE_BE); }
TEST(UtilCov5, PrintFunNode) { test_print_node(NODE_FUN); }
TEST(UtilCov5, PrintNewNode) { test_print_node(NODE_NEW); }
TEST(UtilCov5, PrintVarNode) { test_print_node(NODE_VAR); }
TEST(UtilCov5, PrintLetNode) { test_print_node(NODE_LET); }
TEST(UtilCov5, PrintIfNode) { test_print_node(NODE_IF); }
TEST(UtilCov5, PrintWhileNode) { test_print_node(NODE_WHILE); }
TEST(UtilCov5, PrintForNode) { test_print_node(NODE_FOR); }
TEST(UtilCov5, PrintReturnNode) { test_print_node(NODE_RETURN); }
TEST(UtilCov5, PrintMatchNode) { test_print_node(NODE_MATCH); }
TEST(UtilCov5, PrintMatchArmNode) { test_print_node(NODE_MATCH_ARM); }
TEST(UtilCov5, PrintSendNode) { test_print_node(NODE_SEND); }
TEST(UtilCov5, PrintMsgCallNode) { test_print_node(NODE_MSG_CALL); }
TEST(UtilCov5, PrintLambdaNode) { test_print_node(NODE_LAMBDA); }
TEST(UtilCov5, PrintListNode) { test_print_node(NODE_LIST); }
TEST(UtilCov5, PrintMapNode) { test_print_node(NODE_MAP); }
TEST(UtilCov5, PrintCharNode) { test_print_node(NODE_CHAR); }
TEST(UtilCov5, PrintUnionNode) { test_print_node(NODE_UNION); }
TEST(UtilCov5, PrintTupleNode) { test_print_node(NODE_TUPLE); }
TEST(UtilCov5, PrintArrayNode) { test_print_node(NODE_ARRAY); }
TEST(UtilCov5, PrintAliasNode) { test_print_node(NODE_ALIAS); }
TEST(UtilCov5, PrintIndexAccessNode) { test_print_node(NODE_INDEX_ACCESS); }
TEST(UtilCov5, PrintTypeParamNode) { test_print_node(NODE_TYPE_PARAM); }
TEST(UtilCov5, PrintTypeAliasNode) { test_print_node(NODE_TYPE_ALIAS); }
TEST(UtilCov5, PrintWitWorldNode) { test_print_node(NODE_WIT_WORLD); }
TEST(UtilCov5, PrintCapNode) { test_print_node(NODE_CAP); }
TEST(UtilCov5, PrintImportNode) { test_print_node(NODE_IMPORT); }
TEST(UtilCov5, PrintAssertNode) { test_print_node(NODE_ASSERT); }
TEST(UtilCov5, PrintSuperviseNode) { test_print_node(NODE_SUPERVISE); }
TEST(UtilCov5, PrintSupertreeNode) { test_print_node(NODE_SUPERTREE); }
TEST(UtilCov5, PrintYieldNode) { test_print_node(NODE_YIELD); }

/* type_name TYPE_ARRAY */
TEST(UtilCov5, TypeNameArray) {
    const char *name = type_kind_name(TYPE_ARRAY);
    EXPECT_STREQ(name, "Array");
}

/* ast_node_print_dot */
TEST(UtilCov5, PrintDotActor) {
    ASTNode *n = ast_node_new(NODE_ACTOR, 0, 0);
    ast_node_print_dot(n, stdout);
    ast_node_free(n);
    SUCCEED();
}

TEST(UtilCov5, PrintDotComplex) {
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    ast_node_print_dot(ast, stdout);
    SUCCEED();
}

/* s_file_read 不存在的文件 */
TEST(UtilCov5, FileReadNonexistent) {
    char *content = s_file_read("/nonexistent/file.pny");
    EXPECT_EQ(content, nullptr);
}

/* s_file_read 空文件 */
TEST(UtilCov5, FileReadEmpty) {
    char tmpl[] = "/tmp/ponypp_empty_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    char *content = s_file_read(tmpl);
    if (content) s_free(content);
    
    unlink(tmpl);
    SUCCEED();
}

/* s_file_read 有内容的文件 */
TEST(UtilCov5, FileReadWithContent) {
    char tmpl[] = "/tmp/ponypp_content_XXXXXX";
    int fd = mkstemp(tmpl);
    ASSERT_GE(fd, 0);
    close(fd);
    
    FILE *f = fopen(tmpl, "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "hello world\n");
    fclose(f);
    
    char *content = s_file_read(tmpl);
    ASSERT_NE(content, nullptr);
    EXPECT_STREQ(content, "hello world\n");
    s_free(content);
    
    unlink(tmpl);
}
