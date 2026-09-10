/*
 * mpmc.c - 无锁 MPMC 环形队列 (Vyukov bounded MPMC queue)
 *
 * 基于 Dmitry Vyukov 的经典无锁有界 MPMC 队列算法:
 * - 每个槽位有独立的 sequence 标记
 * - Producer CAS 更新 enqueue_pos
 * - Consumer CAS 更新 dequeue_pos
 * - sequence 保证 ABA 安全
 *
 * 用于替代 mutex WorkDeque, 目标: 10M msg/s
 *
 * ARM64 (RPi5) 使用 __atomic_* builtins (映射到 LDXR/STXR/CAS)
 * x86_64 使用 __atomic_* builtins (映射到 LOCK CMPXCHG)
 */

#include "ponypp/mpmc.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* 缓存行大小 */
#ifndef PNY_CACHE_LINE_SIZE
#define PNY_CACHE_LINE_SIZE 64
#endif

/* 队列槽位 */
typedef struct {
    void *data;
    _Atomic size_t sequence;
} MpmcSlot;

/* 队列结构 (对齐到缓存行) */
struct PnyMpmcQueue {
    /* 生产者端 (缓存行对齐) */
    _Alignas(PNY_CACHE_LINE_SIZE) _Atomic size_t enqueue_pos;
    char _pad1[PNY_CACHE_LINE_SIZE - sizeof(_Atomic size_t)];

    /* 消费者端 (缓存行对齐) */
    _Alignas(PNY_CACHE_LINE_SIZE) _Atomic size_t dequeue_pos;
    char _pad2[PNY_CACHE_LINE_SIZE - sizeof(_Atomic size_t)];

    /* 共享数据 */
    size_t mask;          /* capacity - 1, capacity必须是2的幂 */
    MpmcSlot *slots;

    /* 统计 (非热路径, 不需要缓存行对齐) */
    _Atomic size_t total_enqueued;
    _Atomic size_t total_dequeued;
};

PnyMpmcQueue *pny_mpmc_new(size_t capacity) {
    if (capacity < 2) capacity = 2;

    /* 向上取整到2的幂 */
    size_t cap = 1;
    while (cap < capacity) cap <<= 1;

    /* size必须是alignment的倍数 (C11 aligned_alloc要求) */
    size_t alloc_size = (sizeof(PnyMpmcQueue) + PNY_CACHE_LINE_SIZE - 1)
                        & ~(size_t)(PNY_CACHE_LINE_SIZE - 1);
    PnyMpmcQueue *q = (PnyMpmcQueue *)aligned_alloc(
        PNY_CACHE_LINE_SIZE, alloc_size);
    if (!q) return NULL;
    memset(q, 0, sizeof(*q));

    q->mask = cap - 1;
    size_t slots_size = (cap * sizeof(MpmcSlot) + PNY_CACHE_LINE_SIZE - 1)
                        & ~(size_t)(PNY_CACHE_LINE_SIZE - 1);
    q->slots = (MpmcSlot *)aligned_alloc(
        PNY_CACHE_LINE_SIZE, slots_size);
    if (!q->slots) { free(q); return NULL; }

    /* 初始化每个槽位的sequence */
    for (size_t i = 0; i < cap; i++) {
        atomic_store_explicit(&q->slots[i].sequence, i, memory_order_relaxed);
        q->slots[i].data = NULL;
    }

    atomic_store_explicit(&q->enqueue_pos, 0, memory_order_relaxed);
    atomic_store_explicit(&q->dequeue_pos, 0, memory_order_relaxed);
    atomic_store_explicit(&q->total_enqueued, 0, memory_order_relaxed);
    atomic_store_explicit(&q->total_dequeued, 0, memory_order_relaxed);

    return q;
}

void pny_mpmc_free(PnyMpmcQueue *q) {
    if (!q) return;
    free(q->slots);
    free(q);
}

bool pny_mpmc_enqueue(PnyMpmcQueue *q, void *data) {
    if (!q) return false;

    size_t pos = atomic_load_explicit(&q->enqueue_pos, memory_order_relaxed);

    for (;;) {
        MpmcSlot *slot = &q->slots[pos & q->mask];
        size_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
        intptr_t diff = (intptr_t)seq - (intptr_t)pos;

        if (diff == 0) {
            /* 槽位空闲: CAS占用 */
            if (atomic_compare_exchange_weak_explicit(
                    &q->enqueue_pos, &pos, pos + 1,
                    memory_order_relaxed, memory_order_relaxed)) {
                /* 写入数据 */
                slot->data = data;
                atomic_store_explicit(&slot->sequence, pos + 1,
                                      memory_order_release);
                atomic_fetch_add_explicit(&q->total_enqueued, 1,
                                          memory_order_relaxed);
                return true;
            }
            /* CAS失败: pos已被其他线程更新, 重试 */
        } else if (diff < 0) {
            /* 队列满 */
            return false;
        } else {
            /* 其他生产者正在写入此槽位, 重新读取enqueue_pos */
            pos = atomic_load_explicit(&q->enqueue_pos, memory_order_relaxed);
        }
    }
}

void *pny_mpmc_dequeue(PnyMpmcQueue *q) {
    if (!q) return NULL;

    size_t pos = atomic_load_explicit(&q->dequeue_pos, memory_order_relaxed);

    for (;;) {
        MpmcSlot *slot = &q->slots[pos & q->mask];
        size_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
        intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

        if (diff == 0) {
            /* 槽位有数据: CAS占用 */
            if (atomic_compare_exchange_weak_explicit(
                    &q->dequeue_pos, &pos, pos + 1,
                    memory_order_relaxed, memory_order_relaxed)) {
                void *data = slot->data;
                atomic_store_explicit(&slot->sequence, pos + q->mask + 1,
                                      memory_order_release);
                atomic_fetch_add_explicit(&q->total_dequeued, 1,
                                          memory_order_relaxed);
                return data;
            }
            /* CAS失败: pos已被其他线程更新, 重试 */
        } else if (diff < 0) {
            /* 队列空 */
            return NULL;
        } else {
            /* 其他消费者正在读取此槽位, 重新读取dequeue_pos */
            pos = atomic_load_explicit(&q->dequeue_pos, memory_order_relaxed);
        }
    }
}

size_t pny_mpmc_size(const PnyMpmcQueue *q) {
    if (!q) return 0;
    size_t enq = atomic_load_explicit(&((_Atomic size_t *)&q->total_enqueued)[0],
                                       memory_order_relaxed);
    size_t deq = atomic_load_explicit(&((_Atomic size_t *)&q->total_dequeued)[0],
                                       memory_order_relaxed);
    return enq - deq;
}

size_t pny_mpmc_capacity(const PnyMpmcQueue *q) {
    return q ? q->mask + 1 : 0;
}

bool pny_mpmc_is_empty(const PnyMpmcQueue *q) {
    return pny_mpmc_size(q) == 0;
}

void pny_mpmc_stats(const PnyMpmcQueue *q, size_t *enqueued, size_t *dequeued) {
    if (!q) return;
    if (enqueued) *enqueued = atomic_load_explicit(
        &((_Atomic size_t *)&q->total_enqueued)[0], memory_order_relaxed);
    if (dequeued) *dequeued = atomic_load_explicit(
        &((_Atomic size_t *)&q->total_dequeued)[0], memory_order_relaxed);
}
