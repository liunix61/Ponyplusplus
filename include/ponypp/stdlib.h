/**
 * Pony++ Stdlib Runtime — Phase 3
 * I/O, String, List, Map, Concurrent, Test
 */
#ifndef PNY_STDLIB_H
#define PNY_STDLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== I/O ==================== */

typedef enum {
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_READ_WRITE
} FileMode;

typedef struct PnyFile {
    FILE *fp;
    char *path;
    FileMode mode;
    bool eof;
} PnyFile;

/* File I/O */
PnyFile *pny_file_open(const char *path, FileMode mode);
int pny_file_close(PnyFile *f);
char *pny_file_read_all(PnyFile *f);
char *pny_file_read_line(PnyFile *f);
int pny_file_write(PnyFile *f, const char *data, size_t len);
int pny_file_printf(PnyFile *f, const char *fmt, ...);
bool pny_file_eof(PnyFile *f);
int pny_file_size(const char *path);

/* Console I/O */
void pny_stdout_print(const char *s);
void pny_stdout_println(const char *s);
void pny_stdout_print_int(int64_t n);
void pny_stdout_print_bool(bool b);
int pny_stdin_read_line(char *buf, size_t sz);
int pny_stdin_read_bytes(char *buf, size_t sz);

/* Path utilities */
char *pny_path_join(const char *base, const char *part);
bool pny_path_exists(const char *path);
int pny_file_delete(const char *path);
int pny_dir_list(const char *dir, char ***names, int *count);

/* ==================== String ==================== */

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} PnyString;

PnyString *pny_str_new(const char *s);
PnyString *pny_str_new_with(size_t cap);
void pny_str_free(PnyString *s);
PnyString *pny_str_dup(const PnyString *s);
size_t pny_str_len(const PnyString *s);
bool pny_str_empty(const PnyString *s);
int pny_str_cmp(const PnyString *a, const PnyString *b);
int pny_str_cmp_cstr(const PnyString *s, const char *other);
PnyString *pny_str_cat(PnyString *s, const PnyString *other);
PnyString *pny_str_cat_cstr(PnyString *s, const char *other);
PnyString *pny_str_slice(const PnyString *s, size_t start, size_t end);
bool pny_str_contains(const PnyString *s, const PnyString *sub);
bool pny_str_starts_with(const PnyString *s, const PnyString *prefix);
bool pny_str_ends_with(const PnyString *s, const PnyString *suffix);
PnyString *pny_str_replace(PnyString *s, const PnyString *old, const PnyString *new_);
PnyString *pny_str_to_upper(PnyString *s);
PnyString *pny_str_to_lower(PnyString *s);
PnyString *pny_str_trim(PnyString *s);
PnyString *pny_str_split(const PnyString *s, const PnyString *sep, PnyString ***parts, int *count);
PnyString *pny_str_join(const PnyString *sep, const PnyString *arr[], int count);
PnyString *pny_str_format(const char *fmt, ...);
PnyString *pny_str_from_int(int64_t n);
PnyString *pny_str_from_float(double n);
PnyString *pny_str_from_bool(bool b);
int64_t pny_str_to_int(const PnyString *s);
double pny_str_to_float(const PnyString *s);
bool pny_str_to_bool(const PnyString *s);

/* ==================== List ==================== */

typedef struct PnyListNode {
    void *data;          /* opaque: caller manages */
    struct PnyListNode *next;
    struct PnyListNode *prev;
    size_t size;
} PnyListNode;

typedef struct {
    PnyListNode *head;
    PnyListNode *tail;
    size_t len;
    size_t elem_size;    /* 0 = opaque */
    void (*destructor)(void *);
} PnyList;

PnyList *pny_list_new(size_t elem_size);
void pny_list_free(PnyList *l);
void pny_list_append(PnyList *l, void *data);
void pny_list_prepend(PnyList *l, void *data);
void *pny_list_get(PnyList *l, size_t index);
int pny_list_set(PnyList *l, size_t index, void *data);
int pny_list_remove(PnyList *l, size_t index);
int pny_list_remove_val(PnyList *l, void *data, int (*cmp)(const void *, const void *));
void *pny_list_pop(PnyList *l);
void *pny_list_pop_front(PnyList *l);
void pny_list_insert(PnyList *l, size_t index, void *data);
size_t pny_list_len(const PnyList *l);
bool pny_list_empty(const PnyList *l);
int pny_list_index_of(PnyList *l, void *data, int (*cmp)(const void *, const void *));
void pny_list_sort(PnyList *l, int (*cmp)(const void *, const void *));
void pny_list_reverse(PnyList *l);
void pny_list_foreach(PnyList *l, void (*fn)(void *, void *), void *ctx);
PnyList *pny_list_slice(const PnyList *l, size_t start, size_t end);

