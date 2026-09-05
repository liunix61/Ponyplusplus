/*
 * gc.c - Cheney 半空间复制 GC (Copying GC) 实现
 *
 * 算法:
 *   1. 分配: 在 from_space 中分配 [GCObject header | data]
 *   2. 回收: 从 roots BFS 标记可达对象, 复制到 to_space, 更新指针
 *   3. 半空间复制确保碎片自整理
 *
 * 可达性分析:
 *   - 每个对象有 8 字节头部 (mark + size)
 *   - 保守扫描: 扫描对象 data 中所有 8 字节对齐的字
 *   - 如果在 from_space 范围内且是对象边界 → 标记为可达
 *   - roots 参数直接指向 data, 通过反向查找头部
 */

#include "ponypp/gc.h"
#include <stdlib.h>
#include <string.h>

/* 默认半空间大小: 64KB */
#ifndef GC_DEFAULT_SPACE_SIZE
#define GC_DEFAULT_SPACE_SIZE (64 * 1024)
#endif

/* 对象头部大小 */
#define GC_HDR_SIZE sizeof(GCObject)

/* 对齐到 8 字节 */
static size_t gc_align(size_t s) {
    return (s + 7) & ~(size_t)7;
}

/* 从 data 指针找到对象头部 */
static GCObject *gc_obj_header(const GCHeap *h, void *data) {
    if (!data) return NULL;
    char *d = (char *)data;

    /* 在 from_space 的对象表中查找 */
    if (d >= h->from_space && d < h->from_space + h->from_used) {
        for (size_t i = 0; i < h->from_obj_count; i++) {
            if ((char *)h->from_objects[i] + GC_HDR_SIZE == d)
                return h->from_objects[i];
        }
        return NULL;
    }

    /* 在 to_space 的对象表中查找 */
    if (d >= h->to_space && d < h->to_space + h->to_used) {
        for (size_t i = 0; i < h->to_obj_count; i++) {
            if ((char *)h->to_objects[i] + GC_HDR_SIZE == d)
                return h->to_objects[i];
        }
        return NULL;
    }

    return NULL;
}

/* 判断指针是否在某个半空间内 */
static bool gc_in_space(const char *space, size_t used, void *ptr) {
    char *p = (char *)ptr;
    return p >= space && p < space + used;
}

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

    h->from_objects = NULL;
    h->from_obj_count = 0;
    h->from_obj_cap = 0;
    h->to_objects = NULL;
    h->to_obj_count = 0;
    h->to_obj_cap = 0;

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
    free(heap->from_objects);
    free(heap->to_objects);
    free(heap->from_space);
    free(heap->to_space);
    free(heap);
}

void *gc_alloc(GCHeap *heap, size_t size) {
    if (!heap || !heap->active) return NULL;
    if (size == 0) size = 1;

    size_t aligned = gc_align(size);
    size_t total = GC_HDR_SIZE + aligned;

    if (heap->from_used + total > heap->space_size) {
        return NULL; /* 空间不足 */
    }

    char *base = heap->from_space + heap->from_used;
    GCObject *obj = (GCObject *)base;
    obj->mark = 0;
    obj->size = (uint32_t)aligned;

    /* 记录对象 */
    if (heap->from_obj_count >= heap->from_obj_cap) {
        size_t new_cap = heap->from_obj_cap ? heap->from_obj_cap * 2 : 64;
        GCObject **new_arr = (GCObject **)realloc(heap->from_objects,
                                                   new_cap * sizeof(GCObject *));
        if (!new_arr) return NULL;
        heap->from_objects = new_arr;
        heap->from_obj_cap = new_cap;
    }
    heap->from_objects[heap->from_obj_count++] = obj;

    heap->from_used += total;
    heap->total_alloc += total;

    /* 返回 data 指针 (头部之后) */
    return (char *)obj + GC_HDR_SIZE;
}

