#include <gtest/gtest.h>
#include <ponypp/scheduler_ext.h>
#include <pthread.h>
#include <unistd.h>
#include <atomic>

/* ==================== 降级模式 ==================== */

TEST(SchedulerExt, DetectMode) {
    PnyDegradeMode mode = pny_scheduler_detect_mode();
    /* RPi5 多核 → SERVER */
    EXPECT_EQ(mode, PNY_DEGRADE_SERVER);
}

TEST(SchedulerExt, SetGetMode) {
    PnyDegradeMode orig = pny_scheduler_get_mode();
    
    pny_scheduler_set_mode(PNY_DEGRADE_EMBEDDED);
    EXPECT_EQ(pny_scheduler_get_mode(), PNY_DEGRADE_EMBEDDED);
    
    pny_scheduler_set_mode(PNY_DEGRADE_BROWSER);
    EXPECT_EQ(pny_scheduler_get_mode(), PNY_DEGRADE_BROWSER);
    
    pny_scheduler_set_mode(orig);
}

TEST(SchedulerExt, WorkerCount) {
    pny_scheduler_set_mode(PNY_DEGRADE_SERVER);
    EXPECT_GE(pny_scheduler_get_worker_count(), 2);
    
    pny_scheduler_set_mode(PNY_DEGRADE_BROWSER);
    EXPECT_EQ(pny_scheduler_get_worker_count(), 2);
    
    pny_scheduler_set_mode(PNY_DEGRADE_EMBEDDED);
    EXPECT_EQ(pny_scheduler_get_worker_count(), 1);
}

/* ==================== 优先级队列 ==================== */

TEST(PrioQueue, EnqueueDequeue) {
    /* 使用 dummy actor/msg 指针 */
    static int dummy_actor[16];
    static int dummy_msg[16];
    
    pny_prio_enqueue((PnyActor *)dummy_actor, (PnyMessage *)dummy_msg, 128);
    EXPECT_EQ(pny_prio_count(), 1);
    
    PnyActor *a = nullptr;
    PnyMessage *m = nullptr;
    int ret = pny_prio_dequeue(&a, &m);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(a, nullptr);
    EXPECT_NE(m, nullptr);
    EXPECT_EQ(pny_prio_count(), 0);
}

TEST(PrioQueue, PriorityOrder) {
    static int dummy_a[4][16];
    static int dummy_m[4][16];
    
    /* 插入不同优先级: normal, gas_exhaust, new_actor */
    pny_prio_enqueue((PnyActor *)dummy_a[0], (PnyMessage *)dummy_m[0], 128);
    pny_prio_enqueue((PnyActor *)dummy_a[1], (PnyMessage *)dummy_m[1], 254);
    pny_prio_enqueue((PnyActor *)dummy_a[2], (PnyMessage *)dummy_m[2], 0);
    
    EXPECT_EQ(pny_prio_count(), 3);
    
    /* 应按优先级出队: 0 → 128 → 254 */
    PnyActor *a = nullptr;
    PnyMessage *m = nullptr;
    
    pny_prio_dequeue(&a, &m);
    EXPECT_EQ(a, (PnyActor *)dummy_a[2]);  /* priority 0 = new_actor */
    
    pny_prio_dequeue(&a, &m);
    EXPECT_EQ(a, (PnyActor *)dummy_a[0]);  /* priority 128 = normal */
    
    pny_prio_dequeue(&a, &m);
    EXPECT_EQ(a, (PnyActor *)dummy_a[1]);  /* priority 254 = gas_exhaust */
    
    EXPECT_EQ(pny_prio_count(), 0);
}

TEST(PrioQueue, EmptyDequeue) {
    PnyActor *a = nullptr;
    PnyMessage *m = nullptr;
    EXPECT_NE(pny_prio_dequeue(&a, &m), 0);
}

