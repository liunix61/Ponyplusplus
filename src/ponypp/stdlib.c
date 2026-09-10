#define _POSIX_C_SOURCE 200809L
#include "ponypp/stdlib.h"
#include <math.h>
#include <stdint.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

#ifdef __linux__
#include <pthread.h>
#endif

/* ==================== I/O ==================== */

PnyFile *pny_file_open(const char *path, FileMode mode) {
    if (!path) return NULL;
    const char *m = "rb";
    if (mode == FILE_MODE_WRITE) m = "wb";
    else if (mode == FILE_MODE_APPEND) m = "ab";
    else if (mode == FILE_MODE_READ_WRITE) m = "r+b";
    FILE *fp = fopen(path, m);
    if (!fp) return NULL;
    PnyFile *f = (PnyFile *)malloc(sizeof(PnyFile));
    if (!f) { fclose(fp); return NULL; }
    f->fp = fp;
    f->path = path ? strdup(path) : NULL;
    f->mode = mode;
    f->eof = false;
    return f;
}

int pny_file_close(PnyFile *f) {
    if (!f) return -1;
    fflush(f->fp);
    int r = fclose(f->fp);
    free(f->path);
    free(f);
    return r;
}

char *pny_file_read_all(PnyFile *f) {
    if (!f || !f->fp) return NULL;
    fseek(f->fp, 0, SEEK_END);
    long sz = ftell(f->fp);
    fseek(f->fp, 0, SEEK_SET);
    if (sz < 0) return NULL;
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) return NULL;
    size_t n = fread(buf, 1, (size_t)sz, f->fp);
    buf[n] = '\0';
    return buf;
}

char *pny_file_read_line(PnyFile *f) {
    if (!f || !f->fp) return NULL;
    char *buf = NULL;
    size_t cap = 0;
    ssize_t len = getline(&buf, &cap, f->fp);
    if (len < 0) { free(buf); f->eof = true; return NULL; }
    /* strip trailing \n */
    if (buf[len-1] == '\n') buf[len-1] = '\0';
    return buf;
}

int pny_file_write(PnyFile *f, const char *data, size_t len) {
    if (!f || !f->fp || !data) return -1;
    size_t n = fwrite(data, 1, len, f->fp);
    return (n == len) ? 0 : -1;
}

int pny_file_printf(PnyFile *f, const char *fmt, ...) {
    if (!f || !f->fp || !fmt) return -1;
    va_list ap;
    va_start(ap, fmt);
    int r = vfprintf(f->fp, fmt, ap);
    va_end(ap);
    return r;
}

bool pny_file_eof(PnyFile *f) {
    return f && f->fp && ferror(f->fp) == 0 && feof(f->fp);
}

int pny_file_size(const char *path) {
    if (!path) return -1;
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (int)st.st_size;
}

void pny_stdout_print(const char *s) {
    if (s) fputs(s, stdout);
}

void pny_stdout_println(const char *s) {
    if (s) fputs(s, stdout);
    fputc('\n', stdout);
}

void pny_stdout_print_int(int64_t n) {
    printf("%lld\n", (long long)n);
}

void pny_stdout_print_bool(bool b) {
    printf("%s\n", b ? "true" : "false");
}

int pny_stdin_read_line(char *buf, size_t sz) {
    if (!buf || sz == 0) return -1;
    if (!fgets(buf, (int)sz, stdin)) return -1;
    size_t n = strlen(buf);
    if (n > 0 && buf[n-1] == '\n') { buf[n-1] = '\0'; n--; }
    return (int)n;
}

int pny_stdin_read_bytes(char *buf, size_t sz) {
    if (!buf || sz == 0) return -1;
    size_t n = fread(buf, 1, sz, stdin);
    return (int)n;
}

char *pny_path_join(const char *base, const char *part) {
    if (!base || !part) return NULL;
    size_t bl = strlen(base), pl = strlen(part);
    bool need_sep = bl > 0 && base[bl-1] != '/' && part[0] != '/';
    size_t total = bl + pl + need_sep + 1;
    char *r = (char *)malloc(total);
    if (!r) return NULL;
    memcpy(r, base, bl);
    size_t i = bl;
    if (need_sep) r[i++] = '/';
    memcpy(r + i, part, pl + 1);
    return r;
}

bool pny_path_exists(const char *path) {
    return path && access(path, F_OK) == 0;
}

int pny_file_delete(const char *path) {
    if (!path) return -1;
    return remove(path);
}

int pny_dir_list(const char *dir, char ***names, int *count) {
    (void)dir; (void)names; (void)count;
    /* placeholder — full implementation in Phase 3.1 */
    if (names) *names = NULL;
    if (count) *count = 0;
    return 0;
}

/* ==================== String ==================== */

PnyString *pny_str_new(const char *s) {
    size_t len = s ? strlen(s) : 0;
    PnyString *ps = (PnyString *)malloc(sizeof(PnyString));
    if (!ps) return NULL;
    size_t cap = len + 4;
    ps->data = (char *)malloc(cap + 1);  /* 分配cap+1, 匹配cap字段 */
    if (!ps->data && len > 0) { free(ps); return NULL; }
    if (s) memcpy(ps->data, s, len + 1);
    else ps->data[0] = '\0';
    ps->len = len;
    ps->cap = cap;
    return ps;
}

PnyString *pny_str_new_with(size_t cap) {
    PnyString *ps = (PnyString *)malloc(sizeof(PnyString));
    if (!ps) return NULL;
    size_t alloc = cap > 0 ? cap + 1 : 1;
    ps->data = (char *)calloc(alloc, 1);
    if (!ps->data) { free(ps); return NULL; }
    ps->len = 0;
    ps->cap = cap;
    return ps;
}

static bool str_grow(PnyString *s, size_t need) {
    if (s->cap >= need) return true;
    size_t new_cap = s->cap;
    while (new_cap < need) new_cap *= 2;
    if (new_cap < 16) new_cap = 16;
    char *ndata = (char *)realloc(s->data, new_cap + 1);
    if (!ndata) return false;
    s->data = ndata;
    s->cap = new_cap;
    return true;
}

void pny_str_free(PnyString *s) {
    if (!s) return;
    free(s->data);
    free(s);
}

PnyString *pny_str_dup(const PnyString *s) {
    if (!s) return NULL;
    return pny_str_new(s->data);
}

size_t pny_str_len(const PnyString *s) { return s ? s->len : 0; }
bool pny_str_empty(const PnyString *s) { return !s || s->len == 0; }

int pny_str_cmp(const PnyString *a, const PnyString *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    size_t n = a->len < b->len ? a->len : b->len;
    int r = memcmp(a->data, b->data, n);
    if (r) return r;
    return (a->len < b->len) ? -1 : (a->len > b->len) ? 1 : 0;
}

int pny_str_cmp_cstr(const PnyString *s, const char *other) {
    if (!s) return other ? -1 : 0;
    return strcmp(s->data, other ? other : "");
}

PnyString *pny_str_cat(PnyString *s, const PnyString *other) {
    if (!s || !other) return s;
    if (!str_grow(s, s->len + other->len)) return s;
    memcpy(s->data + s->len, other->data, other->len);
    s->len += other->len;
    s->data[s->len] = '\0';
    return s;
}

PnyString *pny_str_cat_cstr(PnyString *s, const char *other) {
    if (!s || !other) return s;
    size_t ol = strlen(other);
    if (!str_grow(s, s->len + ol)) return s;
    memcpy(s->data + s->len, other, ol);
    s->len += ol;
    s->data[s->len] = '\0';
    return s;
}

PnyString *pny_str_slice(const PnyString *s, size_t start, size_t end) {
    if (!s || start > s->len) return NULL;
    if (end > s->len) end = s->len;
    if (start >= end) return pny_str_new("");
    size_t len = end - start;
    PnyString *r = pny_str_new_with(len);
    if (!r) return NULL;
    memcpy(r->data, s->data + start, len);
    r->len = len;
    r->data[len] = '\0';
    return r;
}

bool pny_str_contains(const PnyString *s, const PnyString *sub) {
    if (!s || !sub) return false;
    if (sub->len > s->len) return false;
    for (size_t i = 0; i + sub->len <= s->len; i++) {
        if (memcmp(s->data + i, sub->data, sub->len) == 0) return true;
    }
    return false;
}

bool pny_str_starts_with(const PnyString *s, const PnyString *prefix) {
    if (!s || !prefix) return false;
    if (prefix->len > s->len) return false;
    return memcmp(s->data, prefix->data, prefix->len) == 0;
}

bool pny_str_ends_with(const PnyString *s, const PnyString *suffix) {
    if (!s || !suffix) return false;
    if (suffix->len > s->len) return false;
    return memcmp(s->data + (s->len - suffix->len), suffix->data, suffix->len) == 0;
}

PnyString *pny_str_replace(PnyString *s, const PnyString *old_, const PnyString *new_) {
    if (!s || !old_ || !new_) return s;
    PnyString *out = pny_str_new_with(s->len + new_->len);
    if (!out) return s;
    size_t i;
    for (i = 0; i <= s->len - old_->len;) {
        if (memcmp(s->data + i, old_->data, old_->len) == 0) {
            pny_str_cat(out, new_);
            i += old_->len;
        } else {
            if (!str_grow(out, out->len + 1)) return s;
            out->data[out->len++] = s->data[i++];
        }
    }
    while (i < s->len) {
        if (!str_grow(out, out->len + 1)) return s;
        out->data[out->len++] = s->data[i++];
    }
    out->data[out->len] = '\0';
    return out;
}

PnyString *pny_str_to_upper(PnyString *s) {
    if (!s) return NULL;
    PnyString *r = pny_str_dup(s);
    if (!r) return NULL;
    for (size_t i = 0; i < r->len; i++) {
        unsigned char c = (unsigned char)r->data[i];
        if (c >= 'a' && c <= 'z') r->data[i] = (char)(c - 32);
    }
    return r;
}

PnyString *pny_str_to_lower(PnyString *s) {
    if (!s) return NULL;
    PnyString *r = pny_str_dup(s);
    if (!r) return NULL;
    for (size_t i = 0; i < r->len; i++) {
        unsigned char c = (unsigned char)r->data[i];
        if (c >= 'A' && c <= 'Z') r->data[i] = (char)(c + 32);
    }
    return r;
}

PnyString *pny_str_trim(PnyString *s) {
    if (!s) return NULL;
    size_t start = 0;
    while (start < s->len && (s->data[start] == ' ' || s->data[start] == '\t' ||
           s->data[start] == '\n' || s->data[start] == '\r')) start++;
    size_t end = s->len;
    while (end > start && (s->data[end-1] == ' ' || s->data[end-1] == '\t' ||
           s->data[end-1] == '\n' || s->data[end-1] == '\r')) end--;
    return pny_str_slice(s, start, end);
}

PnyString *pny_str_split(const PnyString *s, const PnyString *sep, PnyString ***parts, int *count) {
    (void)s; (void)sep;
    if (parts) *parts = NULL;
    if (count) *count = 0;
    return NULL;
}

PnyString *pny_str_join(const PnyString *sep, const PnyString *arr[], int count) {
    if (!arr || count <= 0) return NULL;
    size_t total = 0;
    for (int i = 0; i < count; i++) total += arr[i] ? arr[i]->len : 0;
    if (sep && count > 1) total += (size_t)(count-1) * sep->len;
    PnyString *r = pny_str_new_with(total);
    if (!r) return NULL;
    for (int i = 0; i < count; i++) {
        if (i > 0 && sep) pny_str_cat(r, sep);
        if (arr[i]) pny_str_cat(r, arr[i]);
    }
    return r;
}

PnyString *pny_str_format(const char *fmt, ...) {
    if (!fmt) return NULL;
    va_list ap, ap2;
    va_start(ap, fmt);
    va_start(ap2, fmt);
    int len = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (len < 0) { va_end(ap2); return NULL; }
    size_t alloc = (size_t)len + 1;
    char *buf = (char *)malloc(alloc);
    if (!buf) { va_end(ap2); return NULL; }
    vsnprintf(buf, alloc, fmt, ap2);
    va_end(ap2);
    PnyString *ps = pny_str_new(buf);
    free(buf);
    return ps;
}

PnyString *pny_str_from_int(int64_t n) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)n);
    return pny_str_new(buf);
}

PnyString *pny_str_from_float(double n) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.6f", n);
    return pny_str_new(buf);
}

PnyString *pny_str_from_bool(bool b) {
    return pny_str_new(b ? "true" : "false");
}

