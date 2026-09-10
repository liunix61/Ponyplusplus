#ifndef PONYPP_RUNTIME_H
#define PONYPP_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

/* 跨组件序列化：消息头 */
typedef struct PnyMsgHeader {
    uint16_t version;    /* 当前 = 0x0001 */
    uint32_t actor_id;
    uint16_t method_id;
} PnyMsgHeader;

/* 能力标记 (完整6种引用能力) */
typedef enum {
    PNY_CAP_ISO  = 0x01,  /* 唯一可变引用 — 发送后变box(消费语义), Sendable */
    PNY_CAP_TRN  = 0x02,  /* 转移引用 — 不可发送, 仅同Actor内传递 */
    PNY_CAP_REF  = 0x03,  /* 本地可变引用 — 不可发送 */
    PNY_CAP_VAL  = 0x04,  /* 全局不可变 — 可直接发送(值复制), Sendable */
    PNY_CAP_BOX  = 0x05,  /* 本地只读 — 不可发送, 仅同Actor内读取 */
    PNY_CAP_TAG  = 0x06,  /* 仅标识 — 可发送(引用句柄), Sendable */
    PNY_CAP_ERR  = 0xFF   /* 不可发送类型(错误) */
} PnyCapMark;

/* 判断能力是否可跨Actor发送 (Sendable) */
static inline bool pny_cap_is_sendable(PnyCapMark cap) {
    return cap == PNY_CAP_ISO || cap == PNY_CAP_VAL || cap == PNY_CAP_TAG;
}

/* 能力发送后的转换: iso→box(消费), val→val(复制), tag→tag(句柄), 其他→ERR */
static inline PnyCapMark pny_cap_after_send(PnyCapMark cap) {
    switch (cap) {
        case PNY_CAP_ISO: return PNY_CAP_BOX;  /* 消费语义 */
        case PNY_CAP_VAL: return PNY_CAP_VAL;  /* 值复制 */
        case PNY_CAP_TAG: return PNY_CAP_TAG;  /* 句柄 */
        default: return PNY_CAP_ERR;            /* 不可发送 */
    }
}

/* 能力名称 (调试用) */
static inline const char *pny_cap_name(PnyCapMark cap) {
    switch (cap) {
        case PNY_CAP_ISO: return "iso";
        case PNY_CAP_TRN: return "trn";
        case PNY_CAP_REF: return "ref";
        case PNY_CAP_VAL: return "val";
        case PNY_CAP_BOX: return "box";
        case PNY_CAP_TAG: return "tag";
        default: return "err";
    }
}

/* 背压状态 */
typedef enum {
    BACKPRESSURE_OK,
    BACKPRESSURE_WARN,
    BACKPRESSURE_FULL
} BackpressureState;

/* Actor 引用 */
typedef struct ActorRef {
    int id;
    char *name;
    struct PnyActor *actor;
} ActorRef;

/* Actor 状态 */
typedef enum {
    ACTOR_STATE_INIT,
    ACTOR_STATE_RUNNING,
    ACTOR_STATE_STOPPING,
    ACTOR_STATE_STOPPED,
    ACTOR_STATE_CRASHED,      /* 已崩溃(待监督树处理) */
    ACTOR_STATE_RESTARTING
} ActorState;

/* 监督策略 */
typedef enum {
    SUPERVISE_ONE_FOR_ONE,
    SUPERVISE_ONE_FOR_ALL,
    SUPERVISE_REST_FOR_ONE,   /* 重启崩溃的子 Actor 及其后注册的所有兄弟 */
    SUPERVISE_SIMPLE_ONE_FOR_ONE, /* 动态添加同类子Actor, 崩溃仅重启该实例 */
    SUPERVISE_RESTART,        /* 兼容旧名 = rest_for_one */
    SUPERVISE_NONE
} SuperviseStrategy;

/* 监督元数据 */
typedef struct {
    ActorRef supervisor;
    SuperviseStrategy strategy;
    int max_restarts;
    int restart_count;
} SuperviseMeta;

/* 回复通道（同步请求-响应） */
#define PNY_REPLY_MAX_SIZE 4096

typedef struct PnyReply {
    void *result;
    size_t result_size;
    int done;       /* 0=waiting, 1=done */
    int error;      /* 0=ok, <0=error */
} PnyReply;

