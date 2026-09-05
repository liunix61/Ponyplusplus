#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "ponypp/runtime.h"
#include "ponypp/util.h"

PnyRuntime *pny_runtime_global = NULL;

/* ======================== 运行时 ======================== */

PnyRuntime *pny_runtime_new(void) {
    PnyRuntime *r = (PnyRuntime*)s_malloc(sizeof(PnyRuntime));
    if (!r) return NULL;
    memset(r, 0, sizeof(PnyRuntime));
    r->scheduler.next_id = 1;
    r->scheduler.max_actors = 1024;
    r->scheduler.registry = (PnyActor**)s_malloc(sizeof(PnyActor*) * r->scheduler.max_actors);
    if (!r->scheduler.registry) { s_free(r); return NULL; }
    memset(r->scheduler.registry, 0, sizeof(PnyActor*) * r->scheduler.max_actors);
    pny_runtime_global = r;
    return r;
}

void pny_runtime_free(PnyRuntime *r) {
    if (!r) return;
    PnyActor *a = r->scheduler.actors;
    while (a) {
        PnyActor *next = a->next;
        PnyMessage *m = a->messages;
        while (m) { PnyMessage *mn = m->next; pny_msg_free(m); m = mn; }
        if (a->supervise) { s_free(a->supervise); }
        if (a->children) { s_free(a->children); }
        if (a->delivered_ids) { s_free(a->delivered_ids); }
        s_free(a->state_data);
        s_free(a);
        a = next;
    }
    PnyMessage *dl = r->dead_letters;
    while (dl) { PnyMessage *n = dl->next; pny_msg_free(dl); dl = n; }
    s_free(r->scheduler.registry);
    s_free(r);
}

/* ======================== Actor ======================== */

PnyActor *pny_actor_new(PnyRuntime *r, const char *name, size_t state_size) {
    if (!r) return NULL;
    PnyActor *a = (PnyActor*)s_malloc(sizeof(PnyActor));
    if (!a) return NULL;
    memset(a, 0, sizeof(PnyActor));
    a->name = name;
    a->state_size = state_size;
    a->state_data = state_size > 0 ? s_malloc(state_size) : NULL;
    if (a->state_data) memset(a->state_data, 0, state_size);
    a->actor_state = ACTOR_STATE_INIT;
    a->max_messages = 65536;
    /* 分配 ID */
    int id = r->scheduler.next_id++;
    a->id = id;
    a->self.id = id;
    a->self.name = s_strdup(name ? name : "");
    a->self.actor = a;
    /* 注册 */
    size_t idx = (size_t)id - 1;
    if (idx < r->scheduler.max_actors) {
        r->scheduler.registry[idx] = a;
    }
    /* 插入链表 */
    a->next = r->scheduler.actors;
    r->scheduler.actors = a;
    r->scheduler.actor_count++;
    r->stats.actors_created++;
    return a;
}

void pny_actor_register(PnyRuntime *r, PnyActor *a) {
    if (!r || !a) return;
    /* 已在新版本中由 pny_actor_new 处理，这里保持兼容 */
    if (!a->next) {
        a->next = r->scheduler.actors;
        r->scheduler.actors = a;
        r->scheduler.actor_count++;
    }
}

void pny_actor_destroy(PnyRuntime *r, ActorRef *ref) {
    if (!r || !ref || !ref->actor) return;
    PnyActor *a = ref->actor;
    a->actor_state = ACTOR_STATE_STOPPED;
    /* 从链表移除 */
    PnyActor **pp = &r->scheduler.actors;
    while (*pp) {
        if (*pp == a) {
            *pp = a->next;
            break;
        }
        pp = &(*pp)->next;
    }
    /* 清理 */
    PnyMessage *m = a->messages;
    while (m) { PnyMessage *n = m->next; pny_msg_free(m); m = n; }
    s_free(a->state_data);
    s_free(a->supervise);
    if (a->children) s_free(a->children);
    if (a->delivered_ids) s_free(a->delivered_ids);
    s_free(a->self.name);
    s_free(a);
    r->stats.actors_destroyed++;
    r->stats.actors_alive--;
}

ActorRef *pny_actor_ref(PnyActor *a) {
    if (!a) return NULL;
    return &a->self;
}

/* ======================== 消息 ======================== */

