/**
 * Pony++ Package Manager - ponypp.toml 解析与依赖管理
 */
#include "ponypp/pkg.h"
#include <ctype.h>

static char *str_trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end-1))) end--;
    *end = 0;
    return s;
}

static char *strip_quotes(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    if (len < 2) return strdup(s);
    if ((s[0] == '"' || s[0] == '\'') && s[len-1] == s[0]) {
        char *r = malloc(len - 1);
        if (r) { memcpy(r, s+1, len-2); r[len-2] = 0; }
        return r;
    }
    return strdup(s);
}

PkgManifest *pkg_parse_toml_content(const char *content) {
    if (!content) return NULL;
    PkgManifest *pm = calloc(1, sizeof(PkgManifest));
    if (!pm) return NULL;
    pm->dep_cap = 8;
    pm->deps = calloc(pm->dep_cap, sizeof(PkgDep));
    if (!pm->deps) { free(pm); return NULL; }

    const char *line = content;
    char cur_section[64] = "";
    PkgDep *cur_dep = NULL;

    while (*line) {
        const char *eol = strchr(line, '\n');
        size_t line_len = eol ? (size_t)(eol - line) : strlen(line);
        char *buf = malloc(line_len + 1);
        if (!buf) break;
        memcpy(buf, line, line_len);
        buf[line_len] = 0;
        char *b = str_trim(buf);

        if (*b == '#' || *b == 0) { free(buf); if (eol) line = eol + 1; else break; continue; }

        if (b[0] == '[') {
            char *end = strchr(b, ']');
            if (end) {
                *end = 0;
                char *section = str_trim(b + 1);
                strncpy(cur_section, section, sizeof(cur_section) - 1);
                cur_dep = NULL;
            }
        } else {
            char *eq = strchr(b, '=');
            if (!eq) { free(buf); if (eol) line = eol + 1; else break; continue; }
            *eq = 0;
            char *key = str_trim(b);
            char *val_raw = eq + 1;
            char *val = str_trim(val_raw);
            val = strip_quotes(val);

            if (strcmp(cur_section, "package") == 0) {
                if (strcmp(key, "name") == 0) { free(pm->name); pm->name = val; }
                else if (strcmp(key, "version") == 0) { free(pm->version); pm->version = val; }
                else if (strcmp(key, "description") == 0) { free(pm->description); pm->description = val; }
                else if (strcmp(key, "author") == 0) { free(pm->author); pm->author = val; }
                else if (strcmp(key, "license") == 0) { free(pm->license); pm->license = val; }
                else if (strcmp(key, "homepage") == 0) { free(pm->homepage); pm->homepage = val; }
                else free(val);
            } else if (strncmp(cur_section, "dependency.", 11) == 0) {
                char dep_name[128];
                strncpy(dep_name, cur_section + 11, sizeof(dep_name) - 1);
                dep_name[sizeof(dep_name) - 1] = 0;

                cur_dep = NULL;
                for (size_t i = 0; i < pm->dep_count; i++) {
                    if (pm->deps[i].name && strcmp(pm->deps[i].name, dep_name) == 0) {
                        cur_dep = &pm->deps[i];
                        break;
                    }
                }
                if (!cur_dep) {
                    if (pm->dep_count >= pm->dep_cap) {
                        pm->dep_cap *= 2;
                        pm->deps = realloc(pm->deps, pm->dep_cap * sizeof(PkgDep));
                    }
                    cur_dep = &pm->deps[pm->dep_count++];
                    memset(cur_dep, 0, sizeof(PkgDep));
                    cur_dep->name = strdup(dep_name);
                    cur_dep->enabled = true;
                }

                if (strcmp(key, "version") == 0) { free(cur_dep->version); cur_dep->version = val; }
                else if (strcmp(key, "source") == 0) { free(cur_dep->source); cur_dep->source = val; }
                else if (strcmp(key, "path") == 0) { free(cur_dep->path); cur_dep->path = val; }
                else free(val);
            } else {
                free(val);
            }
        }
        free(buf);
        if (eol) line = eol + 1;
        else break;
    }
    return pm;
}

PkgManifest *pkg_parse_toml(const char *path) {
    if (!path) return NULL;
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len < 0) { fclose(f); return NULL; }
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, len, f);
    buf[len] = 0;
    fclose(f);
    PkgManifest *pm = pkg_parse_toml_content(buf);
    free(buf);
    return pm;
}

void pkg_manifest_free(PkgManifest *pm) {
    if (!pm) return;
    free(pm->name);
    free(pm->version);
    free(pm->description);
    free(pm->author);
    free(pm->license);
    free(pm->homepage);
    for (size_t i = 0; i < pm->dep_count; i++) {
        free(pm->deps[i].name);
        free(pm->deps[i].version);
        free(pm->deps[i].source);
        free(pm->deps[i].path);
    }
    free(pm->deps);
    free(pm);
}

