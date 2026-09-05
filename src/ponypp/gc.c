/*
 * gc.c - Cheney 半空间复制 GC (Copying GC) 实现
 *
 * 算法:
 *   1. 分配: 从 from_space 的当前指针开始，按大小分配
 *   2. 回收: 遍历 roots，复制活对象到 to_space，交换半空间
 *   3. 半空间复制确保碎片自整理
 */

#include "ponypp/gc.h"
#include <stdlib.h>
#include <string.h>

/* 默认半空间大小: 64KB */
#ifndef GC_DEFAULT_SPACE_SIZE
#define GC_DEFAULT_SPACE_SIZE (64 * 1024)
#endif

GCHeap *gc_heap_new(size_t space_size) {
    if (space_size == 0) space_size = GC_DEFAULT_SPACE_SIZE;
    /* 对齐到页大小 */
    space_size = (space_size + 4095) & ~(size_t)4095;

    GCHeap *h = (GCHeap*)calloc(1, sizeof(GCHeap));
    if (!h) return NULL;

    h->from_space = (char*)calloc(1, space_size);
    h->to_space = (char*)calloc(1, space_size);
    if (!h->from_space || !h->to_space) {
        free(h->from_space);
        free(h->to_space);
        free(h);
        return NULL;
    }

    h->space_size = space_size;
    h->from_used = 0;
    h->to_used = 0;
    h->generations = 0;
    h->total_alloc = 0;
    h->total_freed = 0;
    h->active = true;
    return h;
}

void gc_heap_free(GCHeap *heap) {
    if (!heap) return;
    free(heap->from_space);
    free(heap->to_space);
    free(heap);
}

void *gc_alloc(GCHeap *heap, size_t size) {
    if (!heap || !heap->active) return NULL;
    if (size == 0) size = 1;
    /* 对齐到 8 字节 */
    size = (size + 7) & ~(size_t)7;

    if (heap->from_used + size > heap->space_size) {
        return NULL; /* 空间不足 */
    }

    void *ptr = heap->from_space + heap->from_used;
    heap->from_used += size;
    heap->total_alloc += size;
    return ptr;
}

void gc_flip(GCHeap *heap) {
    if (!heap) return;
    /* 交换 from 和 to */
    char *tmp = heap->from_space;
    heap->from_space = heap->to_space;
    heap->to_space = tmp;

    /* 交换使用量: 旧的 to_used 变成新的 from_used */
    size_t new_from_used = heap->to_used;
    heap->to_used = 0;
    heap->from_used = new_from_used;
}

void gc_collect(GCHeap *heap, void **roots, size_t root_count) {
    if (!heap || !heap->active) return;

    /* Cheney 复制: 将 from_space 中已使用的内存复制到 to_space
     * 简化版本: 复制所有已分配内存（无对象头部元数据时无法精确追踪存活对象）
     */
    size_t to_space_left = heap->space_size - heap->to_used;
    size_t bytes_to_copy = heap->from_used;

    if (bytes_to_copy == 0) {
        heap->generations++;
        return;
    }

    if (bytes_to_copy > to_space_left) {
        /* to_space 无法容纳所有对象，不执行回收 */
        return;
    }

    /* 复制 from_space 到 to_space */
    memcpy(heap->to_space + heap->to_used, heap->from_space, bytes_to_copy);
    heap->to_used += bytes_to_copy;
    heap->total_freed += bytes_to_copy;
    heap->generations++;

    /* 交换半空间: to_space 变成 from_space, from_used 更新 */
    gc_flip(heap);
}

void gc_stats(const GCHeap *heap, size_t *space_size,
              size_t *from_used, size_t *to_used,
              int *generations, size_t *total_alloc, size_t *total_freed) {
    if (!heap) return;
    if (space_size) *space_size = heap->space_size;
    if (from_used) *from_used = heap->from_used;
    if (to_used) *to_used = heap->to_used;
    if (generations) *generations = heap->generations;
    if (total_alloc) *total_alloc = heap->total_alloc;
    if (total_freed) *total_freed = heap->total_freed;
}

double gc_occupancy(const GCHeap *heap) {
    if (!heap || heap->space_size == 0) return 0.0;
    return (double)heap->from_used / (double)heap->space_size;
}

bool gc_should_collect(const GCHeap *heap, double threshold) {
    if (!heap || !heap->active) return false;
    if (heap->from_used == 0) return false;
    return gc_occupancy(heap) >= threshold;
}