void gc_flip(GCHeap *heap) {
    if (!heap) return;

    /* 交换空间指针 */
    char *tmp_space = heap->from_space;
    heap->from_space = heap->to_space;
    heap->to_space = tmp_space;

    /* 交换使用量 */
    size_t new_from_used = heap->to_used;
    heap->to_used = 0;
    heap->from_used = new_from_used;

    /* 交换对象表: to_objects 变成 from_objects, 重置 to */
    GCObject **tmp_objs = heap->from_objects;
    size_t tmp_cap = heap->from_obj_cap;

    heap->from_objects = heap->to_objects;
    heap->from_obj_count = heap->to_obj_count;
    heap->from_obj_cap = heap->to_obj_cap;

    heap->to_objects = tmp_objs;
    heap->to_obj_count = 0;
    heap->to_obj_cap = tmp_cap;
}

/* 内部: 将对象复制到 to_space, 返回新的 data 指针 */
static char *gc_copy_obj(GCHeap *heap, GCObject *obj) {
    if (!obj) return NULL;

    size_t total = GC_HDR_SIZE + gc_align(obj->size);
    if (heap->to_used + total > heap->space_size) return NULL;

    char *dst_base = heap->to_space + heap->to_used;
    GCObject *dst = (GCObject *)dst_base;

    /* 复制头部 */
    dst->mark = 1;  /* 标记为已复制 */
    dst->size = obj->size;

    /* 复制数据 */
    memcpy((char *)dst + GC_HDR_SIZE, (char *)obj + GC_HDR_SIZE, obj->size);

    /* 记录对象 */
    if (heap->to_obj_count >= heap->to_obj_cap) {
        size_t new_cap = heap->to_obj_cap ? heap->to_obj_cap * 2 : 64;
        GCObject **new_arr = (GCObject **)realloc(heap->to_objects,
                                                   new_cap * sizeof(GCObject *));
        if (!new_arr) {
            /* 清理已复制的内容 */
            heap->to_used = heap->to_used - total;
            return NULL;
        }
        heap->to_objects = new_arr;
        heap->to_obj_cap = new_cap;
    }
    heap->to_objects[heap->to_obj_count++] = dst;

    heap->to_used += total;
    return (char *)dst + GC_HDR_SIZE;
}

/* 内部: 保守扫描对象数据, 查找指向 from_space 的指针 */
static void gc_scan_object(GCHeap *heap, GCObject *obj,
                           GCObject ***queue, size_t *queue_count,
                           size_t *queue_cap) {
    char *data = (char *)obj + GC_HDR_SIZE;
    size_t sz = gc_align(obj->size);

    /* 8 字节对齐扫描 */
    for (size_t off = 0; off + 8 <= sz; off += 8) {
        uint64_t word;
        memcpy(&word, data + off, sizeof(uint64_t));

        /* 检查是否为 from_space 中的对象指针 */
        char *ptr = (char *)word;
        if (!gc_in_space(heap->from_space, heap->from_used, ptr)) continue;

        /* 查找对应的对象头部 */
        for (size_t i = 0; i < heap->from_obj_count; i++) {
            char *obj_start = (char *)heap->from_objects[i];
            char *obj_end = obj_start + GC_HDR_SIZE + heap->from_objects[i]->size;
            if (ptr >= obj_start && ptr < obj_end) {
                /* 是对象内的指针 (可能是 header 或 data) */
                GCObject *ref_obj = heap->from_objects[i];
                if (ref_obj->mark == 0) {
                    ref_obj->mark = 1;  /* 标记为可达 */
                    /* 加入队列 */
                    if (*queue_count >= *queue_cap) {
                        size_t new_cap = *queue_cap ? *queue_cap * 2 : 64;
                        GCObject **new_q = (GCObject **)realloc(*queue,
                                      new_cap * sizeof(GCObject *));
                        if (!new_q) return;
                        *queue = new_q;
                        *queue_cap = new_cap;
                    }
                    (*queue)[(*queue_count)++] = ref_obj;
                }
                break;
            }
        }
    }
}