int64_t pny_str_to_int(const PnyString *s) {
    if (!s) return 0;
    return (int64_t)atoll(s->data);
}

double pny_str_to_float(const PnyString *s) {
    if (!s) return 0.0;
    return atof(s->data);
}

bool pny_str_to_bool(const PnyString *s) {
    if (!s) return false;
    return strcmp(s->data, "true") == 0 || strcmp(s->data, "1") == 0;
}

/* ==================== List ==================== */

PnyList *pny_list_new(size_t elem_size) {
    PnyList *l = (PnyList *)malloc(sizeof(PnyList));
    if (!l) return NULL;
    l->head = NULL;
    l->tail = NULL;
    l->len = 0;
    l->elem_size = elem_size;
    l->destructor = NULL;
    return l;
}

void pny_list_free(PnyList *l) {
    if (!l) return;
    PnyListNode *cur = l->head;
    while (cur) {
        PnyListNode *next = cur->next;
        if (l->destructor && cur->data) l->destructor(cur->data);
        free(cur);
        cur = next;
    }
    free(l);
}

static PnyListNode *list_alloc_node(PnyList *l, void *data) {
    PnyListNode *n = (PnyListNode *)malloc(sizeof(PnyListNode));
    if (!n) return NULL;
    if (l->elem_size > 0 && data) {
        n->data = malloc(l->elem_size);
        if (!n->data) { free(n); return NULL; }
        memcpy(n->data, data, l->elem_size);
        n->size = l->elem_size;
    } else {
        n->data = data;
        n->size = 0;
    }
    n->next = NULL;
    n->prev = NULL;
    return n;
}

void pny_list_append(PnyList *l, void *data) {
    if (!l) return;
    PnyListNode *n = list_alloc_node(l, data);
    if (!n) return;
    if (!l->head) { l->head = n; l->tail = n; }
    else { n->prev = l->tail; l->tail->next = n; l->tail = n; }
    l->len++;
}

void pny_list_prepend(PnyList *l, void *data) {
    if (!l) return;
    PnyListNode *n = list_alloc_node(l, data);
    if (!n) return;
    n->next = l->head;
    if (l->head) l->head->prev = n;
    l->head = n;
    if (!l->tail) l->tail = n;
    l->len++;
}

void *pny_list_get(PnyList *l, size_t index) {
    if (!l || index >= l->len) return NULL;
    PnyListNode *n = l->head;
    for (size_t i = 0; i < index; i++) n = n->next;
    return n->data;
}

int pny_list_set(PnyList *l, size_t index, void *data) {
    if (!l || index >= l->len) return -1;
    PnyListNode *n = l->head;
    for (size_t i = 0; i < index; i++) n = n->next;
    if (l->destructor && n->data) l->destructor(n->data);
    free(n->data);
    if (l->elem_size > 0) {
        n->data = malloc(l->elem_size);
        if (!n->data) return -1;
        memcpy(n->data, data, l->elem_size);
    } else {
        n->data = data;
    }
    return 0;
}

int pny_list_remove(PnyList *l, size_t index) {
    if (!l || index >= l->len) return -1;
    PnyListNode *n = l->head;
    for (size_t i = 0; i < index; i++) n = n->next;
    if (n->prev) n->prev->next = n->next;
    else l->head = n->next;
    if (n->next) n->next->prev = n->prev;
    else l->tail = n->prev;
    if (l->destructor && n->data) l->destructor(n->data);
    free(n->data);
    free(n);
    l->len--;
    return 0;
}

int pny_list_remove_val(PnyList *l, void *data, int (*cmp)(const void *, const void *)) {
    if (!l) return -1;
    PnyListNode *n = l->head;
    while (n) {
        PnyListNode *next = n->next;
        if (cmp && cmp(n->data, data) == 0) {
            if (n->prev) n->prev->next = n->next;
            else l->head = n->next;
            if (n->next) n->next->prev = n->prev;
            else l->tail = n->prev;
            if (l->destructor && n->data) l->destructor(n->data);
            free(n->data);
            free(n);
            l->len--;
            return 0;
        }
        n = next;
    }
    return -1;
}

void *pny_list_pop(PnyList *l) {
    if (!l || !l->tail) return NULL;
    PnyListNode *n = l->tail;
    void *data = n->data;
    if (n->prev) n->prev->next = NULL;
    else l->head = NULL;
    l->tail = n->prev;
    l->len--;
    free(n);
    return data;
}

void *pny_list_pop_front(PnyList *l) {
    if (!l || !l->head) return NULL;
    PnyListNode *n = l->head;
    void *data = n->data;
    if (n->next) n->next->prev = NULL;
    else l->tail = NULL;
    l->head = n->next;
    l->len--;
    free(n);
    return data;
}

void pny_list_insert(PnyList *l, size_t index, void *data) {
    if (!l || index > l->len) return;
    PnyListNode *n = list_alloc_node(l, data);
    if (!n) return;
    if (index == 0) {
        n->next = l->head;
        if (l->head) l->head->prev = n;
        l->head = n;
        if (!l->tail) l->tail = n;
    } else {
        PnyListNode *pos = l->head;
        for (size_t i = 0; i < index - 1; i++) pos = pos->next;
        n->next = pos->next;
        n->prev = pos;
        if (pos->next) pos->next->prev = n;
        else l->tail = n;
        pos->next = n;
    }
    l->len++;
}

size_t pny_list_len(const PnyList *l) { return l ? l->len : 0; }
bool pny_list_empty(const PnyList *l) { return !l || l->len == 0; }

int pny_list_index_of(PnyList *l, void *data, int (*cmp)(const void *, const void *)) {
    if (!l) return -1;
    PnyListNode *n = l->head;
    for (size_t i = 0; i < l->len; i++) {
        if (cmp && cmp(n->data, data) == 0) return (int)i;
        n = n->next;
    }
    return -1;
}

static void list_swap(PnyListNode *a, PnyListNode *b, size_t sz) {
    unsigned char tmp[64];
    size_t cp = sz <= 64 ? sz : 64;
    memcpy(tmp, a->data, cp);
    memcpy(a->data, b->data, cp);
    memcpy(b->data, tmp, cp);
}

void pny_list_sort(PnyList *l, int (*cmp)(const void *, const void *)) {
    if (!l || l->len <= 1 || !cmp) return;
    /* simple bubble sort */
    for (size_t i = 0; i < l->len - 1; i++) {
        PnyListNode *a = l->head;
        for (size_t j = 0; j < l->len - 1 - i; j++) {
            PnyListNode *b = a->next;
            if (cmp(a->data, b->data) > 0) {
                if (l->elem_size > 0) list_swap(a, b, l->elem_size);
            }
            a = b;
        }
    }
}

void pny_list_reverse(PnyList *l) {
    if (!l) return;
    PnyListNode *cur = l->head;
    PnyListNode *tmp = NULL;
    while (cur) {
        tmp = cur->prev;
        cur->prev = cur->next;
        cur->next = tmp;
        cur = cur->prev;
    }
    tmp = l->head;
    l->head = l->tail;
    l->tail = tmp;
}

void pny_list_foreach(PnyList *l, void (*fn)(void *, void *), void *ctx) {
    if (!l || !fn) return;
    PnyListNode *n = l->head;
    while (n) { fn(n->data, ctx); n = n->next; }
}

PnyList *pny_list_slice(const PnyList *l, size_t start, size_t end) {
    if (!l || start >= l->len || start >= end) return NULL;
    if (end > l->len) end = l->len;
    PnyList *r = pny_list_new(l ? l->elem_size : 0);
    if (!r) return NULL;
    PnyListNode *n = l->head;
    for (size_t i = 0; i < start; i++) n = n->next;
    for (size_t i = start; i < end; i++) {
        pny_list_append(r, n->data);
        n = n->next;
    }
    return r;
}

/* ==================== Map ==================== */

static size_t map_hash_default(const void *key, size_t sz) {
    if (sz == 0) {
        /* pointer-based keys: hash the pointer address */
        return (size_t)key / sizeof(void*);
    }
    const unsigned char *p = (const unsigned char *)key;
    size_t h = 2166136261u;
    for (size_t i = 0; i < sz; i++) h ^= p[i], h *= 16777619u;
    return h;
}

static int map_key_cmp_default(const void *a, const void *b) {
    if (a == b) return 0;
    return (a > b) ? 1 : -1;
}

PnyMap *pny_map_new(size_t cap, size_t key_size, size_t val_size) {
    PnyMap *m = (PnyMap *)malloc(sizeof(PnyMap));
    if (!m) return NULL;
    m->cap = cap > 0 ? cap : 16;
    m->size = 0;
    m->key_size = key_size;
    m->val_size = val_size;
    m->key_free = NULL;
    m->val_free = NULL;
    m->hash = map_hash_default;
    m->key_cmp = map_key_cmp_default;
    m->buckets = (PnyMapNode **)calloc(m->cap, sizeof(PnyMapNode *));
    if (!m->buckets) { free(m); return NULL; }
    return m;
}

void pny_map_free(PnyMap *m) {
    if (!m) return;
    for (size_t i = 0; i < m->cap; i++) {
        PnyMapNode *n = m->buckets[i];
        while (n) {
            PnyMapNode *next = n->next;
            if (m->key_free) m->key_free(n->key);
            else if (n->key && m->key_size > 0) free(n->key);
            if (m->val_free) m->val_free(n->val);
            else if (n->val && m->val_size > 0) free(n->val);
            free(n);
            n = next;
        }
    }
    free(m->buckets);
    free(m);
}

int pny_map_put(PnyMap *m, void *key, void *val) {
    if (!m) return -1;
    size_t h = m->hash(key, m->key_size);
    size_t idx = h % m->cap;
    PnyMapNode *n = m->buckets[idx];
    while (n) {
        if (m->key_cmp(n->key, key) == 0) {
            free(n->val);
            n->val = val;
            return 0;
        }
        n = n->next;
    }
    PnyMapNode *new = (PnyMapNode *)malloc(sizeof(PnyMapNode));
    if (!new) return -1;
    new->key = key ? (m->key_size > 0 ? memcpy(malloc(m->key_size), key, m->key_size) : (void*)key) : NULL;
    new->val = val ? (m->val_size > 0 ? memcpy(malloc(m->val_size), val, m->val_size) : (void*)val) : NULL;
    if (key && m->key_size > 0 && !new->key) { free(new); return -1; }
    if (val && m->val_size > 0 && !new->val) { free(new->key); free(new); return -1; }
    new->h = h;
    new->next = m->buckets[idx];
    m->buckets[idx] = new;
    m->size++;
    return 0;
}

void *pny_map_get(PnyMap *m, const void *key) {
    if (!m) return NULL;
    size_t idx = m->hash(key, m->key_size) % m->cap;
    PnyMapNode *n = m->buckets[idx];
    while (n) {
        if (m->key_cmp(n->key, key) == 0) return n->val;
        n = n->next;
    }
    return NULL;
}

int pny_map_remove(PnyMap *m, const void *key) {
    if (!m) return -1;
    size_t idx = m->hash(key, m->key_size) % m->cap;
    PnyMapNode *n = m->buckets[idx];
    PnyMapNode *prev = NULL;
    while (n) {
        if (m->key_cmp(n->key, key) == 0) {
            if (prev) prev->next = n->next;
            else m->buckets[idx] = n->next;
            if (m->key_free) m->key_free(n->key);
            else free(n->key);
            if (m->val_free) m->val_free(n->val);
            else free(n->val);
            free(n);
            m->size--;
            return 0;
        }
        prev = n;
        n = n->next;
    }
    return -1;
}

bool pny_map_has(PnyMap *m, const void *key) {
    return pny_map_get(m, key) != NULL;
}

size_t pny_map_size(const PnyMap *m) { return m ? m->size : 0; }
bool pny_map_empty(const PnyMap *m) { return !m || m->size == 0; }

void pny_map_clear(PnyMap *m) {
    if (!m) return;
    for (size_t i = 0; i < m->cap; i++) {
        PnyMapNode *n = m->buckets[i];
        while (n) {
            PnyMapNode *next = n->next;
            if (m->key_free) m->key_free(n->key);
            else free(n->key);
            if (m->val_free) m->val_free(n->val);
            else free(n->val);
            free(n);
            n = next;
        }
        m->buckets[i] = NULL;
    }
    m->size = 0;
}

void pny_map_foreach(PnyMap *m, void (*fn)(void *, void *, void *), void *ctx) {
    if (!m || !fn) return;
    for (size_t i = 0; i < m->cap; i++) {
        PnyMapNode *n = m->buckets[i];
        while (n) { fn(n->key, n->val, ctx); n = n->next; }
    }
}

