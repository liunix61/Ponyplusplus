#ifndef PONYPP_H
#define PONYPP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define PONYPP_SOURCE_DIR "/home/liunix/Ponyplusplus"
#define VERSION_STRING "0.1.0"

/* 类型种类 */
typedef enum {
    TYPE_UNKNOWN,
    TYPE_INT64,
    TYPE_INT32,
    TYPE_UINT64,
    TYPE_UINT32,
    TYPE_UINT8,
    TYPE_FLOAT64,
    TYPE_FLOAT32,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_NONE,
    TYPE_ANY,
    TYPE_GENERIC,
    TYPE_TUPLE,
    TYPE_UNION,
    TYPE_ARRAY,
    TYPE_COUNT  /* sentinel */
} TypeKind;

/* 引用能力类型 */
typedef enum {
    CAP_UNKNOWN,
    CAP_ISO,    /* 唯一可变引用 */
    CAP_TRN,    /* 转移引用 */
    CAP_REF,    /* 本地可变 */
    CAP_VAL,    /* 全局不可变 */
    CAP_BOX,    /* 本地只读 */
    CAP_TAG     /* 仅标识 */
} CapabilityKind;

/* Token 类型 */
typedef enum {
    TK_EOF,
    TK_KEYWORD,   /* actor, class, trait, be, fun, new, var, let, if, else, while, for, match, return, supervise */
    TK_IDENT,     /* 标识符 */
    TK_INT,       /* 整数字面量 */
    TK_FLOAT,     /* 浮点字面量 */
    TK_STRING,    /* 字符串字面量 */
    TK_BOOL,      /* true/false */
    TK_TYPE,      /* U64, I64, String, Bool, None 等类型 */
    TK_CAP,       /* iso, trn, ref, val, box, tag */
    TK_PUNCT,     /* ( ) { } . , ; => - + * / = ! < > <= >= == != */
    TK_ARROW,     /* => 箭头 */
    TK_RANGE,     /* .. 范围 */
    TK_HASH,      /* # */
    TK_DOLLAR,    /* $ */
    TK_AMP,       /* & */
    TK_PERCENT,   /* % */
    TK_AT,        /* @ */
    TK_PIPE,      /* | */
    TK_QUESTION,  /* ? */
    TK_COLON,     /* : */
    TK_BRACKET_L, /* [ */
    TK_BRACKET_R, /* ] */
    TK_BRACE_L,   /* { */
    TK_BRACE_R,   /* } */
    TK_PAREN_L,   /* ( */
    TK_PAREN_R,   /* ) */
    TK_SEMI,      /* ; */
    TK_COMMA,     /* , */
    TK_DOT,       /* . */
    TK_DASH,      /* - */
    TK_PLUS,      /* + */
    TK_STAR,      /* * */
    TK_SLASH,     /* / */
    TK_EQ,        /* = */
    TK_BANG,      /* ! */
    TK_LT,        /* < */
    TK_GT,        /* > */
    TK_LE,        /* <= */
    TK_GE,        /* >= */
    TK_EQEQ,      /* == */
    TK_NEQ,       /* != */
    TK_COLONCOLON,/* :: */
    TK_ARROW_ARR /* => */
} TokenType;

/* Token 结构 */
typedef struct Token {
    TokenType type;
    char *value;
    int line;
    int column;
    int length;
} Token;

/* 优化级别 */
typedef enum {
    OPT_OPTIMIZE_0,
    OPT_OPTIMIZE_1,
    OPT_OPTIMIZE_2,
    OPT_OPTIMIZE_3,
    OPT_OPTIMIZE_4
} OptLevel;

/* 编译目标后端 */
typedef enum {
    TARGET_WASI_P2,
    TARGET_COMPONENT,
    TARGET_BROWSER,
    TARGET_MCU_WASM,
    TARGET_NATIVE
} TargetKind;

/* 编译器配置 */
typedef struct CompilerConfig {
    const char *output;
    bool emit_ast;
    bool emit_ast_dot;
    bool emit_ast_tokens;
    bool pretty_print;
    bool wit_only;
    OptLevel optimize;
    bool emit_debug;
    int mode;
    TargetKind target;
    const char *source_dir; /* 源码根目录，native backend 用 */
} CompilerConfig;

/* 前向声明 */
typedef struct Lexer Lexer;
typedef struct Parser Parser;
typedef struct Type Type;
typedef struct TypeContext TypeContext;
typedef struct WasmWriter WasmWriter;
typedef struct WITWriter WITWriter;
typedef struct ASTNode ASTNode;

#ifdef __cplusplus
}
#endif

/* REPL */
int repl_run(void);

#endif /* PONYPP_H */