void gc_collect(GCHeap *heap, void **roots, size_t root_count) {
    if (!heap || !heap->active) return;

    if (heap->from_used == 0) {
        heap->generations++;
        return;
    }

    /* 检查 to_space 是否有足够空间 (最坏情况: 全部复制) */
    if (heap->from_used > heap->space_size - heap->to_used) {
        return; /* 无法容纳, 跳过回收 */
    }

    /* 阶段 0: 重置所有对象标记 (上次收集后遗留的 mark=1) */
    for (size_t i = 0; i < heap->from_obj_count; i++)
        heap->from_objects[i]->mark = 0;

    /* 分配队列 */
    size_t queue_cap = 64;
    GCObject **queue = (GCObject **)malloc(queue_cap * sizeof(GCObject *));
    if (!queue) return;
    size_t queue_count = 0;

    /* 阶段 1: 标记 roots 可达 */
    for (size_t i = 0; i < root_count; i++) {
        if (!roots[i]) continue;
        GCObject *hdr = gc_obj_header(heap, roots[i]);
        if (hdr && hdr->mark == 0) {
            hdr->mark = 1;
            if (queue_count >= queue_cap) {
                size_t new_cap = queue_cap * 2;
                GCObject **new_q = (GCObject **)realloc(queue,
                                          new_cap * sizeof(GCObject *));
                if (!new_q) { free(queue); return; }
                queue = new_q;
                queue_cap = new_cap;
            }
            queue[queue_count++] = hdr;
        }
    }

    /* 阶段 2: BFS 扫描, 标记可达对象 */
    size_t qi = 0;
    while (qi < queue_count) {
        GCObject *obj = queue[qi++];
        gc_scan_object(heap, obj, &queue, &queue_count, &queue_cap);
    }

    /* 阶段 3: 复制标记对象到 to_space */
    size_t freed = 0;
    for (size_t i = 0; i < heap->from_obj_count; i++) {
        GCObject *obj = heap->from_objects[i];
        if (obj->mark) {
            /* 复制 */
            char *new_data = gc_copy_obj(heap, obj);
            if (new_data) {
                /* 更新引用: 扫描其他对象中指向此对象的指针 */
                char *old_data = (char *)obj + GC_HDR_SIZE;
                for (size_t j = 0; j < heap->from_obj_count; j++) {
                    GCObject *sc = heap->from_objects[j];
                    if (!sc->mark) continue;
                    char *sc_data = (char *)sc + GC_HDR_SIZE;
                    size_t sc_sz = gc_align(sc->size);
                    for (size_t off = 0; off + 8 <= sc_sz; off += 8) {
                        uint64_t w;
                        memcpy(&w, sc_data + off, sizeof(uint64_t));
                        if ((char *)w == old_data) {
                            /* 更新为新位置 */
                            for (size_t k = 0; k < heap->to_obj_count; k++) {
                                if ((char *)heap->to_objects[k] + GC_HDR_SIZE == new_data) {
                                    w = (uint64_t)new_data;
                                    memcpy(sc_data + off, &w, sizeof(uint64_t));
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            /* 未标记: 统计释放的字节 */
            freed += GC_HDR_SIZE + gc_align(obj->size);
        }
    }

    /* 更新根引用 */
    for (size_t i = 0; i < root_count; i++) {
        if (!roots[i]) continue;
        GCObject *hdr = gc_obj_header(heap, roots[i]);
        if (hdr && hdr->mark) {
            /* 找到 to_space 中的新副本 */
            for (size_t j = 0; j < heap->to_obj_count; j++) {
                if (heap->to_objects[j]->size == hdr->size &&
                    memcmp((char *)heap->to_objects[j] + GC_HDR_SIZE,
                           (char *)hdr + GC_HDR_SIZE,
                           gc_align(hdr->size)) == 0) {
                    roots[i] = (char *)heap->to_objects[j] + GC_HDR_SIZE;
                    break;
                }
            }
        }
    }

    heap->total_freed += freed;
    heap->generations++;

    /* 交换半空间 */
    gc_flip(heap);

    free(queue);
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
    if (!heap || !heap->space_size) return 0.0;
    return (double)heap->from_used / (double)heap->space_size;
}

bool gc_should_collect(const GCHeap *heap, double threshold) {
    if (!heap || !heap->active) return false;
    if (heap->from_used == 0) return false;
    return gc_occupancy(heap) >= threshold;
}