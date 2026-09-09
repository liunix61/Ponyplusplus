/*
 * gtest_browser.cpp - 浏览器适配器单元测试
 *
 * Phase 3: browser.c 测试
 */
#include <gtest/gtest.h>
extern "C" {
#include "ponypp/browser.h"
}

/* ======================== Runtime 生命周期 ======================== */

TEST(BrowserRuntime, NewDefault) {
    BrowserRuntime *rt = browser_runtime_new(nullptr);
    ASSERT_NE(rt, nullptr);
    browser_runtime_free(rt);
}

TEST(BrowserRuntime, NewWithConfig) {
    BrowserConfig cfg = {8, 1, "custom-worker.js"};
    BrowserRuntime *rt = browser_runtime_new(&cfg);
    ASSERT_NE(rt, nullptr);
    
    BrowserStats stats;
    browser_get_stats(rt, &stats);
    EXPECT_EQ(stats.workers_total, 8);
    EXPECT_EQ(stats.workers_busy, 0);
    EXPECT_EQ(stats.shared_buffer_size, (size_t)(64 * 1024));
    
    browser_runtime_free(rt);
}

TEST(BrowserRuntime, NewZeroWorkers) {
    BrowserConfig cfg = {0, 0, nullptr};
    BrowserRuntime *rt = browser_runtime_new(&cfg);
    ASSERT_NE(rt, nullptr);
    
    BrowserStats stats;
    browser_get_stats(rt, &stats);
    EXPECT_EQ(stats.workers_total, 4); /* 默认值 */
    
    browser_runtime_free(rt);
}

TEST(BrowserRuntime, StartStop) {
    BrowserRuntime *rt = browser_runtime_new(nullptr);
    ASSERT_NE(rt, nullptr);
    
    EXPECT_EQ(browser_runtime_start(rt), 0);
    browser_runtime_stop(rt);
    
    browser_runtime_free(rt);
}

/* ======================== Worker 管理 ======================== */

TEST(BrowserWorker, AssignAndRelease) {
    BrowserRuntime *rt = browser_runtime_new(nullptr);
    ASSERT_NE(rt, nullptr);
    
    int w0 = browser_assign_worker(rt, 100);
    int w1 = browser_assign_worker(rt, 200);
    EXPECT_GE(w0, 0);
    EXPECT_GE(w1, 0);
    EXPECT_NE(w0, w1);
    
    BrowserStats stats;
    browser_get_stats(rt, &stats);
    EXPECT_EQ(stats.workers_busy, 2);
    
    browser_release_worker(rt, 100);
    browser_get_stats(rt, &stats);
    EXPECT_EQ(stats.workers_busy, 1);
    
    browser_runtime_free(rt);
}

TEST(BrowserWorker, ExhaustPool) {
    BrowserConfig cfg = {2, 0, nullptr};
    BrowserRuntime *rt = browser_runtime_new(&cfg);
    ASSERT_NE(rt, nullptr);
    
    EXPECT_GE(browser_assign_worker(rt, 1), 0);
    EXPECT_GE(browser_assign_worker(rt, 2), 0);
    /* 第3个应失败 */
    EXPECT_EQ(browser_assign_worker(rt, 3), -2);
    
    browser_runtime_free(rt);
}

/* ======================== JS API 桥接 ======================== */

TEST(BrowserJS, CallActor) {
    BrowserRuntime *rt = browser_runtime_new(nullptr);
    ASSERT_NE(rt, nullptr);
    
    int result = browser_call_actor(rt, 1, "greet", nullptr, 0);
    EXPECT_EQ(result, 0);
    
    BrowserStats stats;
    browser_get_stats(rt, &stats);
    EXPECT_EQ(stats.messages_posted, 1);
    
    browser_runtime_free(rt);
}

TEST(BrowserJS, CallJs) {
    BrowserRuntime *rt = browser_runtime_new(nullptr);
    ASSERT_NE(rt, nullptr);
    
    int result = browser_call_js(rt, "console.log", nullptr, 0);
    EXPECT_EQ(result, 0);
    
    BrowserStats stats;
    browser_get_stats(rt, &stats);
    EXPECT_EQ(stats.messages_received, 1);
    
    browser_runtime_free(rt);
}

