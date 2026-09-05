#include "ponypp.h"
#include "ponypp/wamr.h"
#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>

/* ===== MCU 类型解析 ===== */
TEST(WAMR, ParseMcu) {
    EXPECT_EQ(wamr_parse_mcu("stm32f4"), MCU_STM32F4);
    EXPECT_EQ(wamr_parse_mcu("stm32h7"), MCU_STM32H7);
    EXPECT_EQ(wamr_parse_mcu("esp32"), MCU_ESP32);
    EXPECT_EQ(wamr_parse_mcu("esp32s3"), MCU_ESP32S3);
    EXPECT_EQ(wamr_parse_mcu("generic"), MCU_GENERIC);
    EXPECT_EQ(wamr_parse_mcu("unknown"), MCU_GENERIC);
    EXPECT_EQ(wamr_parse_mcu(NULL), MCU_GENERIC);
}

/* ===== MCU 名称 ===== */
TEST(WAMR, McuName) {
    EXPECT_STREQ(wamr_mcu_name(MCU_STM32F4), "STM32F4");
    EXPECT_STREQ(wamr_mcu_name(MCU_STM32H7), "STM32H7");
    EXPECT_STREQ(wamr_mcu_name(MCU_ESP32), "ESP32");
    EXPECT_STREQ(wamr_mcu_name(MCU_ESP32S3), "ESP32S3");
    EXPECT_STREQ(wamr_mcu_name(MCU_GENERIC), "Generic");
    EXPECT_STREQ(wamr_mcu_name((McuType)99), "Unknown");
}

/* ===== 默认配置 ===== */
TEST(WAMR, ConfigDefault) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    EXPECT_EQ(cfg.mcu_type, MCU_STM32F4);
    EXPECT_EQ(cfg.memory_pages, 1);
    EXPECT_EQ(cfg.max_memory_pages, 2);
    EXPECT_EQ(cfg.stack_pages, 1);
    EXPECT_TRUE(cfg.enable_bounds_check);
    EXPECT_TRUE(cfg.enable_stack_check);
}

TEST(WAMR, ConfigDefaultH7) {
    WamrConfig cfg = wamr_config_default(MCU_STM32H7);
    EXPECT_EQ(cfg.mcu_type, MCU_STM32H7);
    EXPECT_EQ(cfg.memory_pages, 2);
    EXPECT_EQ(cfg.max_memory_pages, 4);
    EXPECT_EQ(cfg.stack_pages, 2);
}

TEST(WAMR, ConfigDefaultEsp32) {
    WamrConfig cfg = wamr_config_default(MCU_ESP32);
    EXPECT_EQ(cfg.mcu_type, MCU_ESP32);
    EXPECT_EQ(cfg.memory_pages, 1);
}

TEST(WAMR, ConfigDefaultEsp32S3) {
    WamrConfig cfg = wamr_config_default(MCU_ESP32S3);
    EXPECT_EQ(cfg.mcu_type, MCU_ESP32S3);
    EXPECT_EQ(cfg.memory_pages, 2);
}

TEST(WAMR, ConfigDefaultGeneric) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    EXPECT_EQ(cfg.mcu_type, MCU_GENERIC);
    EXPECT_EQ(cfg.memory_pages, 1);
}

TEST(WAMR, ConfigDefaultInvalid) {
    WamrConfig cfg = wamr_config_default((McuType)99);
    EXPECT_EQ(cfg.mcu_type, (McuType)0);
}

/* ===== 模块加载 ===== */
TEST(WAMR, ModuleLoadNullPath) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrModule *mod = NULL;
    EXPECT_EQ(wamr_module_load(&cfg, &mod), -1);
}

TEST(WAMR, ModuleLoadNullOut) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    cfg.wasm_path = "/tmp/nonexistent.wasm";
    EXPECT_EQ(wamr_module_load(&cfg, NULL), -1);
}

TEST(WAMR, ModuleLoadFileNotFound) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    cfg.wasm_path = "/tmp/definitely_not_exists.wasm";
    WamrModule *mod = NULL;
    EXPECT_EQ(wamr_module_load(&cfg, &mod), -1);
}

TEST(WAMR, ModuleFreeNull) {
    EXPECT_EQ(wamr_module_free(NULL), -1);
}

/* ===== 模块加载 + 销毁 (有效 WASM) ===== */
TEST(WAMR, ModuleLoadValidWasm) {
    /* 写入最小有效 WASM 文件 (magic + version) */
    const char *path = "/tmp/ponypp_wamr_test.wasm";
    unsigned char wasm[] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
    FILE *f = fopen(path, "wb");
    ASSERT_NE(f, nullptr);
    fwrite(wasm, 1, sizeof(wasm), f);
    fclose(f);

    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    cfg.wasm_path = path;

    WamrModule *mod = NULL;
    int rc = wamr_module_load(&cfg, &mod);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(mod, nullptr);
    if (mod) {
        EXPECT_EQ(mod->mcu_type, MCU_GENERIC);
        EXPECT_EQ(mod->size, 8);
        wamr_module_free(mod);
    }
    remove(path);
}

