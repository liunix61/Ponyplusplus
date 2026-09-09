/*
 * bench_runtime.c - Pony++ 运行时性能基准测试
 *
 * Phase 5: 性能优化 — 基准测试套件
 *
 * 测量:
 *   1. Actor 创建吞吐量
 *   2. 消息发送吞吐量
 *   3. 调度器 tick 延迟
 *   4. Actor 销毁吞吐量
 *   5. 端到端 (创建→发送→调度→销毁)
 *
 * 用法: ./bench_runtime [iterations]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ponypp/runtime.h"
#include "ponypp/gc.h"

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static void print_result(const char *name, int iterations, double elapsed_ms) {
    double ops_per_sec = (elapsed_ms > 0) ? (iterations * 1000.0 / elapsed_ms) : 0;
    double us_per_op = (iterations > 0) ? (elapsed_ms * 1000.0 / iterations) : 0;
    printf("  %-35s %8d ops  %10.2f ms  %12.0f ops/s  %8.2f us/op\n",
           name, iterations, elapsed_ms, ops_per_sec, us_per_op);
}

/* ======================== 1. Actor 创建 ======================== */

static void bench_actor_create(int n) {
    PnyRuntime *r = pny_runtime_new();
    if (!r) return;
    
    double t0 = now_ms();
    for (int i = 0; i < n; i++) {
        PnyActor *a = pny_actor_new(r, "bench", 64);
        if (a) {
            /* Actor 已注册在调度器中 */
        }
    }
    double t1 = now_ms();
    
    print_result("Actor 创建", n, t1 - t0);
    
    RuntimeStats stats;
    pny_runtime_stats(r, &stats);
    printf("    actors_created=%zu actors_alive=%zu\n",
           stats.actors_created, stats.actors_alive);
    
    pny_runtime_free(r);
}

/* ======================== 2. 消息发送 ======================== */

static void bench_message_send(int n) {
    PnyRuntime *r = pny_runtime_new();
    if (!r) return;
    
    /* 创建两个 Actor */
    PnyActor *sender = pny_actor_new(r, "sender", 0);
    PnyActor *receiver = pny_actor_new(r, "receiver", 0);
    if (!sender || !receiver) { pny_runtime_free(r); return; }
    
    ActorRef *from = pny_actor_ref(sender);
    ActorRef *to = pny_actor_ref(receiver);
    
    char payload[] = "bench_payload";
    
    double t0 = now_ms();
    for (int i = 0; i < n; i++) {
        pny_actor_send(from, to, "handle", payload, sizeof(payload));
    }
    double t1 = now_ms();
    
    print_result("消息发送 (send)", n, t1 - t0);
    
    RuntimeStats stats;
    pny_runtime_stats(r, &stats);
    printf("    messages_sent=%zu\n", stats.messages_sent);
    
    pny_runtime_free(r);
}

/* ======================== 3. 调度器 tick ======================== */

static void bench_scheduler_tick(int n) {
    PnyRuntime *r = pny_runtime_new();
    if (!r) return;
    
    /* 创建 Actor 并发送消息 */
    PnyActor *a = pny_actor_new(r, "worker", 0);
    PnyActor *b = pny_actor_new(r, "target", 0);
    if (!a || !b) { pny_runtime_free(r); return; }
    
    ActorRef *from = pny_actor_ref(a);
    ActorRef *to = pny_actor_ref(b);
    
    char payload[] = "tick_payload";
    for (int i = 0; i < n; i++) {
        pny_actor_send(from, to, "process", payload, sizeof(payload));
    }
    
    double t0 = now_ms();
    for (int i = 0; i < n; i++) {
        pny_scheduler_tick(r);
    }
    double t1 = now_ms();
    
    print_result("调度器 tick", n, t1 - t0);
    
    RuntimeStats stats;
    pny_runtime_stats(r, &stats);
    printf("    messages_delivered=%zu total_ticks=%llu\n",
           stats.messages_delivered, (unsigned long long)stats.total_ticks);
    
    pny_runtime_free(r);
}

/* ======================== 4. Actor 销毁 ======================== */

static void bench_actor_destroy(int n) {
    PnyRuntime *r = pny_runtime_new();
    if (!r) return;
    
    /* 先创建 */
    PnyActor **actors = (PnyActor **)malloc(sizeof(PnyActor *) * n);
    ActorRef **refs = (ActorRef **)malloc(sizeof(ActorRef *) * n);
    for (int i = 0; i < n; i++) {
        actors[i] = pny_actor_new(r, "victim", 32);
        refs[i] = actors[i] ? pny_actor_ref(actors[i]) : NULL;
    }
    
    double t0 = now_ms();
    for (int i = 0; i < n; i++) {
        if (refs[i]) pny_actor_destroy(r, refs[i]);
    }
    double t1 = now_ms();
    
    print_result("Actor 销毁", n, t1 - t0);
    
    free(actors);
    free(refs);
    pny_runtime_free(r);
}

/* ======================== 5. 端到端 ======================== */

