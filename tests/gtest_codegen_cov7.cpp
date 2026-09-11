#include <gtest/gtest.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp/codegen.h>
#include <ponypp.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

static ASTNode *parse_code(const char *src) {
    Lexer *lx = lexer_new("test", src, strlen(src));
    Token *toks = NULL; size_t tc = 0;
    lexer_lex_all(lx, &toks, &tc);
    Parser *p = parser_new("test", toks, tc);
    return parser_parse_program(p);
}

static bool emit_ok(const char *src) {
    ASTNode *ast = parse_code(src);
    if (!ast) return false;
    FILE *devnull = fopen("/dev/null", "w");
    if (!devnull) { ast_node_free(ast); return false; }
    Codegen *cg = codegen_new(devnull);
    if (!cg) { fclose(devnull); ast_node_free(ast); return false; }
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(devnull);
    ast_node_free(ast);
    return true;
}

/* ==================== Method Calls ==================== */

TEST(CodegenCov7, SizeMethod) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { let n: USize = \"hello\".size() }\n"
        "}\n"));
}

TEST(CodegenCov7, ApplyMethod) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { let s: String = \"hello\".apply(0) }\n"
        "}\n"));
}

TEST(CodegenCov7, SubstringMethod) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { let s: String = \"hello\".substring(1, 3) }\n"
        "}\n"));
}

TEST(CodegenCov7, ContainsMethod) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { let b: Bool = \"hello\".contains(\"ell\") }\n"
        "}\n"));
}

TEST(CodegenCov7, StartsWithMethod) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { let b: Bool = \"hello\".startswith(\"he\") }\n"
        "}\n"));
}

TEST(CodegenCov7, EndsWithMethod) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { let b: Bool = \"hello\".endswith(\"lo\") }\n"
        "}\n"));
}

TEST(CodegenCov7, FindMethod) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { let n: USize = \"hello\".find(\"l\") }\n"
        "}\n"));
}

TEST(CodegenCov7, UpperMethod) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { let s: String = \"hello\".upper() }\n"
        "}\n"));
}

TEST(CodegenCov7, LowerMethod) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { let s: String = \"HELLO\".lower() }\n"
        "}\n"));
}

TEST(CodegenCov7, StripMethod) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { let s: String = \" hello \".strip() }\n"
        "}\n"));
}

TEST(CodegenCov7, RepeatMethod) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { let s: String = \"ab\".repeat(3) }\n"
        "}\n"));
}

TEST(CodegenCov7, SplitMethod) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { let a: Array[String] = \"a,b,c\".split(\",\") }\n"
        "}\n"));
}

TEST(CodegenCov7, ReplaceMethod) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { let s: String = \"hello\".replace(\"l\", \"L\") }\n"
        "}\n"));
}

/* ==================== Field Access ==================== */

TEST(CodegenCov7, FieldAccessSimple) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  var _x: U32\n"
        "  new create() => { _x = 42 }\n"
        "}\n"));
}

TEST(CodegenCov7, FieldAccessUnderscore) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  var _name: String\n"
        "  new create() => { _name = \"test\" }\n"
        "}\n"));
}

/* ==================== Print Variants ==================== */

TEST(CodegenCov7, PrintBoolLiteral) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { print(true) }\n"
        "}\n"));
}

TEST(CodegenCov7, PrintBoolVar) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let b: Bool = true\n"
        "    print(b)\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov7, PrintFieldAccess) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  var _x: U32\n"
        "  new create() => {\n"
        "    _x = 42\n"
        "    print(_x)\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov7, PrintCharLiteral) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { print('A') }\n"
        "}\n"));
}

/* ==================== Return Variants ==================== */

TEST(CodegenCov7, ReturnFromNew) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { return }\n"
        "}\n"));
}

TEST(CodegenCov7, ReturnFromBehavior) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { }\n"
        "  be greet() => { return }\n"
        "}\n"));
}

TEST(CodegenCov7, ReturnFromFunction) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => { }\n"
        "  fun _helper(): U32 =>\n"
        "    return 42\n"
        "}\n"));
}

/* ==================== Match Variants ==================== */

TEST(CodegenCov7, MatchWithInt) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: U32 = 1\n"
        "    match x\n"
        "    | 1 => print(\"one\")\n"
        "    | 2 => print(\"two\")\n"
        "    else print(\"other\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov7, MatchWithBool) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let b: Bool = true\n"
        "    match b\n"
        "    | true => print(\"yes\")\n"
        "    | false => print(\"no\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

/* ==================== Lambda ==================== */

TEST(CodegenCov7, LambdaNoCapture) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let f = lambda() { print(\"hi\") }\n"
        "  }\n"
        "}\n"));
}


/* ==================== Error Paths ==================== */

TEST(CodegenCov7, NullCodegen) {
    /* codegen_free(nullptr) should be safe */
    codegen_free(nullptr);
}

TEST(CodegenCov7, EmptyProgram) {
    ASTNode *ast = parse_code("");
    if (ast) {
        FILE *devnull = fopen("/dev/null", "w");
        Codegen *cg = codegen_new(devnull);
        codegen_program(cg, ast);
        codegen_free(cg);
        fclose(devnull);
        ast_node_free(ast);
    }
}

/* ==================== Complex Programs ==================== */

TEST(CodegenCov7, ActorWithMultipleBehaviors) {
    EXPECT_TRUE(emit_ok(
        "actor Worker {\n"
        "  new create() => { }\n"
        "  be process(x: U32) => { print(x) }\n"
        "  be stop() => { print(\"stop\") }\n"
        "}\n"
        "actor main {\n"
        "  new create() => {\n"
        "    let w = Worker.create()\n"
        "    w.process(42)\n"
        "    w.stop()\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov7, ActorWithFieldsAndMethods) {
    EXPECT_TRUE(emit_ok(
        "actor Counter {\n"
        "  var _count: U32\n"
        "  new create() => { _count = 0 }\n"
        "  be increment() => { _count = _count + 1 }\n"
        "  be get() => { print(_count) }\n"
        "}\n"
        "actor main {\n"
        "  new create() => {\n"
        "    let c = Counter.create()\n"
        "    c.increment()\n"
        "    c.get()\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov7, NestedIfElse) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    let x: U32 = 5\n"
        "    if x > 10 then print(\"big\")\n"
        "    elseif x > 3 then print(\"medium\")\n"
        "    elseif x > 1 then print(\"small\")\n"
        "    else print(\"tiny\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov7, TryCatchFinally) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    try\n"
        "      print(\"try\")\n"
        "    then\n"
        "      print(\"then\")\n"
        "    else\n"
        "      print(\"else\")\n"
        "    end\n"
        "  }\n"
        "}\n"));
}

TEST(CodegenCov7, WhileWithBreakContinue) {
    EXPECT_TRUE(emit_ok(
        "actor main {\n"
        "  new create() => {\n"
        "    var i: U32 = 0\n"
        "    while i < 10 do\n"
        "      i = i + 1\n"
        "      if i == 5 then continue end\n"
        "      if i == 8 then break end\n"
        "    end\n"
        "  }\n"
        "}\n"));
}
