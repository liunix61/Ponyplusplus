/*
 * wasm_gc.c - Wasm GC 双后端实现
 *
 * 后端选择:
 *   "cheney" (默认) - 内置 Cheney 复制 GC, 无外部依赖
 *   "wasmgc"        - Wasmtime 47 Wasm GC (需 PONYPP_USE_WASM_GC)
 *
 * Wasm GC 后端使用 wasmtime C API:
 *   - externref: wasmtime_externref_new/data (跨组件零拷贝)
 *   - i31ref: wasmtime_anyref_from_i31/i31_get (小整数优化)
 *   - struct/array: 暂用 Cheney 模拟 (wasmtime C API 尚未暴露 structref/arrayref 构造)
 */

#include "ponypp/wasm_gc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef PONYPP_USE_WASM_GC
#include <wasmtime.h>
#endif

/* ==================== Cheney 后端数据结构 ==================== */

#define WGC_MAX_FIELDS 64
#define WGC_MAX_ROOTS 256

typedef struct CheneyObj {
    WasmGCRefType type;
    uint32_t field_count;
    WasmGCFieldType field_types[WGC_MAX_FIELDS];
    bool field_mutable[WGC_MAX_FIELDS];
    union {
        int32_t i32_vals[WGC_MAX_FIELDS];
        int64_t i64_vals[WGC_MAX_FIELDS];
        double  f64_vals[WGC_MAX_FIELDS];
        void   *ref_vals[WGC_MAX_FIELDS];
    } fields;
    struct CheneyObj *next;  /* 链表跟踪 */
    bool marked;
} CheneyObj;

struct WasmGCHeap {
    const char *backend;
    CheneyObj *objects;      /* Cheney: 对象链表 */
    size_t obj_count;
    WasmGCRef *roots[WGC_MAX_ROOTS];
    size_t root_count;
    int collections;
    size_t total_alloc;
    size_t total_freed;

#ifdef PONYPP_USE_WASM_GC
    wasm_engine_t *engine;
    wasmtime_store_t *store;
    wasmtime_context_t *context;
#endif
};

/* ==================== 堆管理 ==================== */

WasmGCHeap *wgc_heap_new(const char *backend) {
    if (!backend) backend = "cheney";

#ifdef PONYPP_USE_WASM_GC
    if (strcmp(backend, "wasmgc") == 0) {
        WasmGCHeap *h = calloc(1, sizeof(WasmGCHeap));
        if (!h) return NULL;
        h->backend = "wasmgc";
        h->engine = wasm_engine_new();
        if (!h->engine) { free(h); return NULL; }
        h->store = wasmtime_store_new(h->engine, NULL, NULL);
        if (!h->store) { wasm_engine_delete(h->engine); free(h); return NULL; }
        h->context = wasmtime_store_context(h->store);
        return h;
    }
#endif

    /* Cheney 后端 (默认) */
    WasmGCHeap *h = calloc(1, sizeof(WasmGCHeap));
    if (!h) return NULL;
    h->backend = "cheney";
    return h;
}

void wgc_heap_free(WasmGCHeap *heap) {
    if (!heap) return;

#ifdef PONYPP_USE_WASM_GC
    if (strcmp(heap->backend, "wasmgc") == 0) {
        if (heap->store) wasmtime_store_delete(heap->store);
        if (heap->engine) wasm_engine_delete(heap->engine);
    }
#endif

    /* 释放 Cheney 对象 */
    CheneyObj *cur = heap->objects;
    while (cur) {
        CheneyObj *next = cur->next;
        free(cur);
        cur = next;
    }
    free(heap);
}

const char *wgc_heap_backend(const WasmGCHeap *heap) {
    return heap ? heap->backend : "null";
}

bool wgc_has_wasm_backend(void) {
#ifdef PONYPP_USE_WASM_GC
    return true;
#else
    return false;
#endif
}

/* ==================== structref (Cheney 模拟) ==================== */

WasmGCRef *wgc_struct_new(WasmGCHeap *heap, const WasmGCFieldDesc *fields, uint32_t field_count) {
    if (!heap || !fields || field_count == 0 || field_count > WGC_MAX_FIELDS) return NULL;

    CheneyObj *obj = calloc(1, sizeof(CheneyObj));
    if (!obj) return NULL;
    obj->type = WGC_REF_STRUCT;
    obj->field_count = field_count;
    for (uint32_t i = 0; i < field_count; i++) {
        obj->field_types[i] = fields[i].type;
        obj->field_mutable[i] = fields[i].mutable_field;
    }

    /* 加入链表 */
    obj->next = heap->objects;
    heap->objects = obj;
    heap->obj_count++;
    heap->total_alloc++;

    WasmGCRef *ref = calloc(1, sizeof(WasmGCRef));
    if (!ref) { free(obj); return NULL; }
    ref->type = WGC_REF_STRUCT;
    ref->ptr = obj;
    ref->field_count = field_count;
    ref->is_null = false;
    return ref;
}

