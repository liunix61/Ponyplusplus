#include <gtest/gtest.h>
#include <ponypp/wamr.h>
#include <cstring>
#include <cstdlib>

/* ==================== 配置 ==================== */

TEST(WamrFull, ConfigDefaultSTM32F4) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    EXPECT_EQ(cfg.mcu_type, MCU_STM32F4);
    EXPECT_GT(cfg.memory_pages, 0);
    EXPECT_GT(cfg.stack_pages, 0);
}

TEST(WamrFull, ConfigDefaultSTM32H7) {
    WamrConfig cfg = wamr_config_default(MCU_STM32H7);
    EXPECT_EQ(cfg.mcu_type, MCU_STM32H7);
    EXPECT_GT(cfg.memory_pages, 0);
}

TEST(WamrFull, ConfigDefaultESP32) {
    WamrConfig cfg = wamr_config_default(MCU_ESP32);
    EXPECT_EQ(cfg.mcu_type, MCU_ESP32);
}

TEST(WamrFull, ConfigDefaultESP32S3) {
    WamrConfig cfg = wamr_config_default(MCU_ESP32S3);
    EXPECT_EQ(cfg.mcu_type, MCU_ESP32S3);
}

TEST(WamrFull, ConfigDefaultGeneric) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    EXPECT_EQ(cfg.mcu_type, MCU_GENERIC);
    EXPECT_GT(cfg.memory_pages, 0);
}

/* ==================== 模块加载 ==================== */

TEST(WamrFull, ModuleLoadNull) {
    WamrModule *mod = nullptr;
    EXPECT_NE(wamr_module_load(nullptr, &mod), 0);
}

TEST(WamrFull, ModuleLoadNonexistentFile) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    cfg.wasm_path = "/nonexistent/file.wasm";
    WamrModule *mod = nullptr;
    EXPECT_NE(wamr_module_load(&cfg, &mod), 0);
}

TEST(WamrFull, ModuleFreeNull) {
    EXPECT_NE(wamr_module_free(nullptr), 0);
}

/* ==================== 实例 ==================== */

TEST(WamrFull, InstanceCreateNull) {
    WamrInstance *inst = nullptr;
    EXPECT_NE(wamr_instance_create(nullptr, nullptr, &inst), 0);
}

TEST(WamrFull, InstanceFreeNull) {
    EXPECT_NE(wamr_instance_free(nullptr), 0);
}

TEST(WamrFull, InstanceStartNull) {
    EXPECT_NE(wamr_instance_start(nullptr, "main"), 0);
}

/* ==================== 内存操作 ==================== */

TEST(WamrFull, MemAllocNull) {
    void *ptr = nullptr;
    EXPECT_NE(wamr_mem_alloc(nullptr, 64, &ptr), 0);
}

TEST(WamrFull, MemFreeNull) {
    EXPECT_NE(wamr_mem_free(nullptr, nullptr), 0);
}

TEST(WamrFull, MemReadWriteNull) {
    char buf[16];
    EXPECT_NE(wamr_mem_read(nullptr, 0, buf, 16), 0);
    EXPECT_NE(wamr_mem_write(nullptr, 0, buf, 16), 0);
}

/* ==================== 查询函数 ==================== */





/* ==================== HAL 注册 ==================== */

static int dummy_hal(int id, void *arg, void *ret, int *ret_count) {
    (void)id; (void)arg; (void)ret; (void)ret_count;
    return 0;
}

TEST(WamrFull, RegisterHalNull) {
    EXPECT_EQ(wamr_register_hal(nullptr, "test", dummy_hal, nullptr), 0);
}

/* ==================== 函数调用 ==================== */

TEST(WamrFull, CallFuncNull) {
    void *argv[1] = {nullptr};
    void *result[1] = {nullptr};
    int rc = 0;
    EXPECT_EQ(wamr_call_func(nullptr, "main", argv, 1, result, &rc), 0);
}

/* ==================== 编译 ==================== */

TEST(WamrFull, CompileNull) {
    EXPECT_NE(wamr_compile_program(nullptr, nullptr, "/tmp/test.wasm"), 0);
}
