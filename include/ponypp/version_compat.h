/*
 * version_compat.h - Pony++ 版本兼容性模块
 *
 * Phase 5: 版本兼容 — SemVer 解析、兼容性检查、迁移提示
 *
 * 语义化版本: MAJOR.MINOR.PATCH[-prerelease][+build]
 */
#ifndef PONYPP_VERSION_COMPAT_H
#define PONYPP_VERSION_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

/* ======================== 版本号结构 ======================== */

typedef struct SemVer {
    int major;
    int minor;
    int patch;
    char prerelease[64];   /* 如 "alpha.1", "rc.2" — 空串表示正式版 */
    char build[64];        /* 构建元数据 — 不参与比较 */
} SemVer;

/* ======================== 兼容性级别 ======================== */

typedef enum CompatLevel {
    COMPAT_EXACT = 0,       /* 完全相同 */
    COMPAT_PATCH,           /* 仅 PATCH 不同 */
    COMPAT_MINOR,           /* MINOR 不同 (向后兼容) */
    COMPAT_MAJOR,           /* MAJOR 不同 (不兼容) */
    COMPAT_PRERELEASE,      /* 预发布版本差异 */
    COMPAT_INCOMPATIBLE     /* 完全不兼容 */
} CompatLevel;

/* ======================== API 版本信息 ======================== */

/* 当前 Pony++ 运行时版本 */
#define PONYPP_VERSION_MAJOR 0
#define PONYPP_VERSION_MINOR 5
#define PONYPP_VERSION_PATCH 0
#define PONYPP_VERSION_STRING "0.5.0"

/* ======================== SemVer 解析 ======================== */

/* 解析版本字符串 (如 "1.2.3-alpha.1+build.42") */
int semver_parse(const char *str, SemVer *out);

/* 格式化版本号为字符串 */
int semver_format(const SemVer *v, char *buf, size_t buf_size);

/* 比较两个版本号: <0 = a<b, 0 = a==b, >0 = a>b */
int semver_compare(const SemVer *a, const SemVer *b);

/* 检查是否为预发布版本 */
bool semver_is_prerelease(const SemVer *v);

/* ======================== 兼容性检查 ======================== */

/* 检查 required 版本是否与 provided 版本兼容 */
CompatLevel semver_check_compat(const SemVer *required, const SemVer *provided);

/* 检查版本字符串兼容性 */
CompatLevel semver_check_compat_str(const char *required, const char *provided);

/* 获取兼容性级别描述 */
const char *compat_level_name(CompatLevel level);

/* ======================== API 兼容性 ======================== */

/* API 版本记录 */
typedef struct ApiVersion {
    int api_major;       /* API 主版本 */
    int api_minor;       /* API 次版本 */
    const char *name;    /* API 名称 (如 "runtime", "compiler") */
} ApiVersion;

/* 注册 API 版本 */
int api_version_register(const char *name, int major, int minor);

/* 查询 API 版本 */
int api_version_query(const char *name, int *major, int *minor);

/* 检查 API 兼容性 */
bool api_version_check(const char *name, int required_major, int required_minor);

/* 获取当前运行时版本字符串 */
const char *ponypp_version_string(void);

/* ======================== 迁移提示 ======================== */

/* 迁移建议 */
typedef struct MigrationHint {
    int from_major, from_minor;
    int to_major, to_minor;
    const char *description;  /* 迁移说明 */
    const char *breaking;     /* 破坏性变更描述, NULL = 无 */
} MigrationHint;

/* 获取迁移建议 */
const MigrationHint *migration_get_hints(int from_major, int from_minor,
                                          int to_major, int to_minor,
                                          int *count);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_VERSION_COMPAT_H */
