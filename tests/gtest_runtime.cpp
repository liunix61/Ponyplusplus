#include "ponypp/runtime.h"
#include "ponypp/util.h"
#include "ponypp/distributed.h"
#include "ponypp/ffi.h"
#include "ponypp/pkg.h"
#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

void actor_behavior_dummy(PnyActor *self, PnyMessage *msg) {
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

void actor_behavior_reply(PnyActor *self, PnyMessage *msg) {
    (void)self;
    if (msg->reply) {
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
    pny_supervisor_handle_crash(r, &c1->self);
    ASSERT_EQ(c1->actor_state, ACTOR_STATE_RESTARTING);
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
    ASSERT_EQ(a->actor_state, ACTOR_STATE_RUNNING);
    pny_runtime_free(r);
}

/* ======================== Phase P1: 跨组件序列化 ======================== */

TEST(P1Serialization, SerializeDeserialize) {
    uint8_t arg_data[] = {0x34, 0x32};
    PnyMessage *msg = pny_msg_new("increment", arg_data, 2);
    ASSERT_NE(msg, nullptr);
    msg->cap_mark = PNY_CAP_VAL;

    uint8_t buf[256];
    size_t out_size = 0;
    int rc = pny_msg_serialize(msg, buf, sizeof(buf), &out_size);
    ASSERT_EQ(rc, 0);
    EXPECT_GT(out_size, 0);

    PnyMessage *deserialized = NULL;
    rc = pny_msg_deserialize(buf, out_size, &deserialized);
    ASSERT_EQ(rc, 0);
    ASSERT_NE(deserialized, nullptr);
    ASSERT_NE(deserialized->method, nullptr);
    EXPECT_STREQ(deserialized->method, "increment");
    EXPECT_EQ(deserialized->cap_mark, PNY_CAP_VAL);

    pny_msg_free(msg);
    pny_msg_free(deserialized);
}

TEST(P1Serialization, SerializeNull) {
    int rc = pny_msg_serialize(NULL, NULL, 0, NULL);
    EXPECT_EQ(rc, -1);
}

TEST(P1Serialization, DeserializeNull) {
    PnyMessage *out = NULL;
    int rc = pny_msg_deserialize(NULL, 0, &out);
    EXPECT_EQ(rc, -1);
}

TEST(P1Serialization, RoundTripWithArg) {
    uint8_t arg_data[12] = {0};
    PnyMessage *msg = pny_msg_new("add", arg_data, 12);
    ASSERT_NE(msg, nullptr);
    msg->cap_mark = PNY_CAP_ISO;

    uint8_t buf[512];
    size_t out_size = 0;
    int rc = pny_msg_serialize(msg, buf, sizeof(buf), &out_size);
    ASSERT_EQ(rc, 0);

    PnyMessage *out = NULL;
    rc = pny_msg_deserialize(buf, out_size, &out);
    ASSERT_EQ(rc, 0);
    ASSERT_NE(out, nullptr);
    EXPECT_STREQ(out->method, "add");
    EXPECT_EQ(out->arg_size, 12);
    EXPECT_EQ(out->cap_mark, PNY_CAP_ISO);

    pny_msg_free(msg);
    pny_msg_free(out);
}

/* ======================== Phase P1: 背压 ======================== */

TEST(P1Backpressure, CheckEmptyActor) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "BP1", 0);
    ASSERT_EQ(pny_backpressure_check(a), BACKPRESSURE_OK);
    pny_runtime_free(r);
}

TEST(P1Backpressure, CheckFullActor) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "BP2", 0);
    pny_backpressure_set_limit(a, 2);
    ActorRef sender = {0, NULL, NULL};
    pny_actor_send(&sender, &a->self, "m1", NULL, 0);
    pny_actor_send(&sender, &a->self, "m2", NULL, 0);
    EXPECT_EQ(pny_backpressure_check(a), BACKPRESSURE_FULL);
    int rc = pny_actor_send(&sender, &a->self, "m3", NULL, 0);
    EXPECT_EQ(rc, -3);
    pny_runtime_free(r);
}