TEST(BrowserJS, CallNullArgs) {
    BrowserRuntime *rt = browser_runtime_new(nullptr);
    ASSERT_NE(rt, nullptr);
    
    EXPECT_EQ(browser_call_actor(rt, 1, nullptr, nullptr, 0), -1);
    EXPECT_EQ(browser_call_js(rt, nullptr, nullptr, 0), -1);
    
    browser_runtime_free(rt);
}

/* ======================== 回调注册 ======================== */

static int callback_fired = 0;
static void test_callback(const char *event, const void *data, size_t len) {
    (void)event; (void)data; (void)len;
    callback_fired++;
}

TEST(BrowserCallback, Register) {
    BrowserRuntime *rt = browser_runtime_new(nullptr);
    ASSERT_NE(rt, nullptr);
    
    EXPECT_EQ(browser_register_callback(rt, "onMessage", test_callback), 0);
    EXPECT_EQ(browser_register_callback(rt, "onError", test_callback), 0);
    /* NULL 参数 */
    EXPECT_EQ(browser_register_callback(rt, nullptr, test_callback), -1);
    EXPECT_EQ(browser_register_callback(rt, "x", nullptr), -1);
    
    browser_runtime_free(rt);
}

/* ======================== SharedArrayBuffer ======================== */

TEST(BrowserShared, GetBuffer) {
    BrowserRuntime *rt = browser_runtime_new(nullptr);
    ASSERT_NE(rt, nullptr);
    
    size_t sz = 0;
    void *buf = browser_get_shared_buffer(rt, &sz);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(sz, (size_t)(64 * 1024));
    
    browser_runtime_free(rt);
}

TEST(BrowserShared, NoBuffer) {
    BrowserConfig cfg = {4, 0, nullptr};
    BrowserRuntime *rt = browser_runtime_new(&cfg);
    ASSERT_NE(rt, nullptr);
    
    size_t sz = 0;
    void *buf = browser_get_shared_buffer(rt, &sz);
    EXPECT_EQ(buf, nullptr);
    EXPECT_EQ(sz, (size_t)0);
    
    browser_runtime_free(rt);
}

/* ======================== Atomics ======================== */

TEST(BrowserAtomics, AddLoadStore) {
    BrowserRuntime *rt = browser_runtime_new(nullptr);
    ASSERT_NE(rt, nullptr);
    
    int32_t val = 0;
    EXPECT_EQ(browser_atomic_store(rt, &val, 42), 0);
    
    int32_t loaded = 0;
    EXPECT_EQ(browser_atomic_load(rt, &val, &loaded), 0);
    EXPECT_EQ(loaded, 42);
    
    browser_atomic_add(rt, &val, 8);
    EXPECT_EQ(val, 50);
    
    browser_runtime_free(rt);
}

TEST(BrowserAtomics, NullPtr) {
    BrowserRuntime *rt = browser_runtime_new(nullptr);
    ASSERT_NE(rt, nullptr);
    
    EXPECT_EQ(browser_atomic_add(rt, nullptr, 1), -1);
    EXPECT_EQ(browser_atomic_store(rt, nullptr, 1), -1);
    
    browser_runtime_free(rt);
}

/* ======================== NULL 安全 ======================== */

TEST(BrowserNull, AllNullSafe) {
    EXPECT_EQ(browser_runtime_start(nullptr), -1);
    browser_runtime_stop(nullptr);
    browser_runtime_free(nullptr);
    browser_release_worker(nullptr, 0);
    
    BrowserStats stats;
    browser_get_stats(nullptr, &stats); /* 不应崩溃 */
    
    EXPECT_EQ(browser_assign_worker(nullptr, 0), -1);
    EXPECT_EQ(browser_call_actor(nullptr, 0, "x", nullptr, 0), -1);
    EXPECT_EQ(browser_call_js(nullptr, "x", nullptr, 0), -1);
    EXPECT_EQ(browser_register_callback(nullptr, "x", test_callback), -1);
    EXPECT_EQ(browser_get_shared_buffer(nullptr, nullptr), nullptr);
}

/* ======================== Emscripten stub ======================== */

TEST(BrowserEmscripten, StubCalls) {
    /* 非 Emscripten 环境下应正常执行 (stub) */
    browser_emscripten_init();
    browser_emscripten_post_worker("test");
    browser_emscripten_post_worker(nullptr);
}
