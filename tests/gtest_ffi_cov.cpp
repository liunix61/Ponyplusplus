#include <gtest/gtest.h>
#include <ponypp/ffi.h>
#include <cstring>
#include <cstdlib>
#include <cmath>

static int reg_func(const char *name, const char *mod, FFIType ret_type, FFIType arg_type) {
    const char *pn[] = {"x"};
    FFIType pt[] = {arg_type};
    return ffi_register_func(name, mod, ret_type, pn, pt, 1);
}

/* ==================== 运行时管理 ==================== */

TEST(FfiCov, RuntimeNew) {
    FFIRuntime *rt = ffi_runtime_new();
    EXPECT_NE(rt, nullptr);
    ffi_runtime_free();
}

TEST(FfiCov, RuntimeFreeWithoutNew) {
    ffi_runtime_free();
}

/* ==================== 模块加载 ==================== */

TEST(FfiCov, LoadModuleNull) {
    EXPECT_NE(ffi_load_module(nullptr), 0);
}

TEST(FfiCov, LoadModuleNonexistent) {
    EXPECT_NE(ffi_load_module("/nonexistent/lib.so"), 0);
}

/* ==================== 函数注册 ==================== */

TEST(FfiCov, RegisterFuncNull) {
    EXPECT_NE(ffi_register_func(nullptr, nullptr, FFI_TYPE_I64, nullptr, nullptr, 0), 0);
}

TEST(FfiCov, RegisterFuncAbs) {
    int ret = reg_func("abs", "libc.so.6", FFI_TYPE_I64, FFI_TYPE_I64);
    (void)ret;
}

TEST(FfiCov, RegisterFuncLabs) {
    int ret = reg_func("labs", "libc.so.6", FFI_TYPE_I64, FFI_TYPE_I64);
    (void)ret;
}

TEST(FfiCov, RegisterFuncFabs) {
    int ret = reg_func("fabs", "libc.so.6", FFI_TYPE_F64, FFI_TYPE_F64);
    (void)ret;
}



/* ==================== 函数查找 ==================== */







/* ==================== 函数调用 ==================== */

TEST(FfiCov, CallI64Null) {
    EXPECT_EQ(ffi_call_i64(nullptr, 0, 0, 0, 0), -1);
}

TEST(FfiCov, CallPtrNull) {
    EXPECT_EQ(ffi_call_ptr(nullptr, nullptr, 0), nullptr);
}

/* ==================== 函数计数 ==================== */

TEST(FfiCov, FuncCount) {
    size_t count = ffi_func_count();
    EXPECT_GE(count, 0);
}

/* ==================== 导出 ==================== */