PnyMessage *pny_msg_new(const char *method, void *arg, size_t arg_size) {
    PnyMessage *m = (PnyMessage*)s_malloc(sizeof(PnyMessage));
    if (!m) return NULL;
    m->method = s_strdup(method ? method : "");
    m->arg = NULL;
    m->arg_size = arg_size;
    m->next = NULL;
    m->sender.id = -1;
    m->sender.name = NULL;
    m->sender.actor = NULL;
    if (arg && arg_size > 0) {
        m->arg = s_malloc(arg_size);
        if (m->arg) memcpy(m->arg, arg, arg_size);
    }
    return m;
}

void pny_msg_free(PnyMessage *m) {
    if (!m) return;
    s_free(m->method);
    s_free(m->arg);
    /* reply 由 pny_actor_call 管理，这里不清理 */
    m->reply = NULL;
    s_free(m);
}

int pny_actor_send(ActorRef *from, ActorRef *to, const char *method, void *arg, size_t arg_size) {
    if (!to || !to->actor) return -1;
    PnyActor *a = to->actor;
    if (a->actor_state != ACTOR_STATE_RUNNING &&
        a->actor_state != ACTOR_STATE_INIT) return -2;
    if (a->message_count >= a->max_messages) return -3;
    PnyMessage *m = pny_msg_new(method, arg, arg_size);
    if (!m) return -4;
    m->sender.id = from ? from->id : -1;
    m->sender.name = from ? s_strdup(from->name) : NULL;
    m->sender.actor = from ? from->actor : NULL;
    if (a->messages) {
        PnyMessage *tail = a->messages;
        while (tail->next) tail = tail->next;
        tail->next = m;
    } else {
        a->messages = m;
    }
    a->message_count++;
    if (pny_runtime_global) pny_runtime_global->stats.messages_sent++;
    return 0;
}

int pny_actor_call(ActorRef *from, ActorRef *to, const char *method, void *arg, size_t arg_size, void **result, size_t *result_size) {
    if (!to || !to->actor) return -1;
    PnyActor *a = to->actor;
    if (a->actor_state != ACTOR_STATE_RUNNING &&
        a->actor_state != ACTOR_STATE_INIT) return -2;
    if (a->message_count >= a->max_messages) return -3;
    PnyMessage *m = pny_msg_new(method, arg, arg_size);
    if (!m) return -4;
    m->sender.id = from ? from->id : -1;
    m->sender.name = from ? s_strdup(from->name) : NULL;
    m->sender.actor = from ? from->actor : NULL;
    /* 创建回复通道 */
    PnyReply *reply = (PnyReply*)s_malloc(sizeof(PnyReply));
    if (!reply) { pny_msg_free(m); return -5; }
    memset(reply, 0, sizeof(PnyReply));
    reply->done = 0;
    reply->error = 0;
    m->reply = reply;
    /* 入队 */
    if (a->messages) {
        PnyMessage *tail = a->messages;
        while (tail->next) tail = tail->next;
        tail->next = m;
    } else {
        a->messages = m;
    }
    a->message_count++;
    if (pny_runtime_global) pny_runtime_global->stats.messages_sent++;
    /* 同步等待：调度器 tick 直到有结果（最多 1000 次防止死循环） */
    PnyRuntime *rt = pny_runtime_global;
    int max_ticks = 1000;
    while (!reply->done && max_ticks-- > 0 && rt) {
        pny_scheduler_tick(rt);
    }
    if (result) *result = reply->result;    /* 所有权转移给调用者 */
    if (result_size) *result_size = reply->result_size;
    int err = reply->error;
    reply->result = NULL;
    s_free(reply);
    return err;
}

/* 同步调用回复：behavior 中调用此函数返回结果 */
void pny_actor_reply(PnyMessage *msg, void *result, size_t result_size) {
    if (!msg || !msg->reply) return;
    if (result && result_size > 0) {
        void *copy = s_malloc(result_size);
        if (!copy) {
            msg->reply->error = -1;
            msg->reply->done = 1;
            return;
        }
        memcpy(copy, result, result_size);
        msg->reply->result = copy;
        msg->reply->result_size = result_size;
    }
    msg->reply->done = 1;
}

/* ======================== 调度器 ======================== */

void pny_scheduler_tick(PnyRuntime *r) {
    if (!r) return;
    PnyActor *a = r->scheduler.actors;
    while (a) {
        PnyActor *next = a->next;
        if (a->messages && a->actor_state == ACTOR_STATE_RUNNING) {
            PnyMessage *m = a->messages;
            a->messages = m->next;
            a->message_count--;
            if (a->behavior) {
                a->behavior(a, m);
            }
            r->stats.messages_delivered++;
            pny_msg_free(m);
        }
        a = next;
    }
    r->scheduler.tick_count++;
    r->stats.total_ticks++;
}

