#include "ponypp.h"
#include <assert.h>
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/codegen.h"
#include "gtest_helpers.h"
#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return nullptr;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char*)malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = 0;
    fclose(f);
    return buf;
}

char *codegen_to_str(const char *src) {
    const char *path = "/tmp/ponypp_codegen_cov.c";
    FILE *f = fopen(path, "w");
    assert(f != nullptr);
    ASTNode *ast = parse_to_ast(src);
    assert(ast != nullptr);
    Codegen *cg = codegen_new(f);
    codegen_program(cg, ast);
    codegen_free(cg);
    fclose(f);
    ast_node_free(ast);
    return read_file(path);
}

// ===== NODE_SEND: async message send ! =====
TEST(CodegenCov, AsyncSend) {
    char *out = codegen_to_str(
        "actor A { var x: U64 = 0\n"
        "  new create() => {}\n"
        "  be run() => { b!5 }\n"
        "}\n"
        "actor B { new create() => {} }");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "pny_actor_send"), nullptr);
    free(out);
}

TEST(CodegenCov, AsyncSendField) {
    char *out = codegen_to_str(
        "actor A { var x: U64 = 0\n"
        "  new create() => {}\n"
        "  be run() => { other!x }\n"
        "}\n"
        "actor B { new create() => {} }");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== NODE_MSG_CALL: sync call @ =====
TEST(CodegenCov, SyncCall) {
    char *out = codegen_to_str(
        "actor A { var x: U64 = 0\n"
        "  new create() => {}\n"
        "  be run() => { b@10 }\n"
        "}\n"
        "actor B { new create() => {} }");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "pny_actor_send_sync"), nullptr);
    free(out);
}

// ===== NODE_IMPORT: import/use =====
TEST(CodegenCov, ImportModule) {
    char *out = codegen_to_str(
        "import std\n"
        "actor A { new create() => {} }");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "// import std"), nullptr);
    free(out);
}

TEST(CodegenCov, UseModule) {
    char *out = codegen_to_str(
        "use mymod\n"
        "actor A { new create() => {} }");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "// import mymod"), nullptr);
    free(out);
}

// ===== NODE_CAP: capability-prefixed types =====
TEST(CodegenCov, CapTypeIso) {
    char *out = codegen_to_str(
        "actor A {\n"
        "  var x: iso U64 = 0\n"
        "  new create() => {}\n"
        "}\n");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

TEST(CodegenCov, CapTypeVal) {
    char *out = codegen_to_str(
        "actor A {\n"
        "  var x: val String = \"hi\"\n"
        "  new create() => {}\n"
        "}\n");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

TEST(CodegenCov, CapTypeRef) {
    char *out = codegen_to_str(
        "actor A {\n"
        "  var x: ref U64 = 5\n"
        "  new create() => {}\n"
        "}\n");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

TEST(CodegenCov, CapTypeBox) {
    char *out = codegen_to_str(
        "actor A {\n"
        "  var x: box Bool = false\n"
        "  new create() => {}\n"
        "}\n");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== NODE_MATCH: match expression =====
TEST(CodegenCov, MatchBasic) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  be run() => { var x: U64 = 0\n"
        "    match x => { 0 => { print(\"z\") } } }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "_match_expr"), nullptr);
    free(out);
}

TEST(CodegenCov, MatchMultipleArms) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  be run() => { var x: U64 = 1\n"
        "    match x => { 0 => { print(\"a\") } 1 => { print(\"b\") } }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "_match_expr"), nullptr);
    free(out);
}

// ===== NODE_FOR: for loop =====
TEST(CodegenCov, ForLoop) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  be run() => { for i in 0..10 do { print(i) } }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "for (unsigned long long"), nullptr);
    free(out);
}

TEST(CodegenCov, ForLoopNoDo) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  be run() => { for x in 0..5 { print(x) } }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "for (unsigned long long"), nullptr);
    free(out);
}

// ===== NODE_WHILE: while loop =====
TEST(CodegenCov, WhileLoop) {
    char *out = codegen_to_str(
        "actor A { var i: U64 = 0\n"
        "  new create() => {}\n"
        "  be run() => { while i < 10 { i = i + 1 } }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "while ("), nullptr);
    free(out);
}

// ===== NODE_IF with else =====
TEST(CodegenCov, IfElse) {
    char *out = codegen_to_str(
        "actor A { var x: U64 = 1\n"
        "  new create() => {}\n"
        "  be run() => { if x == 1 { print(\"yes\") } else { print(\"no\") } }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "else"), nullptr);
    free(out);
}

// ===== NODE_RETURN with expression =====
TEST(CodegenCov, ReturnExpr) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  fun calc() => { return 42 }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "return"), nullptr);
    free(out);
}

