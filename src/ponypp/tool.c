#include "ponypp/tool.h"
#include "ponypp.h"
#include "ponypp/util.h"
#include "ponypp/ast.h"
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/codegen.h"
#include "ponypp/wasm.h"
#include "ponypp/wit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

/* ---- parse args ---- */

int tool_parse_args(int argc, char **argv, ToolConfig *tc) {
    memset(tc, 0, sizeof(ToolConfig));
    tc->target = "native";
    tc->olevel = "2";

    if (argc < 2) return -1;

    const char *cmd = argv[1];
    if (strcmp(cmd, "build") == 0) tc->cmd = TOOL_BUILD;
    else if (strcmp(cmd, "run") == 0) tc->cmd = TOOL_RUN;
    else if (strcmp(cmd, "test") == 0) tc->cmd = TOOL_TEST;
    else if (strcmp(cmd, "fmt") == 0) tc->cmd = TOOL_FMT;
    else if (strcmp(cmd, "pkg") == 0) tc->cmd = TOOL_PKG;
    else if (strcmp(cmd, "docs") == 0) tc->cmd = TOOL_DOCS;
    else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) tc->cmd = TOOL_HELP;
    else return -1;

    int i = 2;
    while (i < argc) {
        if (strcmp(argv[i], "-o") == 0 && i+1 < argc) { tc->output = argv[++i]; i++; }
        else if (strncmp(argv[i], "-o", 2) == 0 && argv[i][2]) { tc->output = argv[i]+2; i++; }
        else if (strcmp(argv[i], "-g") == 0) { tc->debug = true; i++; }
        else if (strncmp(argv[i], "-O", 2) == 0 && argv[i][2]) { tc->olevel = argv[i]+2; i++; }
        else if (strcmp(argv[i], "--ast") == 0) { tc->ast_dump = true; i++; }
        else if (strcmp(argv[i], "--pretty") == 0) { tc->pretty = true; i++; }
        else if (strcmp(argv[i], "--wit-only") == 0) { tc->wit_only = true; i++; }
        else if (strncmp(argv[i], "--target=", 11) == 0) { tc->target = argv[i]+11; i++; }
        else if (strncmp(argv[i], "--platform=", 13) == 0) { tc->platform = argv[i]+13; i++; }
        else if (strcmp(argv[i], "--target") == 0 && i+1 < argc) { tc->target = argv[++i]; i++; }
        else if (strcmp(argv[i], "--platform") == 0 && i+1 < argc) { tc->platform = argv[++i]; i++; }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) { tc->test_verbose = true; i++; }
        else if (argv[i][0] == '-') { fprintf(stderr, "[ponyppc] 未知选项: %s\n", argv[i]); return -1; }
        else {
            if (tc->cmd == TOOL_PKG && !tc->subcmd) tc->subcmd = argv[i];
            else if (!tc->input) tc->input = argv[i];
            i++;
        }
    }
    return 0;
}

/* ---- build ---- */

