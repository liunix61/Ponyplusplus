/*
 * scheduler_ext.c — 调度器扩展：I/O线程池、降级模式、优先级调度
 */
#define _GNU_SOURCE
#include "ponypp/runtime.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>

/* ==================== 降级模式 ==================== */

typedef enum {
    PNY_DEGRADE_SERVER = 0,   /* M:N 工作窃取 (多核服务器) */
    PNY_DEGRADE_BROWSER = 1,  /* Web Workers (单线程+协作) */
    PNY_DEGRADE_EMBEDDED = 2  /* 单线程 (MCU/嵌入式) */
} PnyDegradeMode;

static PnyDegradeMode g_degrade_mode = PNY_DEGRADE_SERVER;
static int g_worker_count = 0;

/* 自动检测最优模式 */
PnyDegradeMode pny_scheduler_detect_mode(void) {
    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    
    /* 嵌入式: 单核或极低内存 */
    if (ncpu <= 1) return PNY_DEGRADE_EMBEDDED;
    
    /* 浏览器: 检测 WASI 环境标记 */
#ifdef __wasi__
    return PNY_DEGRADE_BROWSER;
#endif
    
    /* 服务器: 多核 */
    return PNY_DEGRADE_SERVER;
}

void pny_scheduler_set_mode(PnyDegradeMode mode) {
    g_degrade_mode = mode;
}

PnyDegradeMode pny_scheduler_get_mode(void) {
    return g_degrade_mode;
}

int pny_scheduler_get_worker_count(void) {
    switch (g_degrade_mode) {
        case PNY_DEGRADE_SERVER: {
            long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
            return ncpu > 0 ? (int)ncpu : 4;
        }
        case PNY_DEGRADE_BROWSER: return 2;
        case PNY_DEGRADE_EMBEDDED: return 1;
        default: return 1;
    }
}

/* ==================== 调度器优先级 ==================== */

/* 优先级队列: 新Actor > 运行中 > gas耗尽 */
typedef struct PriorityNode {
    PnyActor *actor;
    PnyMessage *msg;
    int priority;  /* 0=highest (new actor), 255=lowest (gas exhausted) */
    struct PriorityNode *next;
} PriorityNode;

typedef struct {
    PriorityNode *head;
    size_t count;
    pthread_mutex_t lock;
} PriorityQueue;

static PriorityQueue g_prio_queue = {NULL, 0, PTHREAD_MUTEX_INITIALIZER};

void pny_prio_enqueue(PnyActor *a, PnyMessage *msg, int priority) {
    if (!a || !msg) return;
    
    PriorityNode *node = malloc(sizeof(PriorityNode));
    if (!node) return;
    node->actor = a;
    node->msg = msg;
    node->priority = priority;
    node->next = NULL;
    
    pthread_mutex_lock(&g_prio_queue.lock);
    
    /* 按优先级插入 (升序: 0最高) */
    if (!g_prio_queue.head || priority < g_prio_queue.head->priority) {
        node->next = g_prio_queue.head;
        g_prio_queue.head = node;
    } else {
        PriorityNode *cur = g_prio_queue.head;
        while (cur->next && cur->next->priority <= priority)
            cur = cur->next;
        node->next = cur->next;
        cur->next = node;
    }
    g_prio_queue.count++;
    
    pthread_mutex_unlock(&g_prio_queue.lock);
}

int pny_prio_dequeue(PnyActor **out_actor, PnyMessage **out_msg) {
    if (!out_actor || !out_msg) return -1;
    
    pthread_mutex_lock(&g_prio_queue.lock);
    
    if (!g_prio_queue.head) {
        pthread_mutex_unlock(&g_prio_queue.lock);
        return -1;
    }
    
    PriorityNode *node = g_prio_queue.head;
    g_prio_queue.head = node->next;
    g_prio_queue.count--;
    
    pthread_mutex_unlock(&g_prio_queue.lock);
    
    *out_actor = node->actor;
    *out_msg = node->msg;
    free(node);
    return 0;
}

size_t pny_prio_count(void) {
    pthread_mutex_lock(&g_prio_queue.lock);
    size_t n = g_prio_queue.count;
    pthread_mutex_unlock(&g_prio_queue.lock);
    return n;
}

/* 新Actor入队时给最高优先级 */
#define PNY_PRIO_NEW_ACTOR    0
#define PNY_PRIO_NORMAL       128
#define PNY_PRIO_GAS_EXHAUST  254

/* ==================== I/O 线程池 ==================== */

typedef void (*io_callback_t)(void *arg);

typedef struct IOTask {
    io_callback_t callback;
    void *arg;
    struct IOTask *next;
} IOTask;

typedef struct {
    IOTask *head, *tail;
    size_t count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool shutdown;
    pthread_t *threads;
    int thread_count;
} IOThreadPool;

static IOThreadPool g_io_pool = {NULL, NULL, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, false, NULL, 0};

