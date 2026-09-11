#include <gtest/gtest.h>
#include <ponypp/incremental.h>
#include <ponypp/crypto.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

class IncrementalTest : public ::testing::Test {
protected:
    char cache_dir[256];
    char test_file[256];
    
    void SetUp() override {
        /* 创建临时目录 */
        snprintf(cache_dir, sizeof(cache_dir), "/tmp/ponypp-test-cache-%d", getpid());
        snprintf(test_file, sizeof(test_file), "/tmp/ponypp-test-src-%d.pp", getpid());
        
        /* 创建测试源文件 */
        FILE *f = fopen(test_file, "w");
        if (f) {
            fprintf(f, "actor Main\n  new create(env: Env) =>\n    env.out.print(\"hello\")\n");
            fclose(f);
        }
    }
    
    void TearDown() override {
        /* 清理 */
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf %s %s", cache_dir, test_file);
        system(cmd);
    }
};

/* ==================== 基本操作 ==================== */

TEST_F(IncrementalTest, OpenClose) {
    IncrementalCache *cache = pny_incr_open(cache_dir);
    ASSERT_NE(cache, nullptr);
    pny_incr_close(cache);
}

TEST_F(IncrementalTest, OpenNullDir) {
    EXPECT_EQ(pny_incr_open(nullptr), nullptr);
}

TEST_F(IncrementalTest, CloseNull) {
    pny_incr_close(nullptr);  /* 不应崩溃 */
}

/* ==================== 缓存查询 ==================== */

TEST_F(IncrementalTest, IsNotCachedInitially) {
    IncrementalCache *cache = pny_incr_open(cache_dir);
    ASSERT_NE(cache, nullptr);
    
    EXPECT_FALSE(pny_incr_is_cached(cache, test_file));
    
    pny_incr_close(cache);
}

TEST_F(IncrementalTest, StoreAndRetrieve) {
    IncrementalCache *cache = pny_incr_open(cache_dir);
    ASSERT_NE(cache, nullptr);
    
    const char *output = "/tmp/test-output.o";
    ASSERT_EQ(pny_incr_store(cache, test_file, output, nullptr, 0), 0);
    
    EXPECT_TRUE(pny_incr_is_cached(cache, test_file));
    EXPECT_STREQ(pny_incr_get_output(cache, test_file), output);
    
    pny_incr_close(cache);
}

TEST_F(IncrementalTest, StoreNullArgs) {
    IncrementalCache *cache = pny_incr_open(cache_dir);
    ASSERT_NE(cache, nullptr);
    
    EXPECT_EQ(pny_incr_store(cache, nullptr, "out.o", nullptr, 0), -1);
    EXPECT_EQ(pny_incr_store(cache, "src.pp", nullptr, nullptr, 0), -1);
    
    pny_incr_close(cache);
}

/* ==================== 失效 ==================== */

TEST_F(IncrementalTest, Invalidate) {
    IncrementalCache *cache = pny_incr_open(cache_dir);
    ASSERT_NE(cache, nullptr);
    
    ASSERT_EQ(pny_incr_store(cache, test_file, "/tmp/out.o", nullptr, 0), 0);
    EXPECT_TRUE(pny_incr_is_cached(cache, test_file));
    
    ASSERT_EQ(pny_incr_invalidate(cache, test_file), 0);
    EXPECT_FALSE(pny_incr_is_cached(cache, test_file));
    
    pny_incr_close(cache);
}

TEST_F(IncrementalTest, InvalidateNonexistent) {
    IncrementalCache *cache = pny_incr_open(cache_dir);
    ASSERT_NE(cache, nullptr);
    
    EXPECT_EQ(pny_incr_invalidate(cache, "/nonexistent/file.pp"), -1);
    
    pny_incr_close(cache);
}

/* ==================== 依赖追踪 ==================== */

TEST_F(IncrementalTest, StoreWithDeps) {
    IncrementalCache *cache = pny_incr_open(cache_dir);
    ASSERT_NE(cache, nullptr);
    
    const char *deps[] = {"dep1.pony", "dep2.pony"};
    ASSERT_EQ(pny_incr_store(cache, test_file, "/tmp/out.o", deps, 2), 0);
    
    const char *retrieved[4];
    size_t count = pny_incr_get_deps(cache, test_file, retrieved, 4);
    EXPECT_EQ(count, 2);
    EXPECT_STREQ(retrieved[0], "dep1.pony");
    EXPECT_STREQ(retrieved[1], "dep2.pony");
    
    pny_incr_close(cache);
}