TEST(P1Backpressure, SetLimit) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "BP3", 0);
    pny_backpressure_set_limit(a, 100);
    EXPECT_EQ(a->max_messages, 100);
    pny_backpressure_set_limit(a, 0);
    EXPECT_EQ(a->max_messages, 65536);
    pny_runtime_free(r);
}

/* ======================== Phase P1: exactly-once 去重 ======================== */

TEST(P1ExactlyOnce, NewMessage) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "EOC1", 0);
    EXPECT_EQ(pny_msg_delivered(a, 1), 0);
    pny_runtime_free(r);
}

TEST(P1ExactlyOnce, DuplicateDetection) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "EOC2", 0);
    EXPECT_EQ(pny_msg_delivered(a, 42), 0);
    EXPECT_EQ(pny_msg_delivered(a, 42), 1);
    EXPECT_EQ(pny_msg_delivered(a, 43), 0);
    EXPECT_EQ(pny_msg_delivered(a, 42), 1);
    pny_runtime_free(r);
}

TEST(P1ExactlyOnce, MultipleMessages) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "EOC3", 0);
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(pny_msg_delivered(a, (uint64_t)i), 0);
    }
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(pny_msg_delivered(a, (uint64_t)i), 1);
    }
    pny_runtime_free(r);
}

TEST(P1ExactlyOnce, NullActor) {
    EXPECT_EQ(pny_msg_delivered(NULL, 1), -1);
}

TEST(P1Serialization, CapabilityMarks) {
    PnyMessage *msg = pny_msg_new("test", NULL, 0);
    ASSERT_NE(msg, nullptr);
    msg->cap_mark = PNY_CAP_ISO;
    uint8_t buf[64];
    size_t out_size = 0;
    int rc = pny_msg_serialize(msg, buf, sizeof(buf), &out_size);
    ASSERT_EQ(rc, 0);
    PnyMessage *out = NULL;
    rc = pny_msg_deserialize(buf, out_size, &out);
    ASSERT_EQ(rc, 0);
    EXPECT_EQ(out->cap_mark, PNY_CAP_ISO);
    pny_msg_free(msg);
    pny_msg_free(out);
}

/* ======================== Phase P2: Gas 计数 ======================== */

TEST(P2Gas, UnlimitedGas) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "GasUnlimited", 0);
    uint64_t remaining = pny_gas_consume(a, 1000);
    EXPECT_EQ(remaining, UINT64_MAX);
    pny_runtime_free(r);
}

TEST(P2Gas, LimitedGas) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "GasLimited", 0);
    pny_gas_set_limit(a, 100);
    uint64_t remaining = pny_gas_consume(a, 50);
    EXPECT_EQ(remaining, 50);
    remaining = pny_gas_consume(a, 50);
    EXPECT_EQ(remaining, 0);
    pny_runtime_free(r);
}

TEST(P2Gas, GasOverLimit) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "GasOver", 0);
    pny_gas_set_limit(a, 100);
    uint64_t remaining = pny_gas_consume(a, 150);
    EXPECT_EQ(remaining, 0);
    pny_runtime_free(r);
}

TEST(P2Gas, SetLimit) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "GasSet", 0);
    pny_gas_set_limit(a, 200);
    EXPECT_EQ(a->gas_limit, 200);
    EXPECT_EQ(a->gas_budget, 200);
    pny_runtime_free(r);
}

TEST(P2Gas, GasAccumulate) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "GasAccum", 0);
    pny_gas_set_limit(a, 1000);
    pny_gas_consume(a, 100);
    pny_gas_consume(a, 200);
    pny_gas_consume(a, 300);
    EXPECT_EQ(a->gas_used, 600);
    uint64_t remaining = pny_gas_consume(a, 400);
    EXPECT_EQ(remaining, 0);
    pny_runtime_free(r);
}

/* ======================== Phase P2: 热代码升级 ======================== */

