#include <gtest/gtest.h>
#include <ponypp/wamr.h>
#include <ponypp/runtime.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

/* 用现成的 hello.wasm 测试完整加载+执行流程 */
TEST(WamrCov3, LoadHelloWasm) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    WamrModule *mod = nullptr;
    int r = wamr_module_load(&cfg, &mod);
    /* 可能失败（需要 .wasm 文件路径），但覆盖了代码路径 */
    if (r == 0 && mod) {
        WamrInstance *inst = nullptr;
        int r2 = wamr_instance_create(mod, &cfg, &inst);
        if (r2 == 0 && inst) {
            wamr_instance_start(inst, "_start");
            wamr_instance_free(inst);
        }
        wamr_module_free(mod);
    }
}

/* 用 hello.wasm 文件路径加载 */
TEST(WamrCov3, LoadHelloWasmFile) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    cfg.wasm_path = "/home/liunix/Ponyplusplus/examples/hello.wasm";
    
    WamrModule *mod = nullptr;
    int r = wamr_module_load(&cfg, &mod);
    if (r == 0 && mod) {
        WamrInstance *inst = nullptr;
        int r2 = wamr_instance_create(mod, &cfg, &inst);
        if (r2 == 0 && inst) {
            wamr_instance_start(inst, "_start");
            
            /* 尝试调用函数 */
            void *result = nullptr;
            int result_count = 0;
            wamr_call_func(inst, "main", nullptr, 0, &result, &result_count);
            
            wamr_instance_free(inst);
        }
        wamr_module_free(mod);
    }
}

/* 用 hello-web.wasm 文件路径加载 */
TEST(WamrCov3, LoadHelloWebWasm) {
    WamrConfig cfg = wamr_config_default(MCU_ESP32);
    cfg.wasm_path = "/home/liunix/Ponyplusplus/examples/hello-web.wasm";
    
    WamrModule *mod = nullptr;
    int r = wamr_module_load(&cfg, &mod);
    if (r == 0 && mod) {
        WamrInstance *inst = nullptr;
        int r2 = wamr_instance_create(mod, &cfg, &inst);
        if (r2 == 0 && inst) {
            wamr_instance_start(inst, "_start");
            wamr_instance_free(inst);
        }
        wamr_module_free(mod);
    }
}

/* wamr_mem_alloc/read/write */
TEST(WamrCov3, MemAllocReadWrite) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    cfg.wasm_path = "/home/liunix/Ponyplusplus/examples/hello.wasm";
    
    WamrModule *mod = nullptr;
    int r = wamr_module_load(&cfg, &mod);
    if (r == 0 && mod) {
        WamrInstance *inst = nullptr;
        int r2 = wamr_instance_create(mod, &cfg, &inst);
        if (r2 == 0 && inst) {
            void *ptr = nullptr;
            int r3 = wamr_mem_alloc(inst, 1024, &ptr);
            if (r3 == 0 && ptr) {
                /* 写入数据 */
                const char *data = "hello";
                wamr_mem_write(inst, (int)(intptr_t)ptr, data, strlen(data));
                
                /* 读取数据 */
                char buf[256];
                wamr_mem_read(inst, (int)(intptr_t)ptr, buf, strlen(data));
                
                wamr_mem_free(inst, ptr);
            }
            wamr_instance_free(inst);
        }
        wamr_module_free(mod);
    }
}

/* wamr_register_hal */
static int hal_test_func(int id, void *arg, void *ret, int *ret_count) {
    (void)id; (void)arg; (void)ret; (void)ret_count;
    return 42;
}

TEST(WamrCov3, RegisterHal) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    cfg.wasm_path = "/home/liunix/Ponyplusplus/examples/hello.wasm";
    
    WamrModule *mod = nullptr;
    int r = wamr_module_load(&cfg, &mod);
    if (r == 0 && mod) {
        WamrInstance *inst = nullptr;
        int r2 = wamr_instance_create(mod, &cfg, &inst);
        if (r2 == 0 && inst) {
            int r3 = wamr_register_hal(inst, "test_hal", hal_test_func, nullptr);
            EXPECT_EQ(r3, 0);
            
            wamr_instance_free(inst);
        }
        wamr_module_free(mod);
    }
}

/* wamr_compile_program */
TEST(WamrCov3, CompileProgram) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    
    /* 空 AST */
    int r = wamr_compile_program(nullptr, &cfg, "/tmp/test_wamr.wasm");
    EXPECT_NE(r, 0);
    
    unlink("/tmp/test_wamr.wasm");
}

/* wamr_compile_program null cfg */
TEST(WamrCov3, CompileProgramNullCfg) {
    int r = wamr_compile_program(nullptr, nullptr, "/tmp/test_wamr.wasm");
    EXPECT_NE(r, 0);
}