TEST_F(IncrementalTest, AnalyzeDeps) {
    /* 创建带依赖的源文件 */
    char dep_file[256];
    snprintf(dep_file, sizeof(dep_file), "/tmp/ponypp-test-dep-%d.pp", getpid());
    
    FILE *f = fopen(dep_file, "w");
    if (f) {
        fprintf(f, "use \"std/io\"\nimport \"std/collections\"\nactor Main\n");
        fclose(f);
    }
    
    const char *deps[8];
    size_t count = pny_incr_analyze_deps(dep_file, deps, 8);
    EXPECT_EQ(count, 2);
    
    /* 清理 */
    for (size_t i = 0; i < count; i++) {
        free((void *)deps[i]);
    }
    remove(dep_file);
}

/* ==================== 持久化 ==================== */

TEST_F(IncrementalTest, PersistAcrossOpen) {
    /* 第一次: 存储 */
    {
        IncrementalCache *cache = pny_incr_open(cache_dir);
        ASSERT_NE(cache, nullptr);
        ASSERT_EQ(pny_incr_store(cache, test_file, "/tmp/out.o", nullptr, 0), 0);
        pny_incr_close(cache);
    }
    
    /* 第二次: 重新打开应能看到缓存 */
    {
        IncrementalCache *cache = pny_incr_open(cache_dir);
        ASSERT_NE(cache, nullptr);
        EXPECT_TRUE(pny_incr_is_cached(cache, test_file));
        pny_incr_close(cache);
    }
}

TEST_F(IncrementalTest, ClearCache) {
    IncrementalCache *cache = pny_incr_open(cache_dir);
    ASSERT_NE(cache, nullptr);
    
    ASSERT_EQ(pny_incr_store(cache, test_file, "/tmp/out.o", nullptr, 0), 0);
    EXPECT_TRUE(pny_incr_is_cached(cache, test_file));
    
    ASSERT_EQ(pny_incr_clear(cache), 0);
    EXPECT_FALSE(pny_incr_is_cached(cache, test_file));
    
    pny_incr_close(cache);
}

/* ==================== 统计 ==================== */

TEST_F(IncrementalTest, Stats) {
    IncrementalCache *cache = pny_incr_open(cache_dir);
    ASSERT_NE(cache, nullptr);
    
    /* 存储 + 查询 */
    ASSERT_EQ(pny_incr_store(cache, test_file, "/tmp/out.o", nullptr, 0), 0);
    pny_incr_is_cached(cache, test_file);  /* hit */
    pny_incr_is_cached(cache, "/nonexistent");  /* miss */
    
    PnyIncrStats stats;
    ASSERT_EQ(pny_incr_get_stats(cache, &stats), 0);
    EXPECT_GE(stats.hits, 1);
    EXPECT_GE(stats.misses, 1);
    
    pny_incr_close(cache);
}

TEST_F(IncrementalTest, StatsNullArgs) {
    IncrementalCache *cache = pny_incr_open(cache_dir);
    ASSERT_NE(cache, nullptr);
    
    PnyIncrStats stats;
    EXPECT_EQ(pny_incr_get_stats(cache, nullptr), -1);
    EXPECT_EQ(pny_incr_get_stats(nullptr, &stats), -1);
    
    pny_incr_close(cache);
}

/* ==================== 源文件变化检测 ==================== */

TEST_F(IncrementalTest, DetectSourceChange) {
    IncrementalCache *cache = pny_incr_open(cache_dir);
    ASSERT_NE(cache, nullptr);
    
    /* 存储 */
    ASSERT_EQ(pny_incr_store(cache, test_file, "/tmp/out.o", nullptr, 0), 0);
    EXPECT_TRUE(pny_incr_is_cached(cache, test_file));
    
    /* 修改源文件 */
    FILE *f = fopen(test_file, "a");
    if (f) {
        fprintf(f, "// modified\n");
        fclose(f);
    }
    
    /* 应检测到变化 */
    EXPECT_FALSE(pny_incr_is_cached(cache, test_file));
    
    pny_incr_close(cache);
}
