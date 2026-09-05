/*
 * gc.h - Cheney 半空间复制 GC (Copying GC)
 *
 * 支持可达性分析的保守 GC:
 *   - 对象头部: mark + size
 *   - roots 遍历: BFS 标记 + 复制
 *   - 保守扫描: 扫描所有 8 字节对齐的字，识别 from_space 指针
 *   - 指针更新: 复制后更新所有指向 from_space 的指针
 *
 * Pony++ 引用能力语义:
 *   - iso/trn: GC 可移动/回收
 *   - ref: 进程内有效，GC 回收
 *   - val: 全局不可变，GC 不回收
 *   - box: 全局只读，GC 不回收
 *   - tag: 无引用，不参与 GC
 */

#ifndef PONYPP_GC_H
#define PONYPP_GC_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* GC 对象头部 (8 字节, 8 字节对齐) */
typedef struct GCObject {
    uint32_t mark;      /* 0=未标记, 1=可达 */
    uint32_t size;      /* data 部分对齐后的大小 (字节) */
} GCObject;

/* GC 堆配置 */
typedef struct GCHeap {
    char *from_space;       /* 旧空间 */
    char *to_space;         /* 新空间 */
    size_t space_size;      /* 每个空间的大小 */
    size_t from_used;       /* from 空间已使用 */
    size_t to_used;         /* to 空间已使用 */
    int generations;        /* 已执行回收次数 */
    size_t total_alloc;     /* 总分配量 */
    size_t total_freed;     /* 总回收量 */
    bool active;            /* 是否已初始化 */

    /* 对象跟踪 (from_space 中的对象) */
    GCObject **from_objects;
    size_t from_obj_count;
    size_t from_obj_cap;

    /* 对象跟踪 (to_space 中的对象) */
    GCObject **to_objects;
    size_t to_obj_count;
    size_t to_obj_cap;
} GCHeap;

/* 创建一个 GC 堆
 * space_size: 每个半空间的大小 (字节, 自动对齐到页大小)
 */
GCHeap *gc_heap_new(size_t space_size);

/* 释放 GC 堆 */
void gc_heap_free(GCHeap *heap);

/* 从 GC 堆分配内存
 * 返回指向 data (头部之后) 的指针
 * 失败返回 NULL
 */
void *gc_alloc(GCHeap *heap, size_t size);

/* 触发回收 (从 roots 可达性分析, Cheney 复制到 to_space)
 * roots: 根引用数组 (指向 data 的指针)
 * root_count: 根引用数量
 */
void gc_collect(GCHeap *heap, void **roots, size_t root_count);

/* 交换半空间 (回收后调用) */
void gc_flip(GCHeap *heap);

/* 获取堆统计信息 */
void gc_stats(const GCHeap *heap, size_t *space_size,
              size_t *from_used, size_t *to_used,
              int *generations, size_t *total_alloc, size_t *total_freed);

/* 堆使用率 (0.0 - 1.0) */
double gc_occupancy(const GCHeap *heap);

/* 检查是否需要回收 (超过阈值)
 * threshold: 阈值比例 (0.0 - 1.0)
 */
bool gc_should_collect(const GCHeap *heap, double threshold);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_GC_H */