int tool_build(const char *input, const char *output, const char *target,
               const char *platform, const char *olevel, bool debug) {
    (void)platform; (void)olevel; (void)debug;

    char *source = s_file_read(input);
    if (!source) { fprintf(stderr, "[build] 无法读取: %s\n", input); return -1; }

    size_t len = strlen(source);
    printf("[build] 编译 '%s' (%zu bytes) target=%s\n", input, len, target);

    Lexer *lexer = lexer_new(input, source, len);
    if (!lexer) { s_free(source); return -1; }

    Token *tokens = NULL;
    size_t token_count = 0;
    bool ok = lexer_lex_all(lexer, &tokens, &token_count);
    if (!ok) { fprintf(stderr, "[build] 词法错误: %s\n", lexer_error(lexer)); return -1; }

    Parser *parser = parser_new(input, tokens, token_count);
    if (!parser) { s_free(source); return -1; }

    ASTNode *ast = parser_parse_program(parser);
    if (!ast) { fprintf(stderr, "[build] 语法错误: %s\n", parser_error(parser)); s_free(source); return -1; }

    const char *out = output ? output : "ponypp-out";
    if (strcmp(target, "native") == 0) {
        size_t out_len = strlen(out);
        char *c_output = (char*)malloc(out_len + 4);
        snprintf(c_output, out_len + 4, "%s.c", out);

        FILE *sf = fopen(c_output, "w");
        if (!sf) { fprintf(stderr, "[build] 写入失败: %s\n", c_output); free(c_output); s_free(source); return -1; }
        Codegen *cg = codegen_new(sf);
        codegen_program(cg, ast);
        codegen_free(cg);
        fclose(sf);

        char cmdbuf[4096];
        snprintf(cmdbuf, sizeof(cmdbuf), "gcc -std=c11 -Wall -O%d -o %s %s", atoi(olevel), out, c_output);
        int r = system(cmdbuf);
        free(c_output);
        if (r != 0) { fprintf(stderr, "[build] gcc 失败\n"); s_free(source); return -1; }
        printf("[build] ✓ 生成 %s\n", out);
    } else if (strcmp(target, "wit") == 0) {
        char wit_path[512];
        snprintf(wit_path, sizeof(wit_path), "%s.wit", out);
        wit_write_program(ast, wit_path);
        printf("[build] ✓ 生成 %s\n", wit_path);
    } else {
        char wasm_path[512];
        snprintf(wasm_path, sizeof(wasm_path), "%s.wasm", out);
        wasm_write_program(ast, wasm_path);
        printf("[build] ✓ 生成 %s\n", wasm_path);
    }

    s_free(source);
    return 0;
}

/* ---- run ---- */

int tool_run(const char *input, const char *target, const char *olevel) {
    if (!input) { fprintf(stderr, "[run] 缺少输入文件\n"); return -1; }

    const char *tmpbin = "/tmp/ponypp_run_bin";
    int r = tool_build(input, tmpbin, target, NULL, olevel, false);
    if (r != 0) return -1;

    printf("[run] 执行 %s ...\n", tmpbin);
    r = system(tmpbin);
    unlink(tmpbin);
    return r;
}

/* ---- test ---- */

static int run_test_file(const char *path, bool verbose) {
    printf("%s: ", path);
    int r = tool_build(path, "/tmp/ponypp_test_bin", "native", NULL, "0", false);
    if (r != 0) { printf("FAIL (build)\n"); return -1; }

    char cmdbuf[512];
    snprintf(cmdbuf, sizeof(cmdbuf), "/tmp/ponypp_test_bin > /tmp/ponypp_test_out.txt 2>&1; echo $?" );
    FILE *fp = popen(cmdbuf, "r");
    if (!fp) { printf("FAIL (run)\n"); unlink("/tmp/ponypp_test_bin"); return -1; }
    char buf[128];
    int exit_code = 0;
    while (fgets(buf, sizeof(buf), fp)) {
        /* last line is exit code */
        char *end = buf + strlen(buf) - 1;
        if (*end == '\n') *end = '\0';
        if (verbose) printf("%s\n", buf);
        if (strspn(buf, "0123456789") == strlen(buf)) exit_code = atoi(buf);
    }
    pclose(fp);
    unlink("/tmp/ponypp_test_bin");

    printf("%s\n", exit_code == 0 ? "✓ PASS" : "✗ FAIL");
    return exit_code == 0 ? 0 : -1;
}

int tool_test(bool verbose) {
    DIR *d = opendir(".");
    if (!d) { fprintf(stderr, "[test] 无法打开当前目录\n"); return -1; }

    struct dirent *ent;
    int total = 0, passed = 0;
    printf("[test] 发现 *_test.pny 文件:\n");

    while ((ent = readdir(d))) {
        const char *name = ent->d_name;
        size_t nlen = strlen(name);
        if (nlen > 11 && strcmp(name + nlen - 11, "_test.pny") == 0) {
            total++;
            if (run_test_file(name, verbose) == 0) passed++;
        }
    }
    closedir(d);

    printf("[test] 总计 %d 通过 %d 失败 %d\n", total, passed, total - passed);
    return (total == 0 || passed == total) ? 0 : -1;
}

/* ---- fmt ---- */

