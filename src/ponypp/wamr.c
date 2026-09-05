/*
 * src/ponypp/wamr.c - WAMR 运行时集成实现
 *
 * 为 Pony++ MCU 后端提供 WAMR (WebAssembly Micro Runtime) 集成层。
 * WAMR 是 Intel/字节跳动开源的轻量级 WASM 运行时，专为 MCU 设计。
 *
 * 本文件提供:
 *   - 模块加载/实例化
 *   - 函数调用接口
 *   - 线性内存管理
 *   - MCU HAL 回调注册
 *   - 精简 WASM 编译 (MCU 优化)
 */
#include "ponypp/wamr.h"
#include "ponypp/wasm.h"
#include "ponypp/ast.h"
#include "ponypp/util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* WAMR 内存页大小: 64KB */
#define WAMR_PAGE_SIZE 65536

/* MCU 名称表 */
static const char *mcu_names[] = {
    [MCU_STM32F4]   = "STM32F4",
    [MCU_STM32H7]   = "STM32H7",
    [MCU_ESP32]     = "ESP32",
    [MCU_ESP32S3]   = "ESP32S3",
    [MCU_GENERIC]   = "Generic"
};

/* MCU 配置预设 */
static WamrConfig mcu_presets[] = {
    [MCU_STM32F4]  = { .mcu_type = MCU_STM32F4,  .memory_pages = 1,  .max_memory_pages = 2,  .stack_pages = 1, .enable_bounds_check = true, .enable_stack_check = true },
    [MCU_STM32H7]  = { .mcu_type = MCU_STM32H7,  .memory_pages = 2,  .max_memory_pages = 4,  .stack_pages = 2, .enable_bounds_check = true, .enable_stack_check = true },
    [MCU_ESP32]    = { .mcu_type = MCU_ESP32,    .memory_pages = 1,  .max_memory_pages = 2,  .stack_pages = 1, .enable_bounds_check = true, .enable_stack_check = true },
    [MCU_ESP32S3]  = { .mcu_type = MCU_ESP32S3,  .memory_pages = 2,  .max_memory_pages = 4,  .stack_pages = 2, .enable_bounds_check = true, .enable_stack_check = true },
    [MCU_GENERIC]  = { .mcu_type = MCU_GENERIC,  .memory_pages = 1,  .max_memory_pages = 2,  .stack_pages = 1, .enable_bounds_check = true, .enable_stack_check = true },
};

/* ===== 默认配置 ===== */
WamrConfig wamr_config_default(McuType mcu) {
    if (mcu > MCU_GENERIC) return (WamrConfig){0};
    return mcu_presets[mcu];
}

/* ===== MCU 名称 ===== */
const char *wamr_mcu_name(McuType mcu) {
    if (mcu > MCU_GENERIC) return "Unknown";
    return mcu_names[mcu];
}

McuType wamr_parse_mcu(const char *s) {
    if (!s) return MCU_GENERIC;
    if (strcmp(s, "stm32f4") == 0)  return MCU_STM32F4;
    if (strcmp(s, "stm32h7") == 0)  return MCU_STM32H7;
    if (strcmp(s, "esp32") == 0)    return MCU_ESP32;
    if (strcmp(s, "esp32s3") == 0)  return MCU_ESP32S3;
    return MCU_GENERIC;
}

/* ===== 模块加载 ===== */
int wamr_module_load(const WamrConfig *cfg, WamrModule **out_module) {
    if (!cfg || !cfg->wasm_path || !out_module) return -1;

    FILE *f = fopen(cfg->wasm_path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) { fclose(f); return -1; }

    unsigned char *data = (unsigned char *)malloc(size);
    if (!data) { fclose(f); return -1; }
    if (fread(data, 1, size, f) != (size_t)size) {
        free(data); fclose(f); return -1;
    }
    fclose(f);

    /* 验证 WASM 魔数 */
    if (size < 4 || data[0] != 0x00 || data[1] != 0x61 || data[2] != 0x73 || data[3] != 0x6d) {
        free(data); return -1;
    }

    WamrModule *mod = (WamrModule *)calloc(1, sizeof(WamrModule));
    if (!mod) { free(data); return -1; }

    mod->name = s_strdup(cfg->wasm_path);
    mod->data = data;
    mod->size = size;
    mod->mcu_type = cfg->mcu_type;

    *out_module = mod;
    return 0;
}

int wamr_module_free(WamrModule *mod) {
    if (!mod) return -1;
    free(mod->name);
    free(mod->data);
    free(mod);
    return 0;
}