/* ==================== Map ==================== */

typedef struct PnyMapNode {
    void *key;
    void *val;
    size_t h;
    struct PnyMapNode *next;      /* hash chain */
    struct PnyMapNode *bucket;    /* bucket index */
} PnyMapNode;

typedef struct {
    PnyMapNode **buckets;
    size_t cap;
    size_t size;
    size_t key_size;
    size_t val_size;
    void (*key_free)(void *);
    void (*val_free)(void *);
    size_t (*hash)(const void *key, size_t sz);
    int (*key_cmp)(const void *, const void *);
} PnyMap;

PnyMap *pny_map_new(size_t cap, size_t key_size, size_t val_size);
void pny_map_free(PnyMap *m);
int pny_map_put(PnyMap *m, void *key, void *val);
void *pny_map_get(PnyMap *m, const void *key);
int pny_map_remove(PnyMap *m, const void *key);
bool pny_map_has(PnyMap *m, const void *key);
size_t pny_map_size(const PnyMap *m);
bool pny_map_empty(const PnyMap *m);
void pny_map_clear(PnyMap *m);
void pny_map_foreach(PnyMap *m, void (*fn)(void *, void *, void *), void *ctx);

/* ==================== Concurrent ==================== */

/* Channel */
typedef struct PnyChan PnyChan;

PnyChan *pny_chan_new(size_t cap);    /* cap=0 unbuffered */
void pny_chan_free(PnyChan *c);
int pny_chan_send(PnyChan *c, void *data);     /* 0 ok, -1 closed */
void *pny_chan_recv(PnyChan *c);              /* NULL closed */
bool pny_chan_closed(PnyChan *c);
void pny_chan_close(PnyChan *c);
size_t pny_chan_len(PnyChan *c);

/* Mutex */
typedef struct PnyMutex PnyMutex;
PnyMutex *pny_mutex_new(void);
void pny_mutex_free(PnyMutex *m);
int pny_mutex_lock(PnyMutex *m);     /* 0 ok, -1 error */
int pny_mutex_unlock(PnyMutex *m);
int pny_mutex_trylock(PnyMutex *m);

/* Atomic */
typedef struct PnyAtomicInt64 { int64_t v; } PnyAtomicInt64;
void pny_atomic_store(PnyAtomicInt64 *a, int64_t v);
int64_t pny_atomic_load(const PnyAtomicInt64 *a);
int64_t pny_atomic_add(PnyAtomicInt64 *a, int64_t delta);
int64_t pny_atomic_cas(PnyAtomicInt64 *a, int64_t expected, int64_t desired);

/* ==================== Test ==================== */

typedef enum { TEST_PASS = 0, TEST_FAIL = 1, TEST_SKIP = 2 } TestResult;

typedef struct TestReport {
    int total;
    int passed;
    int failed;
    int skipped;
} TestReport;

typedef void (*TestCase)(void);

typedef struct PnyTestSuite {
    char *name;
    TestCase *tests;
    int count;
    TestReport report;
} PnyTestSuite;

PnyTestSuite *pny_test_suite_new(const char *name);
void pny_test_suite_free(PnyTestSuite *s);
void pny_test_suite_add(PnyTestSuite *s, TestCase tc);
void pny_test_run_suite(PnyTestSuite *s, bool verbose);
void pny_test_report_print(const TestReport *r);

/* Simple assertion helpers for runtime tests */
typedef struct AssertCtx { int line; const char *file; const char *msg; } AssertCtx;

void pny_assert_true(bool cond, AssertCtx ctx);
void pny_assert_eq_int(int64_t a, int64_t b, AssertCtx ctx);
void pny_assert_eq_str(const char *a, const char *b, AssertCtx ctx);
void pny_assert_null(void *p, AssertCtx ctx);
void pny_assert_not_null(void *p, AssertCtx ctx);

/* Macros — only in Pony++ runtime user code, not internal */
#define PNY_ASSERT_TRUE(c) pny_assert_true((c), (AssertCtx){__LINE__, __FILE__, #c})
#define PNY_ASSERT_EQ(a,b) pny_assert_eq_int((int64_t)(a), (int64_t)(b), (AssertCtx){__LINE__, __FILE__, #a " == " #b})
#define PNY_ASSERT_EQ_STR(a,b) pny_assert_eq_str((a),(b), (AssertCtx){__LINE__, __FILE__, #a " == " #b})

#ifdef __cplusplus
}
#endif

#endif /* PNY_STDLIB_H */
