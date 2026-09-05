#include "ponypp.h"
#include "ponypp/gc.h"
#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * GC 测试 - Cheney 半空间复制 GC
 *
 * 重要: gc_alloc 返回 data 指针 (头部之后)
 *   gc_alloc(h, N) 实际分配 = 8 (header) + align8(N)
 *   from_used 包含头部
 *
 * 例如: gc_alloc(h, 64) → total = 8 + 64 = 72
 */

/* ===== GCHeap 创建/销毁 ===== */
TEST(GC, HeapCreate) {
    GCHeap *h = gc_heap_new(0);
    ASSERT_NE(h, nullptr);
    ASSERT_TRUE(h->active);
    ASSERT_GT(h->space_size, 0);
    ASSERT_EQ(h->from_used, 0);
    ASSERT_EQ(h->to_used, 0);
    ASSERT_EQ(h->generations, 0);
    ASSERT_EQ(h->from_obj_count, 0);
    ASSERT_EQ(h->to_obj_count, 0);
    gc_heap_free(h);
}

TEST(GC, HeapCreateCustomSize) {
    GCHeap *h = gc_heap_new(128 * 1024);
    ASSERT_NE(h, nullptr);
    ASSERT_EQ(h->space_size, 128 * 1024);
    gc_heap_free(h);
}

TEST(GC, HeapFreeNull) {
    gc_heap_free(nullptr);
}

/* ===== gc_alloc ===== */
TEST(GC, AllocBasic) {
    GCHeap *h = gc_heap_new(0);
    void *p = gc_alloc(h, 64);
    ASSERT_NE(p, nullptr);
    /* total = 8 (header) + 64 = 72 */
    EXPECT_EQ(h->from_used, 72);
    EXPECT_EQ(h->total_alloc, 72);
    EXPECT_EQ(h->from_obj_count, 1);
    gc_heap_free(h);
}

TEST(GC, AllocAlignment) {
    GCHeap *h = gc_heap_new(0);
    void *p = gc_alloc(h, 1);
    ASSERT_NE(p, nullptr);
    /* total = 8 (header) + 8 (aligned) = 16 */
    EXPECT_EQ(h->from_used, 16);
    gc_heap_free(h);
}

TEST(GC, AllocMultiple) {
    GCHeap *h = gc_heap_new(0);
    void *p1 = gc_alloc(h, 16);  /* 8+16=24 */
    void *p2 = gc_alloc(h, 32);  /* 8+32=40 */
    void *p3 = gc_alloc(h, 48);  /* 8+48=56 */
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    ASSERT_NE(p3, nullptr);
    EXPECT_EQ(h->from_used, 24 + 40 + 56);  /* 120 */
    EXPECT_EQ(h->from_obj_count, 3);
    gc_heap_free(h);
}

TEST(GC, AllocDataUsable) {
    GCHeap *h = gc_heap_new(0);
    void *p = gc_alloc(h, 64);
    ASSERT_NE(p, nullptr);
    /* 写入数据验证可写 */
    memset(p, 0xAB, 64);
    unsigned char *buf = (unsigned char *)p;
    bool all_ok = true;
    for (int i = 0; i < 64; i++) {
        if (buf[i] != 0xAB) { all_ok = false; break; }
    }
    EXPECT_TRUE(all_ok);
    gc_heap_free(h);
}

TEST(GC, AllocExceedsSpace) {
    GCHeap *h = gc_heap_new(4096); /* 一页 */
    /* total = 8 + 8192 = 8200 > 4096 */
    void *p = gc_alloc(h, 8192);
    EXPECT_EQ(p, nullptr);
    gc_heap_free(h);
}

TEST(GC, AllocNullHeap) {
    EXPECT_EQ(gc_alloc(nullptr, 64), nullptr);
}

TEST(GC, AllocInactiveHeap) {
    GCHeap *h = gc_heap_new(0);
    h->active = false;
    EXPECT_EQ(gc_alloc(h, 64), nullptr);
    gc_heap_free(h);
}

TEST(GC, AllocZeroSize) {
    GCHeap *h = gc_heap_new(0);
    void *p = gc_alloc(h, 0);
    ASSERT_NE(p, nullptr);
    /* total = 8 + 8 = 16 */
    EXPECT_EQ(h->from_used, 16);
    gc_heap_free(h);
}

/* ===== gc_collect ===== */
TEST(GC, CollectEmpty) {
    GCHeap *h = gc_heap_new(0);
    gc_collect(h, nullptr, 0);
    EXPECT_EQ(h->generations, 1);
    gc_heap_free(h);
}

