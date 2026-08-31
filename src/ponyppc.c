/*
 * ponyppc - Pony++ C 语言编译器
 * 编译 Pony++ 源代码为 Wasm 组件
 */

#include "ponypp.h"
#include "ponypp/ast.h"
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/types.h"
#include "ponypp/capabilities.h"
#include "ponypp/wasm.h"
#include "ponypp/wit.h"
#include "ponypp/util.h"
#include "ponypp/typecheck.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

typedef enum {
    OPT_DEFAULT,
    OPT_DEBUG,
    OPT_VERSION
} OptMode;

static void print_usage(const char *prog) {
    printf("Pony++ C 语言编译器 (ponyppc) v%s\n", VERSION_STRING);
    printf("\n用法: %s [选项] <源文件>\n", prog);
    printf("\n选项:\n");
    printf("  -o <输出>    指定输出 Wasm 文件名 (默认 <源文件名>.wasm)\n");
    printf("  -g           生成调试信息 (WASM_GC_DEBUG)\n");
    printf("  -O <level>   优化级别 (0-4, 默认 2)\n");
    printf("  --wit-only   仅生成 WIT 接口定义文件\n");
    printf("  --target     目标后端 (wasi-p2|component|browser|mcu-wasm|native, 默认 wasi-p2)\n");
    printf("  --ast        输出 AST\n");
    printf("  --ast-dot    输出 DOT 格式的 AST 图\n");
    printf("  --pretty     美化输出 (pretty print)\n");
    printf("  --version    显示版本信息\n");
    printf("  -h, --help   显示帮助\n");
    printf("\n示例:\n");
    printf("  %s -O2 -o hello.wasm hello.pny\n", prog);
    printf("  %s -g --pretty hello.pny\n", prog);
    printf("  %s --wit-only hello.pny\n", prog);
}

