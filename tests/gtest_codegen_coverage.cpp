#include <gtest/gtest.h>
#include <ponypp/codegen.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp/ast.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

/* 辅助: 编译 Pony++ 源码 (验证不崩溃) */
static bool compile_source(const char *src) {
    Lexer *lex = lexer_new("test.pny", src, strlen(src));
    if (!lex) return false;
    Token *tokens = nullptr;
    size_t count = 0;
    if (!lexer_lex_all(lex, &tokens, &count)) {
        lexer_free(lex);
        return false;
    }
    Parser *p = parser_new("test.pny", tokens, count);
    if (!p) { lexer_free(lex); return false; }
    ASTNode *ast = parser_parse_program(p);
    parser_free(p);
    lexer_free(lex);
    if (!ast) return false;
    
    FILE *out = tmpfile();
    if (!out) { ast_node_free(ast); return false; }
    Codegen *cg = codegen_new(out);
    if (!cg) { fclose(out); ast_node_free(ast); return false; }
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(out);
    ast_node_free(ast);
    return true;
}

/* ==================== 字符字面量 ==================== */

TEST(CodegenEdge, CharLiterals) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: U8 = 'A'\n"
        "    let b: U8 = '\\n'\n"
        "    let c: U8 = '\\t'\n"
    ));
}

TEST(CodegenEdge, CharSpecial) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: U8 = '\\r'\n"
        "    let b: U8 = '\\\\'\n"
    ));
}

/* ==================== 数字字面量 ==================== */

TEST(CodegenEdge, IntLiterals) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: I32 = 0\n"
        "    let b: I32 = 42\n"
        "    let c: I32 = -1\n"
    ));
}

TEST(CodegenEdge, FloatLiterals) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: F64 = 0.0\n"
        "    let b: F64 = 3.14\n"
    ));
}

/* ==================== 字符串字面量 ==================== */

TEST(CodegenEdge, StringLiterals) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: String = \"hello\"\n"
        "    let b: String = \"\"\n"
    ));
}

/* ==================== 运算符 ==================== */

TEST(CodegenEdge, ArithmeticOps) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: I32 = 1 + 2\n"
        "    let b: I32 = 3 - 1\n"
        "    let c: I32 = 2 * 3\n"
        "    let d: I32 = 6 / 2\n"
        "    let e: I32 = 7 % 3\n"
    ));
}

TEST(CodegenEdge, ComparisonOps) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: Bool = 1 == 2\n"
        "    let b: Bool = 1 != 2\n"
        "    let c: Bool = 1 < 2\n"
        "    let d: Bool = 1 > 2\n"
    ));
}

TEST(CodegenEdge, LogicalOps) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: Bool = true and false\n"
        "    let b: Bool = true or false\n"
        "    let c: Bool = not true\n"
    ));
}

/* ==================== 控制流 ==================== */

TEST(CodegenEdge, IfElse) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    if true then\n"
        "      env.out.print(\"yes\")\n"
        "    else\n"
        "      env.out.print(\"no\")\n"
        "    end\n"
    ));
}

TEST(CodegenEdge, WhileLoop) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    var i: I32 = 0\n"
        "    while i < 10 do\n"
        "      i = i + 1\n"
        "    end\n"
    ));
}

TEST(CodegenEdge, RepeatLoop) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    var i: I32 = 0\n"
        "    repeat\n"
        "      i = i + 1\n"
        "    until i >= 10 end\n"
    ));
}

/* ==================== Match 表达式 ==================== */

TEST(CodegenEdge, MatchExpr) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let x: I32 = 42\n"
        "    match x\n"
        "    | 1 => env.out.print(\"one\")\n"
        "    | 2 => env.out.print(\"two\")\n"
        "    else\n"
        "      env.out.print(\"other\")\n"
        "    end\n"
    ));
}

/* ==================== Try 表达式 ==================== */

TEST(CodegenEdge, TryExpr) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    try\n"
        "      env.out.print(\"ok\")\n"
        "    else\n"
        "      env.out.print(\"error\")\n"
        "    end\n"
    ));
}

/* ==================== Actor 定义 ==================== */

TEST(CodegenEdge, ActorWithBehaviors) {
    EXPECT_TRUE(compile_source(
        "actor Worker\n"
        "  var count: I32 = 0\n"
        "  be increment() =>\n"
        "    count = count + 1\n"
        "\n"
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let w = Worker\n"
        "    w.increment()\n"
    ));
}

/* ==================== 类型注解 ==================== */

TEST(CodegenEdge, TypeAnnotations) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: U8 = 1\n"
        "    let b: U16 = 2\n"
        "    let c: U32 = 3\n"
        "    let d: U64 = 4\n"
        "    let e: I8 = 5\n"
        "    let f: I16 = 6\n"
        "    let g: I32 = 7\n"
        "    let h: I64 = 8\n"
        "    let i: F32 = 9.0\n"
        "    let j: F64 = 10.0\n"
        "    let k: Bool = true\n"
    ));
}

/* ==================== 函数调用 ==================== */

TEST(CodegenEdge, FunctionCalls) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    env.out.print(\"hello\")\n"
        "    env.out.print(\"world\")\n"
    ));
}

/* ==================== 复杂表达式 ==================== */

TEST(CodegenEdge, NestedExpressions) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let x: I32 = (1 + 2) * (3 - 1) / 2\n"
    ));
}

TEST(CodegenEdge, UnaryOps) {
    EXPECT_TRUE(compile_source(
        "actor Main\n"
        "  new create(env: Env) =>\n"
        "    let a: I32 = -42\n"
        "    let b: Bool = not true\n"
    ));
}

/* ==================== 注释 ==================== */

TEST(CodegenEdge, Comments) {
    EXPECT_TRUE(compile_source(
        "// Line comment\n"
        "actor Main\n"
        "  /* Block comment */\n"
        "  new create(env: Env) =>\n"
        "    // Another comment\n"
        "    env.out.print(\"test\")\n"
    ));
}

/* ==================== 空程序 ==================== */

TEST(CodegenEdge, EmptyProgram) {
    /* 空程序可能成功或失败, 但不应崩溃 */
    compile_source("");
}

/* ==================== 语法错误处理 ==================== */

TEST(CodegenEdge, SyntaxError) {
    /* 语法错误不应崩溃 */
    compile_source("this is not valid pony++ code {{{");
}
