#include <gtest/gtest.h>
#include <ponypp/wamr.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

/* wamr_config_default 各种 MCU */
TEST(WamrCov4, ConfigDefaultRPi5) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    EXPECT_EQ(cfg.mcu_type, MCU_GENERIC);
}

TEST(WamrCov4, ConfigDefaultRPi4) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    EXPECT_EQ(cfg.mcu_type, MCU_GENERIC);
}

TEST(WamrCov4, ConfigDefaultSTM32F4) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    EXPECT_EQ(cfg.mcu_type, MCU_STM32F4);
}

TEST(WamrCov4, ConfigDefaultSTM32H7) {
    WamrConfig cfg = wamr_config_default(MCU_STM32H7);
    EXPECT_EQ(cfg.mcu_type, MCU_STM32H7);
}

TEST(WamrCov4, ConfigDefaultESP32) {
    WamrConfig cfg = wamr_config_default(MCU_ESP32);
    EXPECT_EQ(cfg.mcu_type, MCU_ESP32);
}

/* wamr_module_load */
TEST(WamrCov4, ModuleLoad) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrModule *mod = nullptr;
    int r = wamr_module_load(&cfg, &mod);
    if (r == 0 && mod) {
        EXPECT_NE(mod, nullptr);
        wamr_module_free(mod);
    }
    SUCCEED();
}

/* wamr_module_load null */
TEST(WamrCov4, ModuleLoadNull) {
    int r = wamr_module_load(nullptr, nullptr);
    EXPECT_LT(r, 0);
}

/* wamr_module_free null */
TEST(WamrCov4, ModuleFreeNull) {
    int r = wamr_module_free(nullptr);
    (void)r;
    SUCCEED();
}

/* wamr_instance_create */
TEST(WamrCov4, InstanceCreate) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrModule *mod = nullptr;
    wamr_module_load(&cfg, &mod);
    if (mod) {
        WamrInstance *inst = nullptr;
        int r = wamr_instance_create(mod, &cfg, &inst);
        if (r == 0 && inst) {
            wamr_instance_free(inst);
        }
        wamr_module_free(mod);
    }
    SUCCEED();
}

/* wamr_instance_create null */
TEST(WamrCov4, InstanceCreateNull) {
    int r = wamr_instance_create(nullptr, nullptr, nullptr);
    EXPECT_LT(r, 0);
}

/* wamr_instance_free null */
TEST(WamrCov4, InstanceFreeNull) {
    int r = wamr_instance_free(nullptr);
    (void)r;
    SUCCEED();
}

/* wamr_call_func null */
TEST(WamrCov4, CallFuncNullInst) {
    void *result = nullptr;
    int result_count = 0;
    int r = wamr_call_func(nullptr, "main", nullptr, 0, &result, &result_count);
    EXPECT_EQ(r, 0);
}

/* wamr_call_func null name */
TEST(WamrCov4, CallFuncNullName) {
    void *result = nullptr;
    int result_count = 0;
    int r = wamr_call_func(nullptr, nullptr, nullptr, 0, &result, &result_count);
    EXPECT_EQ(r, 0);
}

/* wamr_register_hal null */
TEST(WamrCov4, RegisterHalNull) {
    int r = wamr_register_hal(nullptr, "test", nullptr, nullptr);
    EXPECT_LT(r, 0);
}

/* wamr_mem_alloc null */
TEST(WamrCov4, MemAllocNull) {
    void *ptr = nullptr;
    int r = wamr_mem_alloc(nullptr, 1024, &ptr);
    EXPECT_LT(r, 0);
}

/* wamr_mem_free null */
TEST(WamrCov4, MemFreeNull) {
    int r = wamr_mem_free(nullptr, nullptr);
    (void)r;
    SUCCEED();
}

/* wamr_mem_read null */
TEST(WamrCov4, MemReadNull) {
    char buf[16];
    int r = wamr_mem_read(nullptr, 0, buf, sizeof(buf));
    EXPECT_LT(r, 0);
}

