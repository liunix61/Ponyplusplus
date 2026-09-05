/**
 * Pony++ FFI - 外部函数互操作
 *
 * 支持:
 * - 导入 C 函数 (extern "C" 函数指针)
 * - 导出 Pony++ 函数供外部调用
 * - Rust/Go 互操作桥接
 */
#include "ponypp/tool.h"
#include "ponypp/runtime.h"
#include "ponypp/util.h"
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

/* FFI 类型标记 */
typedef enum {
    FFI_TYPE_I32,
    FFI_TYPE_I64,
    FFI_TYPE_U32,
    FFI_TYPE_U64,
    FFI_TYPE_F64,
    FFI_TYPE_STRING,
    FFI_TYPE_PTR,
    FFI_TYPE_VOID,
    FFI_TYPE_STRUCT,
    FFI_TYPE_COUNT
} FFIType;

/* FFI 函数签名 */
typedef struct FFIParam {
    const char *name;
    FFIType type;
} FFIParam;

typedef struct FFIFunc {
    char *name;
    void *func_ptr;
    FFIParam *params;
    size_t param_count;
    FFIType return_type;
    char *source_module;  /* 来源: "C", "Rust", "Go" */
} FFIFunc;

/* FFI 注册表 */
typedef struct FFIRuntime {
    FFIFunc **funcs;
    size_t func_count;
    size_t func_cap;
    void **modules;       /* dlopen 句柄 */
    size_t module_count;
    size_t module_cap;
} FFIRuntime;

static FFIRuntime g_ffi_runtime;

/* ======================== FFI 运行时 ======================== */

FFIRuntime *ffi_runtime_new(void) {
    memset(&g_ffi_runtime, 0, sizeof(g_ffi_runtime));
    g_ffi_runtime.func_cap = 16;
    g_ffi_runtime.funcs = (FFIFunc **)calloc(g_ffi_runtime.func_cap, sizeof(FFIFunc *));
    g_ffi_runtime.module_cap = 8;
    g_ffi_runtime.modules = (void **)calloc(g_ffi_runtime.module_cap, sizeof(void *));
    return g_ffi_runtime.funcs ? &g_ffi_runtime : NULL;
}

void ffi_runtime_free(void) {
    FFIRuntime *rt = &g_ffi_runtime;
    for (size_t i = 0; i < rt->func_count; i++) {
        FFIFunc *f = rt->funcs[i];
        if (f) {
            free(f->name);
            free(f->source_module);
            for (size_t j = 0; j < f->param_count; j++) {
                free((void *)f->params[j].name);
            }
            free(f->params);
            free(f);
        }
    }
    free(rt->funcs);
    /* 关闭加载的模块 */
    for (size_t i = 0; i < rt->module_count; i++) {
        if (rt->modules[i]) dlclose(rt->modules[i]);
    }
    free(rt->modules);
    memset(rt, 0, sizeof(FFIRuntime));
}

/* ======================== 模块加载 ======================== */

int ffi_load_module(const char *path) {
    if (!path) return -1;
    void *handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
        return -1;
    }
    FFIRuntime *rt = &g_ffi_runtime;
    if (rt->module_count >= rt->module_cap) {
        rt->module_cap *= 2;
        rt->modules = (void **)realloc(rt->modules, rt->module_cap * sizeof(void *));
        if (!rt->modules) { dlclose(handle); return -2; }
    }
    rt->modules[rt->module_count++] = handle;
    return 0;
}