TEST(GC, CollectWithRoots) {
    GCHeap *h = gc_heap_new(0);
    void *p1 = gc_alloc(h, 64);  /* 72 bytes */
    void *p2 = gc_alloc(h, 32);  /* 40 bytes */
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    memset(p1, 0xAA, 64);
    memset(p2, 0xBB, 32);

    EXPECT_EQ(h->from_used, 112);  /* 72 + 40 */

    void *roots[] = {p1, p2};
    gc_collect(h, roots, 2);

    /* 复制后: from_used = 复制的字节数, to_used = 0 */
    EXPECT_EQ(h->from_used, 112);
    EXPECT_EQ(h->to_used, 0);
    EXPECT_EQ(h->generations, 1);

    /* 数据应保留 */
    unsigned char *buf1 = (unsigned char *)roots[0];
    unsigned char *buf2 = (unsigned char *)roots[1];
    bool all_aa = true, all_bb = true;
    for (int i = 0; i < 64; i++) if (buf1[i] != 0xAA) all_aa = false;
    for (int i = 0; i < 32; i++) if (buf2[i] != 0xBB) all_bb = false;
    EXPECT_TRUE(all_aa);
    EXPECT_TRUE(all_bb);
    gc_heap_free(h);
}

TEST(GC, CollectUnreachableFreed) {
    GCHeap *h = gc_heap_new(0);
    void *p1 = gc_alloc(h, 64);  /* 72 bytes, reachable */
    void *p2 = gc_alloc(h, 32);  /* 40 bytes, unreachable */
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    memset(p1, 0xAA, 64);
    memset(p2, 0xBB, 32);

    size_t total_alloc_before = h->total_alloc;  /* 112 */

    /* 只传入 p1 作为 root, p2 不可达 */
    void *roots[] = {p1};
    gc_collect(h, roots, 1);

    /* p2 被回收, total_freed = 40 */
    EXPECT_GT(h->total_freed, 0);
    /* p1 数据保留 */
    unsigned char *buf1 = (unsigned char *)roots[0];
    bool all_aa = true;
    for (int i = 0; i < 64; i++) if (buf1[i] != 0xAA) all_aa = false;
    EXPECT_TRUE(all_aa);

    /* from_used 应只包含 p1 的 72 字节 */
    EXPECT_EQ(h->from_used, 72);
    gc_heap_free(h);
}

TEST(GC, CollectNullRoot) {
    GCHeap *h = gc_heap_new(0);
    void *p = gc_alloc(h, 64);
    ASSERT_NE(p, nullptr);

    /* root 为 null, 应跳过 */
    void *roots[] = {nullptr, p};
    gc_collect(h, roots, 2);

    EXPECT_EQ(h->from_used, 72);
    EXPECT_EQ(h->generations, 1);
    gc_heap_free(h);
}

TEST(GC, CollectMultipleGenerations) {
    GCHeap *h = gc_heap_new(0);
    for (int i = 0; i < 3; i++) {
        void *p = gc_alloc(h, 16);  /* 24 bytes */
        ASSERT_NE(p, nullptr);
        memset(p, i, 16);
        void *roots[] = {p};
        gc_collect(h, roots, 1);
        EXPECT_EQ(h->generations, i + 1);
    }
    gc_heap_free(h);
}

TEST(GC, CollectNullHeap) {
    gc_collect(nullptr, nullptr, 0);
}

TEST(GC, CollectChain) {
    /* 对象链: root → A → B */
    GCHeap *h = gc_heap_new(0);

    void *b = gc_alloc(h, 16);  /* 24 bytes */
    void *a = gc_alloc(h, 24);  /* 32 bytes */
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    memset(b, 0xBB, 16);

    /* A 的数据中包含指向 B 的指针 */
    void **a_data = (void **)a;
    a_data[0] = b;

    /* 只传入 A 作为 root, B 通过 A 可达 */
    void *roots[] = {a};
    gc_collect(h, roots, 1);

    /* A 和 B 都应保留 */
    EXPECT_EQ(h->from_used, 24 + 32);  /* 56 */
    gc_heap_free(h);
}