TEST(CodegenCov, ReturnIdent) {
    char *out = codegen_to_str(
        "actor A { var x: U64 = 5\n"
        "  new create() => {}\n"
        "  fun get() => { return x }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== Float literal in expression =====
TEST(CodegenCov, FloatLiteral) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  be run() => { var x: F64 = 3.14 }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== Bool literal in expression =====
TEST(CodegenCov, BoolLiteral) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  be run() => { var x: Bool = true }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== All type mappings in cg_type_of =====
TEST(CodegenCov, AllTypes) {
    char *out = codegen_to_str(
        "actor A {\n"
        "  var a: U8 = 0\n"
        "  var b: U16 = 0\n"
        "  var c: U32 = 0\n"
        "  var d: U64 = 0\n"
        "  var e: I8 = 0\n"
        "  var f: I16 = 0\n"
        "  var g: I32 = 0\n"
        "  var h: I64 = 0\n"
        "  var i: F32 = 0.0\n"
        "  var j: F64 = 0.0\n"
        "  var k: String = \"s\"\n"
        "  var l: Bool = false\n"
        "  new create() => {}\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "unsigned char"), nullptr);
    ASSERT_NE((void*)strstr(out, "unsigned short"), nullptr);
    ASSERT_NE((void*)strstr(out, "unsigned int"), nullptr);
    ASSERT_NE((void*)strstr(out, "unsigned long long"), nullptr);
    ASSERT_NE((void*)strstr(out, "signed char"), nullptr);
    ASSERT_NE((void*)strstr(out, "signed short"), nullptr);
    ASSERT_NE((void*)strstr(out, "signed int"), nullptr);
    ASSERT_NE((void*)strstr(out, "signed long long"), nullptr);
    ASSERT_NE((void*)strstr(out, "float"), nullptr);
    ASSERT_NE((void*)strstr(out, "double"), nullptr);
    ASSERT_NE((void*)strstr(out, "const char *"), nullptr);
    ASSERT_NE((void*)strstr(out, "int"), nullptr);
    free(out);
}

// ===== None type =====
TEST(CodegenCov, NoneType) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  fun nothing() => {}\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== Constructor with params =====
TEST(CodegenCov, CtorWithParams) {
    char *out = codegen_to_str(
        "actor Counter(val name: String = \"\", val total: U64 = 0) {\n"
        "  var count: U64 = total\n"
        "  new create(name: String, total: U64) => {}\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "Counter_create"), nullptr);
    ASSERT_NE((void*)strstr(out, "const char *"), nullptr);
    free(out);
}

// ===== Field access in constructor (self.field) vs method (self->field) =====
TEST(CodegenCov, FieldInCtor) {
    char *out = codegen_to_str(
        "actor A { var x: U64 = 0\n"
        "  new create() => { x = 5 }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

TEST(CodegenCov, FieldInMethod) {
    char *out = codegen_to_str(
        "actor A { var x: U64 = 0\n"
        "  new create() => {}\n"
        "  be set() => { x = 5 }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== Multiple supervision strategies =====
TEST(CodegenCov, SuperviseOneForAll) {
    char *out = codegen_to_str(
        "actor A { new create() => {} }\n"
        "supervise A one_for_all\n");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "supervise"), nullptr);
    free(out);
}

TEST(CodegenCov, SuperviseRestart) {
    char *out = codegen_to_str(
        "actor A { new create() => {} }\n"
        "supervise A restart\n");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

TEST(CodegenCov, SuperviseNone) {
    char *out = codegen_to_str(
        "actor A { new create() => {} }\n"
        "supervise A none\n");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== Actor with type params (actor field referencing another actor) =====
TEST(CodegenCov, ActorFieldRef) {
    char *out = codegen_to_str(
        "actor B { new create() => {} }\n"
        "actor A { var worker: B\n"
        "  new create() => {}\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "B_t"), nullptr);
    free(out);
}

// ===== Multi-method actor with multiple be =====
TEST(CodegenCov, MultiMethods) {
    char *out = codegen_to_str(
        "actor A {\n"
        "  var x: U64 = 0\n"
        "  new create() => {}\n"
        "  be foo() => { x = 1 }\n"
        "  be bar() => { x = 2 }\n"
        "  fun baz() => { return 3 }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "A_foo"), nullptr);
    ASSERT_NE((void*)strstr(out, "A_bar"), nullptr);
    ASSERT_NE((void*)strstr(out, "A_baz"), nullptr);
    free(out);
}

// ===== Fun with return type =====
TEST(CodegenCov, FunWithReturnType) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  fun name(): String => { return \"test\" }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== print with identifier field argument =====
TEST(CodegenCov, PrintFieldArg) {
    char *out = codegen_to_str(
        "actor A { var x: U64 = 42\n"
        "  new create() => {}\n"
        "  be run() => { print(x) }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== print with non-field identifier =====
TEST(CodegenCov, PrintNonFieldIdent) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  be run() => { print(y) }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== Assignment with string rhs =====
TEST(CodegenCov, AssignString) {
    char *out = codegen_to_str(
        "actor A { var s: String = \"\"\n"
        "  new create() => {}\n"
        "  be set() => { s = \"hello\" }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== String escape in print =====
TEST(CodegenCov, PrintStringEscape) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  be run() => { print(\"a\\\"b\") }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "\\\""), nullptr);
    free(out);
}

// ===== String escape with backslash =====
TEST(CodegenCov, PrintBackslashEscape) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  be run() => { print(\"a\\\\b\") }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== Constructor without body =====
TEST(CodegenCov, CtorNoBody) {
    char *out = codegen_to_str(
        "actor A { new create() => { x = 1 } }");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== Multiple actors with run method =====
TEST(CodegenCov, MainActor) {
    char *out = codegen_to_str(
        "actor main { new create() => {}\n"
        "  be run() => { print(\"hello\") }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "int main"), nullptr);
    free(out);
}

// ===== Actor with only fields (no methods) =====
TEST(CodegenCov, ActorOnlyFields) {
    char *out = codegen_to_str(
        "actor A { var x: U64 = 0\n"
        "  new create() => {}\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "typedef struct"), nullptr);
    free(out);
}

// ===== Nested if-else =====
TEST(CodegenCov, NestedIf) {
    char *out = codegen_to_str(
        "actor A { var x: U64 = 1\n"
        "  new create() => {}\n"
        "  be run() => { if x == 1 { if x == 2 { print(\"y\") } } else { print(\"n\") } }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== print with string arg (printf format) =====
TEST(CodegenCov, PrintString) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  be run() => { print(\"hello\") }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "printf(\"hello"), nullptr);
    free(out);
}

// ===== print with no args (empty printf) =====
TEST(CodegenCov, PrintNoArgs) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  be run() => { print() }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "printf(\"\\n\")"), nullptr);
    free(out);
}

// ===== Multiple supervise with different strategies =====
TEST(CodegenCov, MultiSupervise) {
    char *out = codegen_to_str(
        "actor A { new create() => {} }\n"
        "actor B { new create() => {} }\n"
        "actor C { new create() => {} }\n"
        "supervise A one_for_all\n"
        "supervise B restart\n"
        "supervise C none\n");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== Actor with params in constructor =====
TEST(CodegenCov, CtorParamsAssignment) {
    char *out = codegen_to_str(
        "actor Counter(val initial: U64 = 0) {\n"
        "  var count: U64 = 0\n"
        "  new create(initial: U64) => { count = initial }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "count"), nullptr);
    ASSERT_NE((void*)strstr(out, "initial"), nullptr);
    free(out);
}

// ===== Generic method (method with type param) =====
TEST(CodegenCov, MethodWithParam) {
    char *out = codegen_to_str(
        "actor A { new create() => {}\n"
        "  be add(x: U64) => { print(x) }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== Multiple fields in different types =====
TEST(CodegenCov, MixedFields) {
    char *out = codegen_to_str(
        "actor A {\n"
        "  var s: String = \"hello\"\n"
        "  var n: U64 = 42\n"
        "  var b: Bool = true\n"
        "  var f: F64 = 3.14\n"
        "  new create() => {}\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "const char *"), nullptr);
    ASSERT_NE((void*)strstr(out, "unsigned long long"), nullptr);
    ASSERT_NE((void*)strstr(out, "double"), nullptr);
    free(out);
}

// ===== Supervise without strategy =====
TEST(CodegenCov, SuperviseNoStrategy) {
    char *out = codegen_to_str(
        "actor A { new create() => {} }\n"
        "supervise A\n");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "supervise"), nullptr);
    free(out);
}

// ===== Actor param type =====
TEST(CodegenCov, ActorParam) {
    char *out = codegen_to_str(
        "actor B { new create() => {} }\n"
        "actor A { new create() => {}\n"
        "  be send(w: B) => { w!1 }\n"
        "}");
    ASSERT_TRUE(out != nullptr);
    free(out);
}

// ===== Import with semicolon =====
TEST(CodegenCov, ImportWithSemicolon) {
    char *out = codegen_to_str(
        "import std;\n"
        "actor A { new create() => {} }");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "// import std"), nullptr);
    free(out);
}

// ===== Multiple imports =====
TEST(CodegenCov, MultipleImports) {
    char *out = codegen_to_str(
        "import std\n"
        "import math\n"
        "actor A { new create() => {} }");
    ASSERT_TRUE(out != nullptr);
    ASSERT_NE((void*)strstr(out, "// import std"), nullptr);
    ASSERT_NE((void*)strstr(out, "// import math"), nullptr);
    free(out);
}
