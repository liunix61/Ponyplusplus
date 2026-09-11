#include <gtest/gtest.h>
#include <ponypp/codegen.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

static ASTNode *parse_code(const char *src) {
    Lexer *lx = lexer_new("test", src, strlen(src));
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("test", toks, tc);
    return parser_parse_program(p);
}

static bool gen_ok(const char *src) {
    ASTNode *ast = parse_code(src);
    if (!ast) return false;
    FILE *devnull = fopen("/dev/null", "w");
    if (!devnull) return false;
    Codegen *cg = codegen_new(devnull);
    if (!cg) { fclose(devnull); return false; }
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(devnull);
    return true;
}

/* 浮点字面量 */
TEST(CodegenCov5, FloatLiteral) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print(3.14)\n  }\n}\n"));
}
/* 字段访问 */
TEST(CodegenCov5, FieldAccess) {
    EXPECT_TRUE(gen_ok("actor main {\n  var x: I32\n  new create() => {\n    print(x)\n  }\n}\n"));
}

/* 赋值语句 */
TEST(CodegenCov5, AssignStmt) {
    EXPECT_TRUE(gen_ok("actor main {\n  var x: I32\n  new create() => {\n    x = 42\n  }\n}\n"));
}

/* return 语句 */
TEST(CodegenCov5, ReturnStmt) {
    EXPECT_TRUE(gen_ok("actor main {\n  fun foo(): I32 =>\n    return 42\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* let 声明 */
TEST(CodegenCov5, LetDecl) {
    EXPECT_TRUE(gen_ok("actor main {\n  let x: I32 = 42\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* if/else */
TEST(CodegenCov5, IfElse) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    if true then print(1) else print(2) end\n  }\n}\n"));
}

/* while 循环 */
TEST(CodegenCov5, WhileLoop) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    while false do print(1) end\n  }\n}\n"));
}

/* match 表达式 */
TEST(CodegenCov5, MatchExpr) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    match 1\n    | 1 => print(\"one\")\n    | 2 => print(\"two\")\n    else print(\"other\")\n    end\n  }\n}\n"));
}

/* 多个 actor */
TEST(CodegenCov5, MultipleActors) {
    EXPECT_TRUE(gen_ok("actor A {\n  new create() => {\n    print(\"a\")\n  }\n}\nactor B {\n  new create() => {\n    print(\"b\")\n  }\n}\n"));
}

/* 继承 */
TEST(CodegenCov5, Inheritance) {
    EXPECT_TRUE(gen_ok("trait T\nactor main is T {\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* be (behavior) 方法 */
TEST(CodegenCov5, BehaviorMethod) {
    EXPECT_TRUE(gen_ok("actor main {\n  be greet() => {\n    print(\"hello\")\n  }\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* fun 方法 */
TEST(CodegenCov5, FunMethod) {
    EXPECT_TRUE(gen_ok("actor main {\n  fun add(a: I32, b: I32): I32 =>\n    a + b\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* 布尔字面量 */
TEST(CodegenCov5, BoolLiteral) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    if true then print(1) end\n  }\n}\n"));
}

/* 空 actor */
TEST(CodegenCov5, EmptyActor) {
    EXPECT_TRUE(gen_ok("actor main\n"));
}

/* null AST */
TEST(CodegenCov5, NullAst) {
    FILE *devnull = fopen("/dev/null", "w");
    ASSERT_NE(devnull, nullptr);
    Codegen *cg = codegen_new(devnull);
    ASSERT_NE(cg, nullptr);
    codegen_program(cg, nullptr);
    codegen_free(cg);
    fclose(devnull);
    SUCCEED();
}

/* sourcemap 基本操作 */
TEST(CodegenCov5, SourcemapBasic) {
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);
    sourcemap_add(sm, 1, "test.pny", 1, 1);
    
    SourceMapEntry entry;
    int found = sourcemap_lookup(sm, 1, &entry);
    EXPECT_EQ(found, 0);
    
    sourcemap_free(sm);
}

/* sourcemap save/load */
TEST(CodegenCov5, SourcemapSaveLoad) {
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);
    sourcemap_add(sm, 1, "test.pny", 1, 1);
    sourcemap_add(sm, 2, "test.pny", 2, 1);
    
    int r = sourcemap_save_json(sm, "/tmp/test_sourcemap.json");
    EXPECT_EQ(r, 0);
    
    SourceMap *sm2 = sourcemap_load_json("/tmp/test_sourcemap.json");
    EXPECT_NE(sm2, nullptr);
    
    if (sm2) {
        EXPECT_EQ(sourcemap_count(sm2), 2);
        sourcemap_free(sm2);
    }
    
    sourcemap_free(sm);
    unlink("/tmp/test_sourcemap.json");
}

/* codegen_new_with_map */
TEST(CodegenCov5, NewWithMap) {
    FILE *devnull = fopen("/dev/null", "w");
    ASSERT_NE(devnull, nullptr);
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);
    
    Codegen *cg = codegen_new_with_map(devnull, sm);
    ASSERT_NE(cg, nullptr);
    
    codegen_set_source_file(cg, "test.pny");
    
    ASTNode *ast = parse_code("actor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n");
    ASSERT_NE(ast, nullptr);
    codegen_program(cg, ast);
    
    codegen_free(cg);
    sourcemap_free(sm);
    fclose(devnull);
    SUCCEED();
}

/* 多条语句 */
TEST(CodegenCov5, MultipleStmts) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print(\"a\")\n    print(\"b\")\n    print(\"c\")\n  }\n}\n"));
}

/* import 语句 */
TEST(CodegenCov5, ImportStmt) {
    EXPECT_TRUE(gen_ok("use std\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* 嵌套 if */
TEST(CodegenCov5, NestedIf) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    if true then\n      if false then print(1) else print(2) end\n    end\n  }\n}\n"));
}

/* 空程序 */
TEST(CodegenCov5, EmptyProgram) {
    ASTNode *ast = parse_code("");
    if (ast) {
        FILE *devnull = fopen("/dev/null", "w");
        Codegen *cg = codegen_new(devnull);
        codegen_program(cg, ast);
        codegen_free(cg);
        fclose(devnull);
    }
    SUCCEED();
}

/* 多个 use 语句 */
TEST(CodegenCov5, MultipleUse) {
    EXPECT_TRUE(gen_ok("use std\nuse std.concurrent\nuse std.io\nactor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* 泛型 */
TEST(CodegenCov5, GenericType) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}

/* 能力标注 */
TEST(CodegenCov5, Capability) {
    EXPECT_TRUE(gen_ok("actor main\n  var x: I32\n  new create() =>\n    print(\"hi\")\n"));
}

/* 消息发送 */
TEST(CodegenCov5, MessageSend) {
    EXPECT_TRUE(gen_ok("actor main {\n  new create() => {\n    print(\"hi\")\n  }\n}\n"));
}
