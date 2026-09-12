#include <gtest/gtest.h>
#include <ponypp/typecheck.h>
#include <ponypp/lexer.h>
#include <ponypp/parser.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>

/* 针对性覆盖 src/typecheck.c 未覆盖行:
 * 165 (actor_fields 匹配成功 found=1),
 * 185-193 (NODE_CALL print 参数检查),
 * 198-200 (NODE_EMPTY assign 两侧递归),
 * 271-276 (tc_module_types 具体模块导入注册) */

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

static int check_code(const char *code) {
    ASTNode *ast = parse_code(code);
    if (!ast) return -1;
    TypeCheckResult result;
    memset(&result, 0, sizeof(result));
    int ret = typecheck_program(ast, &result);
    typecheck_free_result(&result);
    ast_node_free(ast);
    return ret;
}

/* 165: actor 字段在方法中被引用 -> actor_fields 匹配 found=1 */
TEST(TypeCov27, FieldReferencedInMethod) {
    check_code(
        "actor Counter\n"
        "  var count: U64 = 0\n"
        "  fun get(): U64 => count\n"
    );
}

TEST(TypeCov27, FieldReferencedThisPrefix) {
    check_code(
        "actor Counter\n"
        "  var count: U64 = 0\n"
        "  fun get(): U64 => this.count\n"
    );
}

TEST(TypeCov27, FieldInBehavior) {
    check_code(
        "actor Worker\n"
        "  var n: U64 = 0\n"
        "  be bump() => n\n"
    );
}

/* 185-193: NODE_CALL print 检查 */
TEST(TypeCov27, PrintSingleArg) {
    check_code(
        "actor Main\n"
        "  new create() => print(\"ok\")\n"
    );
}

TEST(TypeCov27, PrintTwoArgs) {
    check_code(
        "actor Main\n"
        "  new create() => print(\"a\", \"b\")\n"
    );
}

TEST(TypeCov27, PrintThreeArgs) {
    check_code(
        "actor Main\n"
        "  new create() => print(\"a\", \"b\", \"c\")\n"
    );
}

TEST(TypeCov27, PrintNoArgs) {
    check_code(
        "actor Main\n"
        "  new create() => print()\n"
    );
}

/* 198-200: NODE_EMPTY assign 两侧递归 */
TEST(TypeCov27, AssignToField) {
    check_code(
        "actor Counter\n"
        "  var count: U64 = 0\n"
        "  be reset() => count = 0\n"
    );
}

TEST(TypeCov27, AssignUnknownIdent) {
    check_code(
        "actor Main\n"
        "  new create() => unknown_var = 42\n"
    );
}

TEST(TypeCov27, AssignWithCallRhs) {
    check_code(
        "actor Main\n"
        "  new create() => unknown_var = print(\"x\")\n"
    );
}

/* 271-276: tc_module_types 具体模块导入 (parser 现支持点号路径) */
TEST(TypeCov27, ImportStdIo) {
    check_code(
        "use std.io\n"
        "actor Main\n"
        "  new create() => None\n"
    );
}

TEST(TypeCov27, ImportStdConcurrent) {
    check_code(
        "use std.concurrent\n"
        "actor Main\n"
        "  new create() => None\n"
    );
}

TEST(TypeCov27, ImportStdJson) {
    check_code(
        "use std.json\n"
        "actor Main\n"
        "  new create() => None\n"
    );
}

TEST(TypeCov27, ImportStdTime) {
    check_code(
        "use std.time\n"
        "actor Main\n"
        "  new create() => None\n"
    );
}

TEST(TypeCov27, ImportStdLog) {
    check_code(
        "use std.log\n"
        "actor Main\n"
        "  new create() => None\n"
    );
}

/* 导入类型在方法中作为类型引用 -> actor_types 匹配 */
TEST(TypeCov27, ImportStdIoUseFile) {
    check_code(
        "use std.io\n"
        "actor Main\n"
        "  var f: File = None\n"
        "  new create() => None\n"
    );
}

TEST(TypeCov27, ImportStdConcurrentUseChannel) {
    check_code(
        "use std.concurrent\n"
        "actor Main\n"
        "  var ch: Channel = None\n"
        "  new create() => None\n"
    );
}

TEST(TypeCov27, ImportDottedUnknown) {
    check_code(
        "use std.nonexistent\n"
        "actor Main\n"
        "  new create() => None\n"
    );
}
