#include "ponypp/runtime.h"
#include "ponypp/util.h"
#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

void actor_behavior_dummy(PnyActor *self, PnyMessage *msg) {
    /* no-op behavior */
    (void)self;
    (void)msg;
}

TEST(RuntimeExtra, NewFree) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_TRUE(r != nullptr);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, ActorNew) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "Main", sizeof(int));
    ASSERT_TRUE(a != nullptr);
    ASSERT_STREQ(a->name, "Main");
    ASSERT_EQ(a->state_size, sizeof(int));
    ASSERT_TRUE(a->state_data != nullptr);
    ASSERT_EQ(a->actor_state, ACTOR_STATE_INIT);
    ASSERT_EQ(a->id, 1);
    ASSERT_EQ(r->scheduler.actor_count, 1);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, ActorRef) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "Worker", 0);
    ASSERT_TRUE(a != nullptr);
    ActorRef *ref = pny_actor_ref(a);
    ASSERT_TRUE(ref != nullptr);
    ASSERT_EQ(ref->id, 1);
    ASSERT_STREQ(ref->name, "Worker");
    ASSERT_EQ(ref->actor, a);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, SendReceive) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *sender = pny_actor_new(r, "Sender", 0);
    PnyActor *receiver = pny_actor_new(r, "Receiver", 0);
    receiver->behavior = actor_behavior_dummy;
    pny_scheduler_start(r);
    const char *msg_data = "hello world";
    int rc = pny_actor_send(&sender->self, &receiver->self, "handle", (void*)msg_data, strlen(msg_data) + 1);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(receiver->message_count, 1);
    ASSERT_TRUE(receiver->messages != nullptr);
    ASSERT_STREQ(receiver->messages->method, "handle");
    ASSERT_EQ(receiver->messages->arg_size, strlen(msg_data) + 1);
    ASSERT_STREQ((const char*)receiver->messages->arg, "hello world");
    pny_runtime_free(r);
}