TEST(GC, CollectOrphanChain) {
    /* 对象链: root → A → B, 但 B 不可达 (A 的指针字段为 0) */
    GCHeap *h = gc_heap_new(0);

    void *b = gc_alloc(h, 16);  /* 24 bytes, orphan */
    void *a = gc_alloc(h, 24);  /* 32 bytes, reachable */
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    memset(a, 0xAA, 24);
    memset(b, 0xBB, 16);

    /* A 的数据不包含指向 B 的指针 */
    void *roots[] = {a};
    gc_collect(h, roots, 1);

    /* B 不可达, 应被回收 */
    EXPECT_EQ(h->from_used, 32);  /* 只有 A */
    EXPECT_GT(h->total_freed, 0);
    gc_heap_free(h);
}

/* ===== gc_flip ===== */
TEST(GC, Flip) {
    GCHeap *h = gc_heap_new(0);
    char *from_before = h->from_space;
    char *to_before = h->to_space;

    gc_flip(h);

    EXPECT_EQ(h->from_space, to_before);
    EXPECT_EQ(h->to_space, from_before);
    gc_heap_free(h);
}

TEST(GC, FlipWithObjects) {
    GCHeap *h = gc_heap_new(0);
    gc_alloc(h, 32);
    gc_alloc(h, 64);

    size_t from_objs = h->from_obj_count;
    EXPECT_EQ(from_objs, 2);

    /* 手动复制到 to_space 然后 flip */
    for (size_t i = 0; i < h->from_obj_count; i++) {
        GCObject *obj = h->from_objects[i];
        size_t total = 8 + obj->size;
        GCObject *dst = (GCObject *)(h->to_space + h->to_used);
        dst->mark = 0;
        dst->size = obj->size;
        memcpy((char *)dst + 8, (char *)obj + 8, obj->size);
        if (h->to_obj_count < h->to_obj_cap || h->to_obj_cap == 0) {
            size_t new_cap = h->to_obj_cap ? h->to_obj_cap * 2 : 64;
            h->to_objects = (GCObject **)realloc(h->to_objects, new_cap * sizeof(GCObject *));
            h->to_obj_cap = new_cap;
        }
        h->to_objects[h->to_obj_count++] = dst;
        h->to_used += total;
    }

    gc_flip(h);
    EXPECT_EQ(h->from_obj_count, 2);
    EXPECT_EQ(h->to_obj_count, 0);
    gc_heap_free(h);
}

/* ===== gc_stats ===== */
TEST(GC, Stats) {
    GCHeap *h = gc_heap_new(0);
    void *p1 = gc_alloc(h, 64);  /* 72 */
    void *p2 = gc_alloc(h, 32);  /* 40 */
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    memset(p1, 0xAA, 64);
    memset(p2, 0xBB, 32);

    void *roots[] = {p1, p2};
    gc_collect(h, roots, 2);

    size_t space_size = 0, from_used = 0, to_used = 0;
    int generations = 0;
    size_t total_alloc = 0, total_freed = 0;

    gc_stats(h, &space_size, &from_used, &to_used,
             &generations, &total_alloc, &total_freed);

    EXPECT_EQ(space_size, h->space_size);
    EXPECT_EQ(from_used, 112);  /* 72 + 40 */
    EXPECT_EQ(generations, 1);
    EXPECT_EQ(total_alloc, 112);
    gc_heap_free(h);
}

TEST(GC, StatsNullOutput) {
    GCHeap *h = gc_heap_new(0);
    gc_stats(h, NULL, NULL, NULL, NULL, NULL, NULL);
    gc_heap_free(h);
}

