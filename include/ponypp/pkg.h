#ifndef PONYPP_PKG_H
#define PONYPP_PKG_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* 包依赖项 */
typedef struct PkgDep {
    char *name;
    char *version;
    char *source;       /* "git", "path", "registry" */
    char *path;         /* 本地路径或仓库 URL */
    bool enabled;
} PkgDep;

/* 包定义 (ponypp.toml) */
typedef struct PkgManifest {
    char *name;
    char *version;
    char *description;
    char *author;
    char *license;
    char *homepage;
    PkgDep *deps;
    size_t dep_count;
    size_t dep_cap;
} PkgManifest;

/* 包管理器 */
typedef struct PkgManager {
    PkgManifest **packages;
    size_t package_count;
    size_t package_cap;
    char workspace[512];
} PkgManager;

/* 解析 ponypp.toml */
PkgManifest *pkg_parse_toml(const char *path);
PkgManifest *pkg_parse_toml_content(const char *content);
void pkg_manifest_free(PkgManifest *pm);
int pkg_manifest_print(const PkgManifest *pm, char *buf, size_t buf_size);

/* 包管理器 */
PkgManager *pkg_manager_new(const char *workspace);
void pkg_manager_free(PkgManager *pm);
int pkg_manager_add(PkgManager *pm, const char *name, const char *version,
                     const char *source, const char *path);
int pkg_manager_remove(PkgManager *pm, const char *name);
PkgManifest *pkg_manager_find(PkgManager *pm, const char *name);
int pkg_manager_resolve(PkgManager *pm);
int pkg_manager_dump(PkgManager *pm, char *buf, size_t buf_size);

/* 版本比较 */
int pkg_version_compare(const char *a, const char *b);
bool pkg_version_satisfies(const char *version, const char *constraint);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_PKG_H */