TEST(RuntimeExtra, SchedulerTick) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "Worker", 0);
    ASSERT_TRUE(a != nullptr);
    a->behavior = actor_behavior_dummy;
    pny_scheduler_start(r);
    const char *d1 = "msg1";
    const char *d2 = "msg2";
    pny_actor_send(&a->self, &a->self, "handle", (void*)d1, strlen(d1) + 1);
    pny_actor_send(&a->self, &a->self, "handle", (void*)d2, strlen(d2) + 1);
    ASSERT_EQ(a->message_count, 2);
    pny_scheduler_tick(r);
    ASSERT_EQ(a->message_count, 1);
    ASSERT_EQ(r->stats.messages_delivered, 1);
    pny_scheduler_tick(r);
    ASSERT_EQ(a->message_count, 0);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, MultiActor) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a1 = pny_actor_new(r, "A", 0);
    PnyActor *a2 = pny_actor_new(r, "B", 0);
    PnyActor *a3 = pny_actor_new(r, "C", 0);
    ASSERT_EQ(a1->id, 1);
    ASSERT_EQ(a2->id, 2);
    ASSERT_EQ(a3->id, 3);
    ASSERT_EQ(r->scheduler.actor_count, 3);
    pny_scheduler_start(r);
    ASSERT_EQ(a1->actor_state, ACTOR_STATE_RUNNING);
    ASSERT_EQ(a2->actor_state, ACTOR_STATE_RUNNING);
    ASSERT_EQ(a3->actor_state, ACTOR_STATE_RUNNING);
    pny_actor_send(&a1->self, &a2->self, "msg", nullptr, 0);
    ASSERT_EQ(a2->message_count, 1);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, SuperviseRegister) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *parent = pny_actor_new(r, "Supervisor", 0);
    PnyActor *child = pny_actor_new(r, "Worker", 0);
    ASSERT_TRUE(parent != nullptr && child != nullptr);
    pny_supervise_register(parent, &child->self, SUPERVISE_ONE_FOR_ONE, 5);
    ASSERT_TRUE(child->supervise != nullptr);
    ASSERT_EQ(child->supervise->strategy, SUPERVISE_ONE_FOR_ONE);
    ASSERT_EQ(child->supervise->max_restarts, 5);
    ASSERT_EQ(child->supervise->restart_count, 0);
    ASSERT_EQ(child->supervise->supervisor.id, parent->id);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, SuperviseOneForOne) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *parent = pny_actor_new(r, "Supervisor", 0);
    PnyActor *child = pny_actor_new(r, "Worker", 0);
    pny_supervise_register(parent, &child->self, SUPERVISE_ONE_FOR_ONE, 3);
    pny_supervisor_handle_crash(r, &child->self);
    ASSERT_EQ(child->actor_state, ACTOR_STATE_RESTARTING);
    ASSERT_EQ(child->supervise->restart_count, 1);
    ASSERT_EQ(r->stats.restarts, 1);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, SuperviseRestart) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *parent = pny_actor_new(r, "Supervisor", 0);
    PnyActor *child = pny_actor_new(r, "Worker", 0);
    pny_supervise_register(parent, &child->self, SUPERVISE_RESTART, 3);
    pny_supervisor_handle_crash(r, &child->self);
    ASSERT_EQ(child->actor_state, ACTOR_STATE_RESTARTING);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, SuperviseNone) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *parent = pny_actor_new(r, "Supervisor", 0);
    PnyActor *child = pny_actor_new(r, "Worker", 0);
    pny_supervise_register(parent, &child->self, SUPERVISE_NONE, 3);
    pny_supervisor_handle_crash(r, &child->self);
    ASSERT_EQ(child->actor_state, ACTOR_STATE_STOPPED);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, SuperviseMaxRestarts) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *parent = pny_actor_new(r, "Supervisor", 0);
    PnyActor *child = pny_actor_new(r, "Worker", 0);
    pny_supervise_register(parent, &child->self, SUPERVISE_ONE_FOR_ONE, 2);
    pny_supervisor_handle_crash(r, &child->self);
    pny_supervisor_handle_crash(r, &child->self);
    ASSERT_EQ(child->supervise->restart_count, 2);
    /* third crash should stop */
    pny_supervisor_handle_crash(r, &child->self);
    ASSERT_EQ(child->actor_state, ACTOR_STATE_STOPPED);
    ASSERT_EQ(child->supervise->restart_count, 2);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, SchedulerStartStop) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "Worker", 0);
    ASSERT_EQ(a->actor_state, ACTOR_STATE_INIT);
    pny_scheduler_start(r);
    ASSERT_EQ(a->actor_state, ACTOR_STATE_RUNNING);
    pny_scheduler_stop(r);
    ASSERT_EQ(a->actor_state, ACTOR_STATE_STOPPING);
    ASSERT_FALSE(r->scheduler.running);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, Stats) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a1 = pny_actor_new(r, "A", 0);
    PnyActor *a2 = pny_actor_new(r, "B", 0);
    pny_scheduler_start(r);
    pny_actor_send(&a1->self, &a2->self, "m", nullptr, 0);
    pny_scheduler_tick(r);
    RuntimeStats stats;
    pny_runtime_stats(r, &stats);
    ASSERT_EQ(stats.actors_alive, 2);
    ASSERT_EQ(stats.actors_created, 2);
    ASSERT_EQ(stats.messages_sent, 1);
    ASSERT_EQ(stats.messages_delivered, 1);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, ActorDestroy) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "Worker", 4);
    ASSERT_TRUE(a != nullptr);
    pny_actor_destroy(r, &a->self);
    ASSERT_EQ(a->actor_state, ACTOR_STATE_STOPPED);
    ASSERT_EQ(r->stats.actors_destroyed, 1);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, SendFullQueue) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "Receiver", 0);
    a->max_messages = 2;
    a->message_count = 2;
    int rc = pny_actor_send(&a->self, &a->self, "m", nullptr, 0);
    ASSERT_EQ(rc, -3);
    pny_runtime_free(r);
}

TEST(RuntimeExtra, SendStopped) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "Receiver", 0);
    a->actor_state = ACTOR_STATE_STOPPED;
    int rc = pny_actor_send(&a->self, &a->self, "m", nullptr, 0);
    ASSERT_EQ(rc, -2);
    pny_runtime_free(r);
}

/* ======================== Reply Channel ======================== */

/* behavior: 收到消息时回复 result */
void actor_behavior_reply(PnyActor *self, PnyMessage *msg) {
    (void)self;
    if (msg->reply) {
        /* 用 arg 内容作为回复 */
        if (msg->arg && msg->arg_size > 0) {
            pny_actor_reply(msg, msg->arg, msg->arg_size);
        } else {
            pny_actor_reply(msg, NULL, 0);
        }
    }
}

