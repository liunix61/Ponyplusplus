# Pony++ Phase 2: 多 Actor 交互与消息传递系统

> 设计日期: 2026-09-01  
> 状态: 设计中

---

## 1. 目标

Phase 1 完成了单 Actor 编译。Phase 2 实现：

1. **Actor 间消息传递** — Actor 可以 `!` 和 `@` 互相发送消息
2. **监督树 (Supervision Tree)** — Supervisor 模式管理子 Actor 生命周期
3. **Actor Registry** — 全局 Actor 注册表，支持按名称查找
4. **消息队列** — 每个 Actor 独立的 mailbox
5. **`supervise` 语法** — `supervise A one_for_one | one_for_all | restart | none`
6. **Actor 初始化与启动顺序** — Supervisor 先启动，被监督者后启动

---

## 2. 语法扩展

```pony
# 消息发送语法
actor Sender {
  var target: ActorRef
  be send(msg: String) => {
    target ! msg        # 发送并继续（fire-and-forget）
    val reply = target @ query()  # 请求-响应
  }
}

# 接收消息
actor Receiver {
  be handle(msg: String) => {
    print(msg)
  }
}

# 监督树
supervise Worker one_for_one    # 子 Actor 崩溃 -> 只重启该子 Actor
supervise Worker one_for_all    # 子 Actor 崩溃 -> 重启所有子 Actor
supervise Worker restart        # 崩溃后重启（带退避）
supervise Worker none           # 崩溃后不重启
```

---

## 3. 核心数据结构

```c
/* 消息队列项 */
typedef struct MsgItem {
    char *method;         /* 方法名 */
    void *arg;            /* 参数指针 */
    size_t arg_size;      /* 参数大小 */
    struct MsgItem *next; /* 链表 */
    ActorRef sender;      /* 发送方引用 */
} MsgItem;

/* Actor 引用（轻量级 handle） */
typedef struct ActorRef {
    int id;              /* 全局唯一 ID */
    char *name;          /* 名称 */
    struct PnyActor *actor; /* 指向实际 Actor 结构 */
} ActorRef;

/* Actor 状态机 */
typedef enum {
    ACTOR_STATE_INIT,
    ACTOR_STATE_RUNNING,
    ACTOR_STATE_STOPPING,
    ACTOR_STATE_STOPPED,
    ACTOR_STATE_RESTARTING
} ActorState;

/* 监督策略 */
typedef enum {
    SUPERVISE_ONE_FOR_ONE,
    SUPERVISE_ONE_FOR_ALL,
    SUPERVISE_RESTART,
    SUPERVISE_NONE
} SuperviseStrategy;

/* 监督元数据 */
typedef struct {
    ActorRef supervisor;        /* 监督者 */
    SuperviseStrategy strategy; /* 策略 */
    int max_restarts;           /* 最大重启次数（默认 3） */
    int restart_count;          /* 当前重启计数 */
} SuperviseMeta;

/* 增强后的 Actor */
typedef struct PnyActor {
    const char *name;
    void *state;
    size_t state_size;
    struct PnyActor *next;
    MsgItem *messages;          /* 消息队列 */
    size_t message_count;       /* 消息数 */
    size_t max_messages;        /* 队列上限 */
    ActorState state;           /* 状态机 */
    ActorRef self;              /* 自引用 */
    SuperviseMeta *supervise;   /* 监督信息 */
    struct PnyActor **children; /* 子 Actor 列表 */
    size_t child_count;
    int id;                     /* 全局 ID */
    void (*behavior)(struct PnyActor *self, MsgItem *msg); /* 行为函数 */
} PnyActor;
```

---

## 4. 运行时调度器设计

```c
/* Actor 运行时调度器 */
typedef struct Scheduler {
    PnyActor *actors;           /* 所有 Actor 链表 */
    size_t actor_count;
    size_t max_actors;
    ActorRef *registry;         /* ID -> ActorRef 映射 */
    int next_id;                /* 下一个 ID */
    bool running;               /* 调度器运行标志 */
} Scheduler;

/* 调度循环 */
void scheduler_tick(Scheduler *sched);
/* - 遍历所有 Actor 的 message queue
 * - 按 FIFO 顺序消费消息
 * - 如果消息处理中 Actor 崩溃 -> 触发监督策略
 * - 如果队列非空 -> 继续处理
 */

/* 消息发送 */
int actor_send(ActorRef *from, ActorRef *to, const char *method, const void *arg, size_t arg_size);
int actor_call(ActorRef *from, ActorRef *to, const char *method, const void *arg, size_t arg_size, void **result, size_t *result_size);

/* 监督 */
void supervisor_handle_crash(Scheduler *sched, ActorRef *crashed);
void supervisor_restart_child(Scheduler *sched, ActorRef *child);
```

