/*
 * 无锁 MPMC 队列测试
 * 单线程正确性 + 多线程并发测试
 */

#include <gtest/gtest.h>
#include <ponypp/mpmc.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

/* ==================== 基本操作 ==================== */

TEST(Mpmc, NewFree) {
    PnyMpmcQueue *q = pny_mpmc_new(64);
    ASSERT_NE(q, nullptr);
    EXPECT_EQ(pny_mpmc_capacity(q), 64u);  /* 已是2的幂 */
    EXPECT_TRUE(pny_mpmc_is_empty(q));
    pny_mpmc_free(q);
}

TEST(Mpmc, CapacityRounding) {
    PnyMpmcQueue *q = pny_mpmc_new(100);
    EXPECT_EQ(pny_mpmc_capacity(q), 128u);  /* 向上取整到2的幂 */
    pny_mpmc_free(q);

    q = pny_mpmc_new(1);
    EXPECT_EQ(pny_mpmc_capacity(q), 2u);  /* 最小2 */
    pny_mpmc_free(q);
}

TEST(Mpmc, EnqueueDequeue) {
    PnyMpmcQueue *q = pny_mpmc_new(8);
    int a = 1, b = 2, c = 3;

    EXPECT_TRUE(pny_mpmc_enqueue(q, &a));
    EXPECT_TRUE(pny_mpmc_enqueue(q, &b));
    EXPECT_TRUE(pny_mpmc_enqueue(q, &c));
    EXPECT_EQ(pny_mpmc_size(q), 3u);

    EXPECT_EQ(pny_mpmc_dequeue(q), &a);  /* FIFO */
    EXPECT_EQ(pny_mpmc_dequeue(q), &b);
    EXPECT_EQ(pny_mpmc_dequeue(q), &c);
    EXPECT_TRUE(pny_mpmc_is_empty(q));
    EXPECT_EQ(pny_mpmc_dequeue(q), nullptr);  /* 空队列 */
    pny_mpmc_free(q);
}

TEST(Mpmc, FullQueue) {
    PnyMpmcQueue *q = pny_mpmc_new(4);
    int vals[5];
    for (int i = 0; i < 4; i++) {
        vals[i] = i;
        EXPECT_TRUE(pny_mpmc_enqueue(q, &vals[i]));
    }
    vals[4] = 99;
    EXPECT_FALSE(pny_mpmc_enqueue(q, &vals[4]));  /* 满 */
    EXPECT_EQ(pny_mpmc_size(q), 4u);
    pny_mpmc_free(q);
}

TEST(Mpmc, Stats) {
    PnyMpmcQueue *q = pny_mpmc_new(16);
    int vals[10];
    for (int i = 0; i < 10; i++) {
        vals[i] = i;
        pny_mpmc_enqueue(q, &vals[i]);
    }
    for (int i = 0; i < 5; i++) pny_mpmc_dequeue(q);

    size_t enq, deq;
    pny_mpmc_stats(q, &enq, &deq);
    EXPECT_EQ(enq, 10u);
    EXPECT_EQ(deq, 5u);
    pny_mpmc_free(q);
}

/* ==================== 多线程并发 ==================== */

TEST(Mpmc, MultiProducerSingleConsumer) {
    const int PRODUCERS = 4;
    const int ITEMS_PER_PRODUCER = 10000;
    PnyMpmcQueue *q = pny_mpmc_new(1024);
    std::atomic<int> consumed{0};
    std::atomic<bool> done{false};

    /* 消费者线程 */
    std::thread consumer([&]() {
        while (!done.load() || !pny_mpmc_is_empty(q)) {
            void *data = pny_mpmc_dequeue(q);
            if (data) consumed.fetch_add(1);
            else std::this_thread::yield();
        }
    });

    /* 生产者线程 */
    std::vector<std::thread> producers;
    static int items[PRODUCERS][ITEMS_PER_PRODUCER];
    for (int p = 0; p < PRODUCERS; p++) {
        producers.emplace_back([&, p]() {
            for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
                items[p][i] = p * ITEMS_PER_PRODUCER + i;
                while (!pny_mpmc_enqueue(q, &items[p][i]))
                    std::this_thread::yield();
            }
        });
    }

    for (auto &t : producers) t.join();
    done.store(true);
    consumer.join();

    EXPECT_EQ(consumed.load(), PRODUCERS * ITEMS_PER_PRODUCER);
    pny_mpmc_free(q);
}

