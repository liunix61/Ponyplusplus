/*
 * version_compat.c - Pony++ 版本兼容性实现
 *
 * Phase 5: 版本兼容
 */

#include "ponypp/version_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ======================== SemVer 解析 ======================== */

int semver_parse(const char *str, SemVer *out) {
    if (!str || !out) return -1;
    
    memset(out, 0, sizeof(SemVer));
    
    const char *p = str;
    
    /* 解析 MAJOR */
    if (!isdigit((unsigned char)*p)) return -1;
    out->major = 0;
    while (isdigit((unsigned char)*p)) {
        out->major = out->major * 10 + (*p - '0');
        p++;
    }
    
    /* 期望 '.' */
    if (*p != '.') return -1;
    p++;
    
    /* 解析 MINOR */
    if (!isdigit((unsigned char)*p)) return -1;
    out->minor = 0;
    while (isdigit((unsigned char)*p)) {
        out->minor = out->minor * 10 + (*p - '0');
        p++;
    }
    
    /* 期望 '.' */
    if (*p != '.') return -1;
    p++;
    
    /* 解析 PATCH */
    if (!isdigit((unsigned char)*p)) return -1;
    out->patch = 0;
    while (isdigit((unsigned char)*p)) {
        out->patch = out->patch * 10 + (*p - '0');
        p++;
    }
    
    /* 解析预发布标识 (可选) */
    if (*p == '-') {
        p++;
        size_t i = 0;
        while (*p && *p != '+' && i < sizeof(out->prerelease) - 1) {
            out->prerelease[i++] = *p++;
        }
        out->prerelease[i] = '\0';
    }
    
    /* 解析构建元数据 (可选) */
    if (*p == '+') {
        p++;
        size_t i = 0;
        while (*p && i < sizeof(out->build) - 1) {
            out->build[i++] = *p++;
        }
        out->build[i] = '\0';
    }
    
    return 0;
}

int semver_format(const SemVer *v, char *buf, size_t buf_size) {
    if (!v || !buf || buf_size < 8) return -1;
    
    int len;
    if (v->prerelease[0]) {
        len = snprintf(buf, buf_size, "%d.%d.%d-%s", 
                       v->major, v->minor, v->patch, v->prerelease);
    } else {
        len = snprintf(buf, buf_size, "%d.%d.%d", 
                       v->major, v->minor, v->patch);
    }
    
    if (v->build[0] && (size_t)len < buf_size - 1) {
        len += snprintf(buf + len, buf_size - len, "+%s", v->build);
    }
    
    return len;
}

int semver_compare(const SemVer *a, const SemVer *b) {
    if (!a || !b) return 0;
    
    /* 比较 MAJOR.MINOR.PATCH */
    if (a->major != b->major) return a->major - b->major;
    if (a->minor != b->minor) return a->minor - b->minor;
    if (a->patch != b->patch) return a->patch - b->patch;
    
    /* 预发布版本比较 */
    bool a_pre = semver_is_prerelease(a);
    bool b_pre = semver_is_prerelease(b);
    
    if (!a_pre && !b_pre) return 0;  /* 都是正式版, 相等 */
    if (a_pre && !b_pre) return -1;  /* 预发布 < 正式版 */
    if (!a_pre && b_pre) return 1;
    
    /* 都是预发布: 字符串比较 */
    return strcmp(a->prerelease, b->prerelease);
}

bool semver_is_prerelease(const SemVer *v) {
    return v && v->prerelease[0] != '\0';
}

/* ======================== 兼容性检查 ======================== */

CompatLevel semver_check_compat(const SemVer *required, const SemVer *provided) {
    if (!required || !provided) return COMPAT_INCOMPATIBLE;
    
    /* 完全相同 */
    if (semver_compare(required, provided) == 0) return COMPAT_EXACT;
    
    /* MAJOR 不同 = 不兼容 */
    if (required->major != provided->major) return COMPAT_INCOMPATIBLE;
    
    /* MINOR 不同 = 向后兼容 (SemVer 规范) */
    if (required->minor != provided->minor) return COMPAT_MINOR;
    
    /* 仅 PATCH 不同 */
    if (required->patch != provided->patch) return COMPAT_PATCH;
    
    /* 预发布差异 */
    if (semver_is_prerelease(required) || semver_is_prerelease(provided)) {
        return COMPAT_PRERELEASE;
    }
    
    return COMPAT_EXACT;
}

