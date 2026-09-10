/*
 * mpmc.h - 无锁 MPMC 环形队列
 *
 * Vyukov bounded MPMC queue:
 * - 多生产者多消费者安全
 * - 无锁 (CAS-based)
 * - 有界环形缓冲区
 * - 缓存行对齐避免 false sharing
 *
 * 用于替代 mutex WorkDeque, 提升消息吞吐到 10M msg/s
 *
 * 平台支持:
 * - ARM64 (RPi5): LDXR/STXR/CAS 指令
 * - x86_64: LOCK CMPXCHG
 * - GCC/Clang: __atomic_* builtins
 */
#ifndef PONYPP_MPMC_H
#define PONYPP_MPMC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>

/* 不透明队列类型 */
typedef struct PnyMpmcQueue PnyMpmcQueue;

/* 创建队列 (capacity自动向上取整到2的幂) */
PnyMpmcQueue *pny_mpmc_new(size_t capacity);

/* 释放队列 */
void pny_mpmc_free(PnyMpmcQueue *q);

/* 入队 (非阻塞, 满则返回false) */
bool pny_mpmc_enqueue(PnyMpmcQueue *q, void *data);

/* 出队 (非阻塞, 空则返回NULL) */
void *pny_mpmc_dequeue(PnyMpmcQueue *q);

/* 当前元素数 */
size_t pny_mpmc_size(const PnyMpmcQueue *q);

/* 容量 */
size_t pny_mpmc_capacity(const PnyMpmcQueue *q);

/* 是否为空 */
bool pny_mpmc_is_empty(const PnyMpmcQueue *q);

/* 统计 (累计入队/出队数) */
void pny_mpmc_stats(const PnyMpmcQueue *q, size_t *enqueued, size_t *dequeued);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_MPMC_H */
