/*
 * ponypp/wamr.h - WAMR (WebAssembly Micro Runtime) 集成层
 *
 * 集成 Intel 开源 WAMR 运行时，为 MCU 目标 (STM32/ESP32) 提供:
 *   - WASM 模块加载与执行
 *   - 线性内存管理
 *   - 函数调用接口
 *   - 硬件抽象层 (HAL) 回调
 *
 * WAMR: https://github.com/bytecodealliance/wasm-micro-runtime
 * 文档: https://docs.wasm-micro-runtime.com/en/
 */
#ifndef PONYPP_WAMR_H
#define PONYPP_WAMR_H
#ifdef __cplusplus
extern "C" {
#endif

#include "ponypp.h"

/* MCU 目标类型 */
typedef enum {
    MCU_STM32F4,
    MCU_STM32H7,
    MCU_ESP32,
    MCU_ESP32S3,
    MCU_GENERIC   /* 通用 WASM，不绑定具体 MCU */
} McuType;

/* WAMR 模块句柄 */
typedef struct WamrModule {
    char *name;
    unsigned char *data;
    size_t size;
    McuType mcu_type;
} WamrModule;

/* WAMR 实例 */
typedef struct WamrInstance {
    WamrModule *module;
    char *memory;
    int mem_pages;
    int mem_used;
    int stack_pages;
    int stack_used;
    McuType mcu_type;
} WamrInstance;

/* MCU 配置 */
typedef struct WamrConfig {
    McuType mcu_type;
    const char *wasm_path;      /* .wasm 文件路径 */
    int memory_pages;           /* 初始内存页数 (每页 64KB) */
    int max_memory_pages;       /* 最大内存页数 */
    int stack_pages;            /* 栈页数 */
    bool enable_bounds_check;   /* 内存边界检查 */
    bool enable_stack_check;    /* 栈深度检查 */
} WamrConfig;

/* 默认 MCU 配置 */
WamrConfig wamr_config_default(McuType mcu);

/* 模块加载 */
int wamr_module_load(const WamrConfig *cfg, WamrModule **out_module);
int wamr_module_free(WamrModule *mod);

/* 实例创建与执行 */
int wamr_instance_create(WamrModule *mod, const WamrConfig *cfg,
                         WamrInstance **out_inst);
int wamr_instance_start(WamrInstance *inst, const char *entry_func);
int wamr_instance_free(WamrInstance *inst);

/* 函数调用 */
int wamr_call_func(WamrInstance *inst, const char *func_name,
                   void **argv, int argc, void **result, int *result_count);

/* 内存操作 */
int wamr_mem_alloc(WamrInstance *inst, int size, void **ptr);
int wamr_mem_free(WamrInstance *inst, void *ptr);
int wamr_mem_read(WamrInstance *inst, int offset, void *buf, int size);
int wamr_mem_write(WamrInstance *inst, int offset, const void *buf, int size);

/* 实例状态查询 */
int wamr_instance_mem_pages(WamrInstance *inst);
int wamr_instance_mem_used(WamrInstance *inst);
int wamr_instance_stack_pages(WamrInstance *inst);
int wamr_instance_stack_used(WamrInstance *inst);
McuType wamr_instance_mcu_type(WamrInstance *inst);

/* 模块状态查询 */
size_t wamr_module_size(WamrModule *mod);
McuType wamr_module_mcu_type(WamrModule *mod);

/* MCU HAL 回调 */
typedef int (*WamrHalFunc)(int id, void *arg, void *ret, int *ret_count);

/* 注册 MCU 硬件回调 */
int wamr_register_hal(WamrInstance *inst, const char *name, WamrHalFunc cb, void *arg);

/* WASM 编译: 为 MCU 生成精简 WASM */
int wamr_compile_program(ASTNode *ast, const WamrConfig *cfg, const char *output);

/* 获取 MCU 目标名称 */
const char *wamr_mcu_name(McuType mcu);

/* 从字符串解析 MCU 类型 */
McuType wamr_parse_mcu(const char *s);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_WAMR_H */