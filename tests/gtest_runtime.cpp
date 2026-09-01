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