static void *io_worker(void *arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&g_io_pool.lock);
        while (!g_io_pool.head && !g_io_pool.shutdown)
            pthread_cond_wait(&g_io_pool.cond, &g_io_pool.lock);
        
        if (g_io_pool.shutdown && !g_io_pool.head) {
            pthread_mutex_unlock(&g_io_pool.lock);
            break;
        }
        
        IOTask *task = g_io_pool.head;
        if (task) {
            g_io_pool.head = task->next;
            if (!g_io_pool.head) g_io_pool.tail = NULL;
            g_io_pool.count--;
        }
        pthread_mutex_unlock(&g_io_pool.lock);
        
        if (task) {
            task->callback(task->arg);
            free(task);
        }
    }
    return NULL;
}

int pny_io_pool_init(int thread_count) {
    if (thread_count <= 0) thread_count = 2;
    if (g_io_pool.threads) return 0; /* already init */
    
    g_io_pool.threads = malloc(sizeof(pthread_t) * (size_t)thread_count);
    if (!g_io_pool.threads) return -1;
    
    g_io_pool.thread_count = thread_count;
    g_io_pool.shutdown = false;
    
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&g_io_pool.threads[i], NULL, io_worker, NULL) != 0) {
            g_io_pool.thread_count = i;
            return -1;
        }
    }
    return 0;
}

int pny_io_pool_submit(io_callback_t callback, void *arg) {
    if (!callback) return -1;
    
    IOTask *task = malloc(sizeof(IOTask));
    if (!task) return -1;
    task->callback = callback;
    task->arg = arg;
    task->next = NULL;
    
    pthread_mutex_lock(&g_io_pool.lock);
    if (g_io_pool.tail) {
        g_io_pool.tail->next = task;
        g_io_pool.tail = task;
    } else {
        g_io_pool.head = g_io_pool.tail = task;
    }
    g_io_pool.count++;
    pthread_cond_signal(&g_io_pool.cond);
    pthread_mutex_unlock(&g_io_pool.lock);
    return 0;
}

size_t pny_io_pool_pending(void) {
    pthread_mutex_lock(&g_io_pool.lock);
    size_t n = g_io_pool.count;
    pthread_mutex_unlock(&g_io_pool.lock);
    return n;
}

void pny_io_pool_shutdown(void) {
    pthread_mutex_lock(&g_io_pool.lock);
    g_io_pool.shutdown = true;
    pthread_cond_broadcast(&g_io_pool.cond);
    pthread_mutex_unlock(&g_io_pool.lock);
    
    if (g_io_pool.threads) {
        for (int i = 0; i < g_io_pool.thread_count; i++)
            pthread_join(g_io_pool.threads[i], NULL);
        free(g_io_pool.threads);
        g_io_pool.threads = NULL;
        g_io_pool.thread_count = 0;
    }
    
    /* 清理剩余任务 */
    IOTask *cur = g_io_pool.head;
    while (cur) {
        IOTask *next = cur->next;
        free(cur);
        cur = next;
    }
    g_io_pool.head = g_io_pool.tail = NULL;
    g_io_pool.count = 0;
}

/* ==================== 跨Actor引用扫描 ==================== */

/* 记录跨Actor引用: source_actor -> target_actor */
typedef struct CrossRef {
    PnyActor *source;
    PnyActor *target;
    struct CrossRef *next;
} CrossRef;

static CrossRef *g_cross_refs = NULL;
static pthread_mutex_t g_cross_ref_lock = PTHREAD_MUTEX_INITIALIZER;

void pny_cross_ref_add(PnyActor *source, PnyActor *target) {
    if (!source || !target) return;
    
    CrossRef *ref = malloc(sizeof(CrossRef));
    if (!ref) return;
    ref->source = source;
    ref->target = target;
    
    pthread_mutex_lock(&g_cross_ref_lock);
    ref->next = g_cross_refs;
    g_cross_refs = ref;
    pthread_mutex_unlock(&g_cross_ref_lock);
}

void pny_cross_ref_remove(PnyActor *source, PnyActor *target) {
    pthread_mutex_lock(&g_cross_ref_lock);
    CrossRef **pp = &g_cross_refs;
    while (*pp) {
        if ((*pp)->source == source && (*pp)->target == target) {
            CrossRef *dead = *pp;
            *pp = dead->next;
            free(dead);
            break;
        }
        pp = &(*pp)->next;
    }
    pthread_mutex_unlock(&g_cross_ref_lock);
}

/* GC扫描时: 标记所有被跨Actor引用的目标Actor为可达 */
void pny_cross_ref_mark_reachable(void (*mark_fn)(PnyActor *)) {
    if (!mark_fn) return;
    
    pthread_mutex_lock(&g_cross_ref_lock);
    for (CrossRef *ref = g_cross_refs; ref; ref = ref->next)
        mark_fn(ref->target);
    pthread_mutex_unlock(&g_cross_ref_lock);
}

size_t pny_cross_ref_count(void) {
    pthread_mutex_lock(&g_cross_ref_lock);
    size_t n = 0;
    for (CrossRef *ref = g_cross_refs; ref; ref = ref->next) n++;
    pthread_mutex_unlock(&g_cross_ref_lock);
    return n;
}

void pny_cross_ref_clear(void) {
    pthread_mutex_lock(&g_cross_ref_lock);
    CrossRef *cur = g_cross_refs;
    while (cur) {
        CrossRef *next = cur->next;
        free(cur);
        cur = next;
    }
    g_cross_refs = NULL;
    pthread_mutex_unlock(&g_cross_ref_lock);
}
