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
#include <stdint.h>

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

/* ==================== Protobuf 序列化 ==================== */
typedef struct PnyProtoBuf PnyProtoBuf;

PnyProtoBuf *pny_proto_new(void);
void pny_proto_free(PnyProtoBuf *pb);
size_t pny_proto_len(const PnyProtoBuf *pb);
const uint8_t *pny_proto_data(const PnyProtoBuf *pb);

int pny_proto_write_varint(PnyProtoBuf *pb, uint64_t val);
int pny_proto_write_tag(PnyProtoBuf *pb, uint32_t field_num, uint8_t wire_type);
int pny_proto_write_int32(PnyProtoBuf *pb, uint32_t field_num, int32_t val);
int pny_proto_write_int64(PnyProtoBuf *pb, uint32_t field_num, int64_t val);
int pny_proto_write_uint32(PnyProtoBuf *pb, uint32_t field_num, uint32_t val);
int pny_proto_write_uint64(PnyProtoBuf *pb, uint32_t field_num, uint64_t val);
int pny_proto_write_bool(PnyProtoBuf *pb, uint32_t field_num, bool val);
int pny_proto_write_float(PnyProtoBuf *pb, uint32_t field_num, float val);
int pny_proto_write_double(PnyProtoBuf *pb, uint32_t field_num, double val);
int pny_proto_write_string(PnyProtoBuf *pb, uint32_t field_num, const char *str);
int pny_proto_write_bytes(PnyProtoBuf *pb, uint32_t field_num, const void *data, size_t len);
int pny_proto_write_message(PnyProtoBuf *pb, uint32_t field_num, const uint8_t *msg, size_t len);

typedef struct {
    uint32_t field_num;
    uint8_t wire_type;
    uint64_t varint_val;
    double double_val;
    struct { const uint8_t *ptr; size_t len; } bytes_val;
} PnyProtoField;

int pny_proto_read_varint(const uint8_t *buf, size_t buf_len, uint64_t *out);
int pny_proto_read_field(const uint8_t *buf, size_t buf_len, PnyProtoField *out);

#define PROTO_WIRE_VARINT 0
#define PROTO_WIRE_FIXED64 1
#define PROTO_WIRE_LEN_DELIM 2
#define PROTO_WIRE_FIXED32 5

/* ==================== MessagePack (二进制序列化) ==================== */
/* 轻量级msgpack编码器/解码器 (支持nil/bool/int/float/str/bin/array/map) */

/* 编码: 写入buf, 返回写入字节数或-1 */
int pny_msgpack_write_nil(uint8_t *buf, size_t buf_len);
int pny_msgpack_write_bool(uint8_t *buf, size_t buf_len, bool val);
int pny_msgpack_write_int(uint8_t *buf, size_t buf_len, int64_t val);
int pny_msgpack_write_uint(uint8_t *buf, size_t buf_len, uint64_t val);
int pny_msgpack_write_float(uint8_t *buf, size_t buf_len, float val);
int pny_msgpack_write_double(uint8_t *buf, size_t buf_len, double val);
int pny_msgpack_write_str(uint8_t *buf, size_t buf_len, const char *str);
int pny_msgpack_write_bin(uint8_t *buf, size_t buf_len, const void *data, size_t len);
int pny_msgpack_write_array_header(uint8_t *buf, size_t buf_len, uint32_t count);
int pny_msgpack_write_map_header(uint8_t *buf, size_t buf_len, uint32_t count);

/* 解码: 从buf读取, 返回读取字节数或负错误码 */
/* 类型标记 */
typedef enum {
    PNY_MSGPACK_NIL = 0,
    PNY_MSGPACK_BOOL,
    PNY_MSGPACK_INT,
    PNY_MSGPACK_UINT,
    PNY_MSGPACK_FLOAT,
    PNY_MSGPACK_DOUBLE,
    PNY_MSGPACK_STR,
    PNY_MSGPACK_BIN,
    PNY_MSGPACK_ARRAY,
    PNY_MSGPACK_MAP
} PnyMsgpackType;

typedef struct {
    PnyMsgpackType type;
    union {
        bool bool_val;
        int64_t int_val;
        uint64_t uint_val;
        float float_val;
        double double_val;
        struct { const uint8_t *ptr; uint32_t len; } str_bin;
        uint32_t count;  /* array/map */
    };
} PnyMsgpackValue;

int pny_msgpack_read(const uint8_t *buf, size_t buf_len, PnyMsgpackValue *out);

/* ==================== Stream (流式I/O) ==================== */
typedef struct PnyStream PnyStream;

