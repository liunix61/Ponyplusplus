#ifndef PONYPP_TYPES_H
#define PONYPP_TYPES_H

#include "ponypp.h"
#include "ast.h"

/* 类型节点 */
typedef struct Type Type;
struct Type {
    TypeKind kind;
    char *name;
    Type **generic_args;
    size_t generic_arg_count;
    Type **tuple_types;    /* 元组元素类型 */
    size_t tuple_count;
    Type **union_types;    /* Union 成员类型 */
    size_t union_count;
    Type *array_element;   /* 数组元素类型 */
    int ref_count;
};

/* 类型上下文 */
typedef struct TypeContext {
    Type *builtin_types[TYPE_COUNT];
    Type *unknown;
} TypeContext;

/* 创建内置类型 */
Type *type_new(TypeKind kind, const char *name);
Type *type_int64(void);
Type *type_string(void);
Type *type_bool(void);
Type *type_none(void);
Type *type_any(void);
Type *type_generic(const char *name);
Type *type_tuple(Type **types, size_t count);
Type *type_union(Type **types, size_t count);
Type *type_array(Type *element);
Type *type_generic_with_args(Type *base, Type **args, size_t arg_count);

/* 释放类型 */
void type_free(Type *t);
void type_release(Type *t);

/* 类型操作 */
bool type_equals(const Type *a, const Type *b);
bool type_is_sendable(const Type *t);
bool type_is_immutable(const Type *t);
const char *type_kind_name(TypeKind kind);

/* 初始化类型上下文 */
TypeContext type_context_new(void);
void type_context_free(TypeContext *ctx);

/* 类型显示 */
void type_print(const Type *t, char *buf, size_t buf_size);

#endif /* PONYPP_TYPES_H */