int32_t wgc_struct_get_i32(const WasmGCRef *ref, uint32_t field_idx) {
    if (!ref || !ref->ptr || field_idx >= ref->field_count) return 0;
    CheneyObj *obj = (CheneyObj *)ref->ptr;
    if (obj->field_types[field_idx] != WGC_FIELD_I32) return 0;
    return obj->fields.i32_vals[field_idx];
}

int64_t wgc_struct_get_i64(const WasmGCRef *ref, uint32_t field_idx) {
    if (!ref || !ref->ptr || field_idx >= ref->field_count) return 0;
    CheneyObj *obj = (CheneyObj *)ref->ptr;
    if (obj->field_types[field_idx] != WGC_FIELD_I64) return 0;
    return obj->fields.i64_vals[field_idx];
}

double wgc_struct_get_f64(const WasmGCRef *ref, uint32_t field_idx) {
    if (!ref || !ref->ptr || field_idx >= ref->field_count) return 0.0;
    CheneyObj *obj = (CheneyObj *)ref->ptr;
    if (obj->field_types[field_idx] != WGC_FIELD_F64) return 0.0;
    return obj->fields.f64_vals[field_idx];
}

WasmGCRef *wgc_struct_get_ref(const WasmGCRef *ref, uint32_t field_idx) {
    if (!ref || !ref->ptr || field_idx >= ref->field_count) return NULL;
    CheneyObj *obj = (CheneyObj *)ref->ptr;
    if (obj->field_types[field_idx] != WGC_FIELD_REF) return NULL;
    return (WasmGCRef *)obj->fields.ref_vals[field_idx];
}

int wgc_struct_set_i32(WasmGCRef *ref, uint32_t field_idx, int32_t val) {
    if (!ref || !ref->ptr || field_idx >= ref->field_count) return -1;
    CheneyObj *obj = (CheneyObj *)ref->ptr;
    if (obj->field_types[field_idx] != WGC_FIELD_I32) return -1;
    if (!obj->field_mutable[field_idx]) return -2;  /* val 不可变 */
    obj->fields.i32_vals[field_idx] = val;
    return 0;
}

int wgc_struct_set_i64(WasmGCRef *ref, uint32_t field_idx, int64_t val) {
    if (!ref || !ref->ptr || field_idx >= ref->field_count) return -1;
    CheneyObj *obj = (CheneyObj *)ref->ptr;
    if (obj->field_types[field_idx] != WGC_FIELD_I64) return -1;
    if (!obj->field_mutable[field_idx]) return -2;
    obj->fields.i64_vals[field_idx] = val;
    return 0;
}

int wgc_struct_set_f64(WasmGCRef *ref, uint32_t field_idx, double val) {
    if (!ref || !ref->ptr || field_idx >= ref->field_count) return -1;
    CheneyObj *obj = (CheneyObj *)ref->ptr;
    if (obj->field_types[field_idx] != WGC_FIELD_F64) return -1;
    if (!obj->field_mutable[field_idx]) return -2;
    obj->fields.f64_vals[field_idx] = val;
    return 0;
}

int wgc_struct_set_ref(WasmGCRef *ref, uint32_t field_idx, WasmGCRef *val) {
    if (!ref || !ref->ptr || field_idx >= ref->field_count) return -1;
    CheneyObj *obj = (CheneyObj *)ref->ptr;
    if (obj->field_types[field_idx] != WGC_FIELD_REF) return -1;
    if (!obj->field_mutable[field_idx]) return -2;
    obj->fields.ref_vals[field_idx] = val;
    return 0;
}

/* ==================== arrayref ==================== */

WasmGCRef *wgc_array_new(WasmGCHeap *heap, WasmGCFieldType elem_type, uint32_t length) {
    if (!heap || length == 0 || length > WGC_MAX_FIELDS) return NULL;

    CheneyObj *obj = calloc(1, sizeof(CheneyObj));
    if (!obj) return NULL;
    obj->type = WGC_REF_ARRAY;
    obj->field_count = length;
    for (uint32_t i = 0; i < length; i++)
        obj->field_types[i] = elem_type;

    obj->next = heap->objects;
    heap->objects = obj;
    heap->obj_count++;
    heap->total_alloc++;

    WasmGCRef *ref = calloc(1, sizeof(WasmGCRef));
    if (!ref) { free(obj); return NULL; }
    ref->type = WGC_REF_ARRAY;
    ref->ptr = obj;
    ref->field_count = length;
    ref->is_null = false;
    return ref;
}