PnyStream *pny_stream_new(void);
void pny_stream_free(PnyStream *st);
int pny_stream_write(PnyStream *st, const void *data, size_t len);
int pny_stream_read(PnyStream *st, void *buf, size_t buf_len);
size_t pny_stream_available(const PnyStream *st);
void pny_stream_close(PnyStream *st);  /* 标记写端关闭 */
bool pny_stream_is_closed(const PnyStream *st);

/* ==================== Error (I/O错误类型) ==================== */
typedef enum {
    PNY_ERR_NONE = 0,
    PNY_ERR_EOF,
    PNY_ERR_TIMEOUT,
    PNY_ERR_PERMISSION,
    PNY_ERR_NOT_FOUND,
    PNY_ERR_ALREADY_EXISTS,
    PNY_ERR_INVALID_ARG,
    PNY_ERR_IO,
    PNY_ERR_NO_MEMORY,
    PNY_ERR_UNKNOWN
} PnyErrorCode;

const char *pny_error_str(PnyErrorCode code);

/* ==================== Mailbox (Actor邮箱) ==================== */
typedef struct PnyMailbox PnyMailbox;

PnyMailbox *pny_mailbox_new(size_t max_size);
void pny_mailbox_free(PnyMailbox *mb);
bool pny_mailbox_put(PnyMailbox *mb, void *msg);
void *pny_mailbox_take(PnyMailbox *mb);
size_t pny_mailbox_size(const PnyMailbox *mb);
bool pny_mailbox_is_empty(const PnyMailbox *mb);

/* ==================== Group (Actor组) ==================== */
typedef struct PnyGroup PnyGroup;

PnyGroup *pny_group_new(const char *name);
void pny_group_free(PnyGroup *g);
int pny_group_add(PnyGroup *g, int actor_id);
int pny_group_remove(PnyGroup *g, int actor_id);
size_t pny_group_size(const PnyGroup *g);
bool pny_group_contains(const PnyGroup *g, int actor_id);
const char *pny_group_name(const PnyGroup *g);
int pny_group_members(const PnyGroup *g, int *out_ids, size_t max);

/* ==================== Collections 扩展 ==================== */

/* Set (哈希集合, 基于map实现) */
typedef struct PnySet PnySet;

PnySet *pny_set_new(void);
void pny_set_free(PnySet *s);
bool pny_set_add(PnySet *s, const char *key);       /* true=新增, false=已存在 */
bool pny_set_contains(const PnySet *s, const char *key);
bool pny_set_remove(PnySet *s, const char *key);
size_t pny_set_size(const PnySet *s);
void pny_set_clear(PnySet *s);

/* Queue (FIFO队列) */
typedef struct PnyQueue PnyQueue;

PnyQueue *pny_queue_new(size_t cap);  /* 0=无限制 */
void pny_queue_free(PnyQueue *q);
bool pny_queue_push(PnyQueue *q, void *data);   /* false=满 */
void *pny_queue_pop(PnyQueue *q);               /* NULL=空 */
void *pny_queue_peek(const PnyQueue *q);
size_t pny_queue_size(const PnyQueue *q);
bool pny_queue_is_empty(const PnyQueue *q);
bool pny_queue_is_full(const PnyQueue *q);

/* Stack (LIFO栈) */
typedef struct PnyStack PnyStack;

PnyStack *pny_stack_new(size_t cap);  /* 0=无限制 */
void pny_stack_free(PnyStack *s);
bool pny_stack_push(PnyStack *s, void *data);
void *pny_stack_pop(PnyStack *s);
void *pny_stack_peek(const PnyStack *s);
size_t pny_stack_size(const PnyStack *s);
bool pny_stack_is_empty(const PnyStack *s);

/* Buffer (字节缓冲区, 动态增长) */
typedef struct PnyBuffer PnyBuffer;

PnyBuffer *pny_buffer_new(size_t initial_cap);
void pny_buffer_free(PnyBuffer *b);
bool pny_buffer_append(PnyBuffer *b, const void *data, size_t len);
bool pny_buffer_append_byte(PnyBuffer *b, uint8_t byte);
bool pny_buffer_append_cstr(PnyBuffer *b, const char *str);
const uint8_t *pny_buffer_data(const PnyBuffer *b);
size_t pny_buffer_len(const PnyBuffer *b);
void pny_buffer_clear(PnyBuffer *b);
bool pny_buffer_reserve(PnyBuffer *b, size_t extra);

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

/* Promise/Future */
typedef struct PnyPromise PnyPromise;