/* ==================== Set (字符串哈希集合) ==================== */
typedef struct SetEntry {
    char *key;
    struct SetEntry *next;
} SetEntry;

struct PnySet {
    SetEntry **buckets;
    size_t bucket_count;
    size_t size;
};

static size_t set_hash(const char *key, size_t bucket_count) {
    size_t h = 5381;
    while (*key) h = ((h << 5) + h) + (unsigned char)*key++;
    return h % bucket_count;
}

PnySet *pny_set_new(void) {
    PnySet *s = (PnySet *)calloc(1, sizeof(PnySet));
    if (!s) return NULL;
    s->bucket_count = 64;
    s->buckets = (SetEntry **)calloc(s->bucket_count, sizeof(SetEntry *));
    if (!s->buckets) { free(s); return NULL; }
    return s;
}

void pny_set_free(PnySet *s) {
    if (!s) return;
    for (size_t i = 0; i < s->bucket_count; i++) {
        SetEntry *e = s->buckets[i];
        while (e) { SetEntry *n = e->next; free(e->key); free(e); e = n; }
    }
    free(s->buckets);
    free(s);
}

bool pny_set_add(PnySet *s, const char *key) {
    if (!s || !key) return false;
    size_t idx = set_hash(key, s->bucket_count);
    for (SetEntry *e = s->buckets[idx]; e; e = e->next) {
        if (strcmp(e->key, key) == 0) return false;  /* 已存在 */
    }
    SetEntry *e = (SetEntry *)malloc(sizeof(SetEntry));
    if (!e) return false;
    e->key = strdup(key);
    e->next = s->buckets[idx];
    s->buckets[idx] = e;
    s->size++;
    return true;
}

bool pny_set_contains(const PnySet *s, const char *key) {
    if (!s || !key) return false;
    size_t idx = set_hash(key, s->bucket_count);
    for (SetEntry *e = s->buckets[idx]; e; e = e->next) {
        if (strcmp(e->key, key) == 0) return true;
    }
    return false;
}

bool pny_set_remove(PnySet *s, const char *key) {
    if (!s || !key) return false;
    size_t idx = set_hash(key, s->bucket_count);
    SetEntry **prev = &s->buckets[idx];
    for (SetEntry *e = *prev; e; prev = &e->next, e = e->next) {
        if (strcmp(e->key, key) == 0) {
            *prev = e->next;
            free(e->key);
            free(e);
            s->size--;
            return true;
        }
    }
    return false;
}

size_t pny_set_size(const PnySet *s) { return s ? s->size : 0; }

void pny_set_clear(PnySet *s) {
    if (!s) return;
    for (size_t i = 0; i < s->bucket_count; i++) {
        SetEntry *e = s->buckets[i];
        while (e) { SetEntry *n = e->next; free(e->key); free(e); e = n; }
        s->buckets[i] = NULL;
    }
    s->size = 0;
}

/* ==================== Queue (FIFO, 环形缓冲区) ==================== */
struct PnyQueue {
    void **items;
    size_t cap;
    size_t head;
    size_t tail;
    size_t count;
    size_t max;  /* 0=无限制 */
};

PnyQueue *pny_queue_new(size_t cap) {
    if (cap == 0) cap = 64;
    PnyQueue *q = (PnyQueue *)calloc(1, sizeof(PnyQueue));
    if (!q) return NULL;
    q->items = (void **)calloc(cap, sizeof(void *));
    if (!q->items) { free(q); return NULL; }
    q->cap = cap;
    q->max = 0;  /* 无限制 */
    return q;
}

void pny_queue_free(PnyQueue *q) {
    if (!q) return;
    free(q->items);
    free(q);
}

static bool queue_grow(PnyQueue *q) {
    size_t new_cap = q->cap * 2;
    void **new_items = (void **)calloc(new_cap, sizeof(void *));
    if (!new_items) return false;
    /* 重排: head..end, 0..tail */
    for (size_t i = 0; i < q->count; i++) {
        new_items[i] = q->items[(q->head + i) % q->cap];
    }
    free(q->items);
    q->items = new_items;
    q->cap = new_cap;
    q->head = 0;
    q->tail = q->count;
    return true;
}

bool pny_queue_push(PnyQueue *q, void *data) {
    if (!q) return false;
    if (q->max > 0 && q->count >= q->max) return false;
    if (q->count >= q->cap) {
        if (!queue_grow(q)) return false;
    }
    q->items[q->tail] = data;
    q->tail = (q->tail + 1) % q->cap;
    q->count++;
    return true;
}

void *pny_queue_pop(PnyQueue *q) {
    if (!q || q->count == 0) return NULL;
    void *data = q->items[q->head];
    q->head = (q->head + 1) % q->cap;
    q->count--;
    return data;
}

void *pny_queue_peek(const PnyQueue *q) {
    if (!q || q->count == 0) return NULL;
    return q->items[q->head];
}

size_t pny_queue_size(const PnyQueue *q) { return q ? q->count : 0; }
bool pny_queue_is_empty(const PnyQueue *q) { return q ? q->count == 0 : true; }
bool pny_queue_is_full(const PnyQueue *q) { return q && q->max > 0 && q->count >= q->max; }

/* ==================== Stack (LIFO, 动态数组) ==================== */
struct PnyStack {
    void **items;
    size_t cap;
    size_t count;
    size_t max;
};

PnyStack *pny_stack_new(size_t cap) {
    if (cap == 0) cap = 32;
    PnyStack *s = (PnyStack *)calloc(1, sizeof(PnyStack));
    if (!s) return NULL;
    s->items = (void **)calloc(cap, sizeof(void *));
    if (!s->items) { free(s); return NULL; }
    s->cap = cap;
    return s;
}

void pny_stack_free(PnyStack *s) {
    if (!s) return;
    free(s->items);
    free(s);
}

bool pny_stack_push(PnyStack *s, void *data) {
    if (!s) return false;
    if (s->max > 0 && s->count >= s->max) return false;
    if (s->count >= s->cap) {
        size_t new_cap = s->cap * 2;
        void **new_items = (void **)realloc(s->items, new_cap * sizeof(void *));
        if (!new_items) return false;
        s->items = new_items;
        s->cap = new_cap;
    }
    s->items[s->count++] = data;
    return true;
}

void *pny_stack_pop(PnyStack *s) {
    if (!s || s->count == 0) return NULL;
    return s->items[--s->count];
}

void *pny_stack_peek(const PnyStack *s) {
    if (!s || s->count == 0) return NULL;
    return s->items[s->count - 1];
}

size_t pny_stack_size(const PnyStack *s) { return s ? s->count : 0; }
bool pny_stack_is_empty(const PnyStack *s) { return s ? s->count == 0 : true; }

/* ==================== Buffer (字节缓冲区) ==================== */
struct PnyBuffer {
    uint8_t *data;
    size_t len;
    size_t cap;
};

PnyBuffer *pny_buffer_new(size_t initial_cap) {
    if (initial_cap == 0) initial_cap = 64;
    PnyBuffer *b = (PnyBuffer *)calloc(1, sizeof(PnyBuffer));
    if (!b) return NULL;
    b->data = (uint8_t *)malloc(initial_cap);
    if (!b->data) { free(b); return NULL; }
    b->cap = initial_cap;
    return b;
}

void pny_buffer_free(PnyBuffer *b) {
    if (!b) return;
    free(b->data);
    free(b);
}

static bool buffer_reserve_internal(PnyBuffer *b, size_t need) {
    if (b->cap >= need) return true;
    size_t new_cap = b->cap;
    while (new_cap < need) new_cap *= 2;
    uint8_t *new_data = (uint8_t *)realloc(b->data, new_cap);
    if (!new_data) return false;
    b->data = new_data;
    b->cap = new_cap;
    return true;
}

bool pny_buffer_append(PnyBuffer *b, const void *data, size_t len) {
    if (!b || (!data && len > 0)) return false;
    if (!buffer_reserve_internal(b, b->len + len)) return false;
    if (len > 0) memcpy(b->data + b->len, data, len);
    b->len += len;
    return true;
}

bool pny_buffer_append_byte(PnyBuffer *b, uint8_t byte) {
    return pny_buffer_append(b, &byte, 1);
}

bool pny_buffer_append_cstr(PnyBuffer *b, const char *str) {
    if (!str) return false;
    return pny_buffer_append(b, str, strlen(str));
}

const uint8_t *pny_buffer_data(const PnyBuffer *b) { return b ? b->data : NULL; }
size_t pny_buffer_len(const PnyBuffer *b) { return b ? b->len : 0; }
void pny_buffer_clear(PnyBuffer *b) { if (b) b->len = 0; }

bool pny_buffer_reserve(PnyBuffer *b, size_t extra) {
    if (!b) return false;
    return buffer_reserve_internal(b, b->len + extra);
}

/* ==================== Concurrent ==================== */

struct PnyChan {
    void **buf;
    size_t cap;
    size_t head;
    size_t tail;
    size_t len;
    bool closed;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
};

PnyChan *pny_chan_new(size_t cap) {
    PnyChan *c = (PnyChan *)malloc(sizeof(PnyChan));
    if (!c) return NULL;
    c->cap = cap > 0 ? cap : 1;
    c->buf = (void **)calloc(c->cap, sizeof(void *));
    if (!c->buf) { free(c); return NULL; }
    c->head = 0;
    c->tail = 0;
    c->len = 0;
    c->closed = false;
    pthread_mutex_init(&c->mutex, NULL);
    pthread_cond_init(&c->not_full, NULL);
    pthread_cond_init(&c->not_empty, NULL);
    return c;
}

void pny_chan_free(PnyChan *c) {
    if (!c) return;
    pthread_mutex_destroy(&c->mutex);
    pthread_cond_destroy(&c->not_full);
    pthread_cond_destroy(&c->not_empty);
    free(c->buf);
    free(c);
}

int pny_chan_send(PnyChan *c, void *data) {
    if (!c) return -1;
    pthread_mutex_lock(&c->mutex);
    while (c->len == c->cap && !c->closed)
        pthread_cond_wait(&c->not_full, &c->mutex);
    if (c->closed) { pthread_mutex_unlock(&c->mutex); return -1; }
    c->buf[c->tail] = data;
    c->tail = (c->tail + 1) % c->cap;
    c->len++;
    pthread_cond_signal(&c->not_empty);
    pthread_mutex_unlock(&c->mutex);
    return 0;
}

void *pny_chan_recv(PnyChan *c) {
    if (!c) return NULL;
    pthread_mutex_lock(&c->mutex);
    while (c->len == 0 && !c->closed)
        pthread_cond_wait(&c->not_empty, &c->mutex);
    void *data = NULL;
    if (c->len > 0) {
        data = c->buf[c->head];
        c->buf[c->head] = NULL;
        c->head = (c->head + 1) % c->cap;
        c->len--;
        pthread_cond_signal(&c->not_full);
    }
    pthread_mutex_unlock(&c->mutex);
    return data;
}

bool pny_chan_closed(PnyChan *c) {
    if (!c) return true;
    return c->closed;
}

void pny_chan_close(PnyChan *c) {
    if (!c) return;
    pthread_mutex_lock(&c->mutex);
    c->closed = true;
    pthread_cond_broadcast(&c->not_empty);
    pthread_cond_broadcast(&c->not_full);
    pthread_mutex_unlock(&c->mutex);
}

size_t pny_chan_len(PnyChan *c) {
    if (!c) return 0;
    pthread_mutex_lock(&c->mutex);
    size_t l = c->len;
    pthread_mutex_unlock(&c->mutex);
    return l;
}

struct PnyMutex {
    pthread_mutex_t m;
};

PnyMutex *pny_mutex_new(void) {
    PnyMutex *m = (PnyMutex *)malloc(sizeof(PnyMutex));
    if (!m) return NULL;
    pthread_mutex_init(&m->m, NULL);
    return m;
}

void pny_mutex_free(PnyMutex *m) {
    if (!m) return;
    pthread_mutex_destroy(&m->m);
    free(m);
}

int pny_mutex_lock(PnyMutex *m) {
    if (!m) return -1;
    return pthread_mutex_lock(&m->m) == 0 ? 0 : -1;
}

int pny_mutex_unlock(PnyMutex *m) {
    if (!m) return -1;
    return pthread_mutex_unlock(&m->m) == 0 ? 0 : -1;
}