TEST(RuntimeReply, ActorCallSync) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "Worker", 0);
    a->behavior = actor_behavior_reply;
    pny_scheduler_start(r);
    const char *req = "ping";
    void *result = NULL;
    size_t result_size = 0;
    int rc = pny_actor_call(&a->self, &a->self, "query", (void*)req, strlen(req) + 1, &result, &result_size);
    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(result != nullptr);
    ASSERT_EQ(result_size, strlen(req) + 1);
    ASSERT_STREQ((const char*)result, "ping");
    free(result);
    pny_runtime_free(r);
}

TEST(RuntimeReply, ActorCallNoResult) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "Worker", 0);
    a->behavior = actor_behavior_reply;
    pny_scheduler_start(r);
    void *result = NULL;
    size_t result_size = 0;
    int rc = pny_actor_call(&a->self, &a->self, "noop", NULL, 0, &result, &result_size);
    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(result == nullptr);
    ASSERT_EQ(result_size, 0);
    pny_runtime_free(r);
}

TEST(RuntimeReply, ActorCallTargetStopped) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "Worker", 0);
    a->actor_state = ACTOR_STATE_STOPPED;
    void *result = NULL;
    size_t result_size = 0;
    int rc = pny_actor_call(&a->self, &a->self, "m", NULL, 0, &result, &result_size);
    ASSERT_EQ(rc, -2);
    pny_runtime_free(r);
}

TEST(RuntimeReply, ActorCallNullTarget) {
    PnyRuntime *r = pny_runtime_new();
    (void)r;
    void *result = NULL;
    size_t result_size = 0;
    int rc = pny_actor_call(NULL, NULL, "m", NULL, 0, &result, &result_size);
    ASSERT_EQ(rc, -1);
    pny_runtime_free(r);
}

/* ======================== Supervision Tree Children ======================== */

TEST(RuntimeSupervision, ChildrenTracking) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *parent = pny_actor_new(r, "Supervisor", 0);
    PnyActor *c1 = pny_actor_new(r, "Worker1", 0);
    PnyActor *c2 = pny_actor_new(r, "Worker2", 0);
    pny_supervise_register(parent, &c1->self, SUPERVISE_ONE_FOR_ONE, 3);
    pny_supervise_register(parent, &c2->self, SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_EQ(parent->child_count, 2);
    ASSERT_TRUE(parent->children != nullptr);
    ASSERT_TRUE(parent->children[0] == c1);
    ASSERT_TRUE(parent->children[1] == c2);
    pny_runtime_free(r);
}

TEST(RuntimeSupervision, OneForAllRestartsAll) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *parent = pny_actor_new(r, "Supervisor", 0);
    PnyActor *c1 = pny_actor_new(r, "Worker1", 0);
    PnyActor *c2 = pny_actor_new(r, "Worker2", 0);
    pny_supervise_register(parent, &c1->self, SUPERVISE_ONE_FOR_ALL, 3);
    pny_supervise_register(parent, &c2->self, SUPERVISE_ONE_FOR_ALL, 3);
    pny_scheduler_start(r);
    /* c1 崩溃，ONE_FOR_ALL 应重启所有子 Actor */
    pny_supervisor_handle_crash(r, &c1->self);
    ASSERT_EQ(c1->actor_state, ACTOR_STATE_RESTARTING);
    /* c2 也应为 RESTARTING（如果 children 被正确填充）*/
    ASSERT_EQ(c2->actor_state, ACTOR_STATE_RESTARTING);
    ASSERT_EQ(c1->supervise->restart_count, 1);
    pny_runtime_free(r);
}

TEST(RuntimeSupervision, SingleChild) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *parent = pny_actor_new(r, "Sup", 0);
    PnyActor *c1 = pny_actor_new(r, "W1", 0);
    pny_supervise_register(parent, &c1->self, SUPERVISE_ONE_FOR_ONE, 1);
    ASSERT_EQ(parent->child_count, 1);
    pny_supervisor_handle_crash(r, &c1->self);
    ASSERT_EQ(c1->actor_state, ACTOR_STATE_RESTARTING);
    /* 第二次崩溃应停止 */
    pny_supervisor_handle_crash(r, &c1->self);
    ASSERT_EQ(c1->actor_state, ACTOR_STATE_STOPPED);
    pny_runtime_free(r);
}

TEST(RuntimeSupervision, NoSupervisionNoCrash) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "Orphan", 0);
    pny_scheduler_start(r);
    ASSERT_EQ(a->actor_state, ACTOR_STATE_RUNNING);
    pny_supervisor_handle_crash(r, &a->self);
    /* 无监督元数据，不应改变状态 */
    ASSERT_EQ(a->actor_state, ACTOR_STATE_RUNNING);
    pny_runtime_free(r);
}
