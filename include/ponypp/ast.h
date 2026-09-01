#ifndef PONYPP_AST_H
#define PONYPP_AST_H

#include "ponypp.h"

/* AST 节点类型 */
typedef enum {
    NODE_EMPTY,
    NODE_PROGRAM,       /* 顶层程序 */
    NODE_ACTOR,         /* Actor 声明 */
    NODE_CLASS,         /* Class 声明 */
    NODE_TRAIT,         /* Trait 声明 */
    NODE_INTERFACE,     /* Interface 声明 */
    NODE_BE,            /* Behavior 方法 */
    NODE_FUN,           /* Function 方法 */
    NODE_NEW,           /* Constructor */
    NODE_VAR,           /* 可变字段 */
    NODE_LET,           /* 不可变字段 */
    NODE_IF,            /* if 语句 */
    NODE_ELSE,          /* else 分支 */
    NODE_WHILE,         /* while 循环 */
    NODE_FOR,           /* for 循环 */
    NODE_MATCH,         /* 模式匹配 */
    NODE_RETURN,        /* return 语句 */
    NODE_PRINT,         /* 打印语句 */
    NODE_ASSERT,        /* 断言 */
    NODE_SUPERVISE,     /* 监督声明 */
    NODE_SUPERTREE,     /* 监督树 */
    NODE_YIELD,         /* yield */
    NODE_SEND,          /* 消息发送 */
    NODE_CALL,          /* 方法调用 */
    NODE_IDENT,         /* 标识符 / 变量名 */
    NODE_LAMBDA,        /* Lambda */
    NODE_LIST,          /* 列表字面量 */
    NODE_MAP,           /* 映射字面量 */
    NODE_STRING,        /* 字符串 */
    NODE_INT,           /* 整数 */
    NODE_FLOAT,         /* 浮点数 */
    NODE_BOOL,          /* 布尔值 */
    NODE_UNION,         /* Union 类型 */
    NODE_TUPLE,         /* 元组类型 */
    NODE_ARRAY,         /* 数组类型 */
    NODE_ALIAS,         /* 类型别名 */
} ASTNodeType;

/* AST 节点 */
typedef struct ASTNode ASTNode;
struct ASTNode {
    ASTNodeType type;
    void *data;           /* 节点特定数据 */
    ASTNode **children;
    size_t child_count;
    size_t child_cap;
    int line;
    int column;
    char *source_loc;     /* 源码位置描述 */
    int ref_count;
};

/* 程序节点 */
typedef struct {
    ASTNode *actors;      /* Actor 声明列表 */
    ASTNode *classes;     /* Class 声明列表 */
    ASTNode *traits;      /* Trait 声明列表 */
    ASTNode *interfaces;  /* Interface 声明列表 */
    size_t actor_count;
    size_t class_count;
} Program;

/* Actor 节点 */
typedef struct {
    char *name;
    ASTNode **fields;     /* 字段列表 */
    ASTNode **methods;    /* 方法列表 */
    ASTNode **constructors; /* 构造函数列表 */
    size_t field_count;
    size_t method_count;
    size_t constructor_count;
} Actor;

/* 方法节点 (be/fun) */
typedef struct {
    char *name;
    ASTNode *params;      /* 参数列表 */
    ASTNode *return_type; /* 返回类型 */
    ASTNode *body;        /* 方法体 */
    CapabilityKind capability; /* 引用能力 */
} Method;

/* 字段节点 (var/let) */
typedef struct {
    char *name;
    ASTNode *type;        /* 类型 */
    CapabilityKind capability; /* 引用能力 */
    ASTNode *init;        /* 初始值 */
} Field;

/* 参数节点 */
typedef struct {
    char *name;
    ASTNode *type;
    CapabilityKind capability;
    ASTNode *default_value; /* 默认值 */
} Param;

/* 构造函数节点 (new) */
typedef struct {
    char *name;
    ASTNode *params;      /* 参数列表 */
    ASTNode *body;        /* 构造体 */
    CapabilityKind capability;
} Constructor;

/* 监督声明 */
typedef struct {
    ASTNode *target;      /* 被监督的 Actor */
    char *strategy;       /* 重启策略 */
    int max_restarts;
    int interval;         /* 重启间隔（秒） */
} SuperviseDecl;

/* 字符串字面量 */
typedef struct {
    char *value;
} StringLit;

/* 整数字面量 */
typedef struct {
    uint64_t value;
} IntLit;

/* 浮点字面量 */
typedef struct {
    double value;
} FloatLit;

/* 布尔字面量 */
typedef struct {
    bool value;
} BoolLit;

/* 方法调用 */
typedef struct {
    ASTNode *callee;      /* 被调用者 */
    char *method_name;    /* 方法名 */
    ASTNode **args;       /* 参数列表 */
    size_t arg_count;
    bool is_async;        /* 是否异步调用 */
} MethodCall;

/* 消息发送 */
typedef struct {
    ASTNode *target;      /* 目标 Actor */
    char *method_name;    /* 方法名 */
    ASTNode **args;       /* 参数列表 */
    size_t arg_count;
} MessageSend;

/* 创建 AST 节点 */
ASTNode *ast_node_new(ASTNodeType type, int line, int column);
void ast_node_free(ASTNode *node);
void ast_node_add_child(ASTNode *parent, ASTNode *child);
void ast_node_release(ASTNode *node);

/* 创建具体节点 */
ASTNode *ast_program_new(int line, int column);
ASTNode *ast_actor_new(const char *name, int line, int column);
ASTNode *ast_method_new(const char *name, bool is_be, int line, int column);
ASTNode *ast_field_new(const char *name, bool is_var, int line, int column);
ASTNode *ast_string_new(const char *value, int line, int column);
ASTNode *ast_int_new(uint64_t value, int line, int column);
ASTNode *ast_bool_new(bool value, int line, int column);
ASTNode *ast_call_new(ASTNode *callee, const char *method, bool async, int line, int column);

/* 释放辅助 */
void ast_actor_free(Actor *actor);
void ast_method_free(Method *method);
void ast_field_free(Field *field);

#endif /* PONYPP_AST_H */