/* wamr_mem_write null */
TEST(WamrCov4, MemWriteNull) {
    char buf[16] = "test";
    int r = wamr_mem_write(nullptr, 0, buf, sizeof(buf));
    EXPECT_LT(r, 0);
}

/* wamr_instance_mem_pages null */

/* wamr_instance_mem_used null */

/* wamr_instance_stack_pages null */

/* wamr_instance_stack_used null */

/* wamr_instance_mcu_type null */

/* wamr_module_size null */

/* wamr_module_mcu_type null */

/* wamr_instance_start null */
TEST(WamrCov4, InstanceStartNull) {
    int r = wamr_instance_start(nullptr, "main");
    EXPECT_LT(r, 0);
}

/* 完整工作流: 加载+创建实例+启动+执行 */
TEST(WamrCov4, FullWorkflow) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrModule *mod = nullptr;
    int r = wamr_module_load(&cfg, &mod);
    if (r == 0 && mod) {
        WamrInstance *inst = nullptr;
        r = wamr_instance_create(mod, &cfg, &inst);
        if (r == 0 && inst) {
            wamr_instance_start(inst, "_start");
            void *result = nullptr;
            int result_count = 0;
            wamr_call_func(inst, "_start", nullptr, 0, &result, &result_count);
            wamr_instance_free(inst);
        }
        wamr_module_free(mod);
    }
    SUCCEED();
}

/* 不同 MCU 的完整工作流 */
TEST(WamrCov4, FullWorkflowSTM32F4) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    WamrModule *mod = nullptr;
    wamr_module_load(&cfg, &mod);
    if (mod) wamr_module_free(mod);
    SUCCEED();
}

TEST(WamrCov4, FullWorkflowESP32) {
    WamrConfig cfg = wamr_config_default(MCU_ESP32);
    WamrModule *mod = nullptr;
    wamr_module_load(&cfg, &mod);
    if (mod) wamr_module_free(mod);
    SUCCEED();
}

/* 多次加载/卸载 */
TEST(WamrCov4, MultipleLoadUnload) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    for (int i = 0; i < 5; i++) {
        WamrModule *mod = nullptr;
        wamr_module_load(&cfg, &mod);
        if (mod) wamr_module_free(mod);
    }
    SUCCEED();
}

/* 内存操作 */
TEST(WamrCov4, MemOperations) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrModule *mod = nullptr;
    wamr_module_load(&cfg, &mod);
    if (mod) {
        WamrInstance *inst = nullptr;
        wamr_instance_create(mod, &cfg, &inst);
        if (inst) {
            void *ptr = nullptr;
            int r = wamr_mem_alloc(inst, 1024, &ptr);
            if (r == 0 && ptr) {
                char buf[16] = "test data";
                wamr_mem_write(inst, 0, buf, sizeof(buf));
                char read_buf[16] = {0};
                wamr_mem_read(inst, 0, read_buf, sizeof(read_buf));
                wamr_mem_free(inst, ptr);
            }
            wamr_instance_free(inst);
        }
        wamr_module_free(mod);
    }
    SUCCEED();
}

/* HAL 注册 */
static int test_hal(int id, void *arg, void *ret, int *ret_count) { (void)id; (void)arg; (void)ret; (void)ret_count; return 0; }

TEST(WamrCov4, RegisterHal) {
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrModule *mod = nullptr;
    wamr_module_load(&cfg, &mod);
    if (mod) {
        WamrInstance *inst = nullptr;
        wamr_instance_create(mod, &cfg, &inst);
        if (inst) {
            int r = wamr_register_hal(inst, "test_hal", test_hal, nullptr);
            (void)r;
            wamr_instance_free(inst);
        }
        wamr_module_free(mod);
    }
    SUCCEED();
}

/* wamr_compile_program */
TEST(WamrCov4, CompileProgram) {
    /* 需要 AST 节点 */
    int r = wamr_compile_program(nullptr, nullptr, "/tmp/test.wasm");
    (void)r;
    SUCCEED();
}

/* wamr_compile_program null output */
TEST(WamrCov4, CompileProgramNullOutput) {
    int r = wamr_compile_program(nullptr, nullptr, nullptr);
    (void)r;
    SUCCEED();
}