TEST(WAMR, ModuleLoadInvalidWasm) {
    /* 无效魔数 */
    const char *path = "/tmp/ponypp_wamr_invalid.wasm";
    unsigned char bad[] = {0x00, 0x00, 0x00, 0x00};
    FILE *f = fopen(path, "wb");
    ASSERT_NE(f, nullptr);
    fwrite(bad, 1, sizeof(bad), f);
    fclose(f);

    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    cfg.wasm_path = path;
    WamrModule *mod = NULL;
    EXPECT_EQ(wamr_module_load(&cfg, &mod), -1);
    EXPECT_EQ(mod, nullptr);
    remove(path);
}

/* ===== 实例创建与销毁 ===== */
TEST(WAMR, InstanceCreate) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    cfg.wasm_path = "/tmp/dummy.wasm";

    /* 创建模块 (不需要有效文件，实例创建独立于模块加载) */
    WamrModule mod = {0};
    mod.mcu_type = MCU_GENERIC;

    WamrInstance *inst = NULL;
    int rc = wamr_instance_create(&mod, &cfg, &inst);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(inst, nullptr);
    if (inst) {
        EXPECT_EQ(inst->mem_pages, 1);
        EXPECT_EQ(inst->mem_used, 0);
        EXPECT_EQ(inst->stack_pages, 1);
        EXPECT_EQ(inst->stack_used, 0);
        wamr_instance_free(inst);
    }
}

TEST(WAMR, InstanceCreateNull) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    EXPECT_EQ(wamr_instance_create(NULL, &cfg, NULL), -1);
    EXPECT_EQ(wamr_instance_create(NULL, NULL, NULL), -1);
}

TEST(WAMR, InstanceFreeNull) {
    EXPECT_EQ(wamr_instance_free(NULL), -1);
}

TEST(WAMR, InstanceStart) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrModule mod = {0};
    WamrInstance *inst = NULL;
    ASSERT_EQ(wamr_instance_create(&mod, &cfg, &inst), 0);
    EXPECT_EQ(wamr_instance_start(inst, "main"), 0);
    EXPECT_EQ(wamr_instance_start(inst, NULL), -1);
    wamr_instance_free(inst);
}

/* ===== 内存操作 ===== */
TEST(WAMR, MemAlloc) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrModule mod = {0};
    WamrInstance *inst = NULL;
    ASSERT_EQ(wamr_instance_create(&mod, &cfg, &inst), 0);

    void *ptr = NULL;
    EXPECT_EQ(wamr_mem_alloc(inst, 64, &ptr), 0);
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(inst->mem_used, 64);

    void *ptr2 = NULL;
    EXPECT_EQ(wamr_mem_alloc(inst, 32, &ptr2), 0);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_EQ(inst->mem_used, 96);

    wamr_instance_free(inst);
}

TEST(WAMR, MemAllocExceeds) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    cfg.memory_pages = 1; /* 64KB */
    WamrModule mod = {0};
    WamrInstance *inst = NULL;
    ASSERT_EQ(wamr_instance_create(&mod, &cfg, &inst), 0);

    void *ptr = NULL;
    /* 分配超过内存限制 */
    EXPECT_EQ(wamr_mem_alloc(inst, 70000, &ptr), -1);

    wamr_instance_free(inst);
}

TEST(WAMR, MemAllocNull) {
    EXPECT_EQ(wamr_mem_alloc(NULL, 64, NULL), -1);
    EXPECT_EQ(wamr_mem_alloc(NULL, 64, (void **)1), -1);
}

TEST(WAMR, MemRead) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrModule mod = {0};
    WamrInstance *inst = NULL;
    ASSERT_EQ(wamr_instance_create(&mod, &cfg, &inst), 0);

    /* 写入数据 */
    int data = 0x12345678;
    EXPECT_EQ(wamr_mem_write(inst, 0, &data, 4), 0);

    /* 读取数据 */
    int read_val = 0;
    EXPECT_EQ(wamr_mem_read(inst, 0, &read_val, 4), 0);
    EXPECT_EQ(read_val, 0x12345678);

    wamr_instance_free(inst);
}

TEST(WAMR, MemReadOutOfRange) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    cfg.memory_pages = 1;
    WamrModule mod = {0};
    WamrInstance *inst = NULL;
    ASSERT_EQ(wamr_instance_create(&mod, &cfg, &inst), 0);

    void *buf = malloc(4);
    EXPECT_EQ(wamr_mem_read(inst, 70000, buf, 4), -1);
    EXPECT_EQ(wamr_mem_read(inst, -1, buf, 4), -1);
    free(buf);

    wamr_instance_free(inst);
}