static void print_version(void) {
    printf("ponyppc version %s\n", VERSION_STRING);
    printf("编译器语言: C11 (gcc %d.%d.%d / clang)\n",
           __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    printf("Wasm 运行时: Wasmtime 47+ / Wasmer\n");
    printf("WASI: Preview 2 + Preview 3 (0.3 async)\n");
    printf("Wasm GC: Cheney 半空间复制 (默认启用)\n");
}

static char *resolve_output(const char *input, const char *opt_output) {
    if (opt_output) {
        return s_strdup(opt_output);
    }
    /* 默认: <输入>.wasm */
    size_t len = s_strlen(input);
    const char *ext = strrchr(input, '.');
    if (ext) {
        char *out = s_malloc(len);
        s_memcpy(out, input, len);
        s_memcpy(out + len - 3, "wasm", 5);
        return out;
    }
    size_t total = len + 6;
    char *out = s_malloc(total);
    s_memset(out, 0, total);
    s_memcpy(out, input, len);
    s_strcpy(out + len, ".wasm");
    return out;
}

static TargetKind parse_target(const char *s) {
    if (!s) return TARGET_WASI_P2;
    if (strcmp(s, "wasi-p2") == 0) return TARGET_WASI_P2;
    if (strcmp(s, "component") == 0) return TARGET_COMPONENT;
    if (strcmp(s, "browser") == 0) return TARGET_BROWSER;
    if (strcmp(s, "mcu-wasm") == 0) return TARGET_MCU_WASM;
    if (strcmp(s, "native") == 0) return TARGET_NATIVE;
    return TARGET_WASI_P2;
}

static const char *target_name(TargetKind t) {
    switch (t) {
        case TARGET_WASI_P2: return "wasi-p2";
        case TARGET_COMPONENT: return "component";
        case TARGET_BROWSER: return "browser";
        case TARGET_MCU_WASM: return "mcu-wasm";
        case TARGET_NATIVE: return "native";
        default: return "wasi-p2";
    }
}

static int compile_file(const char *input_path, CompilerConfig *cfg) {
    printf("[ponyppc] target=%s\n", target_name(cfg->target));
    char *source = s_file_read(input_path);
    if (!source) {
        fprintf(stderr, "错误: 无法读取文件 '%s': %s\n",
                input_path, strerror(errno));
        return EXIT_FAILURE;
    }

    size_t len = s_strlen(source);
    printf("[ponyppc] 编译 '%s' (%zu 字节)\n", input_path, len);

    /* 1. 词法分析 */
    printf("  [1/6] 词法分析...\n");
    Lexer *lexer = lexer_new(input_path, source, len);
    if (!lexer) {
        fprintf(stderr, "错误: 词法分析器创建失败\n");
        s_free(source);
        return EXIT_FAILURE;
    }

    Token *tokens = NULL;
    size_t token_count = 0;
    bool ok = lexer_lex_all(lexer, &tokens, &token_count);
    if (!ok) {
        fprintf(stderr, "词法错误 (第 %d 行): %s\n",
                lexer_line(lexer), lexer_error(lexer));
        lexer_free(lexer);
        s_free(source);
        return EXIT_FAILURE;
    }

    if (cfg->emit_ast_tokens) {
        for (size_t i = 0; i < token_count; i++) {
            token_print(&tokens[i], stderr);
        }
    }

    /* 2. 语法分析 */
    printf("  [2/6] 语法分析...\n");
    Parser *parser = parser_new(input_path, tokens, token_count);
    if (!parser) {
        fprintf(stderr, "错误: 语法分析器创建失败\n");
        lexer_free(lexer);
        s_free(source);
        return EXIT_FAILURE;
    }

    ASTNode *ast = parser_parse_program(parser);
    if (!ast) {
        fprintf(stderr, "语法错误 (第 %d 行): %s\n",
                parser_line(parser), parser_error(parser));
        parser_free(parser);
        lexer_free(lexer);
        s_free(source);
        return EXIT_FAILURE;
    }

    if (cfg->emit_ast) {
        ast_node_print(ast, stderr);
    }

    if (cfg->emit_ast_dot) {
        ast_node_print_dot(ast, stderr);
    }

    /* 3. 类型检查 */
    printf("  [3/6] 类型检查...\n");
    TypeContext ctx = type_context_new();
    bool type_ok = typecheck_check_ast(ast, &ctx);
    if (!type_ok) {
        fprintf(stderr, "类型错误: %s (第 %d 行)\n",
                typecheck_last_error(&ctx), typecheck_last_line(&ctx));
        type_context_free(&ctx);
        parser_free(parser);
        lexer_free(lexer);
        s_free(source);
        return EXIT_FAILURE;
    }

    /* 4. 引用能力验证 */
    printf("  [4/6] 引用能力验证...\n");
    bool cap_ok = cap_verify_ast(ast);
    if (!cap_ok) {
        fprintf(stderr, "引用能力错误: %s (第 %d 行)\n",
                cap_last_error(), cap_last_line());
        type_context_free(&ctx);
        parser_free(parser);
        lexer_free(lexer);
        s_free(source);
        return EXIT_FAILURE;
    }

    /* 5. 代码生成 (WIT 或 Wasm) */
    printf("  [5/6] 代码生成...\n");
    if (cfg->wit_only) {
        char *wit_output = s_resolve_output(cfg->output, "wit");
        WITWriter *wit = wit_writer_new(wit_output);
        wit_write_program(wit, ast);
        wit_writer_close(wit);
        s_free(wit_output);
        printf("  已生成 WIT 接口定义: %s\n", wit_output);
    } else if (cfg->target == TARGET_NATIVE) {
        size_t out_len = cfg->output ? s_strlen(cfg->output) : 0;
        char *c_output = NULL;
        char *binary_output = cfg->output ? s_strdup(cfg->output) : s_strdup("ponypp-native");
        char *print_name = s_strdup(binary_output);
        if (out_len > 0) {
            c_output = s_malloc(out_len + 4);
            s_memcpy(c_output, cfg->output, out_len);
            s_strcpy(c_output + out_len, ".c");
        } else {
            c_output = s_strdup("ponypp-native.c");
        }
        FILE *sf = fopen(c_output, "w");
        if (!sf) {
            s_free(print_name);
            s_free(binary_output);
            s_free(c_output);
            fprintf(stderr, "错误: 无法写入 native 源文件\n");
            return EXIT_FAILURE;
        }
        fprintf(sf, "/* Pony++ native stub */\n");
        fprintf(sf, "#include <stdio.h>\n\n");
        fprintf(sf, "int main(void) {\n");
        fprintf(sf, "    printf(\"Hello from Pony++ native\\n\");\n");
        fprintf(sf, "    return 0;\n");
        fprintf(sf, "}\n");
        fclose(sf);
        char cmdbuf[4096];
        int cmdlen = snprintf(cmdbuf, sizeof(cmdbuf), "gcc -o %s %s", binary_output, c_output);
        if (cmdlen <= 0 || cmdlen >= (int)sizeof(cmdbuf) || system(cmdbuf) != 0) {
            s_free(print_name);
            s_free(binary_output);
            s_free(c_output);
            fprintf(stderr, "错误: native 编译失败\n");
            return EXIT_FAILURE;
        }
        s_free(binary_output);
        s_free(c_output);
        printf("  已生成 native 可执行文件: %s\n", print_name);
        s_free(print_name);
    } else {
        char *output_path = s_resolve_output(cfg->output, "wasm");
        WasmWriter *writer = wasm_writer_new(output_path);
        wasm_write_program(writer, ast, cfg);
        wasm_writer_close(writer);
        s_free(output_path);
        printf("  已生成 Wasm 组件: %s\n", cfg->output ? cfg->output : "default.wasm");
    }

    /* 6. 清理 */
    printf("  [6/6] 清理...\n");
    ast_node_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    s_free(source);
    type_context_free(&ctx);

    printf("[ponyppc] 编译成功 ✓\n");
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    CompilerConfig cfg = {
        .output = NULL,
        .emit_ast = false,
        .emit_ast_dot = false,
        .emit_ast_tokens = false,
        .pretty_print = false,
        .wit_only = false,
        .optimize = OPT_OPTIMIZE_2,
        .emit_debug = false,
        .mode = OPT_DEFAULT
    };

    struct option longopts[] = {
        {"wit-only", no_argument, 0, 'W'},
        {"target",   required_argument, 0, 'T'},
        {"ast",      no_argument, 0, 'A'},
        {"ast-dot",  no_argument, 0, 'D'},
        {"pretty",   no_argument, 0, 'P'},
        {"version",  no_argument, 0, 'V'},
        {"help",     no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:gO:hWADPVT:", longopts, NULL)) != -1) {
        switch (opt) {
            case 'o':
                cfg.output = optarg;
                break;
            case 'g':
                cfg.emit_debug = true;
                cfg.mode = OPT_DEBUG;
                break;
            case 'O':
                cfg.optimize = (OptLevel)atoi(optarg);
                break;
            case 'W':
                cfg.wit_only = true;
                break;
            case 'T':
                cfg.target = parse_target(optarg);
                break;
            case 'A':
                cfg.emit_ast = true;
                break;
            case 'D':
                cfg.emit_ast_dot = true;
                break;
            case 'P':
                cfg.pretty_print = true;
                break;
            case 'V':
                print_version();
                return EXIT_SUCCESS;
            case 'h':
            case '?':
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "错误: 缺少输入文件\n\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    char *input_path = argv[optind];

    /* 检查文件扩展名 */
    const char *ext = strrchr(input_path, '.');
    if (!ext || (s_strcmp(ext, ".pny") != 0 && s_strcmp(ext, ".ponypp") != 0)) {
        fprintf(stderr, "警告: 输入文件扩展名不是 '.pny' 或 '.ponypp'，尝试编译\n");
    }

    int ret = compile_file(input_path, &cfg);
    return ret;
}