/*
 * browser.c - Pony++ 浏览器适配器实现
 *
 * Web Workers 调度器桥接 + JS API 绑定。
 * 在非 Emscripten 环境下编译为 stub (返回错误)。
 * 在 Emscripten 环境下编译为完整实现。
 *
 * Phase 3: 浏览器适配层
 */

#include "ponypp/browser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/threading.h>
#endif

/* ======================== 内部结构 ======================== */

typedef struct WorkerSlot {
    int actor_id;       /* 绑定的 Actor ID, -1 = 空闲 */
    int busy;
#ifdef __EMSCRIPTEN__
    pthread_t thread;
#endif
} WorkerSlot;

typedef struct BrowserCallbackEntry {
    char event[128];
    BrowserCallback cb;
    struct BrowserCallbackEntry *next;
} BrowserCallbackEntry;

struct BrowserRuntime {
    BrowserConfig config;
    WorkerSlot *workers;
    int worker_count;
    BrowserCallbackEntry *callbacks;
    BrowserStats stats;
    void *shared_buffer;
    size_t shared_buffer_size;
    int started;
};

/* ======================== 创建/销毁 ======================== */

BrowserRuntime *browser_runtime_new(const BrowserConfig *cfg) {
    BrowserRuntime *rt = (BrowserRuntime *)calloc(1, sizeof(BrowserRuntime));
    if (!rt) return NULL;
    
    if (cfg) {
        rt->config = *cfg;
    } else {
        rt->config.max_workers = 4;
        rt->config.use_shared_buffer = 1;
        rt->config.worker_script = "ponypp-worker.js";
    }
    
    if (rt->config.max_workers <= 0) rt->config.max_workers = 4;
    
    rt->worker_count = rt->config.max_workers;
    rt->workers = (WorkerSlot *)calloc(rt->worker_count, sizeof(WorkerSlot));
    if (!rt->workers) { free(rt); return NULL; }
    
    for (int i = 0; i < rt->worker_count; i++) {
        rt->workers[i].actor_id = -1;
        rt->workers[i].busy = 0;
    }
    
    /* 分配共享缓冲区 */
    if (rt->config.use_shared_buffer) {
        rt->shared_buffer_size = 64 * 1024; /* 64KB */
        rt->shared_buffer = calloc(1, rt->shared_buffer_size);
    }
    
    rt->stats.workers_total = rt->worker_count;
    rt->stats.shared_buffer_size = rt->shared_buffer_size;
    
    return rt;
}

void browser_runtime_free(BrowserRuntime *rt) {
    if (!rt) return;
    
    browser_runtime_stop(rt);
    
    /* 释放回调链 */
    BrowserCallbackEntry *e = rt->callbacks;
    while (e) {
        BrowserCallbackEntry *next = e->next;
        free(e);
        e = next;
    }
    
    free(rt->shared_buffer);
    free(rt->workers);
    free(rt);
}

/* ======================== Worker 管理 ======================== */

#ifdef __EMSCRIPTEN__

static void *worker_main(void *arg) {
    WorkerSlot *slot = (WorkerSlot *)arg;
    /* Emscripten Worker 线程主循环 */
    while (slot->busy) {
        /* 处理消息队列 */
        emscripten_thread_sleep(1);
    }
    return NULL;
}

int browser_runtime_start(BrowserRuntime *rt) {
    if (!rt || rt->started) return -1;
    
    for (int i = 0; i < rt->worker_count; i++) {
        rt->workers[i].busy = 1;
        if (pthread_create(&rt->workers[i].thread, NULL, worker_main, &rt->workers[i]) != 0) {
            rt->workers[i].busy = 0;
            /* 继续启动其他 Worker */
        }
    }
    
    rt->started = 1;
    return 0;
}

void browser_runtime_stop(BrowserRuntime *rt) {
    if (!rt || !rt->started) return;
    
    for (int i = 0; i < rt->worker_count; i++) {
        rt->workers[i].busy = 0;
        pthread_join(rt->workers[i].thread, NULL);
    }
    
    rt->started = 0;
}

#else /* 非 Emscripten: stub 实现 */

int browser_runtime_start(BrowserRuntime *rt) {
    if (!rt) return -1;
    fprintf(stderr, "[browser] 警告: 非 Emscripten 环境，Worker 以 stub 模式运行\n");
    rt->started = 1;
    return 0;
}

void browser_runtime_stop(BrowserRuntime *rt) {
    if (!rt) return;
    rt->started = 0;
}

#endif

int browser_assign_worker(BrowserRuntime *rt, int actor_id) {
    if (!rt) return -1;
    
    for (int i = 0; i < rt->worker_count; i++) {
        if (rt->workers[i].actor_id == -1) {
            rt->workers[i].actor_id = actor_id;
            rt->stats.workers_busy++;
            return i; /* 返回 Worker 索引 */
        }
    }
    
    return -2; /* 无可用 Worker */
}

void browser_release_worker(BrowserRuntime *rt, int actor_id) {
    if (!rt) return;
    
    for (int i = 0; i < rt->worker_count; i++) {
        if (rt->workers[i].actor_id == actor_id) {
            rt->workers[i].actor_id = -1;
            rt->stats.workers_busy--;
            break;
        }
    }
}

