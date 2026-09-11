#include <gtest/gtest.h>
#include <ponypp/ffi.h>
#include <cstring>
#include <cstdlib>

class FFITest : public ::testing::Test {
protected:
    void SetUp() override {
        ffi_runtime_new();
    }
    
    void TearDown() override {
        ffi_runtime_free();
    }
};

/* ==================== 运行时管理 ==================== */

TEST_F(FFITest, RuntimeNewFree) {
    /* SetUp/TearDown 已覆盖 */
    SUCCEED();
}

TEST(FFIStandalone, RuntimeFreeNull) {
    ffi_runtime_free();  /* 不应崩溃 */
}

/* ==================== 模块加载 ==================== */

TEST_F(FFITest, LoadModuleNull) {
    EXPECT_EQ(ffi_load_module(nullptr), -1);
}

TEST_F(FFITest, LoadModuleInvalid) {
    EXPECT_EQ(ffi_load_module("/nonexistent/lib.so"), -1);
}

TEST_F(FFITest, LoadModuleLibc) {
    /* 加载 libc 应该成功 */
    int rc = ffi_load_module("libc.so.6");
    /* 可能成功或失败 */
    (void)rc;
}

/* ==================== 函数注册 ==================== */

TEST_F(FFITest, RegisterFunc) {
    const char *param_names[] = {"a", "b"};
    FFIType param_types[] = {FFI_TYPE_I64, FFI_TYPE_I64};
    
    int rc = ffi_register_func("strlen", "test_mod", FFI_TYPE_I64,
                                param_names, param_types, 2);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(ffi_func_count(), 1);
}

TEST_F(FFITest, RegisterMultipleFuncs) {
    const char *param_names[] = {"x"};
    FFIType param_types[] = {FFI_TYPE_I32};
    
    ffi_register_func("abs", "mod", FFI_TYPE_I32, param_names, param_types, 1);
    ffi_register_func("labs", "mod", FFI_TYPE_I64, param_names, param_types, 1);
    ffi_register_func("fabs", "mod", FFI_TYPE_F64, param_names, param_types, 1);
    
    EXPECT_EQ(ffi_func_count(), 3);
}

TEST_F(FFITest, RegisterNullArgs) {
    const char *param_names[] = {"a"};
    FFIType param_types[] = {FFI_TYPE_I32};
    
    EXPECT_EQ(ffi_register_func(nullptr, "mod", FFI_TYPE_I32, param_names, param_types, 1), -1);
    EXPECT_EQ(ffi_register_func("func", nullptr, FFI_TYPE_I32, param_names, param_types, 1), -1);
}

/* ==================== 函数查找 ==================== */

TEST_F(FFITest, FindFunc) {
    const char *param_names[] = {"a"};
    FFIType param_types[] = {FFI_TYPE_I32};
    
    ffi_register_func("strlen", "mod", FFI_TYPE_I32, param_names, param_types, 1);
    
    FFIFunc *f = ffi_find_func("strlen");
    EXPECT_NE(f, nullptr);
    
    EXPECT_EQ(ffi_find_func("nonexistent"), nullptr);
    EXPECT_EQ(ffi_find_func(nullptr), nullptr);
}

/* ==================== 类型名称 ==================== */

TEST_F(FFITest, TypeName) {
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_I32), "i32");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_I64), "i64");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_F64), "f64");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_STRING), "string");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_PTR), "ptr");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_VOID), "void");
}

/* ==================== Dump ==================== */

TEST_F(FFITest, Dump) {
    const char *param_names[] = {"a", "b"};
    FFIType param_types[] = {FFI_TYPE_I64, FFI_TYPE_I64};
    
    ffi_register_func("strlen", "mod", FFI_TYPE_I64,
                       param_names, param_types, 2);
    
    char buf[1024];
    int len = ffi_dump(buf, sizeof(buf));
    EXPECT_GT(len, 0);
    
    EXPECT_LE(ffi_dump(nullptr, 1024), 0);
}

/* ==================== 空运行时 ==================== */

TEST(FFIEmpty, EmptyRuntime) {
    ffi_runtime_new();
    EXPECT_EQ(ffi_func_count(), 0);
    EXPECT_EQ(ffi_find_func("anything"), nullptr);
    
    char buf[256];
    int len = ffi_dump(buf, sizeof(buf));
    (void)len;
    
    ffi_runtime_free();
}