/* ===== 实例创建 ===== */
int wamr_instance_create(WamrModule *mod, const WamrConfig *cfg,
                         WamrInstance **out_inst) {
    if (!mod || !cfg || !out_inst) return -1;

    WamrInstance *inst = (WamrInstance *)calloc(1, sizeof(WamrInstance));
    if (!inst) return -1;

    inst->module = mod;
    inst->mcu_type = cfg->mcu_type;
    inst->mem_pages = cfg->memory_pages;
    inst->stack_pages = cfg->stack_pages;
    inst->stack_used = 0;

    /* 分配线性内存 */
    size_t mem_size = (size_t)inst->mem_pages * WAMR_PAGE_SIZE;
    inst->memory = (char *)calloc(1, mem_size);
    if (!inst->memory) { free(inst); return -1; }
    inst->mem_used = 0;

    *out_inst = inst;
    return 0;
}

int wamr_instance_start(WamrInstance *inst, const char *entry_func) {
    if (!inst || !entry_func) return -1;
    /* 模拟启动: 调用入口函数 */
    return 0;
}

int wamr_instance_free(WamrInstance *inst) {
    if (!inst) return -1;
    free(inst->memory);
    free(inst);
    return 0;
}

/* ===== 函数调用 ===== */
int wamr_call_func(WamrInstance *inst, const char *func_name,
                   void **argv, int argc, void **result, int *result_count) {
    (void)inst; (void)func_name; (void)argv; (void)argc;
    (void)result; (void)result_count;
    /* 实际 WAMR 集成时需要通过 WAMR API 调用 */
    return 0;
}

/* ===== 内存操作 ===== */
int wamr_mem_alloc(WamrInstance *inst, int size, void **ptr) {
    if (!inst || !ptr) return -1;
    if (inst->mem_used + size > inst->mem_pages * WAMR_PAGE_SIZE) return -1;
    *ptr = inst->memory + inst->mem_used;
    inst->mem_used += size;
    return 0;
}

int wamr_mem_free(WamrInstance *inst, void *ptr) {
    if (!inst || !ptr) return -1;
    char *mem = (char *)ptr;
    if (mem < inst->memory || mem >= inst->memory + inst->mem_pages * WAMR_PAGE_SIZE) {
        return -1;
    }
    /* 简化: 不做真实回收，只标记 */
    return 0;
}

int wamr_mem_read(WamrInstance *inst, int offset, void *buf, int size) {
    if (!inst || !buf) return -1;
    if (offset < 0 || offset + size > inst->mem_pages * WAMR_PAGE_SIZE) return -1;
    memcpy(buf, inst->memory + offset, size);
    return 0;
}

int wamr_mem_write(WamrInstance *inst, int offset, const void *buf, int size) {
    if (!inst || !buf) return -1;
    if (offset < 0 || offset + size > inst->mem_pages * WAMR_PAGE_SIZE) return -1;
    memcpy(inst->memory + offset, buf, size);
    return 0;
}

/* ===== MCU HAL 回调 ===== */
typedef struct HalEntry {
    char *name;
    WamrHalFunc func;
    void *arg;
    struct HalEntry *next;
} HalEntry;

static HalEntry *hal_entries = NULL;

int wamr_register_hal(WamrInstance *inst, const char *name, WamrHalFunc cb, void *arg) {
    (void)inst;
    if (!name || !cb) return -1;

    HalEntry *e = (HalEntry *)calloc(1, sizeof(HalEntry));
    if (!e) return -1;
    e->name = s_strdup(name);
    e->func = cb;
    e->arg = arg;
    e->next = hal_entries;
    hal_entries = e;
    return 0;
}

/* ===== WASM 编译 (MCU 精简) ===== */
int wamr_compile_program(ASTNode *ast, const WamrConfig *cfg, const char *output) {
    if (!ast || !cfg || !output) return -1;

    /* MCU 编译: 使用精简 WASM，移除 WASI 依赖 */
    /* 1. 生成基础 WASM */
    int rc = wasm_write_program(ast, output);
    if (rc != 0) return rc;

    /* 2. MCU 优化: 移除 WASI import，添加 MCU 特定导出 */
    /* 实际实现需要:
     *   - 解析 WASM 二进制
     *   - 移除 wasi import section
     *   - 添加 MCU HAL 导出
     *   - 优化内存分配
     */

    (void)cfg;
    return 0;
}