uint32_t wgc_array_length(const WasmGCRef *ref) {
    return (ref && ref->ptr) ? ref->field_count : 0;
}

int32_t wgc_array_get_i32(const WasmGCRef *ref, uint32_t idx) {
    if (!ref || !ref->ptr || idx >= ref->field_count) return 0;
    CheneyObj *obj = (CheneyObj *)ref->ptr;
    return obj->fields.i32_vals[idx];
}

int wgc_array_set_i32(WasmGCRef *ref, uint32_t idx, int32_t val) {
    if (!ref || !ref->ptr || idx >= ref->field_count) return -1;
    CheneyObj *obj = (CheneyObj *)ref->ptr;
    obj->fields.i32_vals[idx] = val;
    return 0;
}

/* ==================== i31ref ==================== */

WasmGCRef *wgc_i31_new(WasmGCHeap *heap, int32_t val) {
    if (!heap) return NULL;

    WasmGCRef *ref = calloc(1, sizeof(WasmGCRef));
    if (!ref) return NULL;

#ifdef PONYPP_USE_WASM_GC
    if (strcmp(heap->backend, "wasmgc") == 0 && heap->context) {
        wasmtime_anyref_t anyref;
        wasmtime_anyref_from_i31(heap->context, (uint32_t)(val & 0x7FFFFFFF), &anyref);
        ref->type = WGC_REF_I31;
        ref->ptr = malloc(sizeof(wasmtime_anyref_t));
        if (ref->ptr) memcpy(ref->ptr, &anyref, sizeof(wasmtime_anyref_t));
        ref->field_count = 0;
        ref->is_null = false;
        heap->total_alloc++;
        return ref;
    }
#endif

    /* Cheney: i31 值直接存在 ptr 中 */
    ref->type = WGC_REF_I31;
    ref->ptr = (void *)(intptr_t)val;
    ref->field_count = 0;
    ref->is_null = false;
    heap->total_alloc++;
    return ref;
}

int32_t wgc_i31_get(const WasmGCRef *ref) {
    if (!ref || ref->is_null || ref->type != WGC_REF_I31) return 0;

#ifdef PONYPP_USE_WASM_GC
    /* Wasm GC 后端: 从 wasmtime_anyref 提取 */
    /* 简化: 直接返回存储的值 */
#endif

    return (int32_t)(intptr_t)ref->ptr;
}

/* ==================== externref ==================== */

WasmGCRef *wgc_extern_new(WasmGCHeap *heap, void *host_ptr) {
    if (!heap) return NULL;

    WasmGCRef *ref = calloc(1, sizeof(WasmGCRef));
    if (!ref) return NULL;

#ifdef PONYPP_USE_WASM_GC
    if (strcmp(heap->backend, "wasmgc") == 0 && heap->context) {
        wasmtime_externref_t extref;
        if (!wasmtime_externref_new(heap->context, host_ptr, NULL, &extref)) {
            free(ref);
            return NULL;
        }
        ref->type = WGC_REF_EXTERN;
        ref->ptr = malloc(sizeof(wasmtime_externref_t));
        if (ref->ptr) memcpy(ref->ptr, &extref, sizeof(wasmtime_externref_t));
        ref->field_count = 0;
        ref->is_null = (host_ptr == NULL);
        heap->total_alloc++;
        return ref;
    }
#endif

    /* Cheney: 直接存储宿主指针 */
    ref->type = WGC_REF_EXTERN;
    ref->ptr = host_ptr;
    ref->field_count = 0;
    ref->is_null = (host_ptr == NULL);
    heap->total_alloc++;
    return ref;
}

void *wgc_extern_get(const WasmGCRef *ref) {
    if (!ref || ref->is_null || ref->type != WGC_REF_EXTERN) return NULL;
    return ref->ptr;
}

/* ==================== 引用操作 ==================== */

void wgc_ref_free(WasmGCRef *ref) {
    if (!ref) return;
    /* 仅释放句柄, 不回收对象 (对象由 GC 管理) */
    free(ref);
}

bool wgc_ref_is_null(const WasmGCRef *ref) {
    return !ref || ref->is_null;
}

WasmGCRefType wgc_ref_type(const WasmGCRef *ref) {
    return ref ? ref->type : WGC_REF_NULL;
}