---

## 5. 消息传递协议

```
发送方                    运行时                    接收方
  |                          |                        |
  |--- actor_send(from,to,method,arg) --->|          |
  |                          |--- enqueue(msg) -->|  |
  |                          |<-- 0 (success) ---|  |
  |<--- 0 ------------------|                        |
  |                          |                        |
  |                  [scheduler_tick()]              |
  |                          |--- dequeue(msg) -->|  |
  |                          |--- dispatch(method)-->|
  |                          |                        |
```

### 死信处理

- 如果目标 Actor 已销毁 → 消息进入死信队列
- 如果队列满 → 发送方收到 `ActorBusy` 错误
- 如果发送方等待响应且超时 → 发送 `Timeout` 消息给发送方

---

## 6. 代码生成改动

### 6.1 Actor 生成代码

```c
/* 为每个 Actor 生成：
   - create()       构造函数
   - init()         初始化行为（启动时调用）
   - handle_<method>() 每个 be 方法生成一个 handler
   - behavior()     调度器调用的统一入口
*/
static void A_handle_send(A_t *self, const char *msg) {
    printf("%s\n", msg);
}

static void A_behavior(A_t *self, const char *method, const void *arg, size_t arg_size) {
    if (strcmp(method, "send") == 0) {
        A_handle_send(self, (const char*)arg);
    }
}
```

### 6.2 消息发送生成

```c
/* target ! msg */
actor_send(&sender_ref, &target_ref, "handle", msg, strlen(msg) + 1);

/* val reply = target @ query() */
void *reply = NULL;
size_t reply_size = 0;
actor_call(&sender_ref, &target_ref, "query", NULL, 0, &reply, &reply_size);
```

### 6.3 监督树生成

```c
/* supervise Worker one_for_one */
static void register_supervision(Scheduler *sched, ActorRef supervisor, ActorRef child) {
    SuperviseMeta *meta = (SuperviseMeta*)calloc(1, sizeof(SuperviseMeta));
    meta->supervisor = supervisor;
    meta->strategy = SUPERVISE_ONE_FOR_ONE;
    meta->max_restarts = 3;
    child->supervise = meta;
    /* 将 child 添加到 supervisor 的 children 列表 */
}
```

---

## 7. 编译流程

```
        +----------------+
        |  Parser/AST    |
        +-------+--------+
                |
        +-------v--------+
        |  Actor Graph   |   新增：构建 Actor 依赖图
        |  (topo sort)   |   确定启动顺序
        +-------+--------+
                |
        +-------v--------+
        |  Codegen       |
        |  - 每个 Actor  |
        |  - 监督注册    |
        |  - 消息派发    |
        +-------+--------+
                |
        +-------v--------+
        |  Link + Run    |   链接 Actor + 运行时
        +----------------+
```

---

## 8. 测试计划

| 测试用例 | 说明 | 优先级 |
|----------|------|--------|
| actor_send_basic | 两个 Actor 互发消息 | P0 |
| actor_send_chain | 3+ Actor 链式传递 | P0 |
| actor_call_sync | 同步请求-响应 | P1 |
| actor_supervise_one_for_one | 子崩溃只重启子 | P0 |
| actor_supervise_one_for_all | 子崩溃重启所有 | P1 |
| actor_supervise_restart | 带退避重启 | P1 |
| actor_supervise_none | 崩溃后不重启 | P0 |
| actor_registry_lookup | 按名称查找 | P1 |
| actor_dead_letter | 目标不存在 | P1 |
| actor_queue_full | 队列满时行为 | P2 |

---

## 9. 文件结构

```
Ponyplusplus/
├── src/
│   ├── actor.c          # 新增：Actor 运行时核心
│   ├── actor.h
│   ├── scheduler.c      # 新增：调度器
│   ├── scheduler.h
│   └── ponypp/
│       └── runtime.c    # 修改：集成 Actor 调度
├── include/ponypp/
│   ├── actor.h
│   ├── scheduler.h
│   └── codegen.h        # 修改：添加 Actor 代码生成
```
