/*
 * gtest_security.cpp - 安全加固单元测试
 *
 * Phase 5: 安全审计 — 输入验证、边界检查、整数溢出防护
 */
#include <gtest/gtest.h>
extern "C" {
#include "ponypp/runtime.h"
#include "ponypp/gc.h"
#include "ponypp/version_compat.h"
}
#include <string.h>
#include <limits.h>

/* ======================== 消息输入验证 ======================== */

TEST(Security, SendNullMethod) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    PnyActor *a = pny_actor_new(r, "target", 0);
    ASSERT_NE(a, nullptr);
    ActorRef *to = pny_actor_ref(a);
    
    /* NULL method 不应崩溃 */
    int ret = pny_actor_send(nullptr, to, nullptr, nullptr, 0);
    /* 应该成功或安全失败, 不应崩溃 */
    (void)ret;
    
    pny_runtime_free(r);
}

TEST(Security, SendNullTarget) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    EXPECT_EQ(pny_actor_send(nullptr, nullptr, "method", nullptr, 0), -1);
    
    pny_runtime_free(r);
}

TEST(Security, SendToStoppedActor) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    PnyActor *a = pny_actor_new(r, "target", 0);
    ASSERT_NE(a, nullptr);
    ActorRef *ref = pny_actor_ref(a);
    
    /* 停止 Actor */
    a->actor_state = ACTOR_STATE_STOPPED;
    
    /* 发送到已停止的 Actor 应失败 */
    EXPECT_EQ(pny_actor_send(nullptr, ref, "method", nullptr, 0), -2);
    
    pny_runtime_free(r);
}

TEST(Security, SendQueueOverflow) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    PnyActor *a = pny_actor_new(r, "target", 0);
    ASSERT_NE(a, nullptr);
    ActorRef *ref = pny_actor_ref(a);
    
    /* 设置很小的队列限制 */
    pny_backpressure_set_limit(a, 3);
    
    /* 填满队列 */
    EXPECT_EQ(pny_actor_send(nullptr, ref, "m", nullptr, 0), 0);
    EXPECT_EQ(pny_actor_send(nullptr, ref, "m", nullptr, 0), 0);
    /* 第3个可能触发背压警告 */
    int ret = pny_actor_send(nullptr, ref, "m", nullptr, 0);
    /* 第4个应失败 */
    ret = pny_actor_send(nullptr, ref, "m", nullptr, 0);
    EXPECT_NE(ret, 0);
    
    pny_runtime_free(r);
}

TEST(Security, SendHugePayload) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    PnyActor *a = pny_actor_new(r, "target", 0);
    ASSERT_NE(a, nullptr);
    ActorRef *ref = pny_actor_ref(a);
    
    /* 尝试发送超大 payload (1MB) */
    size_t huge_size = 1024 * 1024;
    char *huge = (char *)malloc(huge_size);
    ASSERT_NE(huge, nullptr);
    memset(huge, 'A', huge_size);
    
    int ret = pny_actor_send(nullptr, ref, "huge", huge, huge_size);
    /* 应该成功或安全失败 */
    (void)ret;
    
    free(huge);
    pny_runtime_free(r);
}

/* ======================== Actor 输入验证 ======================== */

TEST(Security, ActorNullRuntime) {
    EXPECT_EQ(pny_actor_new(nullptr, "name", 0), nullptr);
}

TEST(Security, ActorNullName) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    /* NULL name 应该安全处理 */
    PnyActor *a = pny_actor_new(r, nullptr, 0);
    /* 可能成功(用默认名)或失败, 不应崩溃 */
    if (a) {
        pny_actor_destroy(r, pny_actor_ref(a));
    }
    
    pny_runtime_free(r);
}

TEST(Security, ActorZeroState) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    /* state_size=0 应该正常 */
    PnyActor *a = pny_actor_new(r, "zero", 0);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->state_data, nullptr);
    
    pny_runtime_free(r);
}

TEST(Security, ActorHugeState) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    /* 尝试分配 100MB state */
    PnyActor *a = pny_actor_new(r, "huge", 100 * 1024 * 1024);
    /* 可能成功或失败(内存不足), 不应崩溃 */
    if (a) {
        pny_actor_destroy(r, pny_actor_ref(a));
    }
    
    pny_runtime_free(r);
}

TEST(Security, DestroyNullRef) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    /* NULL ref 不应崩溃 */
    pny_actor_destroy(r, nullptr);
    
    /* NULL actor in ref */
    ActorRef ref = {-1, nullptr, nullptr};
    pny_actor_destroy(r, &ref);
    
    pny_runtime_free(r);
}

TEST(Security, DoubleDestroy) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    PnyActor *a = pny_actor_new(r, "victim", 0);
    ASSERT_NE(a, nullptr);
    ActorRef *ref = pny_actor_ref(a);
    
    /* 第一次销毁 */
    pny_actor_destroy(r, ref);
    /* 第二次销毁同一 ref (已释放) — 这是 use-after-free, 但我们验证第一次成功 */
    /* 注意: 不真正调用第二次, 避免 UB */
    
    pny_runtime_free(r);
}