static int hot_upgrade_flag = 0;
void old_behavior(PnyActor *self, PnyMessage *msg) {
    (void)self; (void)msg;
    hot_upgrade_flag = 1;
}
void new_behavior(PnyActor *self, PnyMessage *msg) {
    (void)self; (void)msg;
    hot_upgrade_flag = 2;
}

TEST(P2HotUpgrade, SetNewBehavior) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "HotUpgrade", 0);
    a->behavior = old_behavior;
    EXPECT_EQ(a->version, 1);
    EXPECT_EQ(pny_hot_upgrade_version(a), 1);
    pny_hot_upgrade(a, new_behavior);
    EXPECT_NE(a->new_behavior, nullptr);
    pny_runtime_free(r);
}

TEST(P2HotUpgrade, VersionCheck) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "HotVer", 0);
    EXPECT_EQ(pny_hot_upgrade_version(a), 1);
    pny_runtime_free(r);
}

TEST(P2HotUpgrade, NullActor) {
    pny_hot_upgrade(NULL, new_behavior);
    EXPECT_EQ(pny_hot_upgrade_version(NULL), -1);
}

/* ======================== Phase P2: 诊断接口 ======================== */

TEST(P2Diagnostics, StatsEmpty) {
    PnyRuntime *r = pny_runtime_new();
    ActorStats stats;
    pny_diagnostics_stats(r, &stats);
    EXPECT_EQ(stats.actors_total, 0);
    EXPECT_EQ(stats.messages_queued, 0);
    pny_runtime_free(r);
}

TEST(P2Diagnostics, StatsWithActors) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a1 = pny_actor_new(r, "DxActor1", 0);
    PnyActor *a2 = pny_actor_new(r, "DxActor2", 0);
    pny_scheduler_start(r);
    ActorRef sender = {0, NULL, NULL};
    pny_actor_send(&sender, &a1->self, "m1", NULL, 0);
    pny_actor_send(&sender, &a2->self, "m2", NULL, 0);
    pny_scheduler_tick(r);
    ActorStats stats;
    pny_diagnostics_stats(r, &stats);
    EXPECT_EQ(stats.actors_total, 2);
    EXPECT_EQ(stats.actors_running, 2);
    EXPECT_EQ(stats.messages_delivered, 2);
    EXPECT_GT(stats.scheduler_ticks, 0);
    pny_runtime_free(r);
}

TEST(P2Diagnostics, DumpText) {
    PnyRuntime *r = pny_runtime_new();
    pny_actor_new(r, "DumpActor", 0);
    pny_scheduler_start(r);
    pny_scheduler_tick(r);
    char buf[1024];
    const char *result = pny_diagnostics_dump(r, buf, sizeof(buf));
    ASSERT_NE(result, nullptr);
    EXPECT_NE(strstr(buf, "Pony++ Diagnostics"), nullptr);
    EXPECT_NE(strstr(buf, "Actors:"), nullptr);
    EXPECT_NE(strstr(buf, "Messages:"), nullptr);
    pny_runtime_free(r);
}

TEST(P2Diagnostics, DumpNull) {
    PnyRuntime *r = pny_runtime_new();
    EXPECT_EQ(pny_diagnostics_dump(NULL, NULL, 0), nullptr);
    pny_runtime_free(r);
}

TEST(P2Gas, GasLimitZero) {
    PnyRuntime *r = pny_runtime_new();
    PnyActor *a = pny_actor_new(r, "GasZero", 0);
    pny_gas_set_limit(a, 0);
    uint64_t remaining = pny_gas_consume(a, 1);
    EXPECT_EQ(remaining, 0);
    pny_runtime_free(r);
}

/* ======================== Phase P2: M:N 工作窃取 ======================== */

TEST(P2MN, InitDestroy) {
    PnyRuntime *r = pny_runtime_new();
    pny_mn_init(r, 2);
    EXPECT_TRUE(pny_mn_global != nullptr);
    pny_mn_destroy(r);
    EXPECT_TRUE(pny_mn_global == nullptr);
    pny_runtime_free(r);
}

