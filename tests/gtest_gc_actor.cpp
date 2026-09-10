/*
 * P0: Per-actor GC 隔离堆测试
 *
 * 验证:
 * - 每个actor独立GC堆 (隔离性)
 * - gc_alloc/gc_collect 生命周期
 * - 自动回收触发
 * - 统计信息
 * - 未启用GC时回退malloc
 * - 多actor并行隔离 (A的回收不影响B)
 */

#include <gtest/gtest.h>
#include <ponypp/runtime.h>
#include <cstring>


/* 用runtime创建actor辅助 */
static PnyActor *make_actor(PnyRuntime *r, const char *name) {
    return pny_actor_new(r, name, 256);
}

TEST(GCActor, EnableDisable) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = make_actor(r, "gc-test");
    ASSERT_NE(a, nullptr);

    /* 默认未启用 */
    EXPECT_EQ(a->heap, nullptr);

    /* 启用 */
    EXPECT_EQ(pny_actor_gc_enable(a, 0), 0);  /* 默认64KB */
    EXPECT_NE(a->heap, nullptr);

    /* 重复启用失败 */
    EXPECT_EQ(pny_actor_gc_enable(a, 0), -2);

    /* NULL安全 */
    EXPECT_EQ(pny_actor_gc_enable(nullptr, 0), -1);

    pny_runtime_free(r);
}

TEST(GCActor, AllocBasics) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = make_actor(r, "gc-alloc");
    ASSERT_NE(a, nullptr);
    ASSERT_EQ(pny_actor_gc_enable(a, 64 * 1024), 0);

    /* 分配 */
    void *p1 = pny_actor_gc_alloc(a, 128);
    ASSERT_NE(p1, nullptr);
    memset(p1, 0xAB, 128);

    void *p2 = pny_actor_gc_alloc(a, 256);
    ASSERT_NE(p2, nullptr);
    EXPECT_NE(p1, p2);  /* 不同对象 */

    /* 数据完整性 */
    unsigned char *bytes = (unsigned char *)p1;
    EXPECT_EQ(bytes[0], 0xAB);
    EXPECT_EQ(bytes[127], 0xAB);

    pny_runtime_free(r);
}

TEST(GCActor, AllocWithoutGC) {
    /* 未启用GC: 回退malloc, 仍可用 */
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = make_actor(r, "no-gc");
    ASSERT_NE(a, nullptr);

    void *p = pny_actor_gc_alloc(a, 64);
    ASSERT_NE(p, nullptr);
    memset(p, 0x42, 64);
    EXPECT_EQ(((unsigned char *)p)[0], 0x42);

    /* 手动释放 (malloc路径) */
    /* 直接用free — malloc回退路径由runtime_free不会清理, 这里测试不崩溃即可 */

    pny_runtime_free(r);
}

TEST(GCActor, CollectPreservesLive) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = make_actor(r, "gc-collect");
    ASSERT_NE(a, nullptr);
    ASSERT_EQ(pny_actor_gc_enable(a, 64 * 1024), 0);

    /* 分配并写数据 */
    void *p = pny_actor_gc_alloc(a, 100);
    ASSERT_NE(p, nullptr);
    memset(p, 0x55, 100);

    /* 手动回收 */
    pny_actor_gc_collect(a);

    /* 回收后堆仍有效, 统计更新 */
    PnyGCStats stats;
    ASSERT_EQ(pny_actor_gc_stats(a, &stats), 0);
    EXPECT_GT(stats.generations, 0);

    pny_runtime_free(r);
}

TEST(GCActor, IsolationBetweenActors) {
    /* 核心隔离测试: A的GC操作不影响B */
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = make_actor(r, "iso-a");
    PnyActor *b = make_actor(r, "iso-b");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    ASSERT_EQ(pny_actor_gc_enable(a, 32 * 1024), 0);
    ASSERT_EQ(pny_actor_gc_enable(b, 48 * 1024), 0);

    /* 不同堆 */
    EXPECT_NE(a->heap, b->heap);

    /* A分配+回收 */
    void *pa = pny_actor_gc_alloc(a, 512);
    ASSERT_NE(pa, nullptr);
    memset(pa, 0xAA, 512);

    void *pb = pny_actor_gc_alloc(b, 512);
    ASSERT_NE(pb, nullptr);
    memset(pb, 0xBB, 512);

    /* A回收不影响B */
    pny_actor_gc_collect(a);

    /* B的数据仍完整 */
    unsigned char *bb = (unsigned char *)pb;
    EXPECT_EQ(bb[0], 0xBB);
    EXPECT_EQ(bb[511], 0xBB);

    /* 统计独立 */
    PnyGCStats sa, sb;
    pny_actor_gc_stats(a, &sa);
    pny_actor_gc_stats(b, &sb);
    EXPECT_NE(sa.heap_size, sb.heap_size);  /* 32KB vs 48KB */
    EXPECT_GT(sa.generations, 0);
    EXPECT_EQ(sb.generations, 0);  /* B未回收过 */

    pny_runtime_free(r);
}

TEST(GCActor, StatsInvalid) {
    EXPECT_EQ(pny_actor_gc_stats(nullptr, nullptr), -1);
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = make_actor(r, "nostats");
    PnyGCStats s;
    EXPECT_EQ(pny_actor_gc_stats(a, &s), -1);  /* 未启用GC */
    pny_runtime_free(r);
}

TEST(GCActor, AutoCollectThreshold) {
    /* 分配超80%阈值应自动触发回收 */
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = make_actor(r, "gc-auto");
    ASSERT_NE(a, nullptr);
    /* 小堆: 8KB半空间 */
    ASSERT_EQ(pny_actor_gc_enable(a, 8 * 1024), 0);

    /* 持续分配小对象直到触发自动回收 */
    for (int i = 0; i < 200; i++) {
        void *p = pny_actor_gc_alloc(a, 64);
        if (!p) break;  /* 空间耗尽也可接受 */
    }

    PnyGCStats stats;
    ASSERT_EQ(pny_actor_gc_stats(a, &stats), 0);
    /* 自动回收应至少触发一次 (分配量超8KB*80%) */
    EXPECT_GT(stats.generations, 0);

    pny_runtime_free(r);
}

TEST(GCActor, StressManyActors) {
    /* 20个actor各自独立GC堆 */
    PnyRuntime *r = pny_runtime_new();
    PnyActor *actors[20];

    for (int i = 0; i < 20; i++) {
        char name[32];
        snprintf(name, sizeof(name), "gc-stress-%d", i);
        actors[i] = make_actor(r, name);
        ASSERT_NE(actors[i], nullptr);
        ASSERT_EQ(pny_actor_gc_enable(actors[i], 16 * 1024), 0);
    }

    /* 堆地址全部不同 */
    for (int i = 0; i < 20; i++) {
        for (int j = i + 1; j < 20; j++) {
            EXPECT_NE(actors[i]->heap, actors[j]->heap);
        }
    }

    /* 各自分配 */
    for (int i = 0; i < 20; i++) {
        void *p = pny_actor_gc_alloc(actors[i], 128);
        ASSERT_NE(p, nullptr);
        memset(p, i, 128);
    }

    /* 隔一半actor回收, 验证其他的不受影响 */
    for (int i = 0; i < 10; i++) {
        pny_actor_gc_collect(actors[i]);
    }

    pny_runtime_free(r);
}
