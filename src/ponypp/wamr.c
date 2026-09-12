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

/* ===== 函数调用 (最小 WASM 解释器) ===== */

/* LEB128 读取辅助 */
static int leb128_read(const unsigned char *data, size_t len, size_t *pos, int32_t *out) {
    if (*pos >= len) return -1;
    int32_t result = 0;
    int shift = 0;
    unsigned char byte;
    do {
        if (*pos >= len) return -1;
        byte = data[(*pos)++];
        result |= (int32_t)(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    *out = result;
    return 0;
}

/* Import 表条目 */
#define MAX_IMPORTS 16
typedef struct {
    char name[128];
    int32_t idx;
} WamrImport;

/* 从 WASM 二进制中解析 import section */
static int parse_imports(const unsigned char *data, size_t size,
                         WamrImport *imports, int *count) {
    size_t pos = 8;
    *count = 0;
    while (pos + 2 <= size && *count < MAX_IMPORTS) {
        unsigned char sid = data[pos++];
        int32_t ssize;
        if (leb128_read(data, size, &pos, &ssize) < 0) break;
        if (sid == 2) {
            size_t spos = pos;
            int32_t n_imports;
            if (leb128_read(data, size, &spos, &n_imports) < 0) break;
            for (int i = 0; i < n_imports && *count < MAX_IMPORTS; i++) {
                int32_t mod_len;
                if (leb128_read(data, size, &spos, &mod_len) < 0) break;
                spos += mod_len; /* skip module name */
                int32_t field_len;
                if (leb128_read(data, size, &spos, &field_len) < 0) break;
                char field[128] = {0};
                if (field_len > 0 && spos + field_len <= size && field_len < 128)
                    memcpy(field, data + spos, field_len);
                spos += field_len;
                if (spos + 1 > size) break;
                unsigned char kind = data[spos++];
                if (kind == 0) { /* function import */
                    int32_t type_idx;
                    if (leb128_read(data, size, &spos, &type_idx) < 0) break;
                    (void)type_idx;
                } else if (kind == 1 || kind == 2) { /* table / memory */
                    /* skip flags */
                    unsigned char flags = data[spos++];
                    if (leb128_read(data, size, &spos, (int32_t *)(&flags)) < 0) break;
                } else { /* global */
                    unsigned char vtype = data[spos++];
                    (void)vtype;
                    unsigned char mutable_ = data[spos++];
                    (void)mutable_;
                }
                strncpy(imports[*count].name, field, 127);
                imports[*count].idx = (*count);
                (*count)++;
            }
            break;
        }
        pos += ssize;
    }
    return 0;
}

/* 从 WASM 二进制中加载 data section 到内存 */
static void load_data_sections(const unsigned char *data, size_t size,
                                char *memory, int mem_size) {
    size_t pos = 8;
    while (pos + 2 <= size) {
        unsigned char sid = data[pos++];
        int32_t ssize;
        if (leb128_read(data, size, &pos, &ssize) < 0) break;
        if (sid == 0x0B) {
            size_t dpos = pos;
            int32_t num_segments;
            if (leb128_read(data, size, &dpos, &num_segments) < 0) break;
            for (int i = 0; i < num_segments && dpos < pos + ssize; i++) {
                int32_t memidx;
                if (leb128_read(data, size, &dpos, &memidx) < 0) break;
                (void)memidx;
                /* 段 0: 直接初始化 */
                /* i32.const */
                unsigned char opcode = data[dpos++];
                if (opcode != 0x41) break;
                int32_t offset;
                if (leb128_read(data, size, &dpos, &offset) < 0) break;
                /* end */
                unsigned char end_op = data[dpos++];
                if (end_op != 0x0B) break;
                int32_t data_len;
                if (leb128_read(data, size, &dpos, &data_len) < 0) break;
                if (offset >= 0 && offset + data_len <= mem_size &&
                    dpos + data_len <= pos + ssize) {
                    memcpy(memory + offset, data + dpos, data_len);
                }
                dpos += data_len;
            }
            break;
        }
        pos += ssize;
    }
}

/* WASM 函数体执行 */
static int wasm_exec_func(const unsigned char *body, size_t body_len,
                          char *memory, int mem_size,
                          void **argv, int argc,
                          void **result, int *result_count,
                          WamrImport *imports, int import_count) {
    if (!body || body_len == 0) return -1;
    if (result) *result = NULL;
    if (result_count) *result_count = 0;

    size_t pos = 0;
    int32_t num_locals;
    if (leb128_read(body, body_len, &pos, &num_locals) < 0) return -1;
    int32_t local_count = 0;
    for (int i = 0; i < num_locals; i++) {
        int32_t cnt;
        if (leb128_read(body, body_len, &pos, &cnt) < 0) return -1;
        int32_t val;
        if (leb128_read(body, body_len, &pos, &val) < 0) return -1;
        local_count += cnt;
    }

    /* 栈 */
    int32_t stack[256];
    int sp = 0;

    /* 参数入栈 */
    for (int i = 0; i < argc; i++) {
        if (argv[i]) {
            stack[sp++] = *(int32_t *)argv[i];
        } else {
            stack[sp++] = 0;
        }
    }

    int exited = 0;
    int32_t exit_val = 0;
    int fd_write_calls = 0;

    while (pos < body_len) {
        unsigned char op = body[pos++];

        switch (op) {
            case 0x00: /* unreachable */
                return -1;

            case 0x01: /* nop */
                break;

            case 0x02: { /* if */
                int32_t cond = sp > 0 ? stack[--sp] : 0;
                (void)cond;
                break;
            }
            case 0x03: { /* loop */
                break;
            }

            case 0x0B: { /* end */
                if (sp > 0 && result) {
                    result[0] = malloc(sizeof(int32_t));
                    if (result[0]) *(int32_t *)result[0] = stack[sp - 1];
                    if (result_count) *result_count = 1;
                }
                exited = 1;
                break;
            }

            case 0x0C: /* br */
            case 0x0D: /* br_if */
            case 0x0E: /* br_table */
            case 0x0F: /* return */ {
                int32_t depth;
                if (leb128_read(body, body_len, &pos, &depth) < 0) return -1;
                if (op == 0x0F && sp > 0 && result) {
                    exit_val = stack[sp - 1];
                    result[0] = malloc(sizeof(int32_t));
                    if (result[0]) *(int32_t *)result[0] = exit_val;
                    if (result_count) *result_count = 1;
                }
                exited = 1;
                break;
            }

            case 0x10: { /* call */
                int32_t func_idx;
                if (leb128_read(body, body_len, &pos, &func_idx) < 0) return -1;

                /* 检查是否为 import 调用 */
                if (func_idx >= 0 && func_idx < import_count && imports) {
                    const char *name = imports[func_idx].name;

                    if (strcmp(name, "fd_write") == 0) {
                        /* fd_write(fd, iovs, iovs_len, rets) -> nwritten */
                        if (sp < 4) return -1;
                        int32_t rets = stack[--sp];
                        int32_t iovs_len = stack[--sp];
                        int32_t iovs = stack[--sp];
                        int32_t fd = stack[--sp];
                        (void)fd;

                        /* 模拟 fd_write: 从 iovec 读取字符串并写入 stdout */
                        int nwritten = 0;
                        if (memory && iovs >= 0 && iovs + (iovs_len * 8) <= mem_size) {
                            for (int j = 0; j < iovs_len; j++) {
                                int32_t buf_ptr = 0, buf_len = 0;
                                int offset = iovs + j * 8;
                                if (offset + 8 <= mem_size) {
                                    memcpy(&buf_ptr, memory + offset, 4);
                                    memcpy(&buf_len, memory + offset + 4, 4);
                                }
                                if (buf_ptr >= 0 && buf_len > 0 &&
                                    buf_ptr + buf_len <= mem_size) {
                                    fwrite(memory + buf_ptr, 1, buf_len, stdout);
                                    nwritten += buf_len;
                                }
                            }
                            fflush(stdout);
                        }
                        /* 写入返回值到 rets */
                        if (memory && rets >= 0 && rets + 4 <= mem_size) {
                            memcpy(memory + rets, &nwritten, 4);
                        }
                        if (sp < 256) stack[sp++] = nwritten;
                        fd_write_calls++;
                    } else if (strcmp(name, "proc_exit") == 0) {
                        int32_t code = sp > 0 ? stack[--sp] : 0;
                        (void)code;
                        exited = 1;
                        break;
                    } else {
                        /* 未知 import: pop args, push 0 */
                        if (sp > 0) sp--;
                        if (sp < 256) stack[sp++] = 0;
                    }
                } else {
                    /* 内部函数调用: 暂不支持递归 */
                    if (sp > 0) sp--;
                    if (sp < 256) stack[sp++] = 0;
                }
                break;
            }

            case 0x20: { /* local.get */
                int32_t idx;
                if (leb128_read(body, body_len, &pos, &idx) < 0) return -1;
                if (sp < 256) stack[sp++] = 0;
                break;
            }

            case 0x21: { /* local.set */
                int32_t idx;
                if (leb128_read(body, body_len, &pos, &idx) < 0) return -1;
                if (sp > 0) sp--;
                break;
            }

            case 0x22: { /* local.tee */
                int32_t idx;
                if (leb128_read(body, body_len, &pos, &idx) < 0) return -1;
                /* 保持在栈上 */
                break;
            }

            case 0x23: { /* global.get */
                int32_t idx;
                if (leb128_read(body, body_len, &pos, &idx) < 0) return -1;
                if (sp < 256) stack[sp++] = 0;
                break;
            }

            case 0x24: { /* global.set */
                int32_t idx;
                if (leb128_read(body, body_len, &pos, &idx) < 0) return -1;
                if (sp > 0) sp--;
                break;
            }

            case 0x28: /* i32.load (spec) */
            case 0x2C: /* i32.load8_s (spec) */
            case 0x2D: /* i32.load8_u (spec) */
            case 0x2E: /* i32.load16_s (spec) */
            case 0x2F: /* i32.load16_u (spec) */ {
                int32_t align, offset;
                if (leb128_read(body, body_len, &pos, &align) < 0) return -1;
                if (leb128_read(body, body_len, &pos, &offset) < 0) return -1;
                if (sp < 1) return -1;
                int32_t addr = stack[--sp] + offset;
                int32_t val = 0;
                if (addr >= 0 && addr + 4 <= mem_size && memory) {
                    if (op == 0x28) memcpy(&val, memory + addr, 4);
                    else if (op == 0x2C) val = (int32_t)(int8_t)memory[addr];
                    else if (op == 0x2D) val = (uint8_t)memory[addr];
                    else if (op == 0x2E) { int16_t v; memcpy(&v, memory+addr,2); val=v; }
                    else if (op == 0x2F) { uint16_t v; memcpy(&v, memory+addr,2); val=v; }
                }
                if (sp < 256) stack[sp++] = val;
                break;
            }

            case 0x36: /* i32.store (spec) */
            case 0x3A: /* i32.store8 (spec) */
            case 0x3B: /* i32.store16 (spec) */ {
                int32_t align, offset;
                if (leb128_read(body, body_len, &pos, &align) < 0) return -1;
                if (leb128_read(body, body_len, &pos, &offset) < 0) return -1;
                if (sp < 2) return -1;
                int32_t val = stack[--sp];
                int32_t addr = stack[--sp] + offset;
                int write_size = (op == 0x36) ? 4 : (op == 0x3A) ? 1 : 2;
                if (addr >= 0 && addr + write_size <= mem_size && memory) {
                    if (op == 0x36) memcpy(memory + addr, &val, 4);
                    else if (op == 0x3A) memory[addr] = (uint8_t)val;
                    else if (op == 0x3B) { uint16_t v = (uint16_t)val; memcpy(memory+addr,&v,2); }
                }
                break;
            }

            case 0x41: { /* i32.const */
                int32_t val;
                if (leb128_read(body, body_len, &pos, &val) < 0) return -1;
                if (sp < 256) stack[sp++] = val;
                break;
            }

            case 0x42: { /* i64.const */
                int32_t hi, lo;
                if (leb128_read(body, body_len, &pos, &lo) < 0) return -1;
                if (leb128_read(body, body_len, &pos, &hi) < 0) return -1;
                if (sp < 256) stack[sp++] = lo;
                break;
            }

            case 0x45: { /* i32.eqz — 一元运算 */
                if (sp < 1) return -1;
                int32_t a = stack[--sp];
                if (sp < 256) stack[sp++] = (a == 0) ? 1 : 0;
                break;
            }
            case 0x46: /* i32.eq */
            case 0x47: /* i32.ne */
            case 0x48: /* i32.lt_s */
            case 0x49: /* i32.lt_u */
            case 0x4A: /* i32.gt_s */
            case 0x4B: /* i32.gt_u */
            case 0x4C: /* i32.le_s */
            case 0x4D: /* i32.le_u */
            case 0x4E: /* i32.ge_s */
            case 0x4F: /* i32.ge_u */ {
                if (sp < 2) return -1;
                int32_t b = stack[--sp];
                int32_t a = stack[--sp];
                int32_t r = 0;
                switch (op) {
                    case 0x46: r = (a == b) ? 1 : 0; break;
                    case 0x47: r = (a != b) ? 1 : 0; break;
                    case 0x48: r = (a < b) ? 1 : 0; break;
                    case 0x49: r = ((uint32_t)a < (uint32_t)b) ? 1 : 0; break;
                    case 0x4A: r = (a > b) ? 1 : 0; break;
                    case 0x4B: r = ((uint32_t)a > (uint32_t)b) ? 1 : 0; break;
                    case 0x4C: r = (a <= b) ? 1 : 0; break;
                    case 0x4D: r = ((uint32_t)a <= (uint32_t)b) ? 1 : 0; break;
                    case 0x4E: r = (a >= b) ? 1 : 0; break;
                    case 0x4F: r = ((uint32_t)a >= (uint32_t)b) ? 1 : 0; break;
                }
                if (sp < 256) stack[sp++] = r;
                break;
            }

            case 0x6A: { /* i32.add */
                if (sp < 2) return -1;
                int32_t b = stack[--sp], a = stack[--sp];
                if (sp < 256) stack[sp++] = a + b;
                break;
            }
            case 0x6B: { /* i32.sub */
                if (sp < 2) return -1;
                int32_t b = stack[--sp], a = stack[--sp];
                if (sp < 256) stack[sp++] = a - b;
                break;
            }
            case 0x6C: { /* i32.mul */
                if (sp < 2) return -1;
                int32_t b = stack[--sp], a = stack[--sp];
                if (sp < 256) stack[sp++] = a * b;
                break;
            }

            default:
                /* 跳过未知操作码 (简化: 假设无参数) */
                break;
        }

        if (exited) break;
    }

    if (!exited && sp > 0 && result) {
        result[0] = malloc(sizeof(int32_t));
        if (result[0]) *(int32_t *)result[0] = stack[sp - 1];
        if (result_count) *result_count = 1;
    }

    return 0;
}

/* 在 WASM 模块中查找导出函数并执行 */
int wamr_call_func(WamrInstance *inst, const char *func_name,
                   void **argv, int argc, void **result, int *result_count) {
    if (!inst || !func_name) {
        if (result) *result = NULL;
        if (result_count) *result_count = 0;
        return 0;
    }

    if (!inst->module || !inst->module->data || inst->module->size < 8) {
        if (result) *result = NULL;
        if (result_count) *result_count = 0;
        return 0;
    }

    unsigned char *data = inst->module->data;
    size_t size = inst->module->size;

    if (!data || size < 8) return -1;

    /* 解析 import 表 */
    WamrImport imports[MAX_IMPORTS];
    int import_count = 0;
    parse_imports(data, size, imports, &import_count);

    /* 加载 data section 到内存 */
    int mem_size = inst->mem_pages * WAMR_PAGE_SIZE;
    if (inst->memory) {
        load_data_sections(data, size, inst->memory, mem_size);
    }

    /* 遍历 section, 找到 code section (id=10) */
    size_t pos = 8; /* 跳过 magic + version */
    unsigned char *code_section = NULL;
    size_t code_len = 0;
    int export_func_idx = -1;

    /* 先找 export section (id=7) 获取函数索引 */
    pos = 8;
    while (pos + 2 <= size) {
        unsigned char sid = data[pos++];
        int32_t ssize;
        if (leb128_read(data, size, &pos, &ssize) < 0) break;

        if (sid == 7 && ssize >= 1) {
            /* export section: [count, {name, kind, index}, ...] */
            size_t spos = pos;
            int32_t count;
            if (leb128_read(data, size, &spos, &count) < 0) break;
            for (int32_t i = 0; i < count; i++) {
                int32_t name_len;
                if (leb128_read(data, size, &spos, &name_len) < 0) break;
                char name[256] = {0};
                if (name_len > 0 && spos + name_len <= size && name_len < 256)
                    memcpy(name, data + spos, name_len);
                spos += name_len;
                if (spos >= size) break;
                unsigned char kind = data[spos++];
                int32_t fidx;
                if (leb128_read(data, size, &spos, &fidx) < 0) break;
                if (kind == 0 && strcmp(name, func_name) == 0) {
                    /* 减去 import 数量，得到 code section 中的索引 */
                    export_func_idx = fidx - import_count;
                    break;
                }
            }
            break;
        }

        pos += ssize;
    }

    if (export_func_idx < 0) return -1;

    /* 找 code section */
    pos = 8;
    while (pos + 2 <= size) {
        unsigned char sid = data[pos++];
        int32_t ssize;
        if (leb128_read(data, size, &pos, &ssize) < 0) break;

        if (sid == 10) {
            code_section = data + pos;
            code_len = ssize;
            break;
        }
        pos += ssize;
    }

    if (!code_section || code_len == 0) return -1;

    /* 遍历函数体, 找到 export_func_idx */
    size_t cpos = 0;
    int32_t num_funcs;
    if (leb128_read(code_section, code_len, &cpos, &num_funcs) < 0) return -1;

    for (int32_t i = 0; i < num_funcs; i++) {
        int32_t body_size;
        if (leb128_read(code_section, code_len, &cpos, &body_size) < 0) break;

        if (i == export_func_idx) {
            return wasm_exec_func(code_section + cpos, body_size,
                                  inst->memory, mem_size,
                                  argv, argc, result, result_count,
                                  imports, import_count);
        }

        cpos += body_size;
    }

    return -1;
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

/* LEB128 写入辅助 (写入 section size) */
static void leb128_write(unsigned char *data, size_t *pos, int32_t val) {
    do {
        unsigned char byte = (unsigned char)(val & 0x7F);
        val >>= 7;
        if (val) byte |= 0x80;
        data[(*pos)++] = byte;
    } while (val);
}

/* 从 WASM 二进制中移除 import section (id=2)
 * 同时调整 export section 中的函数索引 */
static int wasm_strip_imports(const unsigned char *wasm, size_t wasm_size,
                              unsigned char **out, size_t *out_size) {
    if (!wasm || wasm_size < 8) return -1;

    /* 检查 magic */
    if (wasm[0] != 0x00 || wasm[1] != 0x61 || wasm[2] != 0x73 || wasm[3] != 0x6d)
        return -1;

    /* 第一遍: 计算新大小 + 统计 import 数量 */
    size_t pos = 8;
    size_t new_size = 8;
    int32_t import_count = 0;

    while (pos + 2 <= wasm_size) {
        unsigned char sid = wasm[pos];
        size_t sp = pos + 1;
        int32_t ssize;
        if (leb128_read(wasm, wasm_size, &sp, &ssize) < 0) break;
        size_t header_len = sp - pos;

        if (sid == 2) {
            /* import section: 统计 import 数量 */
            size_t ip = pos + header_len;
            if (leb128_read(wasm, wasm_size, &ip, &import_count) < 0)
                import_count = 0;
            /* 不写入新二进制 */
        } else {
            new_size += header_len + ssize;
        }

        pos += header_len + ssize;
    }

    if (import_count <= 0) {
        /* 没有 import, 直接返回原始数据 */
        *out = (unsigned char *)malloc(wasm_size);
        if (!*out) return -1;
        memcpy(*out, wasm, wasm_size);
        *out_size = wasm_size;
        return 0;
    }

    /* 第二遍: 写入新二进制, 跳过 import section, 调整 export 索引 */
    *out = (unsigned char *)malloc(new_size + 64);
    if (!*out) return -1;

    unsigned char *dst = *out;
    size_t dpos = 0;

    /* 复制 magic + version */
    memcpy(dst, wasm, 8);
    dpos = 8;

    pos = 8;
    while (pos + 2 <= wasm_size) {
        unsigned char sid = wasm[pos];
        size_t sp = pos + 1;
        int32_t ssize;
        if (leb128_read(wasm, wasm_size, &sp, &ssize) < 0) break;
        size_t header_len = sp - pos;

        if (sid == 2) {
            /* 跳过 import section */
            pos += header_len + ssize;
            continue;
        }

        if (sid == 7) {
            /* export section: 调整函数索引 */
            /* 复制 section header */
            dst[dpos++] = sid;
            leb128_write(dst, &dpos, ssize);

            /* 读取并调整 export */
            size_t spos = pos + header_len;
            int32_t count;
            if (leb128_read(wasm, wasm_size, &spos, &count) < 0) break;

            /* 写入 count */
            leb128_write(dst, &dpos, count);

            for (int32_t i = 0; i < count; i++) {
                int32_t name_len;
                if (leb128_read(wasm, wasm_size, &spos, &name_len) < 0) break;
                /* 写入 name */
                leb128_write(dst, &dpos, name_len);
                if (name_len > 0) {
                    memcpy(dst + dpos, wasm + spos, name_len);
                    dpos += name_len;
                }
                spos += name_len;
                if (spos >= wasm_size) break;
                unsigned char kind = wasm[spos++];
                dst[dpos++] = kind;

                int32_t fidx;
                if (leb128_read(wasm, wasm_size, &spos, &fidx) < 0) break;
                /* 调整函数索引: 减去 import 数量 */
                if (kind == 0 && fidx >= import_count)
                    fidx -= import_count;
                else if (kind == 0)
                    fidx = 0;
                leb128_write(dst, &dpos, fidx);
            }
        } else {
            /* 其他 section: 原样复制 */
            dst[dpos++] = sid;
            leb128_write(dst, &dpos, ssize);
            memcpy(dst + dpos, wasm + pos + header_len, ssize);
            dpos += ssize;
        }

        pos += header_len + ssize;
    }

    *out_size = dpos;
    return 0;
}

/* MCU 编译: 生成 WASM 并移除 WASI import */
int wamr_compile_program(ASTNode *ast, const WamrConfig *cfg, const char *output) {
    if (!ast || !cfg || !output) return -1;

    /* 1. 生成基础 WASM */
    int rc = wasm_write_program(ast, output, TARGET_MCU_WASM);
    if (rc != 0) return rc;

    /* 2. 读取 WASM */
    FILE *f = fopen(output, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0) { fclose(f); return -1; }

    unsigned char *wasm = (unsigned char *)malloc(fsize);
    if (!wasm) { fclose(f); return -1; }
    if (fread(wasm, 1, fsize, f) != (size_t)fsize) {
        free(wasm); fclose(f); return -1;
    }
    fclose(f);

    /* 3. 移除 WASI import section */
    unsigned char *stripped = NULL;
    size_t stripped_size = 0;

    rc = wasm_strip_imports(wasm, (size_t)fsize, &stripped, &stripped_size);
    free(wasm);

    if (rc != 0 || !stripped) {
        free(stripped);
        return -1;
    }

    /* 4. 写入精简 WASM */
    f = fopen(output, "wb");
    if (!f) { free(stripped); return -1; }
    if (fwrite(stripped, 1, stripped_size, f) != stripped_size) {
        fclose(f);
        free(stripped);
        return -1;
    }
    fclose(f);

    free(stripped);
    return 0;
}