TEST(P2MN, InitWithMinWorkers) {
    PnyRuntime *r = pny_runtime_new();
    pny_mn_init(r, 0);
    EXPECT_TRUE(pny_mn_global != nullptr);
    EXPECT_EQ(pny_mn_global->worker_count, 1);
    pny_mn_destroy(r);
    pny_runtime_free(r);
}

TEST(P2MN, EnqueueAndDequeue) {
    PnyRuntime *r = pny_runtime_new();
    pny_mn_init(r, 2);
    PnyActor *a = pny_actor_new(r, "MNActor", 0);
    PnyMessage *msg = pny_msg_new("m1", NULL, 0);
    int rc = pny_mn_enqueue(a, msg, 0);
    EXPECT_EQ(rc, 0);
    PnyMessage *got = pny_mn_dequeue_local(&pny_mn_global->workers[0]);
    EXPECT_NE(got, nullptr);
    if (got) {
        EXPECT_STREQ(got->method, "m1");
        pny_msg_free(got);
    }
    pny_mn_destroy(r);
    pny_runtime_free(r);
}

TEST(P2MN, StealFromOtherWorker) {
    PnyRuntime *r = pny_runtime_new();
    pny_mn_init(r, 2);
    PnyActor *a = pny_actor_new(r, "MNSteal", 0);
    PnyMessage *msg = pny_msg_new("steal", NULL, 0);
    pny_mn_enqueue(a, msg, 1);
    PnyMessage *stolen = pny_mn_steal(&pny_mn_global->workers[0]);
    EXPECT_NE(stolen, nullptr);
    if (stolen) {
        EXPECT_STREQ(stolen->method, "steal");
        pny_msg_free(stolen);
    }
    pny_mn_destroy(r);
    pny_runtime_free(r);
}

TEST(P2MN, StealStats) {
    PnyRuntime *r = pny_runtime_new();
    pny_mn_init(r, 2);
    PnyActor *a = pny_actor_new(r, "MNStats", 0);
    for (int i = 0; i < 5; i++) {
        PnyMessage *msg = pny_msg_new("steal_stat", NULL, 0);
        pny_mn_enqueue(a, msg, 1);
    }
    for (int i = 0; i < 5; i++) {
        PnyMessage *stolen = pny_mn_steal(&pny_mn_global->workers[0]);
        if (stolen) pny_msg_free(stolen);
    }
    size_t steals = pny_mn_steal_stats(r);
    EXPECT_GT(steals, 0);
    pny_mn_destroy(r);
    pny_runtime_free(r);
}

TEST(P2MN, RunTick) {
    PnyRuntime *r = pny_runtime_new();
    pny_mn_init(r, 2);
    pny_actor_new(r, "MNTick", 0);
    pny_scheduler_start(r);
    PnyMessage *msg = pny_msg_new("tick_msg", NULL, 0);
    pny_mn_enqueue(r->scheduler.actors, msg, 0);
    pny_mn_run_tick(r);
    EXPECT_GT(r->scheduler.tick_count, 0);
    pny_mn_destroy(r);
    pny_runtime_free(r);
}

/* ======================== Phase P2: 跨组件监督 ======================== */

TEST(P2CrossSupervise, NewFree) {
    CrossComponentSupervisor *cs = pny_cross_supervise_new();
    ASSERT_NE(cs, nullptr);
    EXPECT_EQ(cs->child_count, 0);
    pny_cross_supervise_free(cs);
}

TEST(P2CrossSupervise, RegisterChildren) {
    CrossComponentSupervisor *cs = pny_cross_supervise_new();
    ActorRef ref1 = {1, NULL, NULL};
    ActorRef ref2 = {2, NULL, NULL};
    EXPECT_EQ(pny_cross_supervise_register(cs, &ref1, "child1"), 0);
    EXPECT_EQ(pny_cross_supervise_register(cs, &ref2, "child2"), 0);
    EXPECT_EQ(cs->child_count, 2);
    EXPECT_STREQ(pny_cross_supervise_state(cs, 0), "child1");
    EXPECT_STREQ(pny_cross_supervise_state(cs, 1), "child2");
    pny_cross_supervise_free(cs);
}