void pny_scheduler_start(PnyRuntime *r) {
    if (!r) return;
    r->scheduler.running = true;
    /* 先启动 Supervisor（有 children 的 Actor） */
    PnyActor *a = r->scheduler.actors;
    while (a) {
        if (a->children && a->child_count > 0) {
            a->actor_state = ACTOR_STATE_RUNNING;
            r->stats.actors_alive++;
        }
        a = a->next;
    }
    /* 再启动子 Actor（被监督的） */
    a = r->scheduler.actors;
    while (a) {
        if (a->supervise) {
            a->actor_state = ACTOR_STATE_RUNNING;
            r->stats.actors_alive++;
        }
        a = a->next;
    }
    /* 最后启动独立 Actor（无监督） */
    a = r->scheduler.actors;
    while (a) {
        if (!a->supervise && !(a->children && a->child_count > 0) &&
            a->actor_state != ACTOR_STATE_RUNNING) {
            a->actor_state = ACTOR_STATE_RUNNING;
            r->stats.actors_alive++;
        }
        a = a->next;
    }
}

void pny_scheduler_stop(PnyRuntime *r) {
    if (!r) return;
    r->scheduler.running = false;
    PnyActor *a = r->scheduler.actors;
    while (a) {
        a->actor_state = ACTOR_STATE_STOPPING;
        a = a->next;
    }
}

/* ======================== 监督 ======================== */

void pny_supervise_register(PnyActor *parent, ActorRef *child, SuperviseStrategy strategy, int max_restarts) {
    if (!parent || !child) return;
    PnyActor *c = child->actor;
    if (!c) return;
    c->supervise = (SuperviseMeta*)s_malloc(sizeof(SuperviseMeta));
    if (!c->supervise) return;
    memset(c->supervise, 0, sizeof(SuperviseMeta));
    c->supervise->supervisor.id = parent->id;
    c->supervise->supervisor.name = s_strdup(parent->name);
    c->supervise->supervisor.actor = parent;
    c->supervise->strategy = strategy;
    c->supervise->max_restarts = max_restarts > 0 ? max_restarts : 3;
    c->supervise->restart_count = 0;
    /* 添加 child 到 parent 的 children 数组 */
    PnyActor **new_children = (PnyActor**)s_malloc(sizeof(PnyActor*) * (parent->child_count + 1));
    if (!new_children) return;
    if (parent->child_count > 0 && parent->children) {
        memcpy(new_children, parent->children, sizeof(PnyActor*) * parent->child_count);
    }
    new_children[parent->child_count] = c;
    s_free(parent->children);
    parent->children = new_children;
    parent->child_count++;
}

void pny_supervisor_handle_crash(PnyRuntime *r, ActorRef *crashed) {
    if (!r || !crashed || !crashed->actor) return;
    PnyActor *a = crashed->actor;
    if (!a->supervise) return;
    SuperviseMeta *meta = a->supervise;
    if (meta->restart_count >= meta->max_restarts) {
        /* 达到最大重启次数，停止 */
        a->actor_state = ACTOR_STATE_STOPPED;
        return;
    }
    meta->restart_count++;
    r->stats.restarts++;
    switch (meta->strategy) {
        case SUPERVISE_ONE_FOR_ONE:
            /* 只重启崩溃的 Actor */
            a->actor_state = ACTOR_STATE_RESTARTING;
            break;
        case SUPERVISE_ONE_FOR_ALL:
            /* 重启所有子 Actor */
            if (meta->supervisor.actor && meta->supervisor.actor->children) {
                for (size_t i = 0; i < meta->supervisor.actor->child_count; i++) {
                    PnyActor *c = meta->supervisor.actor->children[i];
                    if (c) c->actor_state = ACTOR_STATE_RESTARTING;
                }
            }
            break;
        case SUPERVISE_RESTART:
            a->actor_state = ACTOR_STATE_RESTARTING;
            break;
        case SUPERVISE_NONE:
            a->actor_state = ACTOR_STATE_STOPPED;
            break;
    }
}

void pny_runtime_stats(PnyRuntime *r, RuntimeStats *out) {
    if (!r || !out) return;
    memcpy(out, &r->stats, sizeof(RuntimeStats));
}

/* ======================== 跨组件序列化 ======================== */

