/**
 * pony++ 增量编译缓存
 * 
 * 基于源文件哈希的增量编译：
 * - 缓存编译结果 (AST + 代码)
 * - 源文件未变化时跳过重编译
 * - 缓存存储在 .ponypp-cache/ 目录
 */
#ifndef PONYPP_INCREMENTAL_H
#define PONYPP_INCREMENTAL_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 缓存管理 ==================== */

typedef struct IncrementalCache IncrementalCache;

/* 打开/创建缓存 */
IncrementalCache *pny_incr_open(const char *cache_dir);

/* 关闭缓存 */
void pny_incr_close(IncrementalCache *cache);

/* 清空缓存 */
int pny_incr_clear(IncrementalCache *cache);

/* ==================== 缓存查询 ==================== */

/* 检查源文件是否有缓存的编译结果
 * 返回: true=有缓存且源文件未变化, false=需要重编译 */
bool pny_incr_is_cached(IncrementalCache *cache, const char *source_path);

/* 获取缓存的输出文件路径 (如 .o/.wasm)
 * 返回: 缓存的输出路径, 或 NULL */
const char *pny_incr_get_output(IncrementalCache *cache, const char *source_path);

/* 获取缓存的依赖列表
 * 返回: 依赖文件数量 */
size_t pny_incr_get_deps(IncrementalCache *cache, const char *source_path,
                          const char **deps, size_t max_deps);

/* ==================== 缓存更新 ==================== */

/* 记录编译结果到缓存 */
int pny_incr_store(IncrementalCache *cache,
                    const char *source_path,
                    const char *output_path,
                    const char **deps, size_t dep_count);

/* 标记缓存失效 */
int pny_incr_invalidate(IncrementalCache *cache, const char *source_path);

/* ==================== 依赖追踪 ==================== */

/* 分析源文件的依赖 (import/include) */
size_t pny_incr_analyze_deps(const char *source_path,
                              const char **deps, size_t max_deps);

/* ==================== 统计 ==================== */

typedef struct {
    size_t total_files;      /* 总文件数 */
    size_t cached_files;     /* 有缓存的文件数 */
    size_t hits;             /* 缓存命中次数 */
    size_t misses;           /* 缓存未命中次数 */
    size_t invalidations;    /* 缓存失效次数 */
} PnyIncrStats;

/* 获取统计信息 */
int pny_incr_get_stats(IncrementalCache *cache, PnyIncrStats *stats);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_INCREMENTAL_H */