TEST(P2CrossSupervise, CrashOneForOne) {
    PnyRuntime *r = pny_runtime_new();
    CrossComponentSupervisor *cs = pny_cross_supervise_new();
    cs->strategy = SUPERVISE_ONE_FOR_ONE;
    PnyActor *a1 = pny_actor_new(r, "Cross1", 0);
    PnyActor *a2 = pny_actor_new(r, "Cross2", 0);
    pny_scheduler_start(r);
    pny_cross_supervise_register(cs, &a1->self, "a1");
    pny_cross_supervise_register(cs, &a2->self, "a2");
    int rc = pny_cross_supervise_notify_crash(cs, 0);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(cs->restart_count, 1);
    pny_cross_supervise_free(cs);
    pny_runtime_free(r);
}

TEST(P2CrossSupervise, MaxRestarts) {
    PnyRuntime *r = pny_runtime_new();
    CrossComponentSupervisor *cs = pny_cross_supervise_new();
    cs->max_restarts = 2;
    PnyActor *a = pny_actor_new(r, "MaxR", 0);
    pny_cross_supervise_register(cs, &a->self, "child");
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(pny_cross_supervise_notify_crash(cs, 0), 0);
    }
    EXPECT_EQ(pny_cross_supervise_notify_crash(cs, 0), -2);
    pny_cross_supervise_free(cs);
    pny_runtime_free(r);
}

TEST(P2CrossSupervise, InvalidChildIdx) {
    CrossComponentSupervisor *cs = pny_cross_supervise_new();
    EXPECT_EQ(pny_cross_supervise_notify_crash(cs, 99), -1);
    EXPECT_STREQ(pny_cross_supervise_state(cs, 99), "unknown");
    pny_cross_supervise_free(cs);
}

TEST(P2CrossSupervise, OneForAll) {
    PnyRuntime *r = pny_runtime_new();
    CrossComponentSupervisor *cs = pny_cross_supervise_new();
    cs->strategy = SUPERVISE_ONE_FOR_ALL;
    PnyActor *a1 = pny_actor_new(r, "CrossA", 0);
    PnyActor *a2 = pny_actor_new(r, "CrossB", 0);
    pny_scheduler_start(r);
    pny_cross_supervise_register(cs, &a1->self, "a1");
    pny_cross_supervise_register(cs, &a2->self, "a2");
    pny_cross_supervise_notify_crash(cs, 0);
    EXPECT_EQ(a2->actor_state, ACTOR_STATE_RESTARTING);
    pny_cross_supervise_free(cs);
    pny_runtime_free(r);
}

/* ======================== Phase P3: 分布式模型 ======================== */

TEST(P3Dist, ConnListenInvalid) {
    DistConnection *c = dist_conn_listen(0);
    EXPECT_EQ(c, nullptr);
}

TEST(P3Dist, RuntimeNewInvalid) {
    DistributedRuntime *dr = dist_runtime_new(NULL, 8080);
    EXPECT_EQ(dr, nullptr);
}

TEST(P3Dist, RegisterRemote) {
    PnyRuntime *r = pny_runtime_new();
    DistributedRuntime *dr = dist_runtime_new(r, 8080);
    EXPECT_NE(dist_runtime_register_remote(dr, "actor1", "127.0.0.1", 9090, 0), -1);
    EXPECT_NE(dist_runtime_register_remote(dr, "actor2", "10.0.0.1", 9091, 1), -1);
    dist_runtime_free(dr);
    pny_runtime_free(r);
}

TEST(P3Dist, NodeId) {
    PnyRuntime *r = pny_runtime_new();
    DistributedRuntime *dr = dist_runtime_new(r, 8080);
    const char *id = dist_runtime_node_id(dr);
    EXPECT_NE(id, NULL);
    EXPECT_GT(strlen(id), 0u);
    dist_runtime_free(dr);
    pny_runtime_free(r);
}