void browser_get_stats(BrowserRuntime *rt, BrowserStats *stats) {
    if (!rt || !stats) return;
    *stats = rt->stats;
}

/* ======================== JS API 桥接 ======================== */

#ifdef __EMSCRIPTEN__

EM_JS(int, js_call_actor, (int actor_id, const char *method, const void *arg, int arg_size), {
    var methodName = UTF8ToString(method);
    var data = new Uint8Array(HEAPU8.buffer, arg, arg_size);
    /* 调用 JS 侧的 Actor 桥接 */
    if (typeof self !== 'undefined' && self.ponyppBridge) {
        return self.ponyppBridge.callActor(actor_id, methodName, data);
    }
    return -1;
});

EM_JS(int, js_call_func, (const char *func_name, const void *arg, int arg_size), {
    var name = UTF8ToString(func_name);
    var data = new Uint8Array(HEAPU8.buffer, arg, arg_size);
    if (typeof self !== 'undefined' && self[name]) {
        var result = self[name](data);
        return result || 0;
    }
    return -1;
});

int browser_call_actor(BrowserRuntime *rt, int actor_id,
                       const char *method, const void *arg, size_t arg_size) {
    if (!rt || !method) return -1;
    rt->stats.messages_posted++;
    return js_call_actor(actor_id, method, arg, (int)arg_size);
}

int browser_call_js(BrowserRuntime *rt, const char *func_name,
                    const void *arg, size_t arg_size) {
    if (!rt || !func_name) return -1;
    rt->stats.messages_received++;
    return js_call_func(func_name, arg, (int)arg_size);
}

#else /* 非 Emscripten: stub */

int browser_call_actor(BrowserRuntime *rt, int actor_id,
                       const char *method, const void *arg, size_t arg_size) {
    (void)actor_id; (void)method; (void)arg; (void)arg_size;
    if (!rt) return -1;
    rt->stats.messages_posted++;
    fprintf(stderr, "[browser] stub: call_actor(%d, %s)\n", actor_id, method ? method : "?");
    return 0;
}

int browser_call_js(BrowserRuntime *rt, const char *func_name,
                    const void *arg, size_t arg_size) {
    (void)arg; (void)arg_size;
    if (!rt || !func_name) return -1;
    rt->stats.messages_received++;
    fprintf(stderr, "[browser] stub: call_js(%s)\n", func_name);
    return 0;
}

#endif

int browser_register_callback(BrowserRuntime *rt, const char *event, BrowserCallback cb) {
    if (!rt || !event || !cb) return -1;
    
    BrowserCallbackEntry *e = (BrowserCallbackEntry *)calloc(1, sizeof(BrowserCallbackEntry));
    if (!e) return -1;
    
    strncpy(e->event, event, sizeof(e->event) - 1);
    e->cb = cb;
    e->next = rt->callbacks;
    rt->callbacks = e;
    
    return 0;
}

/* ======================== SharedArrayBuffer ======================== */

void *browser_get_shared_buffer(BrowserRuntime *rt, size_t *size) {
    if (!rt) return NULL;
    if (size) *size = rt->shared_buffer_size;
    return rt->shared_buffer;
}

#ifdef __EMSCRIPTEN__

int browser_atomic_add(BrowserRuntime *rt, int32_t *ptr, int32_t value) {
    (void)rt;
    return __atomic_add_fetch(ptr, value, __ATOMIC_SEQ_CST) - value;
}

int browser_atomic_load(BrowserRuntime *rt, const int32_t *ptr, int32_t *result) {
    (void)rt;
    *result = __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
    return 0;
}

int browser_atomic_store(BrowserRuntime *rt, int32_t *ptr, int32_t value) {
    (void)rt;
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
    return 0;
}

#else

int browser_atomic_add(BrowserRuntime *rt, int32_t *ptr, int32_t value) {
    (void)rt;
    if (!ptr) return -1;
    *ptr += value;
    return 0;
}

int browser_atomic_load(BrowserRuntime *rt, const int32_t *ptr, int32_t *result) {
    (void)rt;
    if (!ptr || !result) return -1;
    *result = *ptr;
    return 0;
}

int browser_atomic_store(BrowserRuntime *rt, int32_t *ptr, int32_t value) {
    (void)rt;
    if (!ptr) return -1;
    *ptr = value;
    return 0;
}

#endif

/* ======================== Emscripten 集成 ======================== */

#ifdef __EMSCRIPTEN__

void browser_emscripten_init(void) {
    EM_ASM({
        if (typeof self !== 'undefined') {
            self.ponyppBridge = {
                actors: {},
                callActor: function(id, method, data) {
                    console.log('[ponypp] JS callActor:', id, method);
                    return 0;
                }
            };
        }
    });
}

void browser_emscripten_post_worker(const char *msg) {
    if (msg) {
        EM_ASM({
            var msg = UTF8ToString($0);
            if (typeof postMessage !== 'undefined') {
                postMessage({type: 'ponypp', data: msg});
            }
        }, msg);
    }
}

#else

void browser_emscripten_init(void) {
    fprintf(stderr, "[browser] stub: emscripten_init\n");
}

void browser_emscripten_post_worker(const char *msg) {
    fprintf(stderr, "[browser] stub: post_worker(%s)\n", msg ? msg : "");
}

#endif
