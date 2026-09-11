/**
 * pony++ 增量编译缓存实现
 */
#include "ponypp/incremental.h"
#include "ponypp/crypto.h"
#include "ponypp/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define INCR_MAX_PATH 1024
#define INCR_MAX_DEPS 64
#define INCR_HASH_LEN 32  /* SHA-256 hex = 64 chars, use first 32 */

/* ==================== 内部结构 ==================== */

typedef struct CacheEntry {
    char source_path[INCR_MAX_PATH];
    char output_path[INCR_MAX_PATH];
    char source_hash[INCR_HASH_LEN * 2 + 1];  /* hex string */
    char deps[INCR_MAX_DEPS][INCR_MAX_PATH];
    size_t dep_count;
    time_t mtime;
    struct CacheEntry *next;
} CacheEntry;

struct IncrementalCache {
    char cache_dir[INCR_MAX_PATH];
    char index_path[INCR_MAX_PATH];
    CacheEntry *entries;
    size_t entry_count;
    PnyIncrStats stats;
};

/* ==================== 辅助函数 ==================== */

/* 计算文件 SHA-256 */
static bool file_hash(const char *path, char *out_hex, size_t out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    
    PnySHA256Ctx *ctx = pny_sha256_new();
    if (!ctx) { fclose(f); return false; }
    
    uint8_t buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        pny_sha256_update(ctx, buf, n);
    }
    fclose(f);
    
    uint8_t digest[PNY_SHA256_DIGEST_LEN];
    pny_sha256_final(ctx, digest);
    pny_sha256_free(ctx);
    
    /* 转 hex */
    for (size_t i = 0; i < PNY_SHA256_DIGEST_LEN && (i * 2 + 2) < out_size; i++) {
        snprintf(out_hex + i * 2, 3, "%02x", digest[i]);
    }
    return true;
}

/* 获取文件修改时间 */
static time_t file_mtime(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return st.st_mtime;
}

/* 确保目录存在 */
static bool ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) return S_ISDIR(st.st_mode);
    return mkdir(path, 0755) == 0 || errno == EEXIST;
}

/* ==================== 缓存索引持久化 ==================== */

/* 保存索引到文件 */
static void save_index(IncrementalCache *cache) {
    FILE *f = fopen(cache->index_path, "w");
    if (!f) return;
    
    fprintf(f, "# ponypp-incremental-cache v1\n");
    
    for (CacheEntry *e = cache->entries; e; e = e->next) {
        fprintf(f, "%s|%s|%s|%zu\n", 
                e->source_path, e->output_path, e->source_hash, e->dep_count);
        for (size_t i = 0; i < e->dep_count; i++) {
            fprintf(f, "  dep:%s\n", e->deps[i]);
        }
    }
    
    fclose(f);
}

/* 从文件加载索引 */
static void load_index(IncrementalCache *cache) {
    FILE *f = fopen(cache->index_path, "r");
    if (!f) return;
    
    char line[INCR_MAX_PATH * 2];
    CacheEntry *tail = NULL;
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        
        if (strncmp(line, "  dep:", 6) == 0) {
            /* 依赖行 */
            if (tail && tail->dep_count < INCR_MAX_DEPS) {
                char *dep = line + 6;
                dep[strcspn(dep, "\n")] = 0;
                strncpy(tail->deps[tail->dep_count], dep, INCR_MAX_PATH - 1);
                tail->dep_count++;
            }
        } else {
            /* 主条目行 */
            CacheEntry *e = calloc(1, sizeof(CacheEntry));
            if (!e) continue;
            
            sscanf(line, "%[^|]|%[^|]|%[^|]|%zu",
                   e->source_path, e->output_path, e->source_hash, &e->dep_count);
            
            if (!cache->entries) {
                cache->entries = e;
            } else {
                tail->next = e;
            }
            tail = e;
            cache->entry_count++;
        }
    }
    
    fclose(f);
}

/* ==================== 公开 API ==================== */

IncrementalCache *pny_incr_open(const char *cache_dir) {
    if (!cache_dir) return NULL;
    
    IncrementalCache *cache = calloc(1, sizeof(IncrementalCache));
    if (!cache) return NULL;
    
    strncpy(cache->cache_dir, cache_dir, INCR_MAX_PATH - 1);
    snprintf(cache->index_path, INCR_MAX_PATH, "%s/index.txt", cache_dir);
    
    if (!ensure_dir(cache_dir)) {
        free(cache);
        return NULL;
    }
    
    load_index(cache);
    return cache;
}

void pny_incr_close(IncrementalCache *cache) {
    if (!cache) return;
    
    save_index(cache);
    
    CacheEntry *e = cache->entries;
    while (e) {
        CacheEntry *next = e->next;
        free(e);
        e = next;
    }
    
    free(cache);
}

int pny_incr_clear(IncrementalCache *cache) {
    if (!cache) return -1;
    
    CacheEntry *e = cache->entries;
    while (e) {
        CacheEntry *next = e->next;
        free(e);
        e = next;
    }
    
    cache->entries = NULL;
    cache->entry_count = 0;
    memset(&cache->stats, 0, sizeof(cache->stats));
    
    /* 删除索引文件 */
    remove(cache->index_path);
    
    return 0;
}