TEST(GC, StatsNullHeap) {
    gc_stats(nullptr, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* ===== gc_occupancy ===== */
TEST(GC, Occupancy) {
    GCHeap *h = gc_heap_new(4096); /* 一页 */
    EXPECT_EQ(gc_occupancy(h), 0.0);

    gc_alloc(h, 64);
    EXPECT_NEAR(gc_occupancy(h), 72.0 / 4096.0, 0.001);

    gc_alloc(h, 64);
    EXPECT_NEAR(gc_occupancy(h), 144.0 / 4096.0, 0.001);

    gc_heap_free(h);
}

TEST(GC, OccupancyFull) {
    GCHeap *h = gc_heap_new(4096);
    /* total = 8 + 4072 = 4080 < 4096 */
    gc_alloc(h, 4072);
    EXPECT_NEAR(gc_occupancy(h), 4080.0 / 4096.0, 0.001);
    gc_heap_free(h);
}

TEST(GC, OccupancyNullHeap) {
    EXPECT_EQ(gc_occupancy(nullptr), 0.0);
}

/* ===== gc_should_collect ===== */
TEST(GC, ShouldCollectLowUsage) {
    GCHeap *h = gc_heap_new(4096);
    gc_alloc(h, 32);
    EXPECT_FALSE(gc_should_collect(h, 0.8));
    gc_heap_free(h);
}

TEST(GC, ShouldCollectHighUsage) {
    GCHeap *h = gc_heap_new(4096);
    /* total = 8 + 3504 = 3512, 3512/4096 ≈ 0.857 > 0.8 */
    gc_alloc(h, 3500);
    EXPECT_TRUE(gc_should_collect(h, 0.8));
    gc_heap_free(h);
}

TEST(GC, ShouldCollectEmpty) {
    GCHeap *h = gc_heap_new(0);
    EXPECT_FALSE(gc_should_collect(h, 0.0));
    gc_heap_free(h);
}

TEST(GC, ShouldCollectNullHeap) {
    EXPECT_FALSE(gc_should_collect(nullptr, 0.8));
}

/* ===== 集成测试 ===== */
TEST(GC, IntegrationAllocUseCollect) {
    GCHeap *h = gc_heap_new(512); /* 4096 aligned */

    for (int round = 0; round < 3; round++) {
        void *p1 = gc_alloc(h, 32);  /* 40 */
        void *p2 = gc_alloc(h, 48);  /* 56 */
        ASSERT_NE(p1, nullptr);
        ASSERT_NE(p2, nullptr);
        memset(p1, round, 32);
        memset(p2, round + 1, 48);

        void *roots[] = {p1, p2};
        gc_collect(h, roots, 2);
        EXPECT_EQ(h->generations, round + 1);
    }

    gc_heap_free(h);
}

TEST(GC, StressManyAllocations) {
    GCHeap *h = gc_heap_new(0);
    int count = 0;
    for (int i = 0; i < 1000; i++) {
        void *p = gc_alloc(h, 8);
        if (!p) break;
        memset(p, i, 8);
        count++;
    }
    EXPECT_GT(count, 0);
    gc_heap_free(h);
}

TEST(GC, GCReclaimsOrphanedObjects) {
    /* 验证 GC 确实回收不可达对象 */
    GCHeap *h = gc_heap_new(0);

    size_t initial_alloc = 0;
    for (int i = 0; i < 10; i++) {
        gc_alloc(h, 64);  /* 72 each */
        initial_alloc += 72;
    }
    EXPECT_EQ(h->from_used, 10 * 72);  /* 720 */

    /* 创建新的可达对象 */
    void *live = gc_alloc(h, 32);  /* 40 */
    ASSERT_NE(live, nullptr);

    /* 收集: 只有 live 可达, 其余 10 个不可达 */
    void *roots[] = {live};
    gc_collect(h, roots, 1);

    /* from_used 应只有 live 的 40 字节 */
    EXPECT_EQ(h->from_used, 40);
    /* 释放了 10 个对象: 720 bytes */
    EXPECT_EQ(h->total_freed, 720);

    gc_heap_free(h);
}

TEST(GC, RootPointerUpdatedAfterCollect) {
    /* 验证 root 指针在收集后被更新到 to_space */
    GCHeap *h = gc_heap_new(0);

    void *p = gc_alloc(h, 64);
    ASSERT_NE(p, nullptr);
    memset(p, 0xCC, 64);

    char *old_from_space = h->from_space;

    void *roots[] = {p};
    gc_collect(h, roots, 1);

    /* roots[0] 应被更新到 to_space (原 from_space 的另一半) */
    EXPECT_NE(roots[0], p);
    EXPECT_NE(h->from_space, old_from_space);

    /* 数据保留 */
    unsigned char *buf = (unsigned char *)roots[0];
    bool all_cc = true;
    for (int i = 0; i < 64; i++) if (buf[i] != 0xCC) all_cc = false;
    EXPECT_TRUE(all_cc);

    gc_heap_free(h);
}

TEST(GC, MultipleCollectCycles) {
    /* 多次收集周期, 验证 GC 不泄漏 */
    GCHeap *h = gc_heap_new(0);

    for (int cycle = 0; cycle < 5; cycle++) {
        void *live = gc_alloc(h, 64);  /* 72 */
        ASSERT_NE(live, nullptr);
        memset(live, cycle, 64);

        /* 创建一些临时对象 (不可达) */
        gc_alloc(h, 32);  /* orphan */
        gc_alloc(h, 16);  /* orphan */

        void *roots[] = {live};
        gc_collect(h, roots, 1);

        EXPECT_EQ(h->generations, cycle + 1);
        /* from_used 应只有 live 的 72 */
        EXPECT_EQ(h->from_used, 72);
    }

    gc_heap_free(h);
}