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
    ActorRef supervisor;
    SuperviseStrategy strategy;
    int max_restarts;
    int restart_count;
} SuperviseMeta;

/* 消息队列项 */
typedef struct PnyMessage {
    char *method;
    void *arg;
    size_t arg_size;
    struct PnyMessage *next;
    ActorRef sender;
} PnyMessage;

/* Actor 结构 */
typedef struct PnyActor {
    const char *name;
    void *state_data;
    size_t state_size;
    struct PnyActor *next;
    PnyMessage *messages;
    size_t message_count;
    size_t max_messages;
    ActorState actor_state;
    ActorRef self;
    SuperviseMeta *supervise;
    struct PnyActor **children;
    size_t child_count;
    int id;
    void (*behavior)(struct PnyActor *self, PnyMessage *msg);
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

/* 消息 */
PnyMessage *pny_msg_new(const char *method, void *arg, size_t arg_size);
void pny_msg_free(PnyMessage *m);
int pny_actor_send(ActorRef *from, ActorRef *to, const char *method, void *arg, size_t arg_size);
int pny_actor_call(ActorRef *from, ActorRef *to, const char *method, void *arg, size_t arg_size, void **result, size_t *result_size);

/* 调度 */
void pny_scheduler_tick(PnyRuntime *r);
void pny_scheduler_start(PnyRuntime *r);
void pny_scheduler_stop(PnyRuntime *r);

/* 监督 */
void pny_supervise_register(PnyActor *parent, ActorRef *child, SuperviseStrategy strategy, int max_restarts);
void pny_supervisor_handle_crash(PnyRuntime *r, ActorRef *crashed);

/* 统计 */
void pny_runtime_stats(PnyRuntime *r, RuntimeStats *out);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_RUNTIME_H */