int pny_mutex_trylock(PnyMutex *m) {
    if (!m) return -1;
    return pthread_mutex_trylock(&m->m) == 0 ? 0 : -1;
}

void pny_atomic_store(PnyAtomicInt64 *a, int64_t v) {
    __atomic_store_n(&a->v, v, __ATOMIC_SEQ_CST);
}

int64_t pny_atomic_load(const PnyAtomicInt64 *a) {
    return __atomic_load_n(&a->v, __ATOMIC_SEQ_CST);
}

int64_t pny_atomic_add(PnyAtomicInt64 *a, int64_t delta) {
    return __atomic_fetch_add(&a->v, delta, __ATOMIC_SEQ_CST) + delta;
}

int64_t pny_atomic_cas(PnyAtomicInt64 *a, int64_t expected, int64_t desired) {
    int64_t old = __atomic_load_n(&a->v, __ATOMIC_SEQ_CST);
    if (old == expected) __atomic_store_n(&a->v, desired, __ATOMIC_SEQ_CST);
    return old;
}

/* ==================== Promise/Future ==================== */
struct PnyPromise {
    bool done;
    void *value;
    size_t value_size;
    void (*callback)(void *value, size_t size, void *ctx);
    void *cb_ctx;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

PnyPromise *pny_promise_new(void) {
    PnyPromise *p = (PnyPromise *)calloc(1, sizeof(PnyPromise));
    if (!p) return NULL;
    pthread_mutex_init(&p->mutex, NULL);
    pthread_cond_init(&p->cond, NULL);
    return p;
}

void pny_promise_free(PnyPromise *p) {
    if (!p) return;
    pthread_mutex_destroy(&p->mutex);
    pthread_cond_destroy(&p->cond);
    if (p->value) free(p->value);
    free(p);
}

int pny_promise_fulfill(PnyPromise *p, void *value, size_t size) {
    if (!p || p->done) return -1;
    pthread_mutex_lock(&p->mutex);
    if (size > 0 && value) {
        p->value = malloc(size);
        if (p->value) { memcpy(p->value, value, size); p->value_size = size; }
    }
    p->done = true;
    pthread_cond_broadcast(&p->cond);
    pthread_mutex_unlock(&p->mutex);
    if (p->callback) p->callback(p->value, p->value_size, p->cb_ctx);
    return 0;
}

bool pny_promise_is_done(const PnyPromise *p) { return p ? p->done : false; }

void *pny_promise_value(const PnyPromise *p, size_t *out_size) {
    if (!p || !p->done) return NULL;
    if (out_size) *out_size = p->value_size;
    return p->value;
}

int pny_promise_then(PnyPromise *p, void (*cb)(void *, size_t, void *), void *ctx) {
    if (!p || !cb) return -1;
    if (p->done) { cb(p->value, p->value_size, ctx); return 0; }
    p->callback = cb; p->cb_ctx = ctx;
    return 0;
}

/* Future */
struct PnyFuture {
    PnyPromise *promise;  /* 借用引用 */
};

PnyFuture *pny_future_from_promise(PnyPromise *p) {
    if (!p) return NULL;
    PnyFuture *f = (PnyFuture *)calloc(1, sizeof(PnyFuture));
    if (f) f->promise = p;
    return f;
}

void pny_future_free(PnyFuture *f) { free(f); }  /* 不free promise */

bool pny_future_is_done(const PnyFuture *f) { return f ? pny_promise_is_done(f->promise) : false; }

void *pny_future_value(const PnyFuture *f, size_t *out_size) {
    return f ? pny_promise_value(f->promise, out_size) : NULL;
}

int pny_future_wait(PnyFuture *f, int timeout_ms) {
    if (!f || !f->promise) return -1;
    PnyPromise *p = f->promise;
    pthread_mutex_lock(&p->mutex);
    if (!p->done) {
        if (timeout_ms < 0) {
            pthread_cond_wait(&p->cond, &p->mutex);
        } else {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += timeout_ms / 1000;
            ts.tv_nsec += (timeout_ms % 1000) * 1000000;
            if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
            pthread_cond_timedwait(&p->cond, &p->mutex, &ts);
        }
    }
    int ret = p->done ? 0 : -1;
    pthread_mutex_unlock(&p->mutex);
    return ret;
}

int pny_future_then(PnyFuture *f, void (*cb)(void *, size_t, void *), void *ctx) {
    return f ? pny_promise_then(f->promise, cb, ctx) : -1;
}

/* ==================== UDP ==================== */
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

struct PnyUdpSocket {
    int fd;
};

PnyUdpSocket *pny_udp_open(const char *bind_addr, int port) {
    PnyUdpSocket *s = (PnyUdpSocket *)calloc(1, sizeof(PnyUdpSocket));
    if (!s) return NULL;
    s->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s->fd < 0) { free(s); return NULL; }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = bind_addr ? inet_addr(bind_addr) : INADDR_ANY;
    if (bind(s->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(s->fd); free(s); return NULL;
    }
    return s;
}

void pny_udp_close(PnyUdpSocket *s) {
    if (!s) return;
    if (s->fd >= 0) close(s->fd);
    free(s);
}

int pny_udp_sendto(PnyUdpSocket *s, const void *data, size_t len,
                   const char *dest_addr, int dest_port) {
    if (!s || !data || !dest_addr) return -1;
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)dest_port);
    if (inet_pton(AF_INET, dest_addr, &addr.sin_addr) != 1) return -2;
    ssize_t sent = sendto(s->fd, data, len, 0, (struct sockaddr *)&addr, sizeof(addr));
    return (int)sent;
}

int pny_udp_recvfrom(PnyUdpSocket *s, void *buf, size_t buf_len,
                     char *src_addr, size_t src_addr_len, int *src_port) {
    if (!s || !buf) return -1;
    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(addr);
    ssize_t n = recvfrom(s->fd, buf, buf_len, 0, (struct sockaddr *)&addr, &addr_len);
    if (n < 0) return -2;
    if (src_addr && src_addr_len > 0) {
        inet_ntop(AF_INET, &addr.sin_addr, src_addr, (socklen_t)src_addr_len);
    }
    if (src_port) *src_port = ntohs(addr.sin_port);
    return (int)n;
}

int pny_udp_set_timeout(PnyUdpSocket *s, int timeout_ms) {
    if (!s) return -1;
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

/* ==================== DNS ==================== */
int pny_dns_resolve(const char *hostname, PnyDnsResult *result) {
    if (!hostname || !result) return -1;
    memset(result, 0, sizeof(PnyDnsResult));
    struct addrinfo hints = {0}, *res = NULL, *cur;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int rc = getaddrinfo(hostname, NULL, &hints, &res);
    if (rc != 0) return -2;
    int count = 0;
    for (cur = res; cur && count < 8; cur = cur->ai_next) {
        struct sockaddr_in *sin = (struct sockaddr_in *)cur->ai_addr;
        inet_ntop(AF_INET, &sin->sin_addr, result->addrs[count], 64);
        count++;
    }
    freeaddrinfo(res);
    result->count = count;
    return count;
}

/* ==================== Date ==================== */
int pny_date_now(PnyDateTime *out) {
    if (!out) return -1;
    time_t t = time(NULL);
    struct tm tm_buf;
    localtime_r(&t, &tm_buf);
    out->year = tm_buf.tm_year + 1900;
    out->month = tm_buf.tm_mon + 1;
    out->day = tm_buf.tm_mday;
    out->hour = tm_buf.tm_hour;
    out->minute = tm_buf.tm_min;
    out->second = tm_buf.tm_sec;
    out->weekday = tm_buf.tm_wday;
    return 0;
}

int pny_date_from_timestamp(int64_t ts, PnyDateTime *out) {
    if (!out) return -1;
    time_t t = (time_t)ts;
    struct tm tm_buf;
    localtime_r(&t, &tm_buf);
    out->year = tm_buf.tm_year + 1900;
    out->month = tm_buf.tm_mon + 1;
    out->day = tm_buf.tm_mday;
    out->hour = tm_buf.tm_hour;
    out->minute = tm_buf.tm_min;
    out->second = tm_buf.tm_sec;
    out->weekday = tm_buf.tm_wday;
    return 0;
}

int64_t pny_date_to_timestamp(const PnyDateTime *dt) {
    if (!dt) return -1;
    struct tm tm_buf = {0};
    tm_buf.tm_year = dt->year - 1900;
    tm_buf.tm_mon = dt->month - 1;
    tm_buf.tm_mday = dt->day;
    tm_buf.tm_hour = dt->hour;
    tm_buf.tm_min = dt->minute;
    tm_buf.tm_sec = dt->second;
    tm_buf.tm_isdst = -1;
    return (int64_t)mktime(&tm_buf);
}

const char *pny_date_format(const PnyDateTime *dt, const char *fmt, char *buf, size_t buf_len) {
    if (!dt || !fmt || !buf || buf_len == 0) return NULL;
    struct tm tm_buf = {0};
    tm_buf.tm_year = dt->year - 1900;
    tm_buf.tm_mon = dt->month - 1;
    tm_buf.tm_mday = dt->day;
    tm_buf.tm_hour = dt->hour;
    tm_buf.tm_min = dt->minute;
    tm_buf.tm_sec = dt->second;
    strftime(buf, buf_len, fmt, &tm_buf);
    return buf;
}

/* ==================== Complex ==================== */
PnyComplex pny_complex_new(double re, double im) {
    PnyComplex z = {re, im};
    return z;
}

PnyComplex pny_complex_add(PnyComplex a, PnyComplex b) {
    PnyComplex z = {a.re + b.re, a.im + b.im};
    return z;
}

PnyComplex pny_complex_sub(PnyComplex a, PnyComplex b) {
    PnyComplex z = {a.re - b.re, a.im - b.im};
    return z;
}

PnyComplex pny_complex_mul(PnyComplex a, PnyComplex b) {
    PnyComplex z = {a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re};
    return z;
}

PnyComplex pny_complex_div(PnyComplex a, PnyComplex b) {
    double d = b.re * b.re + b.im * b.im;
    if (d == 0.0) { PnyComplex z = {0, 0}; return z; }
    PnyComplex z = {(a.re * b.re + a.im * b.im) / d, (a.im * b.re - a.re * b.im) / d};
    return z;
}

double pny_complex_abs(PnyComplex z) { return sqrt(z.re * z.re + z.im * z.im); }
double pny_complex_arg(PnyComplex z) { return atan2(z.im, z.re); }

PnyComplex pny_complex_conj(PnyComplex z) {
    PnyComplex r = {z.re, -z.im};
    return r;
}

/* ==================== Statistics ==================== */
double pny_stats_mean(const double *data, size_t n) {
    if (!data || n == 0) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) sum += data[i];
    return sum / (double)n;
}

double pny_stats_variance(const double *data, size_t n) {
    if (!data || n < 2) return 0.0;
    double mean = pny_stats_mean(data, n);
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        double d = data[i] - mean;
        sum += d * d;
    }
    return sum / (double)(n - 1);
}

double pny_stats_stddev(const double *data, size_t n) {
    return sqrt(pny_stats_variance(data, n));
}

double pny_stats_min(const double *data, size_t n) {
    if (!data || n == 0) return 0.0;
    double m = data[0];
    for (size_t i = 1; i < n; i++) if (data[i] < m) m = data[i];
    return m;
}

double pny_stats_max(const double *data, size_t n) {
    if (!data || n == 0) return 0.0;
    double m = data[0];
    for (size_t i = 1; i < n; i++) if (data[i] > m) m = data[i];
    return m;
}

static int cmp_double(const void *a, const void *b) {
    double da = *(const double *)a, db = *(const double *)b;
    return (da > db) - (da < db);
}

double pny_stats_median(double *data, size_t n) {
    if (!data || n == 0) return 0.0;
    qsort(data, n, sizeof(double), cmp_double);
    if (n % 2 == 1) return data[n / 2];
    return (data[n / 2 - 1] + data[n / 2]) / 2.0;
}

/* ==================== MessagePack ==================== */
int pny_msgpack_write_nil(uint8_t *buf, size_t buf_len) {
    if (!buf || buf_len < 1) return -1;
    buf[0] = 0xc0;
    return 1;
}

int pny_msgpack_write_bool(uint8_t *buf, size_t buf_len, bool val) {
    if (!buf || buf_len < 1) return -1;
    buf[0] = val ? 0xc3 : 0xc2;
    return 1;
}