TEST(WAMR, MemWrite) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrModule mod = {0};
    WamrInstance *inst = NULL;
    ASSERT_EQ(wamr_instance_create(&mod, &cfg, &inst), 0);

    int data = 0xDEADBEEF;
    EXPECT_EQ(wamr_mem_write(inst, 10, &data, 4), 0);

    int read_val = 0;
    EXPECT_EQ(wamr_mem_read(inst, 10, &read_val, 4), 0);
    EXPECT_EQ(read_val, 0xDEADBEEF);

    wamr_instance_free(inst);
}

TEST(WAMR, MemWriteOutOfRange) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    cfg.memory_pages = 1;
    WamrModule mod = {0};
    WamrInstance *inst = NULL;
    ASSERT_EQ(wamr_instance_create(&mod, &cfg, &inst), 0);

    int data = 0;
    EXPECT_EQ(wamr_mem_write(inst, 70000, &data, 4), -1);
    EXPECT_EQ(wamr_mem_write(inst, -1, &data, 4), -1);

    wamr_instance_free(inst);
}

TEST(WAMR, MemFree) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrModule mod = {0};
    WamrInstance *inst = NULL;
    ASSERT_EQ(wamr_instance_create(&mod, &cfg, &inst), 0);

    void *ptr = NULL;
    wamr_mem_alloc(inst, 64, &ptr);
    EXPECT_EQ(wamr_mem_free(inst, ptr), 0);
    EXPECT_EQ(wamr_mem_free(NULL, ptr), -1);
    EXPECT_EQ(wamr_mem_free(inst, NULL), -1);

    wamr_instance_free(inst);
}

/* ===== HAL 回调 ===== */
TEST(WAMR, HalRegister) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrModule mod = {0};
    WamrInstance *inst = NULL;
    ASSERT_EQ(wamr_instance_create(&mod, &cfg, &inst), 0);

    int (*cb)(int, void *, void *, int *) = NULL;
    void *arg = (void *)123;
    EXPECT_EQ(wamr_register_hal(inst, "stm32_gpio_read", cb, arg), -1);
    EXPECT_EQ(wamr_register_hal(inst, NULL, cb, arg), -1);

    wamr_instance_free(inst);
}

/* ===== 函数调用 ===== */
TEST(WAMR, CallFunc) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrModule mod = {0};
    WamrInstance *inst = NULL;
    ASSERT_EQ(wamr_instance_create(&mod, &cfg, &inst), 0);

    void *result = NULL;
    int result_count = 0;
    EXPECT_EQ(wamr_call_func(inst, "main", NULL, 0, &result, &result_count), 0);

    wamr_instance_free(inst);
}

TEST(WAMR, CallFuncNull) {
    EXPECT_EQ(wamr_call_func(NULL, "main", NULL, 0, NULL, NULL), 0);
}

/* ===== 编译 ===== */
TEST(WAMR, CompileNull) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    EXPECT_EQ(wamr_compile_program(NULL, &cfg, "/tmp/out.wasm"), -1);
    EXPECT_EQ(wamr_compile_program(NULL, NULL, "/tmp/out.wasm"), -1);
    EXPECT_EQ(wamr_compile_program(NULL, &cfg, NULL), -1);
}

/* ===== 完整生命周期 ===== */
TEST(WAMR, FullLifecycle) {
    /* 创建配置 */
    WamrConfig cfg = wamr_config_default(MCU_STM32H7);
    EXPECT_EQ(cfg.mcu_type, MCU_STM32H7);

    /* 写入有效 WASM */
    const char *path = "/tmp/ponypp_wamr_full.wasm";
    unsigned char wasm[] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00};
    FILE *f = fopen(path, "wb");
    ASSERT_NE(f, nullptr);
    fwrite(wasm, 1, sizeof(wasm), f);
    fclose(f);

    cfg.wasm_path = path;

    /* 加载模块 */
    WamrModule *mod = NULL;
    EXPECT_EQ(wamr_module_load(&cfg, &mod), 0);
    EXPECT_NE(mod, nullptr);

    /* 创建实例 */
    WamrInstance *inst = NULL;
    EXPECT_EQ(wamr_instance_create(mod, &cfg, &inst), 0);
    EXPECT_NE(inst, nullptr);

    /* 启动 */
    EXPECT_EQ(wamr_instance_start(inst, "main"), 0);

    /* 内存操作 */
    int val = 42;
    EXPECT_EQ(wamr_mem_write(inst, 0, &val, 4), 0);
    int read_val = 0;
    EXPECT_EQ(wamr_mem_read(inst, 0, &read_val, 4), 0);
    EXPECT_EQ(read_val, 42);

    /* 清理 */
    wamr_instance_free(inst);
    wamr_module_free(mod);
    remove(path);
}