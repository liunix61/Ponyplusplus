#include "ponypp/stdlib.h"
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
    ps->data = (char *)malloc(len + 1);
    if (!ps->data && len > 0) { free(ps); return NULL; }
    if (s) memcpy(ps->data, s, len + 1);
    else ps->data[0] = '\0';
    ps->len = len;
    ps->cap = len + 4;
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