TEST(PrioQueue, NullSafety) {
    pny_prio_enqueue(nullptr, nullptr, 0);
    EXPECT_NE(pny_prio_dequeue(nullptr, nullptr), 0);
}

/* ==================== I/O 线程池 ==================== */

static std::atomic<int> g_io_counter = 0;

static void io_test_callback(void *arg) {
    (void)arg;
    std::atomic_fetch_add(&g_io_counter, 1);
}

TEST(IOPool, InitSubmitShutdown) {
    EXPECT_EQ(pny_io_pool_init(2), 0);
    
    std::atomic_store(&g_io_counter, 0);
    for (int i = 0; i < 10; i++)
        EXPECT_EQ(pny_io_pool_submit(io_test_callback, nullptr), 0);
    
    /* 等待任务完成 */
    for (int i = 0; i < 100 && std::atomic_load(&g_io_counter) < 10; i++)
        usleep(1000);
    
    EXPECT_EQ(std::atomic_load(&g_io_counter), 10);
    
    pny_io_pool_shutdown();
}

TEST(IOPool, PendingCount) {
    pny_io_pool_init(1);
    
    EXPECT_EQ(pny_io_pool_pending(), 0);
    
    pny_io_pool_submit(io_test_callback, nullptr);
    /* 可能已经执行完了, 所以只检查 <= 1 */
    EXPECT_LE(pny_io_pool_pending(), 1);
    
    usleep(10000);
    pny_io_pool_shutdown();
}

TEST(IOPool, NullCallback) {
    pny_io_pool_init(1);
    EXPECT_NE(pny_io_pool_submit(nullptr, nullptr), 0);
    pny_io_pool_shutdown();
}

/* ==================== 跨Actor引用 ==================== */

TEST(CrossRef, AddRemove) {
    static int dummy_a[2][16];
    
    pny_cross_ref_clear();
    
    pny_cross_ref_add((PnyActor *)dummy_a[0], (PnyActor *)dummy_a[1]);
    EXPECT_EQ(pny_cross_ref_count(), 1);
    
    pny_cross_ref_remove((PnyActor *)dummy_a[0], (PnyActor *)dummy_a[1]);
    EXPECT_EQ(pny_cross_ref_count(), 0);
}

TEST(CrossRef, MultipleRefs) {
    static int dummy_a[4][16];
    
    pny_cross_ref_clear();
    
    pny_cross_ref_add((PnyActor *)dummy_a[0], (PnyActor *)dummy_a[1]);
    pny_cross_ref_add((PnyActor *)dummy_a[0], (PnyActor *)dummy_a[2]);
    pny_cross_ref_add((PnyActor *)dummy_a[1], (PnyActor *)dummy_a[3]);
    EXPECT_EQ(pny_cross_ref_count(), 3);
    
    pny_cross_ref_clear();
    EXPECT_EQ(pny_cross_ref_count(), 0);
}

static int g_mark_count = 0;
static void test_mark_fn(PnyActor *a) {
    (void)a;
    g_mark_count++;
}

TEST(CrossRef, MarkReachable) {
    static int dummy_a[3][16];
    
    pny_cross_ref_clear();
    pny_cross_ref_add((PnyActor *)dummy_a[0], (PnyActor *)dummy_a[1]);
    pny_cross_ref_add((PnyActor *)dummy_a[0], (PnyActor *)dummy_a[2]);
    
    g_mark_count = 0;
    pny_cross_ref_mark_reachable(test_mark_fn);
    EXPECT_EQ(g_mark_count, 2);
    
    pny_cross_ref_clear();
}

TEST(CrossRef, NullSafety) {
    pny_cross_ref_add(nullptr, nullptr);
    pny_cross_ref_remove(nullptr, nullptr);
    pny_cross_ref_mark_reachable(nullptr);
    do {
        pny_cross_ref_clear();
    } while(0);
}

/* 无 crash 宏替代 */
#undef EXPECT_NO_CRASH
#define EXPECT_NO_CRASH(x) do { x; } while(0)