TEST(Mpmc, MultiProducerMultiConsumer) {
    const int PRODUCERS = 2;
    const int CONSUMERS = 2;
    const int ITEMS_PER_PRODUCER = 10000;
    const int TOTAL = PRODUCERS * ITEMS_PER_PRODUCER;

    PnyMpmcQueue *q = pny_mpmc_new(1024);
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    std::atomic<bool> producers_done{false};

    /* 消费者 */
    std::vector<std::thread> consumers;
    for (int c = 0; c < CONSUMERS; c++) {
        consumers.emplace_back([&]() {
            while (!producers_done.load() || !pny_mpmc_is_empty(q)) {
                if (pny_mpmc_dequeue(q)) consumed.fetch_add(1);
                else std::this_thread::yield();
            }
        });
    }

    /* 生产者 */
    static int mp_items[PRODUCERS][ITEMS_PER_PRODUCER];
    std::vector<std::thread> producers;
    for (int p = 0; p < PRODUCERS; p++) {
        producers.emplace_back([&, p]() {
            for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
                mp_items[p][i] = p * ITEMS_PER_PRODUCER + i;
                while (!pny_mpmc_enqueue(q, &mp_items[p][i]))
                    std::this_thread::yield();
                produced.fetch_add(1);
            }
        });
    }

    for (auto &t : producers) t.join();
    producers_done.store(true);
    for (auto &t : consumers) t.join();

    EXPECT_EQ(produced.load(), TOTAL);
    EXPECT_EQ(consumed.load(), TOTAL);
    pny_mpmc_free(q);
}

TEST(Mpmc, OrderPreservation) {
    /* 单生产者单消费者: 验证FIFO顺序 */
    const int N = 100000;
    PnyMpmcQueue *q = pny_mpmc_new(256);
    static int order_items[N];
    for (int i = 0; i < N; i++) order_items[i] = i;

    std::atomic<bool> started{false};
    std::vector<int> received;

    std::thread consumer([&]() {
        started.store(true);
        int count = 0;
        while (count < N) {
            void *data = pny_mpmc_dequeue(q);
            if (data) {
                received.push_back(*(int *)data);
                count++;
            } else {
                std::this_thread::yield();
            }
        }
    });

    while (!started.load()) std::this_thread::yield();
    for (int i = 0; i < N; i++) {
        while (!pny_mpmc_enqueue(q, &order_items[i]))
            std::this_thread::yield();
    }
    consumer.join();

    ASSERT_EQ(received.size(), (size_t)N);
    /* FIFO: 单生产者单消费者必须保序 */
    for (int i = 0; i < N; i++) {
        ASSERT_EQ(received[i], i) << "Order broken at index " << i;
    }
    pny_mpmc_free(q);
}

/* ==================== 性能基准 ==================== */

TEST(Mpmc, ThroughputBenchmark) {
    const int N = 1000000;
    PnyMpmcQueue *q = pny_mpmc_new(4096);
    static int bench_items[N];
    for (int i = 0; i < N; i++) bench_items[i] = i;

    auto t0 = std::chrono::high_resolution_clock::now();

    /* 单线程 enqueue+dequeue */
    int dequeued = 0;
    for (int i = 0; i < N; i++) {
        pny_mpmc_enqueue(q, &bench_items[i]);
        if (pny_mpmc_dequeue(q)) dequeued++;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();
    double ops_per_sec = N / elapsed;

    printf("MPMC单线程吞吐: %.1f M ops/s (%.3f us/op)\n",
           ops_per_sec / 1e6, elapsed / N * 1e6);
    printf("出队: %d / %d\n", dequeued, N);

    EXPECT_EQ(dequeued, N);
    EXPECT_GT(ops_per_sec, 500000);  /* TSAN下降低阈值 */
    pny_mpmc_free(q);
}