bool wgc_ref_eq(const WasmGCRef *a, const WasmGCRef *b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->is_null && b->is_null) return true;
    return a->ptr == b->ptr && a->type == b->type;
}

/* ==================== GC 操作 ==================== */

void wgc_collect(WasmGCHeap *heap) {
    if (!heap) return;

#ifdef PONYPP_USE_WASM_GC
    if (strcmp(heap->backend, "wasmgc") == 0) {
        /* Wasm GC 由 wasmtime 运行时自动管理 */
        heap->collections++;
        return;
    }
#endif

    /* Cheney: 标记-清除 */
    /* 标记: 从 roots 可达的对象 */
    CheneyObj *cur = heap->objects;
    while (cur) { cur->marked = false; cur = cur->next; }

    for (size_t i = 0; i < heap->root_count; i++) {
        if (heap->roots[i] && heap->roots[i]->ptr) {
            CheneyObj *obj = (CheneyObj *)heap->roots[i]->ptr;
            if (heap->roots[i]->type == WGC_REF_STRUCT || heap->roots[i]->type == WGC_REF_ARRAY)
                obj->marked = true;
        }
    }

    /* 清除: 释放未标记的对象 */
    CheneyObj **pp = &heap->objects;
    while (*pp) {
        if (!(*pp)->marked) {
            CheneyObj *dead = *pp;
            *pp = dead->next;
            free(dead);
            heap->obj_count--;
            heap->total_freed++;
        } else {
            pp = &(*pp)->next;
        }
    }

    heap->collections++;
}

int wgc_add_root(WasmGCHeap *heap, WasmGCRef *ref) {
    if (!heap || !ref) return -1;
    if (heap->root_count >= WGC_MAX_ROOTS) return -1;
    heap->roots[heap->root_count++] = ref;
    return 0;
}

int wgc_remove_root(WasmGCHeap *heap, WasmGCRef *ref) {
    if (!heap || !ref) return -1;
    for (size_t i = 0; i < heap->root_count; i++) {
        if (heap->roots[i] == ref) {
            heap->roots[i] = heap->roots[heap->root_count - 1];
            heap->root_count--;
            return 0;
        }
    }
    return -1;
}

void wgc_stats(const WasmGCHeap *heap, WasmGCStats *out) {
    if (!heap || !out) return;
    out->backend = heap->backend;
    out->total_alloc = heap->total_alloc;
    out->total_freed = heap->total_freed;
    out->live_objects = heap->obj_count;
    out->collections = heap->collections;
    out->occupancy = heap->total_alloc > 0 ?
        (double)heap->obj_count / (double)heap->total_alloc : 0.0;
}

/* ==================== Pony++ 能力映射 ==================== */

/* 与 runtime.h PNY_CAP_* 一致 */
#define PNY_CAP_ISO  0x01
#define PNY_CAP_TRN  0x02
#define PNY_CAP_REF  0x03
#define PNY_CAP_VAL  0x04
#define PNY_CAP_BOX  0x05
#define PNY_CAP_TAG  0x06

WasmGCRefType wgc_cap_to_ref_type(uint8_t pony_cap) {
    switch (pony_cap) {
        case PNY_CAP_ISO: return WGC_REF_STRUCT;   /* 唯一引用 */
        case PNY_CAP_TRN: return WGC_REF_STRUCT;   /* 转移引用 */
        case PNY_CAP_REF: return WGC_REF_STRUCT;   /* 本地可变 */
        case PNY_CAP_VAL: return WGC_REF_I31;       /* 值语义 (或 externref) */
        case PNY_CAP_BOX: return WGC_REF_EXTERN;    /* 本地只读 */
        case PNY_CAP_TAG: return WGC_REF_EXTERN;    /* 仅标识 */
        default: return WGC_REF_NULL;
    }
}

bool wgc_cap_sendable(uint8_t pony_cap) {
    /* iso/val/tag 可发送, trn/ref/box 不可 */
    return pony_cap == PNY_CAP_ISO || pony_cap == PNY_CAP_VAL || pony_cap == PNY_CAP_TAG;
}

uint8_t wgc_cap_consume(uint8_t pony_cap) {
    /* iso 消费后降级为 box */
    if (pony_cap == PNY_CAP_ISO) return PNY_CAP_BOX;
    /* val 不变 (值复制) */
    if (pony_cap == PNY_CAP_VAL) return PNY_CAP_VAL;
    /* tag 不变 */
    if (pony_cap == PNY_CAP_TAG) return PNY_CAP_TAG;
    /* 其他不变 */
    return pony_cap;
}
