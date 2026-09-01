#include "ponypp/util.h"
#include "ponypp/ast.h"
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

TEST(Util, SMalloc) {
    char *p = s_malloc(100);
    ASSERT_TRUE(p != nullptr);
    s_free(p);
}

TEST(Util, SStrdup) {
    char *d = s_strdup("hello pony++");
    ASSERT_STREQ(d, "hello pony++");
    s_free(d);
    ASSERT_EQ(s_strdup(NULL), (char*)NULL);
}

TEST(Util, SStrndup) {
    char *d = s_strndup("hello world", 5);
    ASSERT_STREQ(d, "hello");
    s_free(d);
}

TEST(Util, SStrLen) {
    ASSERT_EQ(s_strlen("hi"), 2u);
    ASSERT_EQ(s_strlen(NULL), 0u);
}

TEST(Util, SStrCmp) {
    ASSERT_EQ(s_strcmp("a", "a"), 0);
    ASSERT_EQ(s_strcmp("a", "b") < 0, true);
    ASSERT_EQ(s_strcmp("b", "a") > 0, true);
    ASSERT_EQ(s_strcmp(NULL, nullptr), 0);
    ASSERT_EQ(s_strcmp(NULL, "a"), -1);
    ASSERT_EQ(s_strcmp("a", nullptr), 1);
}

TEST(Util, SStrlcpy) {
    char dst[4] = {0};
    size_t ret = s_strlcpy(dst, "hello", sizeof(dst));
    ASSERT_STREQ(dst, "hel");
    ASSERT_EQ(ret, 5u);
}

TEST(Util, SStrCat) {
    char buf[16];
    strcpy(buf, "hello");
    char *r = s_strcat(buf, " world");
    ASSERT_STREQ(buf, "hello world");
    ASSERT_EQ(r, buf);
}

TEST(Util, SFileWriteAndRead) {
    const char *path = "/tmp/ponypp_util_test.txt";
    const char *data = "hello pony++ coverage";
    int r = s_file_write(path, data, strlen(data));
    ASSERT_EQ(r, 0);

    char *buf = s_file_read(path);
    ASSERT_TRUE(buf != nullptr);
    ASSERT_STREQ(buf, data);
    s_free(buf);

    remove(path);
}

TEST(Util, SFileReadNotExist) {
    ASSERT_EQ(s_file_read("/tmp/nonexistent_ponypp_util.txt"), (char*)NULL);
}

TEST(Util, SResolveOutput) {
    char *r = s_resolve_output("/tmp/out.wasm", nullptr);
    ASSERT_STREQ(r, "/tmp/out.wasm");
    s_free(r);
    char *r2 = s_resolve_output(NULL, "wasm");
    ASSERT_TRUE(r2 != nullptr);
    ASSERT_NE((void*)strstr(r2, "ponypp_output"), nullptr);
    s_free(r2);
}

TEST(Util, SMemset) {
    char buf[10];
    s_memset(buf, 'x', sizeof(buf));
    for (int i = 0; i < 10; i++) ASSERT_EQ(buf[i], 'x');
}

TEST(Util, TokenTypeName) {
    ASSERT_STREQ(token_type_name(TK_EOF), "EOF");
    ASSERT_STREQ(token_type_name(TK_KEYWORD), "KEYWORD");
    ASSERT_STREQ(token_type_name(TK_IDENT), "IDENT");
    ASSERT_STREQ(token_type_name(TK_INT), "INT");
    ASSERT_STREQ(token_type_name(TK_PAREN_L), "(");
    ASSERT_STREQ(token_type_name(TK_ARROW_ARR), "=>");
    ASSERT_STREQ(token_type_name((TokenType)999), "?");
}

TEST(Util, CapabilityKindName) {
    ASSERT_STREQ(capability_kind_name(CAP_ISO), "iso");
    ASSERT_STREQ(capability_kind_name(CAP_TRN), "trn");
    ASSERT_STREQ(capability_kind_name(CAP_REF), "ref");
    ASSERT_STREQ(capability_kind_name(CAP_VAL), "val");
    ASSERT_STREQ(capability_kind_name(CAP_BOX), "box");
    ASSERT_STREQ(capability_kind_name(CAP_TAG), "tag");
    ASSERT_STREQ(capability_kind_name(CAP_UNKNOWN), "unknown");
    ASSERT_STREQ(capability_kind_name((CapabilityKind)999), "?");
}

TEST(Util, TypeKindName) {
    ASSERT_STREQ(type_kind_name(TYPE_UINT64), "U64");
    ASSERT_STREQ(type_kind_name(TYPE_INT64), "I64");
    ASSERT_STREQ(type_kind_name(TYPE_FLOAT64), "F64");
    ASSERT_STREQ(type_kind_name(TYPE_STRING), "String");
    ASSERT_STREQ(type_kind_name(TYPE_BOOL), "Bool");
    ASSERT_STREQ(type_kind_name(TYPE_NONE), "None");
    ASSERT_STREQ(type_kind_name(TYPE_ANY), "Any");
    ASSERT_STREQ(type_kind_name(TYPE_GENERIC), "Generic");
    ASSERT_STREQ(type_kind_name(TYPE_TUPLE), "Tuple");
    ASSERT_STREQ(type_kind_name(TYPE_UNION), "Union");
    ASSERT_STREQ(type_kind_name(TYPE_UNKNOWN), "Unknown");
    ASSERT_STREQ(type_kind_name((TypeKind)999), "?");
}

TEST(Util, AstNodePrint) {
    ASTNode *root = ast_node_new(NODE_PROGRAM, 1, 1);
    ast_node_new(NODE_ACTOR, 1, 3);
    ast_node_add_child(root, ast_node_new(NODE_FUN, 2, 5));

    FILE *f = fopen("/tmp/ponypp_ast_print_test.txt", "w");
    ASSERT_TRUE(f != nullptr);
    ast_node_print(root, f);
    fclose(f);

    char *buf = s_file_read("/tmp/ponypp_ast_print_test.txt");
    ASSERT_TRUE(buf != nullptr);
    ASSERT_NE((void*)strstr(buf, "Program"), nullptr);
    ASSERT_NE((void*)strstr(buf, "Function"), nullptr);
    s_free(buf);
    ast_node_free(root);
    remove("/tmp/ponypp_ast_print_test.txt");
}

TEST(Util, AstNodePrintNull) {
    FILE *f = fopen("/tmp/ponypp_ast_null.txt", "w");
    ASSERT_TRUE(f != nullptr);
    ast_node_print(NULL, f);
    fclose(f);
}

TEST(Util, AstNodePrintDot) {
    ASTNode *root = ast_node_new(NODE_PROGRAM, 1, 1);
    FILE *f = fopen("/tmp/ponypp_ast_dot.txt", "w");
    ASSERT_TRUE(f != nullptr);
    ast_node_print_dot(root, f);
    fclose(f);
    ast_node_free(root);
    remove("/tmp/ponypp_ast_dot.txt");
}