int pny_msgpack_write_uint(uint8_t *buf, size_t buf_len, uint64_t val) {
    if (!buf) return -1;
    if (val <= 0x7f) { if (buf_len < 1) return -1; buf[0] = (uint8_t)val; return 1; }
    if (val <= 0xff) { if (buf_len < 2) return -1; buf[0] = 0xcc; buf[1] = (uint8_t)val; return 2; }
    if (val <= 0xffff) { if (buf_len < 3) return -1; buf[0] = 0xcd; buf[1] = (uint8_t)(val>>8); buf[2] = (uint8_t)val; return 3; }
    if (val <= 0xffffffffULL) { if (buf_len < 5) return -1; buf[0] = 0xce; buf[1]=(uint8_t)(val>>24); buf[2]=(uint8_t)(val>>16); buf[3]=(uint8_t)(val>>8); buf[4]=(uint8_t)val; return 5; }
    if (buf_len < 9) return -1;
    buf[0] = 0xcf;
    for (int i = 0; i < 8; i++) buf[1+i] = (uint8_t)(val >> (56 - i*8));
    return 9;
}

int pny_msgpack_write_int(uint8_t *buf, size_t buf_len, int64_t val) {
    if (!buf) return -1;
    if (val >= 0) return pny_msgpack_write_uint(buf, buf_len, (uint64_t)val);
    if (val >= -32) { if (buf_len < 1) return -1; buf[0] = (uint8_t)(int8_t)val; return 1; }
    if (val >= -128) { if (buf_len < 2) return -1; buf[0] = 0xd0; buf[1] = (uint8_t)(int8_t)val; return 2; }
    if (val >= -32768) { if (buf_len < 3) return -1; buf[0] = 0xd1; buf[1]=(uint8_t)(val>>8); buf[2]=(uint8_t)val; return 3; }
    if (val >= -2147483648LL) { if (buf_len < 5) return -1; buf[0] = 0xd2; buf[1]=(uint8_t)(val>>24); buf[2]=(uint8_t)(val>>16); buf[3]=(uint8_t)(val>>8); buf[4]=(uint8_t)val; return 5; }
    if (buf_len < 9) return -1;
    buf[0] = 0xd3;
    for (int i = 0; i < 8; i++) buf[1+i] = (uint8_t)(val >> (56 - i*8));
    return 9;
}

int pny_msgpack_write_float(uint8_t *buf, size_t buf_len, float val) {
    if (!buf || buf_len < 5) return -1;
    buf[0] = 0xca;
    uint32_t bits; memcpy(&bits, &val, 4);
    for (int i = 0; i < 4; i++) buf[1+i] = (uint8_t)(bits >> (24 - i*8));
    return 5;
}

int pny_msgpack_write_double(uint8_t *buf, size_t buf_len, double val) {
    if (!buf || buf_len < 9) return -1;
    buf[0] = 0xcb;
    uint64_t bits; memcpy(&bits, &val, 8);
    for (int i = 0; i < 8; i++) buf[1+i] = (uint8_t)(bits >> (56 - i*8));
    return 9;
}

int pny_msgpack_write_str(uint8_t *buf, size_t buf_len, const char *str) {
    if (!buf || !str) return -1;
    size_t len = strlen(str);
    if (len <= 31) { if (buf_len < 1+len) return -1; buf[0] = (uint8_t)(0xa0|len); memcpy(buf+1, str, len); return (int)(1+len); }
    if (len <= 0xff) { if (buf_len < 2+len) return -1; buf[0]=0xd9; buf[1]=(uint8_t)len; memcpy(buf+2, str, len); return (int)(2+len); }
    if (len <= 0xffff) { if (buf_len < 3+len) return -1; buf[0]=0xda; buf[1]=(uint8_t)(len>>8); buf[2]=(uint8_t)len; memcpy(buf+3, str, len); return (int)(3+len); }
    if (buf_len < 5+len) return -1;
    buf[0]=0xdb; buf[1]=(uint8_t)(len>>24); buf[2]=(uint8_t)(len>>16); buf[3]=(uint8_t)(len>>8); buf[4]=(uint8_t)len;
    memcpy(buf+5, str, len); return (int)(5+len);
}

int pny_msgpack_write_bin(uint8_t *buf, size_t buf_len, const void *data, size_t len) {
    if (!buf || (!data && len > 0)) return -1;
    if (len <= 0xff) { if (buf_len < 2+len) return -1; buf[0]=0xc4; buf[1]=(uint8_t)len; if(len) memcpy(buf+2, data, len); return (int)(2+len); }
    if (len <= 0xffff) { if (buf_len < 3+len) return -1; buf[0]=0xc5; buf[1]=(uint8_t)(len>>8); buf[2]=(uint8_t)len; if(len) memcpy(buf+3, data, len); return (int)(3+len); }
    if (buf_len < 5+len) return -1;
    buf[0]=0xc6; buf[1]=(uint8_t)(len>>24); buf[2]=(uint8_t)(len>>16); buf[3]=(uint8_t)(len>>8); buf[4]=(uint8_t)len;
    if(len) memcpy(buf+5, data, len); return (int)(5+len);
}

int pny_msgpack_write_array_header(uint8_t *buf, size_t buf_len, uint32_t count) {
    if (!buf) return -1;
    if (count <= 15) { if (buf_len<1) return -1; buf[0]=(uint8_t)(0x90|count); return 1; }
    if (count <= 0xffff) { if (buf_len<3) return -1; buf[0]=0xdc; buf[1]=(uint8_t)(count>>8); buf[2]=(uint8_t)count; return 3; }
    if (buf_len<5) return -1;
    buf[0]=0xdd; buf[1]=(uint8_t)(count>>24); buf[2]=(uint8_t)(count>>16); buf[3]=(uint8_t)(count>>8); buf[4]=(uint8_t)count; return 5;
}

int pny_msgpack_write_map_header(uint8_t *buf, size_t buf_len, uint32_t count) {
    if (!buf) return -1;
    if (count <= 15) { if (buf_len<1) return -1; buf[0]=(uint8_t)(0x80|count); return 1; }
    if (count <= 0xffff) { if (buf_len<3) return -1; buf[0]=0xde; buf[1]=(uint8_t)(count>>8); buf[2]=(uint8_t)count; return 3; }
    if (buf_len<5) return -1;
    buf[0]=0xdf; buf[1]=(uint8_t)(count>>24); buf[2]=(uint8_t)(count>>16); buf[3]=(uint8_t)(count>>8); buf[4]=(uint8_t)count; return 5;
}

int pny_msgpack_read(const uint8_t *buf, size_t buf_len, PnyMsgpackValue *out) {
    if (!buf || buf_len < 1 || !out) return -1;
    uint8_t tag = buf[0];
    memset(out, 0, sizeof(*out));
    if (tag <= 0x7f) { out->type = PNY_MSGPACK_UINT; out->uint_val = tag; return 1; }
    if (tag >= 0xe0) { out->type = PNY_MSGPACK_INT; out->int_val = (int8_t)tag; return 1; }
    if (tag >= 0xa0 && tag <= 0xbf) { uint32_t len = tag & 0x1f; if (buf_len < 1+len) return -1; out->type = PNY_MSGPACK_STR; out->str_bin.ptr = buf+1; out->str_bin.len = len; return (int)(1+len); }
    if (tag >= 0x90 && tag <= 0x9f) { out->type = PNY_MSGPACK_ARRAY; out->count = tag & 0x0f; return 1; }
    if (tag >= 0x80 && tag <= 0x8f) { out->type = PNY_MSGPACK_MAP; out->count = tag & 0x0f; return 1; }
    switch (tag) {
        case 0xc0: out->type = PNY_MSGPACK_NIL; return 1;
        case 0xc2: out->type = PNY_MSGPACK_BOOL; out->bool_val = false; return 1;
        case 0xc3: out->type = PNY_MSGPACK_BOOL; out->bool_val = true; return 1;
        case 0xcc: if(buf_len<2) return -1; out->type=PNY_MSGPACK_UINT; out->uint_val=buf[1]; return 2;
        case 0xcd: if(buf_len<3) return -1; out->type=PNY_MSGPACK_UINT; out->uint_val=((uint64_t)buf[1]<<8)|buf[2]; return 3;
        case 0xce: if(buf_len<5) return -1; out->type=PNY_MSGPACK_UINT; out->uint_val=((uint64_t)buf[1]<<24)|((uint64_t)buf[2]<<16)|((uint64_t)buf[3]<<8)|buf[4]; return 5;
        case 0xcf: if(buf_len<9) return -1; out->type=PNY_MSGPACK_UINT; out->uint_val=0; for(int i=0;i<8;i++) out->uint_val=(out->uint_val<<8)|buf[1+i]; return 9;
        case 0xd0: if(buf_len<2) return -1; out->type=PNY_MSGPACK_INT; out->int_val=(int8_t)buf[1]; return 2;
        case 0xd1: if(buf_len<3) return -1; out->type=PNY_MSGPACK_INT; out->int_val=(int16_t)(((uint16_t)buf[1]<<8)|buf[2]); return 3;
        case 0xd2: if(buf_len<5) return -1; out->type=PNY_MSGPACK_INT; out->int_val=(int32_t)(((uint32_t)buf[1]<<24)|((uint32_t)buf[2]<<16)|((uint32_t)buf[3]<<8)|buf[4]); return 5;
        case 0xd3: if(buf_len<9) return -1; out->type=PNY_MSGPACK_INT; out->int_val=0; for(int i=0;i<8;i++) out->int_val=(out->int_val<<8)|buf[1+i]; return 9;
        case 0xca: if(buf_len<5) return -1; out->type=PNY_MSGPACK_FLOAT; {uint32_t b=0; for(int i=0;i<4;i++) b=(b<<8)|buf[1+i]; memcpy(&out->float_val,&b,4);} return 5;
        case 0xcb: if(buf_len<9) return -1; out->type=PNY_MSGPACK_DOUBLE; {uint64_t b=0; for(int i=0;i<8;i++) b=(b<<8)|buf[1+i]; memcpy(&out->double_val,&b,8);} return 9;
        case 0xd9: { if(buf_len<2) return -1; uint32_t l=buf[1]; if(buf_len<2+l) return -1; out->type=PNY_MSGPACK_STR; out->str_bin.ptr=buf+2; out->str_bin.len=l; return (int)(2+l); }
        case 0xda: { if(buf_len<3) return -1; uint32_t l=((uint32_t)buf[1]<<8)|buf[2]; if(buf_len<3+l) return -1; out->type=PNY_MSGPACK_STR; out->str_bin.ptr=buf+3; out->str_bin.len=l; return (int)(3+l); }
        case 0xdb: { if(buf_len<5) return -1; uint32_t l=((uint32_t)buf[1]<<24)|((uint32_t)buf[2]<<16)|((uint32_t)buf[3]<<8)|buf[4]; if(buf_len<5+l) return -1; out->type=PNY_MSGPACK_STR; out->str_bin.ptr=buf+5; out->str_bin.len=l; return (int)(5+l); }
        case 0xc4: { if(buf_len<2) return -1; uint32_t l=buf[1]; if(buf_len<2+l) return -1; out->type=PNY_MSGPACK_BIN; out->str_bin.ptr=buf+2; out->str_bin.len=l; return (int)(2+l); }
        case 0xc5: { if(buf_len<3) return -1; uint32_t l=((uint32_t)buf[1]<<8)|buf[2]; if(buf_len<3+l) return -1; out->type=PNY_MSGPACK_BIN; out->str_bin.ptr=buf+3; out->str_bin.len=l; return (int)(3+l); }
        case 0xdc: if(buf_len<3) return -1; out->type=PNY_MSGPACK_ARRAY; out->count=((uint32_t)buf[1]<<8)|buf[2]; return 3;
        case 0xdd: if(buf_len<5) return -1; out->type=PNY_MSGPACK_ARRAY; out->count=((uint32_t)buf[1]<<24)|((uint32_t)buf[2]<<16)|((uint32_t)buf[3]<<8)|buf[4]; return 5;
        case 0xde: if(buf_len<3) return -1; out->type=PNY_MSGPACK_MAP; out->count=((uint32_t)buf[1]<<8)|buf[2]; return 3;
        case 0xdf: if(buf_len<5) return -1; out->type=PNY_MSGPACK_MAP; out->count=((uint32_t)buf[1]<<24)|((uint32_t)buf[2]<<16)|((uint32_t)buf[3]<<8)|buf[4]; return 5;
        default: return -2;
    }
}

/* ==================== Stream ==================== */
struct PnyStream { uint8_t *buf; size_t cap, head, tail, count; bool closed; };