int pkg_manifest_print(const PkgManifest *pm, char *buf, size_t buf_size) {
    if (!pm || !buf || buf_size == 0) return 0;
    size_t pos = 0;
    pos += snprintf(buf + pos, buf_size - pos, "Pony++ Package: %s v%s\n",
                    pm->name ? pm->name : "?", pm->version ? pm->version : "?");
    if (pm->description)
        pos += snprintf(buf + pos, buf_size - pos, "  %s\n", pm->description);
    if (pm->author)
        pos += snprintf(buf + pos, buf_size - pos, "  Author: %s\n", pm->author);
    if (pm->license)
        pos += snprintf(buf + pos, buf_size - pos, "  License: %s\n", pm->license);
    pos += snprintf(buf + pos, buf_size - pos, "  Dependencies: %zu\n", pm->dep_count);
    for (size_t i = 0; i < pm->dep_count; i++) {
        pos += snprintf(buf + pos, buf_size - pos, "    - %s %s [%s]\n",
                        pm->deps[i].name ? pm->deps[i].name : "?",
                        pm->deps[i].version ? pm->deps[i].version : "any",
                        pm->deps[i].source ? pm->deps[i].source : "unknown");
    }
    return (int)pos;
}

PkgManager *pkg_manager_new(const char *workspace) {
    PkgManager *pm = calloc(1, sizeof(PkgManager));
    if (!pm) return NULL;
    if (workspace) strncpy(pm->workspace, workspace, sizeof(pm->workspace) - 1);
    pm->package_cap = 16;
    pm->packages = calloc(pm->package_cap, sizeof(PkgManifest *));
    return pm->packages ? pm : NULL;
}

void pkg_manager_free(PkgManager *pm) {
    if (!pm) return;
    for (size_t i = 0; i < pm->package_count; i++) {
        pkg_manifest_free(pm->packages[i]);
    }
    free(pm->packages);
    free(pm);
}

int pkg_manager_add(PkgManager *pm, const char *name, const char *version,
                     const char *source, const char *path) {
    if (!pm || !name) return -1;
    if (pm->package_count >= pm->package_cap) {
        pm->package_cap *= 2;
        pm->packages = realloc(pm->packages, pm->package_cap * sizeof(PkgManifest *));
        if (!pm->packages) return -2;
    }
    PkgManifest *pm_new = calloc(1, sizeof(PkgManifest));
    if (!pm_new) return -3;
    pm_new->name = strdup(name);
    pm_new->version = strdup(version ? version : "0.1.0");
    pm_new->dep_cap = 8;
    pm_new->deps = calloc(pm_new->dep_cap, sizeof(PkgDep));
    if (source) pm_new->description = strdup(source);
    if (path) pm_new->homepage = strdup(path);
    pm->packages[pm->package_count++] = pm_new;
    return 0;
}

int pkg_manager_remove(PkgManager *pm, const char *name) {
    if (!pm || !name) return -1;
    for (size_t i = 0; i < pm->package_count; i++) {
        if (pm->packages[i]->name && strcmp(pm->packages[i]->name, name) == 0) {
            pkg_manifest_free(pm->packages[i]);
            memmove(&pm->packages[i], &pm->packages[i+1],
                    (pm->package_count - i - 1) * sizeof(PkgManifest *));
            pm->package_count--;
            return 0;
        }
    }
    return -2;
}

PkgManifest *pkg_manager_find(PkgManager *pm, const char *name) {
    if (!pm || !name) return NULL;
    for (size_t i = 0; i < pm->package_count; i++) {
        if (pm->packages[i]->name && strcmp(pm->packages[i]->name, name) == 0)
            return pm->packages[i];
    }
    return NULL;
}

int pkg_manager_resolve(PkgManager *pm) {
    if (!pm) return -1;
    int resolved = 0;
    for (size_t i = 0; i < pm->package_count; i++) {
        resolved++;
    }
    return resolved;
}

int pkg_manager_dump(PkgManager *pm, char *buf, size_t buf_size) {
    if (!pm || !buf || buf_size == 0) return 0;
    size_t pos = 0;
    pos += snprintf(buf + pos, buf_size - pos, "Pony++ Package Manager\n");
    pos += snprintf(buf + pos, buf_size - pos, "Workspace: %s\n", pm->workspace);
    pos += snprintf(buf + pos, buf_size - pos, "Packages: %zu\n", pm->package_count);
    for (size_t i = 0; i < pm->package_count; i++) {
        pos += snprintf(buf + pos, buf_size - pos, "  %s v%s\n",
                        pm->packages[i]->name ? pm->packages[i]->name : "?",
                        pm->packages[i]->version ? pm->packages[i]->version : "?");
    }
    return (int)pos;
}

int pkg_version_compare(const char *a, const char *b) {
    if (!a || !b) return 0;
    int ai[3] = {0, 0, 0}, bi[3] = {0, 0, 0};
    sscanf(a, "%d.%d.%d", &ai[0], &ai[1], &ai[2]);
    sscanf(b, "%d.%d.%d", &bi[0], &bi[1], &bi[2]);
    for (int i = 0; i < 3; i++) {
        if (ai[i] < bi[i]) return -1;
        if (ai[i] > bi[i]) return 1;
    }
    return 0;
}

bool pkg_version_satisfies(const char *version, const char *constraint) {
    if (!version || !constraint) return false;
    constraint = str_trim((char *)constraint);
    if (constraint[0] == '^') {
        return pkg_version_compare(version, constraint + 1) >= 0;
    }
    if (constraint[0] == '~') {
        return pkg_version_compare(version, constraint + 1) >= 0;
    }
    return strcmp(version, constraint) == 0;
}