PnyPromise *pny_promise_new(void);
void pny_promise_free(PnyPromise *p);
int pny_promise_fulfill(PnyPromise *p, void *value, size_t size);  /* 0 ok, -1 already fulfilled */
bool pny_promise_is_done(const PnyPromise *p);
void *pny_promise_value(const PnyPromise *p, size_t *out_size);    /* NULL if not done */
int pny_promise_then(PnyPromise *p, void (*cb)(void *value, size_t size, void *ctx), void *ctx);

/* Future: Promise的只读视图 */
typedef struct PnyFuture PnyFuture;

PnyFuture *pny_future_from_promise(PnyPromise *p);  /* 借用引用, 不拥有 */
void pny_future_free(PnyFuture *f);
bool pny_future_is_done(const PnyFuture *f);
void *pny_future_value(const PnyFuture *f, size_t *out_size);
int pny_future_wait(PnyFuture *f, int timeout_ms);  /* 阻塞等待, 0=done, -1=timeout */
int pny_future_then(PnyFuture *f, void (*cb)(void *value, size_t size, void *ctx), void *ctx);

/* ==================== UDP ==================== */
typedef struct PnyUdpSocket PnyUdpSocket;

PnyUdpSocket *pny_udp_open(const char *bind_addr, int port);
void pny_udp_close(PnyUdpSocket *s);
int pny_udp_sendto(PnyUdpSocket *s, const void *data, size_t len,
                   const char *dest_addr, int dest_port);
int pny_udp_recvfrom(PnyUdpSocket *s, void *buf, size_t buf_len,
                     char *src_addr, size_t src_addr_len, int *src_port);
int pny_udp_set_timeout(PnyUdpSocket *s, int timeout_ms);

/* ==================== DNS ==================== */
typedef struct {
    char addrs[8][64];  /* 最多8个IP */
    int count;
} PnyDnsResult;

int pny_dns_resolve(const char *hostname, PnyDnsResult *result);

/* ==================== Date ==================== */
typedef struct {
    int year, month, day;
    int hour, minute, second;
    int weekday;  /* 0=Sunday */
} PnyDateTime;

int pny_date_now(PnyDateTime *out);
int pny_date_from_timestamp(int64_t ts, PnyDateTime *out);
int64_t pny_date_to_timestamp(const PnyDateTime *dt);
const char *pny_date_format(const PnyDateTime *dt, const char *fmt, char *buf, size_t buf_len);

/* ==================== Complex (复数) ==================== */
typedef struct {
    double re, im;
} PnyComplex;

PnyComplex pny_complex_new(double re, double im);
PnyComplex pny_complex_add(PnyComplex a, PnyComplex b);
PnyComplex pny_complex_sub(PnyComplex a, PnyComplex b);
PnyComplex pny_complex_mul(PnyComplex a, PnyComplex b);
PnyComplex pny_complex_div(PnyComplex a, PnyComplex b);
double pny_complex_abs(PnyComplex z);
double pny_complex_arg(PnyComplex z);
PnyComplex pny_complex_conj(PnyComplex z);

/* ==================== Statistics ==================== */
double pny_stats_mean(const double *data, size_t n);
double pny_stats_variance(const double *data, size_t n);
double pny_stats_stddev(const double *data, size_t n);
double pny_stats_min(const double *data, size_t n);
double pny_stats_max(const double *data, size_t n);
double pny_stats_median(double *data, size_t n);  /* 会修改data(排序) */

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

/* ==================== JSON ==================== */

