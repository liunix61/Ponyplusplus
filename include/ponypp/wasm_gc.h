/*
 * wasm_gc.h - Wasm GC 双后端抽象层
 *
 * 编译开关:
 *   PONYPP_USE_WASM_GC  - 启用 Wasmtime 47 Wasm GC 后端
 *   未定义              - 使用内置 Cheney 复制 GC (默认)
 *
 * Wasm GC 对象模型 (Wasmtime 47):
 *   structref  - 结构体引用 (对应 Pony Actor 字段)
 *   arrayref   - 数组引用 (对应 Pony Array)
 *   i31ref     - 31位整数引用 (小整数优化)
 *   externref  - 外部引用 (跨组件零拷贝)
 *   anyref     - 任意引用 (类型层次根)
 *
 * 与 Pony++ 引用能力映射:
 *   iso -> structref (唯一引用, 消费语义)
 *   trn -> structref (转移引用, 同Actor内)
 *   ref -> structref (本地可变)
 *   val -> i31ref/externref (全局不可变, 值复制)
 *   box -> externref (本地只读)
 *   tag -> externref (仅标识)
 */

#ifndef PONYPP_WASM_GC_H
#define PONYPP_WASM_GC_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* ==================== Wasm GC 类型系统 ==================== */

/* Wasm GC 引用类型 (对应 Wasm 规范) */
typedef enum {
    WGC_REF_NULL = 0,    /* nullref */
    WGC_REF_STRUCT,      /* structref */
    WGC_REF_ARRAY,       /* arrayref */
    WGC_REF_I31,         /* i31ref */
    WGC_REF_EXTERN,      /* externref */
    WGC_REF_FUNC,        /* funcref */
    WGC_REF_EQ,          /* eqref (等价性比较) */
    WGC_REF_ANY          /* anyref (类型层次根) */
} WasmGCRefType;

/* Wasm GC 字段类型 */
typedef enum {
    WGC_FIELD_I32 = 0,
    WGC_FIELD_I64,
    WGC_FIELD_F32,
    WGC_FIELD_F64,
    WGC_FIELD_REF,       /* 任意引用 */
    WGC_FIELD_I31        /* i31 小整数 */
} WasmGCFieldType;

/* Wasm GC 引用句柄 (不透明) */
typedef struct WasmGCRef {
    WasmGCRefType type;
    void *ptr;           /* Cheney: 数据指针 | Wasm GC: wasmtime ref */
    uint32_t field_count;
    bool is_null;
} WasmGCRef;

/* Wasm GC 堆 (不透明) */
typedef struct WasmGCHeap WasmGCHeap;

/* Wasm GC 字段描述 */
typedef struct {
    const char *name;
    WasmGCFieldType type;
    bool mutable_field;  /* ref/box 可变, val 不可变 */
} WasmGCFieldDesc;

/* ==================== 堆管理 ==================== */

/* 创建 Wasm GC 堆
 * backend: "cheney" (默认) 或 "wasmgc" (需要 PONYPP_USE_WASM_GC)
 */
WasmGCHeap *wgc_heap_new(const char *backend);

/* 释放堆 */
void wgc_heap_free(WasmGCHeap *heap);

/* 获取当前后端名称 */
const char *wgc_heap_backend(const WasmGCHeap *heap);

/* 检查是否支持 Wasm GC 后端 */
bool wgc_has_wasm_backend(void);

/* ==================== structref 操作 ==================== */

/* 创建 structref (对应 Pony Actor/对象) */
WasmGCRef *wgc_struct_new(WasmGCHeap *heap, const WasmGCFieldDesc *fields, uint32_t field_count);

/* 读取字段 */
int32_t wgc_struct_get_i32(const WasmGCRef *ref, uint32_t field_idx);
int64_t wgc_struct_get_i64(const WasmGCRef *ref, uint32_t field_idx);
double wgc_struct_get_f64(const WasmGCRef *ref, uint32_t field_idx);
WasmGCRef *wgc_struct_get_ref(const WasmGCRef *ref, uint32_t field_idx);

/* 写入字段 */
int wgc_struct_set_i32(WasmGCRef *ref, uint32_t field_idx, int32_t val);
int wgc_struct_set_i64(WasmGCRef *ref, uint32_t field_idx, int64_t val);
int wgc_struct_set_f64(WasmGCRef *ref, uint32_t field_idx, double val);
int wgc_struct_set_ref(WasmGCRef *ref, uint32_t field_idx, WasmGCRef *val);

/* ==================== arrayref 操作 ==================== */

/* 创建 arrayref */
WasmGCRef *wgc_array_new(WasmGCHeap *heap, WasmGCFieldType elem_type, uint32_t length);

/* 数组长度 */
uint32_t wgc_array_length(const WasmGCRef *ref);

/* 数组元素访问 */
int32_t wgc_array_get_i32(const WasmGCRef *ref, uint32_t idx);
int wgc_array_set_i32(WasmGCRef *ref, uint32_t idx, int32_t val);

/* ==================== i31ref 操作 ==================== */

/* 创建 i31ref (31位小整数) */
WasmGCRef *wgc_i31_new(WasmGCHeap *heap, int32_t val);

/* 读取 i31 值 */
int32_t wgc_i31_get(const WasmGCRef *ref);

/* ==================== externref 操作 ==================== */

/* 创建 externref (跨组件引用) */
WasmGCRef *wgc_extern_new(WasmGCHeap *heap, void *host_ptr);

/* 获取宿主指针 */
void *wgc_extern_get(const WasmGCRef *ref);

/* ==================== 引用操作 ==================== */

/* 释放引用句柄 (不回收对象, 仅释放句柄) */
void wgc_ref_free(WasmGCRef *ref);

/* 判断是否为 null */
bool wgc_ref_is_null(const WasmGCRef *ref);

/* 获取引用类型 */
WasmGCRefType wgc_ref_type(const WasmGCRef *ref);

/* 等价性比较 (eqref) */
bool wgc_ref_eq(const WasmGCRef *a, const WasmGCRef *b);

/* ==================== GC 操作 ==================== */

/* 触发回收 */
void wgc_collect(WasmGCHeap *heap);

/* 注册根引用 */
int wgc_add_root(WasmGCHeap *heap, WasmGCRef *ref);
int wgc_remove_root(WasmGCHeap *heap, WasmGCRef *ref);

/* 堆统计 */
typedef struct {
    const char *backend;
    size_t total_alloc;
    size_t total_freed;
    size_t live_objects;
    int collections;
    double occupancy;
} WasmGCStats;

void wgc_stats(const WasmGCHeap *heap, WasmGCStats *out);

/* ==================== Pony++ 能力映射 ==================== */

/* 引用能力 → Wasm GC 引用类型映射 */
WasmGCRefType wgc_cap_to_ref_type(uint8_t pony_cap);

/* 发送检查: 该能力是否可跨 Actor 发送 */
bool wgc_cap_sendable(uint8_t pony_cap);

/* 消费语义: iso 发送后降级为 box */
uint8_t wgc_cap_consume(uint8_t pony_cap);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_WASM_GC_H */