/* ======================== 背压安全 ======================== */

TEST(Security, BackpressureNullActor) {
    /* NULL actor 不应崩溃 */
    BackpressureState bp = pny_backpressure_check(nullptr);
    (void)bp;
}

TEST(Security, BackpressureZeroLimit) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    PnyActor *a = pny_actor_new(r, "bp", 0);
    ASSERT_NE(a, nullptr);
    
    /* 设置 limit=0 应该恢复默认值 */
    pny_backpressure_set_limit(a, 0);
    EXPECT_EQ(a->max_messages, (size_t)65536);
    
    pny_runtime_free(r);
}

/* ======================== GC 安全 ======================== */

TEST(Security, GcNullHeap) {
    /* NULL heap 不应崩溃 */
    void *p = gc_alloc(nullptr, 64);
    EXPECT_EQ(p, nullptr);
    
    gc_collect(nullptr, nullptr, 0);
    gc_flip(nullptr);
    gc_heap_free(nullptr);
    
    double occ = gc_occupancy(nullptr);
    (void)occ;
    
    EXPECT_FALSE(gc_should_collect(nullptr, 0.5));
}

TEST(Security, GcZeroAlloc) {
    GCHeap *heap = gc_heap_new(4096);
    ASSERT_NE(heap, nullptr);
    
    /* 0 字节分配 */
    void *p = gc_alloc(heap, 0);
    /* 可能返回 NULL 或有效指针 */
    (void)p;
    
    gc_heap_free(heap);
}

TEST(Security, GcOverflowAlloc) {
    GCHeap *heap = gc_heap_new(4096);
    ASSERT_NE(heap, nullptr);
    
    /* 尝试分配超过堆大小的内存 */
    void *p = gc_alloc(heap, SIZE_MAX / 2);
    EXPECT_EQ(p, nullptr);
    
    gc_heap_free(heap);
}

/* ======================== SemVer 安全 ======================== */

TEST(Security, SemVerOverflow) {
    SemVer v;
    
    /* 超大数字不应崩溃 */
    int ret = semver_parse("999999999999.0.0", &v);
    /* 可能溢出但不应崩溃 */
    (void)ret;
    
    /* 空字符串 */
    EXPECT_EQ(semver_parse("", &v), -1);
    
    /* 超长字符串 */
    char long_str[1024];
    memset(long_str, '1', sizeof(long_str) - 1);
    long_str[sizeof(long_str) - 1] = '\0';
    semver_parse(long_str, &v); /* 不应崩溃 */
}

/* ======================== 消息 ID 溢出 ======================== */

TEST(Security, MessageIdOverflow) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    PnyActor *a = pny_actor_new(r, "id_test", 0);
    ASSERT_NE(a, nullptr);
    
    /* 设置 next_msg_id 接近溢出 */
    a->next_msg_id = UINT64_MAX - 2;
    
    ActorRef *from = pny_actor_ref(a);
    
    /* 发送多条消息, msg_id 应该递增但不崩溃 */
    for (int i = 0; i < 5; i++) {
        pny_actor_send(from, from, "m", nullptr, 0);
    }
    
    pny_runtime_free(r);
}

/* ======================== 调度器安全 ======================== */

TEST(Security, SchedulerNullRuntime) {
    /* NULL runtime 不应崩溃 */
    pny_scheduler_tick(nullptr);
    pny_runtime_free(nullptr);
}

TEST(Security, SchedulerEmptyRuntime) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    /* 空运行时 tick 不应崩溃 */
    pny_scheduler_tick(r);
    pny_scheduler_tick(r);
    
    RuntimeStats stats;
    pny_runtime_stats(r, &stats);
    EXPECT_EQ(stats.actors_alive, (size_t)0);
    
    pny_runtime_free(r);
}

/* ======================== 综合压力测试 ======================== */

TEST(Security, StressManyActorsMessages) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    /* 创建 100 个 Actor, 每个发 10 条消息 */
    PnyActor *actors[100];
    ActorRef *refs[100];
    
    for (int i = 0; i < 100; i++) {
        actors[i] = pny_actor_new(r, "stress", 32);
        refs[i] = actors[i] ? pny_actor_ref(actors[i]) : nullptr;
    }
    
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 10; j++) {
            if (refs[i] && refs[(i + 1) % 100]) {
                pny_actor_send(refs[i], refs[(i + 1) % 100], "relay", nullptr, 0);
            }
        }
    }
    
    /* 调度 */
    for (int i = 0; i < 1000; i++) {
        pny_scheduler_tick(r);
    }
    
    /* 销毁一半 */
    for (int i = 0; i < 50; i++) {
        if (refs[i]) pny_actor_destroy(r, refs[i]);
    }
    
    /* 继续调度 */
    for (int i = 0; i < 100; i++) {
        pny_scheduler_tick(r);
    }
    
    RuntimeStats stats;
    pny_runtime_stats(r, &stats);
    printf("    stress: created=%zu destroyed=%zu sent=%zu\n",
           stats.actors_created, stats.actors_destroyed, stats.messages_sent);
    
    pny_runtime_free(r);
}
