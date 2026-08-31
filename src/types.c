/*
 * types.c - Pony++ 类型系统
 */

#include "ponypp/types.h"
#include "ponypp/util.h"

Type *type_new(TypeKind kind, const char *name) {
    Type *t = (Type *)calloc(1, sizeof(Type));
    if (!t) return NULL;
    t->kind = kind;
    if (name) t->name = s_strdup(name);
    t->ref_count = 1;
    return t;
}

Type *type_int64(void) { return type_new(TYPE_INT64, "I64"); }
Type *type_string(void) { return type_new(TYPE_STRING, "String"); }
Type *type_bool(void) { return type_new(TYPE_BOOL, "Bool"); }
Type *type_none(void) { return type_new(TYPE_NONE, "None"); }
Type *type_any(void) { return type_new(TYPE_ANY, "Any"); }

Type *type_generic(const char *name) {
    Type *t = type_new(TYPE_GENERIC, name);
    if (t) t->generic_arg_count = 0;
    return t;
}

Type *type_tuple(Type **types, size_t count) {
    Type *t = type_new(TYPE_TUPLE, "Tuple");
    if (!t) return NULL;
    t->tuple_types = (Type **)malloc(count * sizeof(Type *));
    t->tuple_count = count;
    for (size_t i = 0; i < count; i++) {
        if (types[i]) types[i]->ref_count++;
    }
    memcpy(t->tuple_types, types, count * sizeof(Type *));
    return t;
}

Type *type_union(Type **types, size_t count) {
    Type *t = type_new(TYPE_UNION, "Union");
    if (!t) return NULL;
    t->union_types = (Type **)malloc(count * sizeof(Type *));
    t->union_count = count;
    for (size_t i = 0; i < count; i++) {
        if (types[i]) types[i]->ref_count++;
    }
    memcpy(t->union_types, types, count * sizeof(Type *));
    return t;
}

Type *type_array(Type *element) {
    Type *t = type_new(TYPE_ARRAY, "Array");
    if (!t) return NULL;
    t->array_element = element;
    if (element) element->ref_count++;
    return t;
}

Type *type_generic_with_args(Type *base, Type **args, size_t arg_count) {
    if (!base) return NULL;
    base->ref_count++;
    Type *t = type_new(TYPE_GENERIC, base->name);
    if (!t) return NULL;
    t->generic_args = (Type **)malloc(arg_count * sizeof(Type *));
    t->generic_arg_count = arg_count;
    for (size_t i = 0; i < arg_count; i++) {
        if (args[i]) args[i]->ref_count++;
    }
    memcpy(t->generic_args, args, arg_count * sizeof(Type *));
    return t;
}

void type_free(Type *t) {
    if (!t) return;
    if (t->name) free(t->name);
    if (t->generic_args) {
        for (size_t i = 0; i < t->generic_arg_count; i++) {
            if (t->generic_args[i]) t->generic_args[i]->ref_count--;
        }
        free(t->generic_args);
    }
    if (t->tuple_types) {
        for (size_t i = 0; i < t->tuple_count; i++) {
            if (t->tuple_types[i]) t->tuple_types[i]->ref_count--;
        }
        free(t->tuple_types);
    }
    if (t->union_types) {
        for (size_t i = 0; i < t->union_count; i++) {
            if (t->union_types[i]) t->union_types[i]->ref_count--;
        }
        free(t->union_types);
    }
    if (t->array_element) t->array_element->ref_count--;
    free(t);
}

void type_release(Type *t) {
    if (!t) return;
    t->ref_count--;
    if (t->ref_count <= 0) type_free(t);
}

bool type_equals(const Type *a, const Type *b) {
    if (!a || !b) return a == b;
    if (a->kind != b->kind) return false;
    if (a->name && b->name) return strcmp(a->name, b->name) == 0;
    return true;
}

bool type_is_sendable(const Type *t) {
    if (!t) return false;
    /* val, iso, tag 是可发送类型 */
    return t->kind == TYPE_STRING || t->kind == TYPE_BOOL ||
           t->kind == TYPE_INT64 || t->kind == TYPE_UINT64 ||
           t->kind == TYPE_NONE;
}

bool type_is_immutable(const Type *t) {
    if (!t) return false;
    return t->kind == TYPE_STRING || t->kind == TYPE_BOOL ||
           t->kind == TYPE_INT64 || t->kind == TYPE_UINT64 ||
           t->kind == TYPE_NONE;
}

void type_print(const Type *t, char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;
    if (!t) {
        snprintf(buf, buf_size, "Unknown");
        return;
    }
    if (t->name) {
        snprintf(buf, buf_size, "%s", t->name);
    } else {
        snprintf(buf, buf_size, "Unknown");
    }
}

TypeContext type_context_new(void) {
    TypeContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.builtin_types[TYPE_INT64] = type_int64();
    ctx.builtin_types[TYPE_STRING] = type_string();
    ctx.builtin_types[TYPE_BOOL] = type_bool();
    ctx.builtin_types[TYPE_NONE] = type_none();
    ctx.builtin_types[TYPE_ANY] = type_any();
    ctx.unknown = type_new(TYPE_UNKNOWN, "Unknown");
    return ctx;
}

void type_context_free(TypeContext *ctx) {
    for (int i = 0; i < TYPE_COUNT; i++) {
        if (ctx->builtin_types[i]) {
            type_release(ctx->builtin_types[i]);
            ctx->builtin_types[i] = NULL;
        }
    }
    if (ctx->unknown) {
        type_release(ctx->unknown);
        ctx->unknown = NULL;
    }
}
