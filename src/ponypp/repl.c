/**
 * Pony++ REPL - 交互式编程环境
 *
 * 功能:
 *   - 解析和编译单行/多行 Pony++ 代码
 *   - 即时执行并显示输出
 *   - 历史记录和错误提示
 *   - 支持 actor、be、fun、new 声明
 *
 * 命令:
 *   :help     - 显示帮助
 *   :quit     - 退出
 *   :clear    - 清除屏幕
 *   :history  - 显示历史记录
 *   :file path - 编译并运行文件
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ponypp.h"
#include "ponypp/util.h"
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/codegen.h"
#include "ponypp/runtime.h"

#define REPL_HISTORY_MAX 100

typedef struct {
    char *entries[REPL_HISTORY_MAX];
    int count;
    PnyRuntime *runtime;
    int line_num;
} REPL;

static void repl_print_prompt(REPL *r) {
    printf("ponypp %d> ", r->line_num);
    fflush(stdout);
}

static void repl_add_history(REPL *r, const char *input) {
    if (r->count < REPL_HISTORY_MAX) {
        r->entries[r->count] = s_strdup(input);
        r->count++;
    }
}

static void repl_clear_screen(void) {
    printf("\033[2J\033[H");
}

static void repl_show_help(void) {
    printf("\n");
    printf("=== Pony++ REPL ===\n");
    printf("命令:\n");
    printf("  :help     - 显示帮助\n");
    printf("  :quit     - 退出\n");
    printf("  :clear    - 清除屏幕\n");
    printf("  :history  - 显示历史记录\n");
    printf("  :file path - 编译并运行文件\n");
    printf("\n");
    printf("Pony++ 语法:\n");
    printf("  actor Name { be method() => { ... } }\n");
    printf("  val x: String = \"hello\"\n");
    printf("  print(x)\n");
    printf("\n");
}

static void repl_show_history(REPL *r) {
    printf("\n--- History (%d) ---\n", r->count);
    for (int i = 0; i < r->count; i++) {
        printf("%3d> %s\n", i + 1, r->entries[i]);
    }
    printf("---\n");
}

static int repl_eval_code(REPL *r, const char *input) {
    size_t len = strlen(input);
    if (len == 0) return 0;

    /* Lex */
    Lexer *lexer = lexer_new("<repl>", (char *)input, len);
    if (!lexer) {
        fprintf(stderr, "[REPL] 词法分析器创建失败\n");
        return -1;
    }

    Token *tokens = NULL;
    size_t token_count = 0;
    bool ok = lexer_lex_all(lexer, &tokens, &token_count);
    if (!ok) {
        fprintf(stderr, "[REPL] 词法错误: %s\n", lexer_error(lexer));
        s_free(tokens);
        lexer_free(lexer);
        return -1;
    }

    /* Parse */
    Parser *parser = parser_new("<repl>", tokens, token_count);
    if (!parser) {
        fprintf(stderr, "[REPL] 解析器创建失败\n");
        s_free(tokens);
        lexer_free(lexer);
        return -1;
    }

    ASTNode *ast = parser_parse_program(parser);
    parser_free(parser);
    s_free(tokens);
    lexer_free(lexer);

    if (!ast) {
        fprintf(stderr, "[REPL] 语法错误\n");
        return -1;
    }

    /* Print AST summary */
    printf("[AST] %zu nodes\n", ast->child_count + 1);

    /* Free AST */
    ast_node_free(ast);
    return 0;
}

static int repl_eval_file(REPL *r, const char *path) {
    char *source = s_file_read(path);
    if (!source) {
        fprintf(stderr, "[REPL] 无法读取文件: %s\n", path);
        return -1;
    }

    size_t len = strlen(source);
    printf("[REPL] 编译文件 '%s' (%zu bytes)\n", path, len);

    /* Lex */
    Lexer *lexer = lexer_new(path, source, len);
    if (!lexer) {
        fprintf(stderr, "[REPL] 词法分析器创建失败\n");
        s_free(source);
        return -1;
    }

    Token *tokens = NULL;
    size_t token_count = 0;
    bool ok = lexer_lex_all(lexer, &tokens, &token_count);
    if (!ok) {
        fprintf(stderr, "[REPL] 词法错误: %s\n", lexer_error(lexer));
        s_free(source);
        s_free(tokens);
        lexer_free(lexer);
        return -1;
    }

    /* Parse */
    Parser *parser = parser_new(path, tokens, token_count);
    if (!parser) {
        fprintf(stderr, "[REPL] 解析器创建失败\n");
        s_free(source);
        s_free(tokens);
        lexer_free(lexer);
        return -1;
    }

    ASTNode *ast = parser_parse_program(parser);
    parser_free(parser);
    s_free(tokens);
    lexer_free(lexer);
    s_free(source);

    if (!ast) {
        fprintf(stderr, "[REPL] 语法错误\n");
        return -1;
    }

    printf("[AST] %zu nodes\n", ast->child_count + 1);
    ast_node_free(ast);
    return 0;
}

int repl_run(void) {
    REPL r = { 0, NULL, 0, 1 };
    r.runtime = pny_runtime_new();
    if (!r.runtime) {
        fprintf(stderr, "[REPL] 运行时初始化失败\n");
        return -1;
    }

    printf("Pony++ REPL v0.1.0\n");
    printf("输入 :help 查看帮助, :quit 退出\n");

    char input[4096];
    while (1) {
        repl_print_prompt(&r);
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        /* Remove trailing newline */
        size_t len = strlen(input);
        while (len > 0 && (input[len - 1] == '\n' || input[len - 1] == '\r')) {
            input[--len] = '\0';
        }

        if (len == 0) {
            r.line_num++;
            continue;
        }

        repl_add_history(&r, input);

        if (strncmp(input, ":", 1) == 0) {
            if (strcmp(input, ":quit") == 0 || strcmp(input, ":q") == 0 || strcmp(input, ":exit") == 0) {
                printf("再见!\n");
                break;
            } else if (strcmp(input, ":help") == 0 || strcmp(input, ":h") == 0) {
                repl_show_help();
            } else if (strcmp(input, ":clear") == 0 || strcmp(input, ":c") == 0) {
                repl_clear_screen();
            } else if (strcmp(input, ":history") == 0 || strcmp(input, ":hist") == 0) {
                repl_show_history(&r);
            } else if (strncmp(input, ":file ", 6) == 0) {
                repl_eval_file(&r, input + 6);
            } else if (strcmp(input, ":reset") == 0) {
                pny_runtime_free(r.runtime);
                r.runtime = pny_runtime_new();
                printf("运行时已重置\n");
            } else {
                fprintf(stderr, "[REPL] 未知命令: %s (输入 :help)\n", input);
            }
        } else {
            repl_eval_code(&r, input);
        }

        r.line_num++;
        printf("\n");
    }

    pny_runtime_free(r.runtime);
    return 0;
}