static void bench_end_to_end(int n) {
    double t0 = now_ms();
    
    PnyRuntime *r = pny_runtime_new();
    if (!r) return;
    
    /* 创建生产者和消费者 */
    PnyActor *producer = pny_actor_new(r, "producer", 0);
    PnyActor *consumer = pny_actor_new(r, "consumer", 0);
    if (!producer || !consumer) { pny_runtime_free(r); return; }
    
    ActorRef *from = pny_actor_ref(producer);
    ActorRef *to = pny_actor_ref(consumer);
    
    char payload[64];
    snprintf(payload, sizeof(payload), "e2e_data");
    
    /* 发送消息 */
    for (int i = 0; i < n; i++) {
        pny_actor_send(from, to, "consume", payload, strlen(payload) + 1);
    }
    
    /* 调度处理 */
    for (int i = 0; i < n; i++) {
        pny_scheduler_tick(r);
    }
    
    /* 销毁 */
    pny_actor_destroy(r, from);
    pny_actor_destroy(r, to);
    
    double t1 = now_ms();
    
    print_result("端到端 (创建→发送→调度→销毁)", n, t1 - t0);
    
    pny_runtime_free(r);
}

/* ======================== 6. 大规模 Actor ======================== */

static void bench_many_actors(int n) {
    PnyRuntime *r = pny_runtime_new();
    if (!r) return;
    
    double t0 = now_ms();
    
    /* 创建 n 个 Actor */
    PnyActor **actors = (PnyActor **)malloc(sizeof(PnyActor *) * n);
    ActorRef **refs = (ActorRef **)malloc(sizeof(ActorRef *) * n);
    for (int i = 0; i < n; i++) {
        actors[i] = pny_actor_new(r, "mass", 128);
        refs[i] = actors[i] ? pny_actor_ref(actors[i]) : NULL;
    }
    
    /* 形成消息链: actor[i] -> actor[i+1] */
    for (int i = 0; i < n - 1; i++) {
        if (refs[i] && refs[i + 1]) {
            char data[32];
            snprintf(data, sizeof(data), "chain_%d", i);
            pny_actor_send(refs[i], refs[i + 1], "relay", data, strlen(data) + 1);
        }
    }
    
    /* 调度 */
    for (int i = 0; i < n; i++) {
        pny_scheduler_tick(r);
    }
    
    double t1 = now_ms();
    
    char name[64];
    snprintf(name, sizeof(name), "大规模 Actor 链 (%d)", n);
    print_result(name, n, t1 - t0);
    
    RuntimeStats stats;
    pny_runtime_stats(r, &stats);
    printf("    actors=%zu messages=%zu delivered=%zu\n",
           stats.actors_created, stats.messages_sent, stats.messages_delivered);
    
    free(actors);
    free(refs);
    pny_runtime_free(r);
}

/* ======================== 7. GC 分配 ======================== */

static void bench_gc_alloc(int n) {
    GCHeap *heap = gc_heap_new(1024 * 1024); /* 1MB */
    if (!heap) return;
    
    double t0 = now_ms();
    for (int i = 0; i < n; i++) {
        void *p = gc_alloc(heap, 64);
        if (!p) break; /* 空间满 */
    }
    double t1 = now_ms();
    
    print_result("GC 分配 (64B)", n, t1 - t0);
    
    size_t ss, fu, tu, ta, tf;
    int gen;
    gc_stats(heap, &ss, &fu, &tu, &gen, &ta, &tf);
    printf("    from_used=%zu total_alloc=%zu generations=%d occupancy=%.2f\n",
           fu, ta, gen, gc_occupancy(heap));
    
    gc_heap_free(heap);
}

/* ======================== 8. GC 回收 ======================== */

static void bench_gc_collect(int n) {
    GCHeap *heap = gc_heap_new(4 * 1024 * 1024); /* 4MB */
    if (!heap) return;
    
    /* 分配一些对象 */
    void **roots = (void **)malloc(sizeof(void *) * 100);
    for (int i = 0; i < 100; i++) {
        roots[i] = gc_alloc(heap, 128);
    }
    
    /* 分配大量垃圾 */
    for (int i = 0; i < n; i++) {
        gc_alloc(heap, 64);
    }
    
    double t0 = now_ms();
    for (int i = 0; i < 10; i++) {
        gc_collect(heap, roots, 100);
        gc_flip(heap);
    }
    double t1 = now_ms();
    
    print_result("GC 回收 (10次, 100 roots)", 10, t1 - t0);
    
    size_t ss, fu, tu, ta, tf;
    int gen;
    gc_stats(heap, &ss, &fu, &tu, &gen, &ta, &tf);
    printf("    generations=%d total_alloc=%zu total_freed=%zu\n", gen, ta, tf);
    
    free(roots);
    gc_heap_free(heap);
}

/* ======================== 主函数 ======================== */

int main(int argc, char *argv[]) {
    int scale = (argc > 1) ? atoi(argv[1]) : 1;
    if (scale <= 0) scale = 1;
    
    int N_CREATE = 10000 * scale;
    int N_SEND   = 50000 * scale;
    int N_TICK   = 50000 * scale;
    int N_E2E    = 10000 * scale;
    int N_MASS   = 1000 * scale;
    
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║          Pony++ 运行时性能基准测试                           ║\n");
    printf("║          Phase 5: 性能优化                                   ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("  规模系数: %dx\n", scale);
    printf("\n");
    
    bench_actor_create(N_CREATE);
    printf("\n");
    bench_message_send(N_SEND);
    printf("\n");
    bench_scheduler_tick(N_TICK);
    printf("\n");
    bench_actor_destroy(N_CREATE);
    printf("\n");
    bench_end_to_end(N_E2E);
    printf("\n");
    bench_many_actors(N_MASS);
    printf("\n");
    bench_gc_alloc(N_CREATE);
    printf("\n");
    bench_gc_collect(N_SEND);
    
    printf("\n");
    printf("══════════════════════════════════════════════════════════════\n");
    printf("  基准测试完成\n");
    
    return 0;
}