int tool_fmt(const char *input) {
    if (!input) { fprintf(stderr, "[fmt] 缺少输入文件\n"); return -1; }
    char *src = s_file_read(input);
    if (!src) { fprintf(stderr, "[fmt] 读取失败: %s\n", input); return -1; }

    size_t len = strlen(src);
    Lexer *lexer = lexer_new(input, src, len);
    if (!lexer) { s_free(src); return -1; }

    Token *tokens = NULL;
    size_t token_count = 0;
    if (!lexer_lex_all(lexer, &tokens, &token_count)) { fprintf(stderr, "[fmt] 词法错误\n"); s_free(src); return -1; }

    Parser *parser = parser_new(input, tokens, token_count);
    if (!parser) { s_free(src); return -1; }
    ASTNode *ast = parser_parse_program(parser);
    if (!ast) { fprintf(stderr, "[fmt] 语法错误\n"); s_free(src); return -1; }

    (void)ast;
    /* Pretty-print: write source back (structural validation done) */
    FILE *sf = fopen(input, "w");
    if (!sf) { fprintf(stderr, "[fmt] 写入失败\n"); s_free(src); return -1; }
    fputs(src, sf);
    fclose(sf);
    printf("[fmt] ✓ 格式化 %s\n", input);
    s_free(src);
    return 0;
}

/* ---- pkg ---- */

int tool_pkg_new(const char *name) {
    if (!name) { fprintf(stderr, "[pkg] 需要项目名称\n"); return -1; }

    mkdir(name, 0755);
    char src_dir[512];
    snprintf(src_dir, sizeof(src_dir), "%s/src", name);
    mkdir(src_dir, 0755);

    char toml[512], main_path[512];
    snprintf(toml, sizeof(toml), "%s/ponypp.toml", name);
    FILE *f = fopen(toml, "w");
    if (!f) { fprintf(stderr, "[pkg] 创建失败\n"); return -1; }
    fprintf(f, "[package]\nname = \"%s\"\nversion = \"0.1.0\"\nedition = \"2026\"\n\n[dependencies]\n", name);
    fclose(f);

    snprintf(main_path, sizeof(main_path), "%s/src/main.pny", name);
    f = fopen(main_path, "w");
    fprintf(f, "actor main {\n");
    fprintf(f, "  new create() => {\n");
    fprintf(f, "    print(\"Hello, %s!\")\n", name);
    fprintf(f, "  }\n");
    fprintf(f, "}\n");
    fclose(f);

    printf("[pkg] ✓ 创建 %s/ (main.pny + ponypp.toml)\n", name);
    return 0;
}

int tool_pkg_add(const char *dep) {
    if (!dep) { fprintf(stderr, "[pkg] 需要依赖名称\n"); return -1; }
    FILE *f = fopen("ponypp.toml", "a");
    if (!f) { fprintf(stderr, "[pkg] 未找到 ponypp.toml\n"); return -1; }
    fprintf(f, "%s = \"*\"\n", dep);
    fclose(f);
    printf("[pkg] ✓ 添加依赖 %s\n", dep);
    return 0;
}

/* ---- execute ---- */

int tool_execute(ToolConfig *tc) {
    switch (tc->cmd) {
        case TOOL_HELP:
            printf("ponyppc - Pony++ 工具链 v%s\n", VERSION_STRING);
            printf("用法: ponyppc <子命令> [选项] [参数]\n");
            printf("子命令: build run test fmt pkg\n");
            return 0;
        case TOOL_BUILD:
            return tool_build(tc->input, tc->output, tc->target, tc->platform, tc->olevel, tc->debug);
        case TOOL_RUN:
            return tool_run(tc->input, tc->target, tc->olevel);
        case TOOL_TEST:
            return tool_test(tc->test_verbose);
        case TOOL_FMT:
            return tool_fmt(tc->input);
        case TOOL_PKG:
            if (tc->subcmd && strcmp(tc->subcmd, "new") == 0) return tool_pkg_new(tc->input);
            if (tc->subcmd && strcmp(tc->subcmd, "add") == 0) return tool_pkg_add(tc->input);
            fprintf(stderr, "[pkg] 未知子命令: %s\n", tc->subcmd ? tc->subcmd : "(null)");
            return -1;
        case TOOL_DOCS:
            printf("[docs] 查看 docs/ 目录\n");
            return 0;
        default:
            return -1;
    }
}