TEST(P3Dist, SendNoRemote) {
    PnyRuntime *r = pny_runtime_new();
    DistributedRuntime *dr = dist_runtime_new(r, 8080);
    int rc = dist_runtime_send(dr, "nonexistent", "method", "arg", 4);
    EXPECT_EQ(rc, -2);
    dist_runtime_free(dr);
    pny_runtime_free(r);
}

/* ======================== Phase P3: FFI 互操作 ======================== */

TEST(P3FFI, RuntimeInit) {
    FFIRuntime *rt = ffi_runtime_new();
    EXPECT_NE(rt, NULL);
    EXPECT_EQ(ffi_func_count(), 0u);
    ffi_runtime_free();
}

TEST(P3FFI, RegisterFuncNotFound) {
    FFIRuntime *rt = ffi_runtime_new();
    EXPECT_NE(rt, NULL);
    int rc = ffi_register_func("nonexistent_func_xyz", "C", FFI_TYPE_VOID, NULL, NULL, 0);
    EXPECT_EQ(rc, -2);
    FFIFunc *f = ffi_find_func("nonexistent_func_xyz");
    EXPECT_EQ(f, nullptr);
    ffi_runtime_free();
}

TEST(P3FFI, TypeNames) {
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_I32), "i32");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_I64), "i64");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_F64), "f64");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_STRING), "string");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_VOID), "void");
}

TEST(P3FFI, Dump) {
    FFIRuntime *rt = ffi_runtime_new();
    EXPECT_NE(rt, NULL);
    char buf[1024];
    int len = ffi_dump(buf, sizeof(buf));
    EXPECT_GT(len, 0);
    EXPECT_NE(strstr(buf, "Functions:"), nullptr);
    ffi_runtime_free();
}

/* ======================== Phase P3: 包管理 ======================== */

TEST(P3Pkg, ParseToml) {
    const char *toml =
        "[package]\n"
        "name = \"mypkg\"\n"
        "version = \"0.1.0\"\n"
        "description = \"Test package\"\n"
        "author = \"liunix\"\n"
        "\n"
        "[dependency.stdlib]\n"
        "version = \"1.0.0\"\n"
        "source = \"registry\"\n"
        "\n"
        "[dependency.network]\n"
        "version = \"2.0.0\"\n"
        "source = \"git\"\n"
        "path = \"https://github.com/liunix61/network\"\n";

    PkgManifest *pm = pkg_parse_toml_content(toml);
    EXPECT_NE(pm, NULL);
    if (pm) {
        EXPECT_STREQ(pm->name, "mypkg");
        EXPECT_STREQ(pm->version, "0.1.0");
        EXPECT_STREQ(pm->author, "liunix");
        EXPECT_EQ(pm->dep_count, 2u);
        EXPECT_STREQ(pm->deps[0].name, "stdlib");
        EXPECT_STREQ(pm->deps[1].name, "network");
        EXPECT_STREQ(pm->deps[1].source, "git");
        pkg_manifest_free(pm);
    }
}

TEST(P3Pkg, ManifestPrint) {
    const char *toml =
        "[package]\n"
        "name = \"testpkg\"\n"
        "version = \"1.0.0\"\n"
        "author = \"tester\"\n";
    PkgManifest *pm = pkg_parse_toml_content(toml);
    EXPECT_NE(pm, NULL);
    if (pm) {
        char buf[512];
        int len = pkg_manifest_print(pm, buf, sizeof(buf));
        EXPECT_GT(len, 0);
        EXPECT_NE(strstr(buf, "testpkg"), (void *)0);
        pkg_manifest_free(pm);
    }
}