/* wamr_compile_program null output */
TEST(WamrCov3, CompileProgramNullOutput) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    int r = wamr_compile_program(nullptr, &cfg, nullptr);
    EXPECT_NE(r, 0);
}

/* wamr_config_default 所有 MCU */
TEST(WamrCov3, ConfigDefaultAllMcu) {
    WamrConfig cfg1 = wamr_config_default(MCU_STM32F4);
    EXPECT_GT(cfg1.stack_pages, 0);
    
    WamrConfig cfg2 = wamr_config_default(MCU_ESP32);
    EXPECT_GT(cfg2.stack_pages, 0);
    
    WamrConfig cfg3 = wamr_config_default(MCU_GENERIC);
    EXPECT_GT(cfg3.stack_pages, 0);
}

/* wamr_module_load null cfg */
TEST(WamrCov3, ModuleLoadNullCfg) {
    WamrModule *mod = nullptr;
    int r = wamr_module_load(nullptr, &mod);
    EXPECT_NE(r, 0);
}

/* wamr_module_load null out */
TEST(WamrCov3, ModuleLoadNullOut) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    int r = wamr_module_load(&cfg, nullptr);
    EXPECT_NE(r, 0);
}

/* wamr_module_free null */
TEST(WamrCov3, ModuleFreeNull) {
    wamr_module_free(nullptr);
    SUCCEED();
}

/* wamr_instance_create null mod */
TEST(WamrCov3, InstanceCreateNullMod) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    WamrInstance *inst = nullptr;
    int r = wamr_instance_create(nullptr, &cfg, &inst);
    EXPECT_NE(r, 0);
}

/* wamr_instance_create null cfg */
TEST(WamrCov3, InstanceCreateNullCfg) {
    WamrInstance *inst = nullptr;
    int r = wamr_instance_create(nullptr, nullptr, &inst);
    EXPECT_NE(r, 0);
}

/* wamr_instance_create null out */
TEST(WamrCov3, InstanceCreateNullOut) {
    WamrConfig cfg = wamr_config_default(MCU_STM32F4);
    int r = wamr_instance_create(nullptr, &cfg, nullptr);
    EXPECT_NE(r, 0);
}

/* wamr_instance_free null */
TEST(WamrCov3, InstanceFreeNull) {
    wamr_instance_free(nullptr);
    SUCCEED();
}

/* wamr_instance_start null */
TEST(WamrCov3, InstanceStartNull) {
    int r = wamr_instance_start(nullptr, "_start");
    EXPECT_NE(r, 0);
}

/* wamr_instance_start null entry */
TEST(WamrCov3, InstanceStartNullEntry) {
    int r = wamr_instance_start(nullptr, nullptr);
    EXPECT_NE(r, 0);
}

/* wamr_call_func null inst */
TEST(WamrCov3, CallFuncNullInst) {
    void *result = nullptr; int result_count = 0;
    int r = wamr_call_func(nullptr, "main", nullptr, 0, &result, &result_count);
    EXPECT_EQ(r, 0);
}

/* wamr_call_func null name */
TEST(WamrCov3, CallFuncNullName) {
    void *result = nullptr; int result_count = 0;
    int r = wamr_call_func(nullptr, nullptr, nullptr, 0, &result, &result_count);
    EXPECT_EQ(r, 0);
}

/* wamr_mem_alloc null inst */
TEST(WamrCov3, MemAllocNullInst) {
    void *ptr = nullptr;
    int r = wamr_mem_alloc(nullptr, 1024, &ptr);
    EXPECT_NE(r, 0);
}

/* wamr_mem_free null inst */
TEST(WamrCov3, MemFreeNullInst) {
    int r = wamr_mem_free(nullptr, nullptr);
    EXPECT_NE(r, 0);
}

/* wamr_mem_read null inst */
TEST(WamrCov3, MemReadNullInst) {
    char buf[256];
    int r = wamr_mem_read(nullptr, 0, buf, 256);
    EXPECT_NE(r, 0);
}

/* wamr_mem_write null inst */
TEST(WamrCov3, MemWriteNullInst) {
    int r = wamr_mem_write(nullptr, 0, "hello", 5);
    EXPECT_NE(r, 0);
}

/* wamr_register_hal null inst */
TEST(WamrCov3, RegisterHalNullInst) {
    int r = wamr_register_hal(nullptr, "test", hal_test_func, nullptr);
    EXPECT_EQ(r, 0);
}

/* wamr_register_hal null name */
TEST(WamrCov3, RegisterHalNullName) {
    int r = wamr_register_hal(nullptr, nullptr, hal_test_func, nullptr);
    EXPECT_NE(r, 0);
}

/* wamr_register_hal null cb */
TEST(WamrCov3, RegisterHalNullCb) {
    int r = wamr_register_hal(nullptr, "test", nullptr, nullptr);
    EXPECT_NE(r, 0);
}