PnyStream *pny_stream_new(void) {
    PnyStream *st = (PnyStream *)calloc(1, sizeof(PnyStream));
    if (!st) return NULL;
    st->cap = 4096;
    st->buf = (uint8_t *)malloc(st->cap);
    if (!st->buf) { free(st); return NULL; }
    return st;
}

void pny_stream_free(PnyStream *st) { if (st) { free(st->buf); free(st); } }

int pny_stream_write(PnyStream *st, const void *data, size_t len) {
    if (!st || !data || st->closed) return -1;
    if (st->count + len > st->cap) {
        size_t nc = st->cap; while (nc < st->count + len) nc *= 2;
        uint8_t *nb = (uint8_t *)malloc(nc); if (!nb) return -2;
        for (size_t i = 0; i < st->count; i++) nb[i] = st->buf[(st->head + i) % st->cap];
        free(st->buf); st->buf = nb; st->cap = nc; st->head = 0; st->tail = st->count;
    }
    const uint8_t *src = (const uint8_t *)data;
    for (size_t i = 0; i < len; i++) { st->buf[st->tail] = src[i]; st->tail = (st->tail + 1) % st->cap; }
    st->count += len;
    return (int)len;
}

int pny_stream_read(PnyStream *st, void *buf, size_t buf_len) {
    if (!st || !buf) return -1;
    size_t tr = st->count < buf_len ? st->count : buf_len;
    uint8_t *dst = (uint8_t *)buf;
    for (size_t i = 0; i < tr; i++) { dst[i] = st->buf[st->head]; st->head = (st->head + 1) % st->cap; }
    st->count -= tr;
    if (tr == 0 && st->closed) return -2;
    return (int)tr;
}

size_t pny_stream_available(const PnyStream *st) { return st ? st->count : 0; }
void pny_stream_close(PnyStream *st) { if (st) st->closed = true; }
bool pny_stream_is_closed(const PnyStream *st) { return st ? st->closed : true; }

/* ==================== Error ==================== */
const char *pny_error_str(PnyErrorCode code) {
    switch (code) {
        case PNY_ERR_NONE: return "no error";
        case PNY_ERR_EOF: return "end of file";
        case PNY_ERR_TIMEOUT: return "timeout";
        case PNY_ERR_PERMISSION: return "permission denied";
        case PNY_ERR_NOT_FOUND: return "not found";
        case PNY_ERR_ALREADY_EXISTS: return "already exists";
        case PNY_ERR_INVALID_ARG: return "invalid argument";
        case PNY_ERR_IO: return "I/O error";
        case PNY_ERR_NO_MEMORY: return "out of memory";
        default: return "unknown error";
    }
}

/* ==================== Mailbox ==================== */
struct PnyMailbox { void **items; size_t cap, head, tail, count, max; };

PnyMailbox *pny_mailbox_new(size_t max_size) {
    PnyMailbox *mb = (PnyMailbox *)calloc(1, sizeof(PnyMailbox));
    if (!mb) return NULL;
    mb->cap = max_size > 0 ? max_size : 256;
    mb->items = (void **)calloc(mb->cap, sizeof(void *));
    if (!mb->items) { free(mb); return NULL; }
    mb->max = max_size;
    return mb;
}

void pny_mailbox_free(PnyMailbox *mb) { if (mb) { free(mb->items); free(mb); } }

bool pny_mailbox_put(PnyMailbox *mb, void *msg) {
    if (!mb) return false;
    if (mb->max > 0 && mb->count >= mb->max) return false;
    if (mb->count >= mb->cap) {
        size_t nc = mb->cap * 2;
        void **ni = (void **)calloc(nc, sizeof(void *));
        if (!ni) return false;
        for (size_t i = 0; i < mb->count; i++) ni[i] = mb->items[(mb->head + i) % mb->cap];
        free(mb->items); mb->items = ni; mb->cap = nc; mb->head = 0; mb->tail = mb->count;
    }
    mb->items[mb->tail] = msg;
    mb->tail = (mb->tail + 1) % mb->cap;
    mb->count++;
    return true;
}

void *pny_mailbox_take(PnyMailbox *mb) {
    if (!mb || mb->count == 0) return NULL;
    void *msg = mb->items[mb->head];
    mb->head = (mb->head + 1) % mb->cap;
    mb->count--;
    return msg;
}

size_t pny_mailbox_size(const PnyMailbox *mb) { return mb ? mb->count : 0; }
bool pny_mailbox_is_empty(const PnyMailbox *mb) { return mb ? mb->count == 0 : true; }

/* ==================== Group ==================== */
struct PnyGroup { char name[64]; int *ids; size_t count, cap; };

PnyGroup *pny_group_new(const char *name) {
    PnyGroup *g = (PnyGroup *)calloc(1, sizeof(PnyGroup));
    if (!g) return NULL;
    if (name) strncpy(g->name, name, sizeof(g->name) - 1);
    g->cap = 16;
    g->ids = (int *)calloc(g->cap, sizeof(int));
    if (!g->ids) { free(g); return NULL; }
    return g;
}

void pny_group_free(PnyGroup *g) { if (g) { free(g->ids); free(g); } }

int pny_group_add(PnyGroup *g, int actor_id) {
    if (!g) return -1;
    if (pny_group_contains(g, actor_id)) return -2;
    if (g->count >= g->cap) {
        size_t nc = g->cap * 2;
        int *ni = (int *)realloc(g->ids, nc * sizeof(int));
        if (!ni) return -3;
        g->ids = ni; g->cap = nc;
    }
    g->ids[g->count++] = actor_id;
    return 0;
}

int pny_group_remove(PnyGroup *g, int actor_id) {
    if (!g) return -1;
    for (size_t i = 0; i < g->count; i++) {
        if (g->ids[i] == actor_id) { g->ids[i] = g->ids[g->count - 1]; g->count--; return 0; }
    }
    return -2;
}

size_t pny_group_size(const PnyGroup *g) { return g ? g->count : 0; }

bool pny_group_contains(const PnyGroup *g, int actor_id) {
    if (!g) return false;
    for (size_t i = 0; i < g->count; i++) if (g->ids[i] == actor_id) return true;
    return false;
}

const char *pny_group_name(const PnyGroup *g) { return g ? g->name : NULL; }

int pny_group_members(const PnyGroup *g, int *out_ids, size_t max) {
    if (!g || !out_ids) return -1;
    size_t n = g->count < max ? g->count : max;
    for (size_t i = 0; i < n; i++) out_ids[i] = g->ids[i];
    return (int)n;
}

/* ==================== Test ==================== */

PnyTestSuite *pny_test_suite_new(const char *name) {
    PnyTestSuite *s = (PnyTestSuite *)malloc(sizeof(PnyTestSuite));
    if (!s) return NULL;
    s->name = name ? strdup(name) : NULL;
    s->tests = NULL;
    s->count = 0;
    s->report.total = 0;
    s->report.passed = 0;
    s->report.failed = 0;
    s->report.skipped = 0;
    return s;
}

void pny_test_suite_free(PnyTestSuite *s) {
    if (!s) return;
    free(s->name);
    free(s->tests);
    free(s);
}

void pny_test_suite_add(PnyTestSuite *s, TestCase tc) {
    if (!s) return;
    s->tests = (TestCase *)realloc(s->tests, sizeof(TestCase) * (size_t)(s->count + 1));
    if (!s->tests) return;
    s->tests[s->count++] = tc;
}

void pny_test_run_suite(PnyTestSuite *s, bool verbose) {
    if (!s) return;
    s->report.total = 0;
    s->report.passed = 0;
    s->report.failed = 0;
    s->report.skipped = 0;
    for (int i = 0; i < s->count; i++) {
        s->report.total++;
        if (verbose) printf("  RUN %s::test_%d\n", s->name, i);
        s->tests[i]();
        s->report.passed++;
        if (verbose) printf("  ✓ PASS\n");
    }
    pny_test_report_print(&s->report);
}

void pny_test_report_print(const TestReport *r) {
    if (!r) return;
    printf("[TEST] total=%d passed=%d failed=%d skipped=%d\n",
           r->total, r->passed, r->failed, r->skipped);
}

void pny_assert_true(bool cond, AssertCtx ctx) {
    if (!cond) printf("[ASSERT FAIL] %s:%d %s\n", ctx.file, ctx.line, ctx.msg ? ctx.msg : "");
}

void pny_assert_eq_int(int64_t a, int64_t b, AssertCtx ctx) {
    if (a != b) printf("[ASSERT FAIL] %s:%d %s (got %lld, want %lld)\n",
                       ctx.file, ctx.line, ctx.msg ? ctx.msg : "", (long long)a, (long long)b);
}

void pny_assert_eq_str(const char *a, const char *b, AssertCtx ctx) {
    if (strcmp(a ? a : "", b ? b : "") != 0)
        printf("[ASSERT FAIL] %s:%d %s\n", ctx.file, ctx.line, ctx.msg ? ctx.msg : "");
}

void pny_assert_null(void *p, AssertCtx ctx) {
    if (p) printf("[ASSERT FAIL] %s:%d expected NULL\n", ctx.file, ctx.line);
}

void pny_assert_not_null(void *p, AssertCtx ctx) {
    if (!p) printf("[ASSERT FAIL] %s:%d expected non-NULL\n", ctx.file, ctx.line);
}

/* ==================== JSON ==================== */

static char *json_strdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *d = (char *)malloc(n + 1);
    if (!d) return NULL;
    memcpy(d, s, n + 1);
    return d;
}

JsonValue *json_new_null(void) {
    JsonValue *v = (JsonValue *)calloc(1, sizeof(JsonValue));
    if (v) v->type = JSON_NULL;
    return v;
}

JsonValue *json_new_bool(bool b) {
    JsonValue *v = (JsonValue *)calloc(1, sizeof(JsonValue));
    if (v) { v->type = JSON_BOOL; v->b = b; }
    return v;
}

JsonValue *json_new_int(int64_t i) {
    JsonValue *v = (JsonValue *)calloc(1, sizeof(JsonValue));
    if (v) { v->type = JSON_INT; v->i = i; }
    return v;
}

JsonValue *json_new_double(double d) {
    JsonValue *v = (JsonValue *)calloc(1, sizeof(JsonValue));
    if (v) { v->type = JSON_DOUBLE; v->d = d; }
    return v;
}

JsonValue *json_new_string(const char *s) {
    JsonValue *v = (JsonValue *)calloc(1, sizeof(JsonValue));
    if (!v) return NULL;
    v->type = JSON_STRING;
    v->s = json_strdup(s ? s : "");
    return v;
}

JsonValue *json_new_array(void) {
    JsonValue *v = (JsonValue *)calloc(1, sizeof(JsonValue));
    if (v) v->type = JSON_ARRAY;
    return v;
}

JsonValue *json_new_object(void) {
    JsonValue *v = (JsonValue *)calloc(1, sizeof(JsonValue));
    if (v) v->type = JSON_OBJECT;
    return v;
}

int json_arr_push(JsonValue *arr, JsonValue *val) {
    if (!arr || arr->type != JSON_ARRAY || !val) return -1;
    JsonValue **items = (JsonValue **)realloc(arr->arr.items, sizeof(JsonValue *) * (arr->arr.count + 1));
    if (!items) return -1;
    arr->arr.items = items;
    arr->arr.items[arr->arr.count] = val;
    arr->arr.count++;
    return 0;
}

int json_obj_set(JsonValue *obj, const char *key, JsonValue *val) {
    if (!obj || obj->type != JSON_OBJECT || !key || !val) return -1;
    char **keys = (char **)realloc(obj->obj.keys, sizeof(char *) * (obj->obj.count + 1));
    if (!keys) return -1;
    JsonValue **vals = (JsonValue **)realloc(obj->obj.vals, sizeof(JsonValue *) * (obj->obj.count + 1));
    if (!vals) return -1;
    obj->obj.keys = keys;
    obj->obj.vals = vals;
    obj->obj.keys[obj->obj.count] = json_strdup(key);
    obj->obj.vals[obj->obj.count] = val;
    obj->obj.count++;
    return 0;
}

JsonValue *json_obj_get(const JsonValue *obj, const char *key) {
    if (!obj || obj->type != JSON_OBJECT || !key) return NULL;
    for (size_t i = 0; i < obj->obj.count; i++) {
        if (strcmp(obj->obj.keys[i], key) == 0) return obj->obj.vals[i];
    }
    return NULL;
}

bool json_obj_has(const JsonValue *obj, const char *key) {
    return json_obj_get(obj, key) != NULL;
}

