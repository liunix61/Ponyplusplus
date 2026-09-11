#include <gtest/gtest.h>
#include <ponypp/wamr.h>
#include <ponypp/runtime.h>
#include <cstring>
#include <cstdlib>

/* ==================== 配置 ==================== */

TEST(WamrCov, ConfigDefaultSTM32F4) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    EXPECT_TRUE(cfg.memory_pages > 0);
}

TEST(WamrCov, ConfigDefaultSTM32H7) {
    WamrConfig cfg = wamr_config_default(MCU_STM32H7);
    EXPECT_TRUE(cfg.memory_pages > 0);
}

TEST(WamrCov, ConfigDefaultESP32) {
    WamrConfig cfg = wamr_config_default(MCU_ESP32);
    EXPECT_TRUE(cfg.memory_pages > 0);
}

TEST(WamrCov, ConfigDefaultESP32S3) {
    WamrConfig cfg = wamr_config_default(MCU_ESP32S3);
    EXPECT_TRUE(cfg.memory_pages > 0);
}

TEST(WamrCov, ConfigDefaultGeneric) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    EXPECT_TRUE(cfg.memory_pages > 0);
}

TEST(WamrCov, ConfigFieldsSTM32F4) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    EXPECT_TRUE(cfg.mcu_type == MCU_STM32F4);
}

/* ==================== 模块管理 ==================== */

TEST(WamrCov, ModuleLoadNull) {
    EXPECT_NE(wamr_module_load(nullptr, nullptr), 0);
}

TEST(WamrCov, ModuleFreeNull) {
    EXPECT_NE(wamr_module_free(nullptr), 0);
}

/* ==================== 实例管理 ==================== */

TEST(WamrCov, InstanceCreateNull) {
    EXPECT_NE(wamr_instance_create(nullptr, nullptr, nullptr), 0);
}

TEST(WamrCov, InstanceStartNull) {
    EXPECT_NE(wamr_instance_start(nullptr, "main"), 0);
}

TEST(WamrCov, InstanceFreeNull) {
    EXPECT_NE(wamr_instance_free(nullptr), 0);
}

/* ==================== 函数调用 ==================== */

TEST(WamrCov, CallFuncNull) {
    EXPECT_EQ(wamr_call_func(nullptr, "main", nullptr, 0, nullptr, 0), 0);
}

/* ==================== 内存管理 ==================== */

TEST(WamrCov, MemAllocNull) {
    void *ptr = nullptr;
    EXPECT_NE(wamr_mem_alloc(nullptr, 1024, &ptr), 0);
}

TEST(WamrCov, MemFreeNull) {
    EXPECT_NE(wamr_mem_free(nullptr, nullptr), 0);
}

TEST(WamrCov, MemReadNull) {
    char buf[16];
    EXPECT_NE(wamr_mem_read(nullptr, 0, buf, 16), 0);
}

TEST(WamrCov, MemWriteNull) {
    const char buf[16] = {0};
    EXPECT_NE(wamr_mem_write(nullptr, 0, buf, 16), 0);
}

/* ==================== 运行时指标 ==================== */









/* ==================== HAL 注册 ==================== */

TEST(WamrCov, RegisterHalNull) {
    EXPECT_EQ(wamr_register_hal(nullptr, nullptr, nullptr, nullptr), -1);
}
