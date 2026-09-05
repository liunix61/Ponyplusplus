#ifndef PONYPP_FFI_H
#define PONYPP_FFI_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

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

/* FFI 函数参数 */
typedef struct FFIParam {
    const char *name;
    FFIType type;
} FFIParam;

/* FFI 函数签名 */
typedef struct FFIFunc {
    char *name;
    void *func_ptr;
    FFIParam *params;
    size_t param_count;
    FFIType return_type;
    char *source_module;
} FFIFunc;

/* FFI 注册表 */
typedef struct FFIRuntime {
    FFIFunc **funcs;
    size_t func_count;
    size_t func_cap;
    void **modules;
    size_t module_count;
    size_t module_cap;
} FFIRuntime;

/* FFI 运行时 */
FFIRuntime *ffi_runtime_new(void);
void ffi_runtime_free(void);

/* 模块加载 */
int ffi_load_module(const char *path);
int ffi_register_func(const char *name, const char *module, FFIType return_type,
                       const char *param_names[], FFIType *param_types, size_t param_count);
FFIFunc *ffi_find_func(const char *name);
size_t ffi_func_count(void);

/* 调用 FFI 函数 */
int64_t ffi_call_i64(FFIFunc *ff, int64_t a, int64_t b, int64_t c, int64_t d);
double ffi_call_f64(FFIFunc *ff, double a, double b);
void *ffi_call_ptr(FFIFunc *ff, void *arg, int64_t a);

/* 辅助 */
const char *ffi_type_name(FFIType t);
int ffi_dump(char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_FFI_H */