void json_free(JsonValue *v) {
    if (!v) return;
    switch (v->type) {
        case JSON_STRING: free(v->s); break;
        case JSON_ARRAY:
            for (size_t i = 0; i < v->arr.count; i++) json_free(v->arr.items[i]);
            free(v->arr.items);
            break;
        case JSON_OBJECT:
            for (size_t i = 0; i < v->obj.count; i++) {
                free(v->obj.keys[i]);
                json_free(v->obj.vals[i]);
            }
            free(v->obj.keys);
            free(v->obj.vals);
            break;
        default: break;
    }
    free(v);
}

/* ---- JSON Parser ---- */

typedef struct { const char *p; const char *end; const char *start; } JsonParser;

static void json_skip_ws(JsonParser *pp);
static JsonValue *json_parse_value(JsonParser *pp);
static JsonValue *json_parse_string(JsonParser *pp);
static JsonValue *json_parse_number(JsonParser *pp);
static JsonValue *json_parse_array(JsonParser *pp);
static JsonValue *json_parse_object(JsonParser *pp);

static void json_skip_ws(JsonParser *pp) {
    while (pp->p < pp->end && (*pp->p == ' ' || *pp->p == '\t' || *pp->p == '\n' || *pp->p == '\r')) pp->p++;
}

static JsonValue *json_parse_value(JsonParser *pp) {
    json_skip_ws(pp);
    if (pp->p >= pp->end) return NULL;
    char c = *pp->p;
    if (c == 'n') {
        if (pp->end - pp->p >= 4 && strncmp(pp->p, "null", 4) == 0) { pp->p += 4; return json_new_null(); }
        return NULL;
    }
    if (c == 't') {
        if (pp->end - pp->p >= 4 && strncmp(pp->p, "true", 4) == 0) { pp->p += 4; return json_new_bool(true); }
        return NULL;
    }
    if (c == 'f') {
        if (pp->end - pp->p >= 5 && strncmp(pp->p, "false", 5) == 0) { pp->p += 5; return json_new_bool(false); }
        return NULL;
    }
    if (c == '"') return json_parse_string(pp);
    if (c == '[') return json_parse_array(pp);
    if (c == '{') return json_parse_object(pp);
    if (c == '-' || (c >= '0' && c <= '9')) return json_parse_number(pp);
    return NULL;
}

static JsonValue *json_parse_string(JsonParser *pp) {
    if (pp->p >= pp->end || *pp->p != '"') return NULL;
    pp->p++;
    char buf[4096];
    size_t n = 0;
    while (pp->p < pp->end && *pp->p != '"' && n < sizeof(buf) - 1) {
        char c = *pp->p;
        if (c == '\\') {
            pp->p++;
            if (pp->p >= pp->end) return NULL;
            switch (*pp->p) {
                case '"': buf[n++] = '"'; break;
                case '\\': buf[n++] = '\\'; break;
                case '/': buf[n++] = '/'; break;
                case 'b': buf[n++] = '\b'; break;
                case 'f': buf[n++] = '\f'; break;
                case 'n': buf[n++] = '\n'; break;
                case 'r': buf[n++] = '\r'; break;
                case 't': buf[n++] = '\t'; break;
                case 'u':
                    if (pp->end - pp->p >= 5) {
                        char hex[5] = {pp->p[1], pp->p[2], pp->p[3], pp->p[4], 0};
                        pp->p += 4;
                        buf[n++] = (char)strtol(hex, NULL, 16);
                    } else return NULL;
                    break;
                default: return NULL;
            }
        } else {
            buf[n++] = c;
        }
        pp->p++;
    }
    if (pp->p >= pp->end || *pp->p != '"') return NULL;
    pp->p++;
    buf[n] = '\0';
    return json_new_string(buf);
}

static JsonValue *json_parse_number(JsonParser *pp) {
    const char *start = pp->p;
    bool is_float = false;
    if (*pp->p == '-') pp->p++;
    while (pp->p < pp->end && *pp->p >= '0' && *pp->p <= '9') pp->p++;
    if (pp->p < pp->end && *pp->p == '.') { is_float = true; pp->p++; while (pp->p < pp->end && *pp->p >= '0' && *pp->p <= '9') pp->p++; }
    if (pp->p < pp->end && (*pp->p == 'e' || *pp->p == 'E')) { is_float = true; pp->p++; if (*pp->p == '+' || *pp->p == '-') pp->p++; while (pp->p < pp->end && *pp->p >= '0' && *pp->p <= '9') pp->p++; }
    size_t n = (size_t)(pp->p - start);
    char buf[128];
    if (n >= sizeof(buf)) return NULL;
    memcpy(buf, start, n);
    buf[n] = '\0';
    if (is_float) return json_new_double(strtod(buf, NULL));
    return json_new_int(strtoll(buf, NULL, 10));
}

static JsonValue *json_parse_array(JsonParser *pp) {
    if (pp->p >= pp->end || *pp->p != '[') return NULL;
    pp->p++;
    JsonValue *arr = json_new_array();
    if (!arr) return NULL;
    json_skip_ws(pp);
    if (pp->p < pp->end && *pp->p == ']') { pp->p++; return arr; }
    while (pp->p < pp->end) {
        JsonValue *v = json_parse_value(pp);
        if (!v) { json_free(arr); return NULL; }
        if (json_arr_push(arr, v) != 0) { json_free(v); json_free(arr); return NULL; }
        json_skip_ws(pp);
        if (pp->p < pp->end && *pp->p == ',') { pp->p++; json_skip_ws(pp); continue; }
        if (pp->p < pp->end && *pp->p == ']') { pp->p++; return arr; }
        json_free(arr); return NULL;
    }
    json_free(arr); return NULL;
}

static JsonValue *json_parse_object(JsonParser *pp) {
    if (pp->p >= pp->end || *pp->p != '{') return NULL;
    pp->p++;
    JsonValue *obj = json_new_object();
    if (!obj) return NULL;
    json_skip_ws(pp);
    if (pp->p < pp->end && *pp->p == '}') { pp->p++; return obj; }
    while (pp->p < pp->end) {
        json_skip_ws(pp);
        JsonValue *kv = json_parse_string(pp);
        if (!kv || kv->type != JSON_STRING) { json_free(kv); json_free(obj); return NULL; }
        const char *key = kv->s;
        json_skip_ws(pp);
        if (pp->p >= pp->end || *pp->p != ':') { json_free(kv); json_free(obj); return NULL; }
        pp->p++;
        JsonValue *val = json_parse_value(pp);
        if (!val) { json_free(kv); json_free(obj); return NULL; }
        if (json_obj_set(obj, key, val) != 0) { json_free(kv); json_free(val); json_free(obj); return NULL; }
        json_free(kv);
        json_skip_ws(pp);
        if (pp->p < pp->end && *pp->p == ',') { pp->p++; continue; }
        if (pp->p < pp->end && *pp->p == '}') { pp->p++; return obj; }
        json_free(obj); return NULL;
    }
    json_free(obj); return NULL;
}

JsonValue *json_parse(const char *input, size_t len) {
    if (!input || len == 0) return NULL;
    JsonParser pp = { input, input + len, input };
    return json_parse_value(&pp);
}

/* ---- JSON Stringify ---- */

typedef struct { char *buf; size_t len; size_t cap; } JsonBuf;

static int json_buf_write(JsonBuf *b, const char *s, size_t n) {
    if (b->len + n + 1 > b->cap) {
        size_t new_cap = (b->cap * 2) + n + 1;
        if (new_cap < 1024) new_cap = 1024;
        char *new_buf = (char *)realloc(b->buf, new_cap);
        if (!new_buf) return -1;
        b->buf = new_buf;
        b->cap = new_cap;
    }
    memcpy(b->buf + b->len, s, n);
    b->len += n;
    b->buf[b->len] = '\0';
    return 0;
}

static int json_write_string(JsonBuf *b, const char *s) {
    json_buf_write(b, "\"", 1);
    for (const char *p = s; *p; p++) {
        switch (*p) {
            case '"': json_buf_write(b, "\\\"", 2); break;
            case '\\': json_buf_write(b, "\\\\", 2); break;
            case '\b': json_buf_write(b, "\\b", 2); break;
            case '\f': json_buf_write(b, "\\f", 2); break;
            case '\n': json_buf_write(b, "\\n", 2); break;
            case '\r': json_buf_write(b, "\\r", 2); break;
            case '\t': json_buf_write(b, "\\t", 2); break;
            default:
                if ((unsigned char)*p < 0x20) {
                    char esc[8];
                    snprintf(esc, sizeof(esc), "\\u%04x", (unsigned char)*p);
                    json_buf_write(b, esc, 6);
                } else {
                    json_buf_write(b, p, 1);
                }
                break;
        }
    }
    json_buf_write(b, "\"", 1);
    return 0;
}

static int json_write_value(JsonBuf *b, const JsonValue *v) {
    if (!v) { json_buf_write(b, "null", 4); return 0; }
    char buf[128];
    switch (v->type) {
        case JSON_NULL: json_buf_write(b, "null", 4); break;
        case JSON_BOOL: json_buf_write(b, v->b ? "true" : "false", v->b ? 4 : 5); break;
        case JSON_INT: snprintf(buf, sizeof(buf), "%lld", (long long)v->i); json_buf_write(b, buf, strlen(buf)); break;
        case JSON_DOUBLE: snprintf(buf, sizeof(buf), "%g", v->d); json_buf_write(b, buf, strlen(buf)); break;
        case JSON_STRING: json_write_string(b, v->s ? v->s : ""); break;
        case JSON_ARRAY:
            json_buf_write(b, "[", 1);
            for (size_t i = 0; i < v->arr.count; i++) {
                if (i > 0) json_buf_write(b, ",", 1);
                json_write_value(b, v->arr.items[i]);
            }
            json_buf_write(b, "]", 1);
            break;
        case JSON_OBJECT:
            json_buf_write(b, "{", 1);
            for (size_t i = 0; i < v->obj.count; i++) {
                if (i > 0) json_buf_write(b, ",", 1);
                json_write_string(b, v->obj.keys[i]);
                json_buf_write(b, ":", 1);
                json_write_value(b, v->obj.vals[i]);
            }
            json_buf_write(b, "}", 1);
            break;
    }
    return 0;
}

char *json_stringify(const JsonValue *v) {
    JsonBuf b = { NULL, 0, 0 };
    if (json_write_value(&b, v) != 0) { free(b.buf); return NULL; }
    return b.buf;
}

/* ==================== Timer ==================== */

#ifdef __linux__
#include <time.h>
#include <sys/time.h>
#endif

typedef struct PnyTimer {
    int64_t interval_ms;
    TimerCallback cb;
    void *ctx;
    bool repeat;
    bool running;
} PnyTimer;

PnyTimer *pny_timer_new(int64_t interval_ms, TimerCallback cb, void *ctx, bool repeat) {
    PnyTimer *t = (PnyTimer *)calloc(1, sizeof(PnyTimer));
    if (!t) return NULL;
    t->interval_ms = interval_ms;
    t->cb = cb;
    t->ctx = ctx;
    t->repeat = repeat;
    t->running = false;
    return t;
}

void pny_timer_free(PnyTimer *t) {
    if (!t) return;
    free(t);
}

void pny_timer_start(PnyTimer *t) {
    if (!t) return;
    t->running = true;
}

void pny_timer_stop(PnyTimer *t) {
    if (!t) return;
    t->running = false;
}

bool pny_timer_running(const PnyTimer *t) {
    return t && t->running;
}

int64_t pny_timer_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int64_t timer_start_time = 0;
static bool timer_start_time_init = false;

int64_t pny_timer_elapsed_ms(void) {
    if (!timer_start_time_init) {
        timer_start_time = pny_timer_now_ms();
        timer_start_time_init = true;
    }
    return pny_timer_now_ms() - timer_start_time;
}

/* ==================== Logger ==================== */

typedef struct PnyLogger {
    char *name;
    LogLevel level;
} PnyLogger;

static const char *log_level_str(LogLevel level) {
    switch (level) {
        case LOG_TRACE: return "TRACE";
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARN: return "WARN";
        case LOG_ERROR: return "ERROR";
        case LOG_FATAL: return "FATAL";
        default: return "?????";
    }
}

PnyLogger *pny_logger_new(const char *name) {
    PnyLogger *l = (PnyLogger *)calloc(1, sizeof(PnyLogger));
    if (!l) return NULL;
    l->name = json_strdup(name ? name : "root");
    l->level = LOG_INFO;
    return l;
}