TEST(P3Pkg, ManagerCRUD) {
    PkgManager *pm = pkg_manager_new("/workspace");
    EXPECT_NE(pm, NULL);
    EXPECT_NE(pkg_manager_add(pm, "dep1", "1.0.0", NULL, NULL), -1);
    EXPECT_NE(pkg_manager_add(pm, "dep2", "2.0.0", NULL, NULL), -1);
    EXPECT_EQ(pkg_manager_resolve(pm), 2);
    PkgManifest *found = pkg_manager_find(pm, "dep1");
    EXPECT_NE(found, NULL);
    if (found) EXPECT_STREQ(found->name, "dep1");
    EXPECT_NE(pkg_manager_remove(pm, "dep1"), -2);
    found = pkg_manager_find(pm, "dep1");
    EXPECT_EQ(found, nullptr);
    EXPECT_EQ(pkg_manager_resolve(pm), 1);
    char buf[512];
    int len = pkg_manager_dump(pm, buf, sizeof(buf));
    EXPECT_GT(len, 0);
    EXPECT_NE(strstr(buf, "dep2"), (void *)0);
    pkg_manager_free(pm);
}

TEST(P3Pkg, VersionCompare) {
    EXPECT_LT(pkg_version_compare("1.0.0", "1.0.1"), 0);
    EXPECT_GT(pkg_version_compare("2.0.0", "1.9.9"), 0);
    EXPECT_EQ(pkg_version_compare("1.0.0", "1.0.0"), 0);
}

TEST(P3Pkg, VersionSatisfies) {
    EXPECT_TRUE(pkg_version_satisfies("1.0.0", "1.0.0"));
    EXPECT_TRUE(pkg_version_satisfies("1.2.0", "^1.0.0"));
    EXPECT_TRUE(pkg_version_satisfies("1.5.0", "~1.0.0"));
    EXPECT_FALSE(pkg_version_satisfies("0.9.0", "^1.0.0"));
    EXPECT_FALSE(pkg_version_satisfies(NULL, "1.0.0"));
}

/* ======================== Phase P4: Bootstrap 自举 ======================== */

TEST(P4Bootstrap, CompilerFilesExist) {
    const char *files[] = {"compiler/lexer.pny", "compiler/parser.pny",
                           "compiler/codegen.pny", "compiler/main.pny"};
    for (int i = 0; i < 4; i++) {
        FILE *f = fopen(files[i], "r");
        EXPECT_NE(f, nullptr) << files[i];
        if (f) fclose(f);
    }
}

TEST(P4Bootstrap, StdlibFilesExist) {
    const char *files[] = {"stdlib/std/string.pny", "stdlib/std/list.pny",
                           "stdlib/std/io.pny", "stdlib/std/actor.pny",
                           "stdlib/std/json.pny", "stdlib/std/math.pny"};
    for (int i = 0; i < 6; i++) {
        FILE *f = fopen(files[i], "r");
        EXPECT_NE(f, nullptr) << files[i];
        if (f) fclose(f);
    }
}

TEST(P4Bootstrap, CompilerSourceNonEmpty) {
    const char *files[] = {"compiler/lexer.pny", "compiler/parser.pny",
                           "compiler/codegen.pny", "compiler/main.pny"};
    for (int i = 0; i < 4; i++) {
        FILE *f = fopen(files[i], "r");
        if (!f) continue;
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fclose(f);
        EXPECT_GT(size, 100L) << files[i];
    }
}

TEST(P4Bootstrap, LexerKeywords) {
    FILE *f = fopen("compiler/lexer.pny", "r");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(size + 1);
    ASSERT_NE(buf, NULL);
    fread(buf, 1, size, f);
    buf[size] = 0;
    fclose(f);
    EXPECT_NE(strstr(buf, "actor"), NULL);
    EXPECT_NE(strstr(buf, "class"), NULL);
    EXPECT_NE(strstr(buf, "be"), NULL);
    EXPECT_NE(strstr(buf, "fun"), NULL);
    EXPECT_NE(strstr(buf, "new"), NULL);
    EXPECT_NE(strstr(buf, "match"), NULL);
    free(buf);
}