/* 消息队列项 */
typedef struct PnyMessage {
    char *method;           /* 指向 method_buf 或堆分配 */
    char method_buf[32];    /* P5优化: 短方法名内联, 消除strdup */
    void *arg;
    size_t arg_size;
    struct PnyMessage *next;
    ActorRef sender;
    PnyReply *reply;    /* 非 NULL 时为同步调用，behavior 需调用 pny_actor_reply */
    uint64_t msg_id;        /* exactly-once 消息唯一 ID */
    PnyCapMark cap_mark;    /* 能力标记: iso/val/tag */
} PnyMessage;

/* Actor 结构 */
typedef struct PnyActor {
    const char *name;
    void *state_data;
    size_t state_size;
    struct PnyActor *next;
    struct PnyActor *prev;  /* Phase 5: O(1) 链表移除 */
    PnyMessage *messages;
    PnyMessage *messages_tail;  /* Phase 5: O(1) 消息入队 */
    size_t message_count;
    size_t max_messages;
    ActorState actor_state;
    ActorRef self;
    SuperviseMeta *supervise;
    struct PnyActor **children;
    size_t child_count;
    int id;
    void (*behavior)(struct PnyActor *self, PnyMessage *msg);
    uint64_t *delivered_ids;    /* exactly-once: 已投递消息 ID 数组 */
    size_t delivered_count;
    size_t delivered_cap;
    uint64_t next_msg_id;       /* exactly-once: 下一条消息 ID */
    uint64_t gas_budget;        /* 气体预算 */
    uint64_t gas_used;          /* 已消耗气体 */
    int64_t gas_limit;          /* -1 = 无限, 0 = 立即让出, >0 = 消耗后让出 */
    int version;                /* 热代码升级: 行为函数版本 */
    void (*new_behavior)(struct PnyActor *self, PnyMessage *msg); /* 热升级目标行为 */
    struct GCHeap *heap;        /* P0: per-actor GC隔离堆 (NULL=禁用) */
    size_t gc_alloc_since;      /* 上次回收以来的分配量 (触发阈值用) */
} PnyActor;

/* 调度器 */
typedef struct Scheduler {
    PnyActor *actors;
    size_t actor_count;
    size_t max_actors;
    PnyActor **registry;
    int next_id;
    bool running;
    size_t tick_count;
} Scheduler;

/* 运行时统计 */
typedef struct RuntimeStats {
    size_t actors_alive;
    size_t actors_created;
    size_t actors_destroyed;
    size_t messages_sent;
    size_t messages_delivered;
    size_t messages_dead_letter;
    size_t restarts;
    uint64_t total_ticks;
} RuntimeStats;

/* 运行时（包含调度器） */
typedef struct PnyRuntime {
    Scheduler scheduler;
    RuntimeStats stats;
    PnyMessage *dead_letters;
    size_t dead_letter_count;
    /* Phase 5: 消息池 (free-list复用, 消除per-send malloc) */
    PnyMessage *msg_pool;
    size_t msg_pool_count;
    size_t msg_pool_max;
} PnyRuntime;

extern PnyRuntime *pny_runtime_global;

/* 生命周期 */
PnyRuntime *pny_runtime_new(void);
void pny_runtime_free(PnyRuntime *r);

/* Actor */
PnyActor *pny_actor_new(PnyRuntime *r, const char *name, size_t state_size);
void pny_actor_register(PnyRuntime *r, PnyActor *a);
void pny_actor_destroy(PnyRuntime *r, ActorRef *ref);
ActorRef *pny_actor_ref(PnyActor *a);

/* ==================== Per-actor GC (P0: 隔离堆) ==================== */

/* 启用actor的GC隔离堆 (heap_size=0用默认64KB, 必须在actor创建后首次使用前调用)
 * 返回0成功, -1参数错误, -2已启用 */
int pny_actor_gc_enable(PnyActor *a, size_t heap_size);

/* 从actor的GC堆分配 (未启用GC时回退malloc, 对齐8字节) */
void *pny_actor_gc_alloc(PnyActor *a, size_t size);

/* 触发actor的GC回收 (root=actor的state_data, 自动包含state内指针) */
void pny_actor_gc_collect(PnyActor *a);

/* GC统计 */
typedef struct {
    size_t heap_size;       /* 半空间大小 */
    size_t used;            /* from空间已用 */
    size_t total_alloc;     /* 累计分配 */
    size_t total_freed;     /* 累计回收 */
    int generations;        /* 回收次数 */
    double pause_ms;        /* 上次回收耗时(ms) */
} PnyGCStats;

int pny_actor_gc_stats(PnyActor *a, PnyGCStats *out);

