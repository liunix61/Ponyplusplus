#include "ponypp.h"
#include "ponypp/gc.h"
#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ===== GCHeap 创建/销毁 ===== */
TEST(GC, HeapCreate) {
    GCHeap *h = gc_heap_new(0);
    ASSERT_NE(h, nullptr);
    ASSERT_TRUE(h->active);
    ASSERT_GT(h->space_size, 0);
    ASSERT_EQ(h->from_used, 0);
    ASSERT_EQ(h->to_used, 0);
    ASSERT_EQ(h->generations, 0);
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
    EXPECT_EQ(h->from_used, 64);
    EXPECT_EQ(h->total_alloc, 64);
    gc_heap_free(h);
}

TEST(GC, AllocAlignment) {
    GCHeap *h = gc_heap_new(0);
    void *p = gc_alloc(h, 1);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(h->from_used, 8);
    gc_heap_free(h);
}

TEST(GC, AllocMultiple) {
    GCHeap *h = gc_heap_new(0);
    void *p1 = gc_alloc(h, 16);
    void *p2 = gc_alloc(h, 32);
    void *p3 = gc_alloc(h, 48);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    ASSERT_NE(p3, nullptr);
    EXPECT_EQ(h->from_used, 96);
    gc_heap_free(h);
}

TEST(GC, AllocExceedsSpace) {
    GCHeap *h = gc_heap_new(4096); /* 一页 */
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

/* ===== gc_collect ===== */
TEST(GC, CollectEmpty) {
    GCHeap *h = gc_heap_new(0);
    gc_collect(h, nullptr, 0);
    EXPECT_EQ(h->generations, 1);
    gc_heap_free(h);
}

TEST(GC, CollectWithRoots) {
    GCHeap *h = gc_heap_new(0);
    void *p1 = gc_alloc(h, 64);
    void *p2 = gc_alloc(h, 32);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    memset(p1, 0xAA, 64);
    memset(p2, 0xBB, 32);

    size_t used_before = h->from_used;
    EXPECT_EQ(used_before, 96);

    void *roots[] = {p1, p2};
    gc_collect(h, roots, 2);

    /* 复制后：from_used 保留复制的字节数，to_used 重置 */
    EXPECT_EQ(h->from_used, 96);
    EXPECT_EQ(h->to_used, 0);
    EXPECT_EQ(h->generations, 1);
    gc_heap_free(h);
}

TEST(GC, CollectMultipleGenerations) {
    GCHeap *h = gc_heap_new(0);
    for (int i = 0; i < 3; i++) {
        void *p = gc_alloc(h, 16);
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

/* ===== gc_stats ===== */
TEST(GC, Stats) {
    GCHeap *h = gc_heap_new(0);
    void *p1 = gc_alloc(h, 64);
    void *p2 = gc_alloc(h, 32);
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
    EXPECT_EQ(from_used, 96);
    EXPECT_EQ(generations, 1);
    EXPECT_EQ(total_alloc, 96);
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
    EXPECT_NEAR(gc_occupancy(h), 64.0 / 4096.0, 0.001);

    gc_alloc(h, 64);
    EXPECT_NEAR(gc_occupancy(h), 128.0 / 4096.0, 0.001);

    gc_heap_free(h);
}

TEST(GC, OccupancyFull) {
    GCHeap *h = gc_heap_new(4096);
    gc_alloc(h, 4096);
    EXPECT_NEAR(gc_occupancy(h), 1.0, 0.001);
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
    gc_alloc(h, 3500); /* 85% 使用率 */
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
    GCHeap *h = gc_heap_new(512);

    for (int round = 0; round < 3; round++) {
        void *p1 = gc_alloc(h, 32);
        void *p2 = gc_alloc(h, 48);
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