TEST(P4Bootstrap, ParserASTTypes) {
    FILE *f = fopen("compiler/parser.pny", "r");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(size + 1);
    ASSERT_NE(buf, NULL);
    fread(buf, 1, size, f);
    buf[size] = 0;
    fclose(f);
    EXPECT_NE(strstr(buf, "NODE_ACTOR"), NULL);
    EXPECT_NE(strstr(buf, "NODE_PROGRAM"), NULL);
    EXPECT_NE(strstr(buf, "NODE_BE"), NULL);
    EXPECT_NE(strstr(buf, "NODE_CALL"), NULL);
    free(buf);
}

TEST(P4Bootstrap, CodegenTargets) {
    FILE *f = fopen("compiler/codegen.pny", "r");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(size + 1);
    ASSERT_NE(buf, NULL);
    fread(buf, 1, size, f);
    buf[size] = 0;
    fclose(f);
    EXPECT_NE(strstr(buf, "TARGET_NATIVE"), NULL);
    EXPECT_NE(strstr(buf, "TARGET_WASM"), NULL);
    EXPECT_NE(strstr(buf, "TARGET_AST"), NULL);
    free(buf);
}

TEST(P4Bootstrap, PonyppcBootstrapFlag) {
    FILE *f = fopen("src/ponyppc.c", "r");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(size + 1);
    ASSERT_NE(buf, NULL);
    fread(buf, 1, size, f);
    buf[size] = 0;
    fclose(f);
    EXPECT_NE(strstr(buf, "bootstrap"), NULL);
    EXPECT_NE(strstr(buf, "--bootstrap"), NULL);
    free(buf);
}

TEST(P4Bootstrap, ToolBootstrapImpl) {
    FILE *f = fopen("src/ponypp/tool.c", "r");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(size + 1);
    ASSERT_NE(buf, NULL);
    fread(buf, 1, size, f);
    buf[size] = 0;
    fclose(f);
    EXPECT_NE(strstr(buf, "tool_bootstrap"), NULL);
    EXPECT_NE(strstr(buf, "TOOL_BOOTSTRAP"), NULL);
    free(buf);
}

TEST(P4Bootstrap, ToolEnumHasBootstrap) {
    FILE *f = fopen("include/ponypp/tool.h", "r");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(size + 1);
    ASSERT_NE(buf, NULL);
    fread(buf, 1, size, f);
    buf[size] = 0;
    fclose(f);
    EXPECT_NE(strstr(buf, "TOOL_BOOTSTRAP"), NULL);
    free(buf);
}

TEST(P4Bootstrap, StringStdlibAPI) {
    FILE *f = fopen("stdlib/std/string.pny", "r");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(size + 1);
    ASSERT_NE(buf, NULL);
    fread(buf, 1, size, f);
    buf[size] = 0;
    fclose(f);
    EXPECT_NE(strstr(buf, "len"), NULL);
    EXPECT_NE(strstr(buf, "concat"), NULL);
    EXPECT_NE(strstr(buf, "contains"), NULL);
    EXPECT_NE(strstr(buf, "split"), NULL);
    free(buf);
}

TEST(P4Bootstrap, ListStdlibAPI) {
    FILE *f = fopen("stdlib/std/list.pny", "r");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(size + 1);
    ASSERT_NE(buf, NULL);
    fread(buf, 1, size, f);
    buf[size] = 0;
    fclose(f);
    EXPECT_NE(strstr(buf, "append"), NULL);
    EXPECT_NE(strstr(buf, "remove"), NULL);
    EXPECT_NE(strstr(buf, "map"), NULL);
    EXPECT_NE(strstr(buf, "filter"), NULL);
    free(buf);
}

TEST(P4Bootstrap, IOSTDlibAPI) {
    FILE *f = fopen("stdlib/std/io.pny", "r");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(size + 1);
    ASSERT_NE(buf, NULL);
    fread(buf, 1, size, f);
    buf[size] = 0;
    fclose(f);
    EXPECT_NE(strstr(buf, "print"), NULL);
    EXPECT_NE(strstr(buf, "println"), NULL);
    EXPECT_NE(strstr(buf, "file_open"), NULL);
    EXPECT_NE(strstr(buf, "file_read"), NULL);
    free(buf);
}