typedef enum {
    JSON_NULL = 0,
    JSON_BOOL,
    JSON_INT,
    JSON_DOUBLE,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonType;

typedef struct JsonValue {
    JsonType type;
    union {
        bool b;
        int64_t i;
        double d;
        char *s;
        struct {
            struct JsonValue **items;
            size_t count;
        } arr;
        struct {
            char **keys;
            struct JsonValue **vals;
            size_t count;
        } obj;
    };
} JsonValue;

JsonValue *json_parse(const char *input, size_t len);
void json_free(JsonValue *v);
char *json_stringify(const JsonValue *v);
JsonValue *json_obj_get(const JsonValue *obj, const char *key);
bool json_obj_has(const JsonValue *obj, const char *key);
int json_obj_set(JsonValue *obj, const char *key, JsonValue *val);

/* JSON shorthand constructors */
JsonValue *json_new_null(void);
JsonValue *json_new_bool(bool b);
JsonValue *json_new_int(int64_t i);
JsonValue *json_new_double(double d);
JsonValue *json_new_string(const char *s);
JsonValue *json_new_array(void);
JsonValue *json_new_object(void);
int json_arr_push(JsonValue *arr, JsonValue *val);

/* ==================== Timer ==================== */

typedef struct PnyTimer PnyTimer;

typedef void (*TimerCallback)(void *ctx);

PnyTimer *pny_timer_new(int64_t interval_ms, TimerCallback cb, void *ctx, bool repeat);
void pny_timer_free(PnyTimer *t);
void pny_timer_start(PnyTimer *t);
void pny_timer_stop(PnyTimer *t);
bool pny_timer_running(const PnyTimer *t);
int64_t pny_timer_now_ms(void);
int64_t pny_timer_elapsed_ms(void);

/* ==================== Logger ==================== */

typedef enum {
    LOG_TRACE = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

typedef struct PnyLogger PnyLogger;

PnyLogger *pny_logger_new(const char *name);
void pny_logger_free(PnyLogger *l);
void pny_logger_set_level(PnyLogger *l, LogLevel level);
LogLevel pny_logger_get_level(const PnyLogger *l);
int pny_logger_log(PnyLogger *l, LogLevel level, const char *fmt, ...);
int pny_logger_trace(PnyLogger *l, const char *fmt, ...);
int pny_logger_debug(PnyLogger *l, const char *fmt, ...);
int pny_logger_info(PnyLogger *l, const char *fmt, ...);
int pny_logger_warn(PnyLogger *l, const char *fmt, ...);
int pny_logger_error(PnyLogger *l, const char *fmt, ...);
int pny_logger_fatal(PnyLogger *l, const char *fmt, ...);

/* ==================== Math ==================== */

double pny_math_sqrt(double x);
double pny_math_pow(double base, double exp);
double pny_math_sin(double x);
double pny_math_cos(double x);
double pny_math_tan(double x);
double pny_math_asin(double x);
double pny_math_acos(double x);
double pny_math_atan(double x);
double pny_math_atan2(double y, double x);
double pny_math_log(double x);
double pny_math_log2(double x);
double pny_math_exp(double x);
double pny_math_ceil(double x);
double pny_math_floor(double x);
double pny_math_abs(double x);
int64_t pny_math_abs_int(int64_t x);
double pny_math_random(void);       /* [0, 1) */
int64_t pny_math_random_int(int64_t max);
int64_t pny_math_min(int64_t a, int64_t b);
int64_t pny_math_max(int64_t a, int64_t b);
double pny_math_fmod(double x, double y);
int64_t pny_math_factorial(int64_t n);

/* ==================== UUID ==================== */

/* 生成 UUID v4 (随机), buf至少37字节 (36+\0) */
int pny_uuid_v4(char *buf, size_t buf_size);

/* 生成短UUID (8字节hex), buf至少17字节 */
int pny_uuid_short(char *buf, size_t buf_size);

/* ==================== Base64 ==================== */

/* Base64编码. 返回编码后长度, -1错误. out需要 (len+2)/3*4+1 字节 */
int pny_base64_encode(const void *data, size_t len, char *out, size_t out_size);

/* Base64解码. 返回解码后长度, -1错误. out需要 (len/4)*3 字节 */
int pny_base64_decode(const char *input, size_t len, void *out, size_t out_size);

/* ==================== Hex ==================== */

/* 十六进制编码. 返回编码后长度, -1错误. out需要 len*2+1 字节 */
int pny_hex_encode(const void *data, size_t len, char *out, size_t out_size);

/* 十六进制解码. 返回解码后长度, -1错误. out需要 len/2 字节 */
int pny_hex_decode(const char *input, size_t len, void *out, size_t out_size);

/* ==================== CRC32 ==================== */

/* CRC32校验 */
uint32_t pny_crc32(const void *data, size_t len);

/* ==================== String Utilities ==================== */

/* 字符串分割. 返回token数, -1错误. tokens数组由调用方管理 */
int pny_str_split_c(const char *s, char delim, char **tokens, int max_tokens);

/* 字符串trim (去除首尾空白). 结果写入out */
int pny_str_trim_c(const char *s, char *out, size_t out_size);

/* 字符串替换. 返回替换次数, -1错误 */
int pny_str_replace_c(const char *s, const char *from, const char *to, char *out, size_t out_size);

/* 字符串转大写/小写 */
int pny_str_toupper_c(const char *s, char *out, size_t out_size);
int pny_str_tolower_c(const char *s, char *out, size_t out_size);

/* 字符串是否以prefix开头/结尾 */
bool pny_str_starts_with_c(const char *s, const char *prefix);
bool pny_str_ends_with_c(const char *s, const char *suffix);

#define PNY_MATH_PI  3.14159265358979323846
#define PNY_MATH_E   2.71828182845904523536

#ifdef __cplusplus
}
#endif

#endif /* PNY_STDLIB_H */