int pny_msg_serialize(PnyMessage *msg, void *buf, size_t buf_size, size_t *out_size) {
    if (!msg || !buf || !out_size) return -1;

    PnyMsgHeader hdr;
    hdr.version = 0x0001;
    hdr.actor_id = (uint32_t)(msg->sender.actor ? msg->sender.actor->id : 0);
    hdr.method_id = 0;

    size_t pos = 0;
    size_t need = sizeof(PnyMsgHeader) + 1 + 2;
    if (msg->method) need += strlen(msg->method);
    need += 4;
    if (msg->arg && msg->arg_size > 0) need += msg->arg_size;

    if (buf_size < need) return -2;

    memcpy(buf, &hdr, sizeof(hdr));
    pos += sizeof(hdr);

    ((uint8_t *)buf)[pos++] = (uint8_t)(msg->cap_mark ? msg->cap_mark : PNY_CAP_VAL);

    uint16_t method_len = msg->method ? (uint16_t)strlen(msg->method) : 0;
    memcpy((uint8_t *)buf + pos, &method_len, 2);
    pos += 2;
    if (msg->method && method_len > 0) {
        memcpy((uint8_t *)buf + pos, msg->method, method_len);
        pos += method_len;
    }

    uint32_t arg_size = msg->arg_size;
    memcpy((uint8_t *)buf + pos, &arg_size, 4);
    pos += 4;
    if (msg->arg && arg_size > 0) {
        memcpy((uint8_t *)buf + pos, msg->arg, arg_size);
        pos += arg_size;
    }

    *out_size = pos;
    return 0;
}

int pny_msg_deserialize(void *buf, size_t buf_size, PnyMessage **out) {
    if (!buf || !out || buf_size < 11) return -1;
    if (*out) return -2;

    size_t pos = 0;
    PnyMsgHeader hdr;
    memcpy(&hdr, buf, sizeof(hdr));
    pos += sizeof(hdr);

    uint8_t cap_mark = ((uint8_t *)buf)[pos++];
    uint16_t method_len;
    memcpy(&method_len, (uint8_t *)buf + pos, 2);
    pos += 2;

    if (buf_size < pos + method_len + 4) return -3;

    PnyMessage *msg = (PnyMessage *)s_malloc(sizeof(PnyMessage));
    if (!msg) return -4;
    memset(msg, 0, sizeof(PnyMessage));
    msg->cap_mark = (PnyCapMark)cap_mark;

    if (method_len > 0) {
        msg->method = s_malloc(method_len + 1);
        if (!msg->method) { s_free(msg); return -5; }
        memcpy(msg->method, (uint8_t *)buf + pos, method_len);
        msg->method[method_len] = 0;
        pos += method_len;
    }

    uint32_t arg_size;
    memcpy(&arg_size, (uint8_t *)buf + pos, 4);
    pos += 4;

    if (arg_size > 0 && buf_size >= pos + arg_size) {
        msg->arg = s_malloc(arg_size);
        if (!msg->arg) { s_free(msg->method); s_free(msg); return -6; }
        memcpy(msg->arg, (uint8_t *)buf + pos, arg_size);
        msg->arg_size = arg_size;
    }

    *out = msg;
    return 0;
}

/* ======================== 背压 ======================== */

BackpressureState pny_backpressure_check(PnyActor *a) {
    if (!a) return BACKPRESSURE_FULL;
    if (a->message_count >= a->max_messages) return BACKPRESSURE_FULL;
    if (a->message_count >= a->max_messages / 2) return BACKPRESSURE_WARN;
    return BACKPRESSURE_OK;
}

void pny_backpressure_set_limit(PnyActor *a, size_t max_messages) {
    if (!a) return;
    a->max_messages = max_messages > 0 ? max_messages : 65536;
}

/* ======================== exactly-once 去重 ======================== */

int pny_msg_delivered(PnyActor *a, uint64_t msg_id) {
    if (!a) return -1;

    for (size_t i = 0; i < a->delivered_count; i++) {
        if (a->delivered_ids[i] == msg_id) return 1;
    }

    if (a->delivered_count >= a->delivered_cap) {
        a->delivered_cap = a->delivered_cap ? a->delivered_cap * 2 : 128;
        uint64_t *new_ids = (uint64_t *)s_malloc(sizeof(uint64_t) * a->delivered_cap);
        if (!new_ids) return -2;
        if (a->delivered_ids) {
            memcpy(new_ids, a->delivered_ids, sizeof(uint64_t) * a->delivered_count);
            s_free(a->delivered_ids);
        }
        a->delivered_ids = new_ids;
    }
    a->delivered_ids[a->delivered_count++] = msg_id;
    return 0;
}
