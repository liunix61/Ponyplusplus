/*
 * browser.h - Pony++ 浏览器适配器
 *
 * Web Workers 调度器桥接 + JS API 绑定。
 * 将 Pony++ Actor 模型映射到浏览器 Web Workers。
 *
 * Phase 3: 浏览器适配层
 */
#ifndef PONYPP_BROWSER_H
#define PONYPP_BROWSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/* ======================== 浏览器运行时配置 ======================== */

typedef struct BrowserConfig {
    int max_workers;        /* 最大 Worker 线程数 (默认 4) */
    int use_shared_buffer;  /* 使用 SharedArrayBuffer (1) 或 postMessage (0) */
    const char *worker_script; /* Worker 脚本路径 */
} BrowserConfig;

/* ======================== 浏览器运行时状态 ======================== */

typedef struct BrowserRuntime BrowserRuntime;

/* 创建浏览器运行时 */
BrowserRuntime *browser_runtime_new(const BrowserConfig *cfg);

/* 销毁浏览器运行时 */
void browser_runtime_free(BrowserRuntime *rt);

/* 启动 Worker 池 */
int browser_runtime_start(BrowserRuntime *rt);

/* 停止 Worker 池 */
void browser_runtime_stop(BrowserRuntime *rt);

/* ======================== Worker 管理 ======================== */

/* 分配一个 Worker 给 Actor */
int browser_assign_worker(BrowserRuntime *rt, int actor_id);

/* 释放 Worker */
void browser_release_worker(BrowserRuntime *rt, int actor_id);

/* 获取 Worker 使用统计 */
typedef struct BrowserStats {
    int workers_total;
    int workers_busy;
    int messages_posted;
    int messages_received;
    size_t shared_buffer_size;
} BrowserStats;

void browser_get_stats(BrowserRuntime *rt, BrowserStats *stats);

/* ======================== JS API 桥接 ======================== */

/* 从 JS 调用 Pony++ Actor 方法 */
int browser_call_actor(BrowserRuntime *rt, int actor_id,
                       const char *method, const void *arg, size_t arg_size);

/* 从 Pony++ 调用 JS 函数 */
int browser_call_js(BrowserRuntime *rt, const char *func_name,
                    const void *arg, size_t arg_size);

/* 注册 JS 回调 */
typedef void (*BrowserCallback)(const char *event, const void *data, size_t len);
int browser_register_callback(BrowserRuntime *rt, const char *event, BrowserCallback cb);

/* ======================== SharedArrayBuffer 支持 ======================== */

/* 获取共享内存缓冲区 */
void *browser_get_shared_buffer(BrowserRuntime *rt, size_t *size);

/* 原子操作 (映射到 Atomics) */
int browser_atomic_add(BrowserRuntime *rt, int32_t *ptr, int32_t value);
int browser_atomic_load(BrowserRuntime *rt, const int32_t *ptr, int32_t *result);
int browser_atomic_store(BrowserRuntime *rt, int32_t *ptr, int32_t value);

/* ======================== Emscripten 集成 ======================== */

#ifdef __EMSCRIPTEN__
/* Emscripten 特有: 直接调用 EM_ASM */
void browser_emscripten_init(void);
void browser_emscripten_post_worker(const char *msg);
#endif

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_BROWSER_H */