int ffi_register_func(const char *name, const char *module, FFIType return_type,
                       const char *param_names[], FFIType *param_types, size_t param_count) {
    if (!name || !module) return -1;
    FFIRuntime *rt = &g_ffi_runtime;

    /* 查找函数指针 */
    void *func_ptr = NULL;
    for (size_t i = 0; i < rt->module_count; i++) {
        func_ptr = dlsym(rt->modules[i], name);
        if (func_ptr) break;
    }
    if (!func_ptr) {
        /* 也尝试从当前进程查找 */
        func_ptr = dlsym(RTLD_DEFAULT, name);
    }
    if (!func_ptr) return -2;

    /* 创建 FFIFunc */
    FFIFunc *ff = (FFIFunc *)calloc(1, sizeof(FFIFunc));
    if (!ff) return -3;
    ff->name = strdup(name);
    ff->source_module = strdup(module);
    ff->func_ptr = func_ptr;
    ff->return_type = return_type;
    ff->param_count = param_count;
    if (param_count > 0 && param_names) {
        ff->params = (FFIParam *)calloc(param_count, sizeof(FFIParam));
        if (!ff->params) { free(ff->name); free(ff->source_module); free(ff); return -4; }
        for (size_t i = 0; i < param_count; i++) {
            ff->params[i].name = strdup(param_names[i] ? param_names[i] : "");
            ff->params[i].type = param_types[i];
        }
    }

    if (rt->func_count >= rt->func_cap) {
        rt->func_cap *= 2;
        rt->funcs = (FFIFunc **)realloc(rt->funcs, rt->func_cap * sizeof(FFIFunc *));
        if (!rt->funcs) {
            free(ff->name); free(ff->source_module); free(ff->params); free(ff);
            return -5;
        }
    }
    rt->funcs[rt->func_count++] = ff;
    return 0;
}

FFIFunc *ffi_find_func(const char *name) {
    if (!name) return NULL;
    FFIRuntime *rt = &g_ffi_runtime;
    for (size_t i = 0; i < rt->func_count; i++) {
        if (strcmp(rt->funcs[i]->name, name) == 0) return rt->funcs[i];
    }
    return NULL;
}

size_t ffi_func_count(void) {
    return g_ffi_runtime.func_count;
}

/* ======================== 调用 FFI 函数 ======================== */

int64_t ffi_call_i64(FFIFunc *ff, int64_t a, int64_t b, int64_t c, int64_t d) {
    if (!ff || !ff->func_ptr) return -1;
    typedef int64_t (*ffifn)(int64_t, int64_t, int64_t, int64_t);
    ffifn fn = (ffifn)ff->func_ptr;
    return fn(a, b, c, d);
}

double ffi_call_f64(FFIFunc *ff, double a, double b) {
    if (!ff || !ff->func_ptr) return -1.0;
    typedef double (*ffifn)(double, double);
    ffifn fn = (ffifn)ff->func_ptr;
    return fn(a, b);
}

void *ffi_call_ptr(FFIFunc *ff, void *arg, int64_t a) {
    if (!ff || !ff->func_ptr) return NULL;
    typedef void *(*ffifn)(void *, int64_t);
    ffifn fn = (ffifn)ff->func_ptr;
    return fn(arg, a);
}

/* ======================== FFI 序列化描述 ======================== */

const char *ffi_type_name(FFIType t) {
    switch (t) {
        case FFI_TYPE_I32: return "i32";
        case FFI_TYPE_I64: return "i64";
        case FFI_TYPE_U32: return "u32";
        case FFI_TYPE_U64: return "u64";
        case FFI_TYPE_F64: return "f64";
        case FFI_TYPE_STRING: return "string";
        case FFI_TYPE_PTR: return "ptr";
        case FFI_TYPE_VOID: return "void";
        case FFI_TYPE_STRUCT: return "struct";
        default: return "unknown";
    }
}

int ffi_dump(char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) return 0;
    size_t pos = 0;
    pos += snprintf(buf + pos, buf_size - pos, "Pony++ FFI Registry\n");
    pos += snprintf(buf + pos, buf_size - pos, "===================\n");
    pos += snprintf(buf + pos, buf_size - pos, "Functions: %zu\n", g_ffi_runtime.func_count);
    pos += snprintf(buf + pos, buf_size - pos, "Modules: %zu\n", g_ffi_runtime.module_count);
    for (size_t i = 0; i < g_ffi_runtime.func_count && pos < buf_size - 1; i++) {
        FFIFunc *f = g_ffi_runtime.funcs[i];
        pos += snprintf(buf + pos, buf_size - pos, "  %s(", f->name);
        for (size_t j = 0; j < f->param_count && pos < buf_size - 1; j++) {
            if (j > 0) pos += snprintf(buf + pos, buf_size - pos, ", ");
            pos += snprintf(buf + pos, buf_size - pos, "%s %s",
                           ffi_type_name(f->params[j].type),
                           f->params[j].name);
        }
        pos += snprintf(buf + pos, buf_size - pos, ") -> %s [%s]\n",
                       ffi_type_name(f->return_type), f->source_module);
    }
    return (int)pos;
}