/* 消息 */
PnyMessage *pny_msg_new(const char *method, void *arg, size_t arg_size);
void pny_msg_free(PnyMessage *m);
int pny_actor_send(ActorRef *from, ActorRef *to, const char *method, void *arg, size_t arg_size);
int pny_actor_call(ActorRef *from, ActorRef *to, const char *method, void *arg, size_t arg_size, void **result, size_t *result_size);
void pny_actor_reply(PnyMessage *msg, void *result, size_t result_size);

/* 调度 */
void pny_scheduler_tick(PnyRuntime *r);
void pny_scheduler_start(PnyRuntime *r);
void pny_scheduler_stop(PnyRuntime *r);

/* 监督 */
void pny_supervise_register(PnyActor *parent, ActorRef *child, SuperviseStrategy strategy, int max_restarts);
void pny_supervisor_handle_crash(PnyRuntime *r, ActorRef *crashed);

/* 统计 */
void pny_runtime_stats(PnyRuntime *r, RuntimeStats *out);

/* 跨组件序列化 */
int pny_msg_serialize(PnyMessage *msg, void *buf, size_t buf_size, size_t *out_size);
int pny_msg_deserialize(void *buf, size_t buf_size, PnyMessage **out);

/* 背压 */
BackpressureState pny_backpressure_check(PnyActor *a);
void pny_backpressure_set_limit(PnyActor *a, size_t max_messages);

/* exactly-once */
int pny_msg_delivered(PnyActor *a, uint64_t msg_id);  /* 检查是否已投递 (去重) */

/* Gas 计数（抢占式调度） */
void pny_gas_set_limit(PnyActor *a, int64_t limit);
uint64_t pny_gas_consume(PnyActor *a, uint64_t amount);  /* 返回剩余气体, 0=应让出 */

/* 热代码升级 */
void pny_hot_upgrade(PnyActor *a, void (*new_behavior)(PnyActor *, PnyMessage *));
int pny_hot_upgrade_version(PnyActor *a);

/* 诊断接口 */
typedef struct ActorStats {
    size_t actors_total;
    size_t actors_running;
    size_t actors_stopped;
    size_t messages_queued;
    size_t messages_delivered;
    size_t gas_used;
    uint64_t scheduler_ticks;
} ActorStats;
void pny_diagnostics_stats(PnyRuntime *r, ActorStats *out);
const char *pny_diagnostics_dump(PnyRuntime *r, char *buf, size_t buf_size);

/* M:N 工作窃取调度器 */
typedef struct WorkDeque {
    PnyMessage *items[64];
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
} WorkDeque;

typedef struct WorkerThread {
    int id;
    WorkDeque *local;
    PnyRuntime *runtime;
    bool running;
    size_t steal_count;
    size_t local_count;
    pthread_t thread;
} WorkerThread;

typedef struct MNWorker {
    WorkerThread *workers;
    int worker_count;
    WorkDeque *global;         /* 全局就绪队列 */
    pthread_mutex_t global_lock;
    pthread_cond_t global_cv;
    bool running;
    uint64_t total_steals;
    uint64_t total_local_deliveries;
    PnyRuntime *runtime;
} MNWorker;

void pny_mn_init(PnyRuntime *r, int worker_count);
void pny_mn_destroy(PnyRuntime *r);
extern MNWorker *pny_mn_global;
int pny_mn_enqueue(PnyActor *a, PnyMessage *msg, int preferred_worker);
PnyMessage *pny_mn_dequeue_local(WorkerThread *wt);
PnyMessage *pny_mn_steal(WorkerThread *wt);
void pny_mn_run_tick(PnyRuntime *r);
size_t pny_mn_steal_stats(PnyRuntime *r);
void pny_mn_start(PnyRuntime *r);
void pny_mn_stop(PnyRuntime *r);

/* 跨组件监督 */
typedef struct CrossComponentSupervisor {
    ActorRef *children;
    size_t child_count;
    size_t child_cap;
    SuperviseStrategy strategy;
    int max_restarts;
    int restart_count;
    char **child_names;
    void **child_states;      /* 序列化状态用于恢复 */
    size_t *child_state_sizes;
} CrossComponentSupervisor;

CrossComponentSupervisor *pny_cross_supervise_new(void);
void pny_cross_supervise_free(CrossComponentSupervisor *cs);
int pny_cross_supervise_register(CrossComponentSupervisor *cs, ActorRef *child, const char *name);
int pny_cross_supervise_notify_crash(CrossComponentSupervisor *cs, size_t child_idx);
const char *pny_cross_supervise_state(CrossComponentSupervisor *cs, size_t child_idx);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_RUNTIME_H */