CompatLevel semver_check_compat_str(const char *required, const char *provided) {
    SemVer req, prov;
    if (semver_parse(required, &req) != 0) return COMPAT_INCOMPATIBLE;
    if (semver_parse(provided, &prov) != 0) return COMPAT_INCOMPATIBLE;
    return semver_check_compat(&req, &prov);
}

const char *compat_level_name(CompatLevel level) {
    switch (level) {
        case COMPAT_EXACT:        return "exact";
        case COMPAT_PATCH:        return "patch-compatible";
        case COMPAT_MINOR:        return "minor-compatible";
        case COMPAT_MAJOR:        return "major-diff";
        case COMPAT_PRERELEASE:   return "prerelease-diff";
        case COMPAT_INCOMPATIBLE: return "incompatible";
        default:                  return "unknown";
    }
}

/* ======================== API 兼容性 ======================== */

#define MAX_API_VERSIONS 16

static ApiVersion api_versions[MAX_API_VERSIONS];
static int api_version_count = 0;

int api_version_register(const char *name, int major, int minor) {
    if (!name || api_version_count >= MAX_API_VERSIONS) return -1;
    
    /* 检查是否已注册 */
    for (int i = 0; i < api_version_count; i++) {
        if (strcmp(api_versions[i].name, name) == 0) {
            api_versions[i].api_major = major;
            api_versions[i].api_minor = minor;
            return 0;
        }
    }
    
    api_versions[api_version_count].name = name;
    api_versions[api_version_count].api_major = major;
    api_versions[api_version_count].api_minor = minor;
    api_version_count++;
    
    return 0;
}

int api_version_query(const char *name, int *major, int *minor) {
    if (!name) return -1;
    
    for (int i = 0; i < api_version_count; i++) {
        if (strcmp(api_versions[i].name, name) == 0) {
            if (major) *major = api_versions[i].api_major;
            if (minor) *minor = api_versions[i].api_minor;
            return 0;
        }
    }
    
    return -1;  /* 未找到 */
}

bool api_version_check(const char *name, int required_major, int required_minor) {
    int major = 0, minor = 0;
    if (api_version_query(name, &major, &minor) != 0) return false;
    
    /* API 版本检查: 主版本必须相同, 次版本必须 >= 要求 */
    if (major != required_major) return false;
    return minor >= required_minor;
}

const char *ponypp_version_string(void) {
    return PONYPP_VERSION_STRING;
}

/* ======================== 迁移提示 ======================== */

static const MigrationHint migration_hints[] = {
    {0, 1, 0, 2, 
     "Channel/Future 需要 import std.concurrent", 
     "Channel API 从 std.channel 移至 std.concurrent"},
    {0, 2, 0, 3,
     "新增 M:N 调度器, 启用 pny_mn_start/pny_mn_stop",
     NULL},
    {0, 3, 0, 4,
     "新增 WASI P2 支持, fd_read/clock_time_get/random_get 可用",
     NULL},
    {0, 4, 0, 5,
     "新增 AOT 优化分级(-O0~O4)和浏览器适配器",
     NULL},
};

static const int migration_hint_count = sizeof(migration_hints) / sizeof(migration_hints[0]);

const MigrationHint *migration_get_hints(int from_major, int from_minor,
                                          int to_major, int to_minor,
                                          int *count) {
    if (count) *count = 0;
    
    /* 简单实现: 返回 from->to 范围内的所有提示 */
    static MigrationHint result[16];
    int n = 0;
    
    for (int i = 0; i < migration_hint_count && n < 16; i++) {
        const MigrationHint *h = &migration_hints[i];
        /* 检查是否在 from->to 范围内 */
        if (h->from_major > from_major || 
            (h->from_major == from_major && h->from_minor >= from_minor)) {
            if (h->to_major < to_major ||
                (h->to_major == to_major && h->to_minor <= to_minor)) {
                result[n++] = *h;
            }
        }
    }
    
    if (count) *count = n;
    return n > 0 ? result : NULL;
}