void pny_logger_free(PnyLogger *l) {
    if (!l) return;
    free(l->name);
    free(l);
}

void pny_logger_set_level(PnyLogger *l, LogLevel level) {
    if (l) l->level = level;
}

LogLevel pny_logger_get_level(const PnyLogger *l) {
    return l ? l->level : LOG_INFO;
}

int pny_logger_log(PnyLogger *l, LogLevel level, const char *fmt, ...) {
    if (!l || level < l->level) return -1;
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return -1;
    printf("[%s] [%s] %s: %s\n", log_level_str(level), l->name ? l->name : "root", "", buf);
    return n;
}

int pny_logger_trace(PnyLogger *l, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[4096];
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return -1;
    if (l && LOG_TRACE >= l->level) {
        printf("[TRACE] [%s] %s\n", l->name ? l->name : "root", buf);
        return n;
    }
    return -1;
}

int pny_logger_debug(PnyLogger *l, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[4096];
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return -1;
    if (l && LOG_DEBUG >= l->level) {
        printf("[DEBUG] [%s] %s\n", l->name ? l->name : "root", buf);
        return n;
    }
    return -1;
}

int pny_logger_info(PnyLogger *l, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[4096];
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return -1;
    if (l && LOG_INFO >= l->level) {
        printf("[INFO]  [%s] %s\n", l->name ? l->name : "root", buf);
        return n;
    }
    return -1;
}

int pny_logger_warn(PnyLogger *l, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[4096];
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return -1;
    if (l && LOG_WARN >= l->level) {
        printf("[WARN]  [%s] %s\n", l->name ? l->name : "root", buf);
        return n;
    }
    return -1;
}

int pny_logger_error(PnyLogger *l, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[4096];
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return -1;
    if (l && LOG_ERROR >= l->level) {
        fprintf(stderr, "[ERROR] [%s] %s\n", l->name ? l->name : "root", buf);
        return n;
    }
    return -1;
}

int pny_logger_fatal(PnyLogger *l, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[4096];
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return -1;
    if (l && LOG_FATAL >= l->level) {
        fprintf(stderr, "[FATAL] [%s] %s\n", l->name ? l->name : "root", buf);
        return n;
    }
    return -1;
}

/* ==================== Math ==================== */

double pny_math_sqrt(double x) { return sqrt(x); }
double pny_math_pow(double base, double exp) { return pow(base, exp); }
double pny_math_sin(double x) { return sin(x); }
double pny_math_cos(double x) { return cos(x); }
double pny_math_tan(double x) { return tan(x); }
double pny_math_asin(double x) { return asin(x); }
double pny_math_acos(double x) { return acos(x); }
double pny_math_atan(double x) { return atan(x); }
double pny_math_atan2(double y, double x) { return atan2(y, x); }
double pny_math_log(double x) { return log(x); }
double pny_math_log2(double x) { return log2(x); }
double pny_math_exp(double x) { return exp(x); }
double pny_math_ceil(double x) { return ceil(x); }
double pny_math_floor(double x) { return floor(x); }
double pny_math_abs(double x) { return fabs(x); }
int64_t pny_math_abs_int(int64_t x) { return x < 0 ? -x : x; }
double pny_math_fmod(double x, double y) { return fmod(x, y); }
int64_t pny_math_min(int64_t a, int64_t b) { return a < b ? a : b; }
int64_t pny_math_max(int64_t a, int64_t b) { return a > b ? a : b; }
int64_t pny_math_factorial(int64_t n) {
    if (n <= 1) return 1;
    if (n > 20) return -1; /* overflow guard */
    int64_t result = 1;
    for (int64_t i = 2; i <= n; i++) result *= i;
    return result;
}

static uint64_t pny_rand_state = 0x12345678ULL;

double pny_math_random(void) {
    /* xoshiro256** simplified */
    pny_rand_state ^= pny_rand_state >> 12;
    pny_rand_state ^= pny_rand_state << 25;
    pny_rand_state ^= pny_rand_state >> 27;
    return (double)(pny_rand_state & 0x3FFFFFFFFFFFFFFFULL) / (double)0x4000000000000000ULL;
}

int64_t pny_math_random_int(int64_t max) {
    if (max <= 0) return 0;
    return (int64_t)(pny_math_random() * (double)max);
}


/* ==================== UUID ==================== */

#include <time.h>

int pny_uuid_v4(char *buf, size_t buf_size) {
    if (!buf || buf_size < 37) return -1;
    srand((unsigned)time(NULL) ^ (unsigned)(uintptr_t)buf);
    uint8_t bytes[16];
    for (int i = 0; i < 16; i++) bytes[i] = (uint8_t)(rand() & 0xFF);
    bytes[6] = (bytes[6] & 0x0F) | 0x40; /* version 4 */
    bytes[8] = (bytes[8] & 0x3F) | 0x80; /* variant */
    snprintf(buf, 37, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             bytes[0],bytes[1],bytes[2],bytes[3],bytes[4],bytes[5],bytes[6],bytes[7],
             bytes[8],bytes[9],bytes[10],bytes[11],bytes[12],bytes[13],bytes[14],bytes[15]);
    return 0;
}

int pny_uuid_short(char *buf, size_t buf_size) {
    if (!buf || buf_size < 17) return -1;
    srand((unsigned)time(NULL) ^ (unsigned)(uintptr_t)buf);
    uint8_t bytes[8];
    for (int i = 0; i < 8; i++) bytes[i] = (uint8_t)(rand() & 0xFF);
    for (int i = 0; i < 8; i++) sprintf(buf + i*2, "%02x", bytes[i]);
    buf[16] = '\0';
    return 0;
}

/* ==================== Base64 ==================== */

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int pny_base64_encode(const void *data, size_t len, char *out, size_t out_size) {
    if (!data || !out) return -1;
    size_t need = ((len + 2) / 3) * 4 + 1;
    if (out_size < need) return -1;
    
    const uint8_t *in = (const uint8_t *)data;
    size_t o = 0;
    for (size_t i = 0; i < len; i += 3) {
        uint32_t v = (uint32_t)in[i] << 16;
        if (i+1 < len) v |= (uint32_t)in[i+1] << 8;
        if (i+2 < len) v |= (uint32_t)in[i+2];
        
        out[o++] = b64_table[(v >> 18) & 0x3F];
        out[o++] = b64_table[(v >> 12) & 0x3F];
        out[o++] = (i+1 < len) ? b64_table[(v >> 6) & 0x3F] : '=';
        out[o++] = (i+2 < len) ? b64_table[v & 0x3F] : '=';
    }
    out[o] = '\0';
    return (int)o;
}

static int b64_val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

int pny_base64_decode(const char *input, size_t len, void *out, size_t out_size) {
    if (!input || !out) return -1;
    if (out_size < (len / 4) * 3) return -1;
    
    uint8_t *o = (uint8_t *)out;
    size_t oi = 0;
    for (size_t i = 0; i < len; i += 4) {
        int v[4];
        for (int j = 0; j < 4; j++) {
            v[j] = (i+j < len && input[i+j] != '=') ? b64_val(input[i+j]) : 0;
            if (v[j] < 0 && input[i+j] != '=') return -1;
        }
        uint32_t val = ((uint32_t)v[0] << 18) | ((uint32_t)v[1] << 12) | ((uint32_t)v[2] << 6) | (uint32_t)v[3];
        o[oi++] = (uint8_t)(val >> 16);
        if (i+2 < len && input[i+2] != '=') o[oi++] = (uint8_t)(val >> 8);
        if (i+3 < len && input[i+3] != '=') o[oi++] = (uint8_t)val;
    }
    return (int)oi;
}

/* ==================== Hex ==================== */

static const char hex_table[] = "0123456789abcdef";

int pny_hex_encode(const void *data, size_t len, char *out, size_t out_size) {
    if (!data || !out) return -1;
    if (out_size < len * 2 + 1) return -1;
    
    const uint8_t *in = (const uint8_t *)data;
    for (size_t i = 0; i < len; i++) {
        out[i*2] = hex_table[in[i] >> 4];
        out[i*2+1] = hex_table[in[i] & 0x0F];
    }
    out[len*2] = '\0';
    return (int)(len * 2);
}

int pny_hex_decode(const char *input, size_t len, void *out, size_t out_size) {
    if (!input || !out) return -1;
    if (len % 2 != 0) return -1;
    if (out_size < len / 2) return -1;
    
    uint8_t *o = (uint8_t *)out;
    for (size_t i = 0; i < len; i += 2) {
        int hi = -1, lo = -1;
        char c1 = input[i], c2 = input[i+1];
        if (c1 >= '0' && c1 <= '9') hi = c1 - '0';
        else if (c1 >= 'a' && c1 <= 'f') hi = c1 - 'a' + 10;
        else if (c1 >= 'A' && c1 <= 'F') hi = c1 - 'A' + 10;
        else return -1;
        if (c2 >= '0' && c2 <= '9') lo = c2 - '0';
        else if (c2 >= 'a' && c2 <= 'f') lo = c2 - 'a' + 10;
        else if (c2 >= 'A' && c2 <= 'F') lo = c2 - 'A' + 10;
        else return -1;
        o[i/2] = (uint8_t)(hi * 16 + lo);
    }
    return (int)(len / 2);
}

/* ==================== CRC32 ==================== */

static uint32_t crc32_table[256];
static bool crc32_table_init = false;

static void crc32_init_table(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++)
            c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        crc32_table[i] = c;
    }
    crc32_table_init = true;
}

uint32_t pny_crc32(const void *data, size_t len) {
    if (!data) return 0;
    if (!crc32_table_init) crc32_init_table();
    
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++)
        crc = crc32_table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}

/* ==================== String Utilities ==================== */

int pny_str_split_c(const char *s, char delim, char **tokens, int max_tokens) {
    if (!s || !tokens || max_tokens < 1) return -1;
    int count = 0;
    const char *start = s;
    for (const char *p = s; ; p++) {
        if (*p == delim || *p == '\0') {
            if (count >= max_tokens) return count;
            size_t tlen = (size_t)(p - start);
            tokens[count] = (char *)s_malloc(tlen + 1);
            if (!tokens[count]) return -1;
            memcpy(tokens[count], start, tlen);
            tokens[count][tlen] = '\0';
            count++;
            if (*p == '\0') break;
            start = p + 1;
        }
    }
    return count;
}

int pny_str_trim_c(const char *s, char *out, size_t out_size) {
    if (!s || !out) return -1;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' || s[len-1] == '\n' || s[len-1] == '\r')) len--;
    if (out_size < len + 1) return -1;
    memcpy(out, s, len);
    out[len] = '\0';
    return (int)len;
}

int pny_str_replace_c(const char *s, const char *from, const char *to, char *out, size_t out_size) {
    if (!s || !from || !to || !out) return -1;
    size_t from_len = strlen(from), to_len = strlen(to);
    int count = 0;
    size_t oi = 0;
    const char *p = s;
    
    while (*p) {
        if (from_len > 0 && strncmp(p, from, from_len) == 0) {
            if (oi + to_len >= out_size) return -1;
            memcpy(out + oi, to, to_len);
            oi += to_len;
            p += from_len;
            count++;
        } else {
            if (oi + 1 >= out_size) return -1;
            out[oi++] = *p++;
        }
    }
    out[oi] = '\0';
    return count;
}

int pny_str_toupper_c(const char *s, char *out, size_t out_size) {
    if (!s || !out) return -1;
    size_t len = strlen(s);
    if (out_size < len + 1) return -1;
    for (size_t i = 0; i < len; i++)
        out[i] = (s[i] >= 'a' && s[i] <= 'z') ? (char)(s[i] - 32) : s[i];
    out[len] = '\0';
    return (int)len;
}

int pny_str_tolower_c(const char *s, char *out, size_t out_size) {
    if (!s || !out) return -1;
    size_t len = strlen(s);
    if (out_size < len + 1) return -1;
    for (size_t i = 0; i < len; i++)
        out[i] = (s[i] >= 'A' && s[i] <= 'Z') ? (char)(s[i] + 32) : s[i];
    out[len] = '\0';
    return (int)len;
}

bool pny_str_starts_with_c(const char *s, const char *prefix) {
    if (!s || !prefix) return false;
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

bool pny_str_ends_with_c(const char *s, const char *suffix) {
    if (!s || !suffix) return false;
    size_t slen = strlen(s), xlen = strlen(suffix);
    if (xlen > slen) return false;
    return strcmp(s + slen - xlen, suffix) == 0;
}