bool pny_incr_is_cached(IncrementalCache *cache, const char *source_path) {
    if (!cache || !source_path) return false;
    
    /* 计算当前源文件哈希 */
    char current_hash[INCR_HASH_LEN * 2 + 1];
    if (!file_hash(source_path, current_hash, sizeof(current_hash))) {
        cache->stats.misses++;
        return false;
    }
    
    /* 查找缓存条目 */
    for (CacheEntry *e = cache->entries; e; e = e->next) {
        if (strcmp(e->source_path, source_path) == 0) {
            /* 检查哈希是否匹配 */
            if (strcmp(e->source_hash, current_hash) == 0) {
                cache->stats.hits++;
                return true;
            }
            cache->stats.misses++;
            return false;
        }
    }
    
    cache->stats.misses++;
    return false;
}

const char *pny_incr_get_output(IncrementalCache *cache, const char *source_path) {
    if (!cache || !source_path) return NULL;
    
    for (CacheEntry *e = cache->entries; e; e = e->next) {
        if (strcmp(e->source_path, source_path) == 0) {
            return e->output_path;
        }
    }
    
    return NULL;
}

size_t pny_incr_get_deps(IncrementalCache *cache, const char *source_path,
                          const char **deps, size_t max_deps) {
    if (!cache || !source_path || !deps) return 0;
    
    for (CacheEntry *e = cache->entries; e; e = e->next) {
        if (strcmp(e->source_path, source_path) == 0) {
            size_t count = e->dep_count < max_deps ? e->dep_count : max_deps;
            for (size_t i = 0; i < count; i++) {
                deps[i] = e->deps[i];
            }
            return count;
        }
    }
    
    return 0;
}

int pny_incr_store(IncrementalCache *cache,
                    const char *source_path,
                    const char *output_path,
                    const char **deps, size_t dep_count) {
    if (!cache || !source_path || !output_path) return -1;
    
    /* 计算源文件哈希 */
    char hash[INCR_HASH_LEN * 2 + 1];
    if (!file_hash(source_path, hash, sizeof(hash))) return -1;
    
    /* 查找现有条目 */
    CacheEntry *entry = NULL;
    for (CacheEntry *e = cache->entries; e; e = e->next) {
        if (strcmp(e->source_path, source_path) == 0) {
            entry = e;
            break;
        }
    }
    
    /* 创建新条目 */
    if (!entry) {
        entry = calloc(1, sizeof(CacheEntry));
        if (!entry) return -1;
        
        strncpy(entry->source_path, source_path, INCR_MAX_PATH - 1);
        
        /* 添加到链表 */
        if (!cache->entries) {
            cache->entries = entry;
        } else {
            CacheEntry *tail = cache->entries;
            while (tail->next) tail = tail->next;
            tail->next = entry;
        }
        
        cache->entry_count++;
        cache->stats.total_files++;
    }
    
    /* 更新条目 */
    strncpy(entry->output_path, output_path, INCR_MAX_PATH - 1);
    strncpy(entry->source_hash, hash, sizeof(entry->source_hash) - 1);
    entry->mtime = file_mtime(source_path);
    
    /* 更新依赖 */
    entry->dep_count = 0;
    if (deps && dep_count > 0) {
        size_t count = dep_count < INCR_MAX_DEPS ? dep_count : INCR_MAX_DEPS;
        for (size_t i = 0; i < count; i++) {
            strncpy(entry->deps[i], deps[i], INCR_MAX_PATH - 1);
            entry->dep_count++;
        }
    }
    
    cache->stats.cached_files++;
    
    return 0;
}

int pny_incr_invalidate(IncrementalCache *cache, const char *source_path) {
    if (!cache || !source_path) return -1;
    
    CacheEntry **prev = &cache->entries;
    CacheEntry *e = cache->entries;
    
    while (e) {
        if (strcmp(e->source_path, source_path) == 0) {
            *prev = e->next;
            free(e);
            cache->entry_count--;
            if (cache->entry_count == 0) {
                cache->stats.cached_files = 0;
            } else if (cache->stats.cached_files > 0) {
                cache->stats.cached_files--;
            }
            cache->stats.invalidations++;
            return 0;
        }
        prev = &e->next;
        e = e->next;
    }
    
    return -1;  /* 未找到 */
}

size_t pny_incr_analyze_deps(const char *source_path,
                              const char **deps, size_t max_deps) {
    if (!source_path || !deps || max_deps == 0) return 0;
    
    FILE *f = fopen(source_path, "r");
    if (!f) return 0;
    
    size_t dep_count = 0;
    char line[INCR_MAX_PATH];
    
    while (fgets(line, sizeof(line), f) && dep_count < max_deps) {
        /* 匹配 use/import/include 语句 */
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        
        if (strncmp(p, "use ", 4) == 0 || 
            strncmp(p, "import ", 7) == 0 ||
            strncmp(p, "#include ", 9) == 0) {
            
            /* 提取路径 */
            char *start = strchr(p, '"');
            if (start) {
                start++;
                char *end = strchr(start, '"');
                if (end && (size_t)(end - start) < INCR_MAX_PATH) {
                    size_t len = end - start;
                    char *dep = malloc(len + 1);
                    if (dep) {
                        memcpy(dep, start, len);
                        dep[len] = 0;
                        deps[dep_count++] = dep;
                    }
                }
            }
        }
    }
    
    fclose(f);
    return dep_count;
}

int pny_incr_get_stats(IncrementalCache *cache, PnyIncrStats *stats) {
    if (!cache || !stats) return -1;
    
    *stats = cache->stats;
    stats->total_files = cache->entry_count;
    
    return 0;
}
