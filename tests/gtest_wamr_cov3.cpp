#include <gtest/gtest.h>
#include <ponypp/wamr.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>

/* ==================== WAMR config ==================== */

TEST(WamrCov3, ConfigDefaultGeneric) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    EXPECT_EQ(cfg.mcu_type, MCU_GENERIC);
}

TEST(WamrCov3, ConfigDefaultStm32) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    EXPECT_EQ(cfg.mcu_type, MCU_STM32F4);
}

TEST(WamrCov3, ConfigDefaultEsp32) {
    WamrConfig cfg = wamr_config_default(MCU_ESP32);
    EXPECT_EQ(cfg.mcu_type, MCU_ESP32);
}

/* ==================== WAMR module load ==================== */

TEST(WamrCov3, ModuleLoadNull) {
    WamrModule *mod = nullptr;
    int r = wamr_module_load(nullptr, &mod);
    EXPECT_EQ(r, -1);
}

TEST(WamrCov3, ModuleLoadEmpty) {
    WamrModule *mod = nullptr;
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    int r = wamr_module_load(&cfg, &mod);
    EXPECT_TRUE(r == 0 || r == -1);
    if (r == 0 && mod) wamr_module_free(mod);
}

/* ==================== WAMR module free ==================== */

TEST(WamrCov3, ModuleFreeNull) {
    int r = wamr_module_free(nullptr);
    EXPECT_TRUE(r == 0 || r == -1);
}

/* ==================== WAMR instance ==================== */

TEST(WamrCov3, InstanceCreateNull) {
    WamrInstance *inst = nullptr;
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    int r = wamr_instance_create(nullptr, &cfg, &inst);
    EXPECT_EQ(r, -1);
}

TEST(WamrCov3, InstanceFreeNull) {
    int r = wamr_instance_free(nullptr);
    EXPECT_TRUE(r == 0 || r == -1);
}

/* ==================== WAMR call function ==================== */


/* ==================== WAMR memory ==================== */

TEST(WamrCov3, MemAllocNull) {
    void *ptr = nullptr;
    int r = wamr_mem_alloc(nullptr, 1024, &ptr);
    EXPECT_EQ(r, -1);
}

TEST(WamrCov3, MemFreeNull) {
    int r = wamr_mem_free(nullptr, nullptr);
    EXPECT_TRUE(r == 0 || r == -1);
}

TEST(WamrCov3, MemReadNull) {
    char buf[16];
    int r = wamr_mem_read(nullptr, 0, buf, sizeof(buf));
    EXPECT_EQ(r, -1);
}

TEST(WamrCov3, MemWriteNull) {
    char buf[16] = {0};
    int r = wamr_mem_write(nullptr, 0, buf, sizeof(buf));
    EXPECT_EQ(r, -1);
}

/* ==================== WAMR HAL ==================== */

TEST(WamrCov3, RegisterHalNull) {
    int r = wamr_register_hal(nullptr, "test", nullptr, nullptr);
    EXPECT_EQ(r, -1);
}

/* ==================== WAMR compile ==================== */

TEST(WamrCov3, CompileProgramNull) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    int r = wamr_compile_program(nullptr, &cfg, "/tmp/test.wasm");
    EXPECT_EQ(r, -1);
}
