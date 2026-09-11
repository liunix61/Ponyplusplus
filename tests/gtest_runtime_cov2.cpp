#include <gtest/gtest.h>
#include <ponypp/runtime.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

/* pny_actor_register */

/* pny_actor_register null */

/* pny_actor_ref */
TEST(RuntimeCov2, ActorRef) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "test", 64);
    ASSERT_NE(a, nullptr);
    ActorRef *ref = pny_actor_ref(a);
    EXPECT_NE(ref, nullptr);
    pny_runtime_free(rt);
}

/* pny_actor_send */
TEST(RuntimeCov2, ActorSend) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a1 = pny_actor_new(rt, "sender", 64);
    PnyActor *a2 = pny_actor_new(rt, "receiver", 64);
    ASSERT_NE(a1, nullptr);
    ASSERT_NE(a2, nullptr);
    ActorRef *r1 = pny_actor_ref(a1);
    ActorRef *r2 = pny_actor_ref(a2);
    int r = pny_actor_send(r1, r2, "test", nullptr, 0);
    (void)r;
    pny_runtime_free(rt);
}

/* pny_actor_send null */
TEST(RuntimeCov2, ActorSendNull) {
    int r = pny_actor_send(nullptr, nullptr, "test", nullptr, 0);
    (void)r;
    SUCCEED();
}

/* pny_actor_call */
TEST(RuntimeCov2, ActorCall) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a1 = pny_actor_new(rt, "caller", 64);
    PnyActor *a2 = pny_actor_new(rt, "callee", 64);
    ASSERT_NE(a1, nullptr);
    ASSERT_NE(a2, nullptr);
    ActorRef *r1 = pny_actor_ref(a1);
    ActorRef *r2 = pny_actor_ref(a2);
    void *result = nullptr;
    size_t result_size = 0;
    int r = pny_actor_call(r1, r2, "test", nullptr, 0, &result, &result_size);
    (void)r;
    pny_runtime_free(rt);
}

/* pny_actor_call null */
TEST(RuntimeCov2, ActorCallNull) {
    void *result = nullptr;
    size_t result_size = 0;
    int r = pny_actor_call(nullptr, nullptr, "test", nullptr, 0, &result, &result_size);
    (void)r;
    SUCCEED();
}

/* pny_actor_reply */
TEST(RuntimeCov2, ActorReply) {
    pny_actor_reply(nullptr, nullptr, 0);
    SUCCEED();
}

/* pny_scheduler_tick */
TEST(RuntimeCov2, SchedulerTick) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    pny_scheduler_tick(rt);
    pny_runtime_free(rt);
}

/* pny_scheduler_tick null */
TEST(RuntimeCov2, SchedulerTickNull) {
    pny_scheduler_tick(nullptr);
    SUCCEED();
}

/* pny_scheduler_start/stop */
TEST(RuntimeCov2, SchedulerStartStop) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    pny_scheduler_start(rt);
    pny_scheduler_stop(rt);
    pny_runtime_free(rt);
}

/* pny_scheduler_start null */
TEST(RuntimeCov2, SchedulerStartNull) {
    pny_scheduler_start(nullptr);
    SUCCEED();
}

/* pny_scheduler_stop null */
TEST(RuntimeCov2, SchedulerStopNull) {
    pny_scheduler_stop(nullptr);
    SUCCEED();
}

/* pny_supervise_register */
TEST(RuntimeCov2, SuperviseRegister) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *parent = pny_actor_new(rt, "parent", 64);
    PnyActor *child = pny_actor_new(rt, "child", 64);
    ASSERT_NE(parent, nullptr);
    ASSERT_NE(child, nullptr);
    ActorRef *cref = pny_actor_ref(child);
    pny_supervise_register(parent, cref, SUPERVISE_ONE_FOR_ONE, 3);
    pny_runtime_free(rt);
}

/* pny_supervise_register null */
TEST(RuntimeCov2, SuperviseRegisterNull) {
    pny_supervise_register(nullptr, nullptr, SUPERVISE_ONE_FOR_ONE, 3);
    SUCCEED();
}

/* pny_supervisor_handle_crash */
TEST(RuntimeCov2, SupervisorHandleCrash) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "test", 64);
    ASSERT_NE(a, nullptr);
    ActorRef *ref = pny_actor_ref(a);
    pny_supervisor_handle_crash(rt, ref);
    pny_runtime_free(rt);
}

/* pny_supervisor_handle_crash null */
TEST(RuntimeCov2, SupervisorHandleCrashNull) {
    pny_supervisor_handle_crash(nullptr, nullptr);
    SUCCEED();
}

/* pny_runtime_stats */
TEST(RuntimeCov2, RuntimeStats) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    RuntimeStats stats;
    memset(&stats, 0, sizeof(stats));
    pny_runtime_stats(rt, &stats);
    pny_runtime_free(rt);
}

/* pny_runtime_stats null */
TEST(RuntimeCov2, RuntimeStatsNull) {
    RuntimeStats stats;
    memset(&stats, 0, sizeof(stats));
    pny_runtime_stats(nullptr, &stats);
    SUCCEED();
}

/* pny_actor_gc_enable */
TEST(RuntimeCov2, GcEnable) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "test", 64);
    ASSERT_NE(a, nullptr);
    int r = pny_actor_gc_enable(a, 4096);
    (void)r;
    pny_runtime_free(rt);
}

/* pny_actor_gc_enable null */
TEST(RuntimeCov2, GcEnableNull) {
    int r = pny_actor_gc_enable(nullptr, 4096);
    (void)r;
    SUCCEED();
}

/* pny_actor_gc_collect */
TEST(RuntimeCov2, GcCollect) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "test", 64);
    ASSERT_NE(a, nullptr);
    pny_actor_gc_enable(a, 4096);
    pny_actor_gc_collect(a);
    pny_runtime_free(rt);
}

/* pny_actor_gc_collect null */
TEST(RuntimeCov2, GcCollectNull) {
    pny_actor_gc_collect(nullptr);
    SUCCEED();
}

/* pny_actor_gc_stats */
TEST(RuntimeCov2, GcStats) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "test", 64);
    ASSERT_NE(a, nullptr);
    pny_actor_gc_enable(a, 4096);
    PnyGCStats stats;
    memset(&stats, 0, sizeof(stats));
    int r = pny_actor_gc_stats(a, &stats);
    (void)r;
    pny_runtime_free(rt);
}

/* pny_actor_gc_stats null */
TEST(RuntimeCov2, GcStatsNull) {
    PnyGCStats stats;
    memset(&stats, 0, sizeof(stats));
    int r = pny_actor_gc_stats(nullptr, &stats);
    (void)r;
    SUCCEED();
}

/* pny_backpressure_set_limit */
TEST(RuntimeCov2, BackpressureSetLimit) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "test", 64);
    ASSERT_NE(a, nullptr);
    pny_backpressure_set_limit(a, 100);
    pny_runtime_free(rt);
}

/* pny_backpressure_set_limit null */
TEST(RuntimeCov2, BackpressureSetLimitNull) {
    pny_backpressure_set_limit(nullptr, 100);
    SUCCEED();
}

/* pny_msg_delivered */
TEST(RuntimeCov2, MsgDelivered) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "test", 64);
    ASSERT_NE(a, nullptr);
    int r = pny_msg_delivered(a, 1);
    (void)r;
    pny_runtime_free(rt);
}

/* pny_msg_delivered null */
TEST(RuntimeCov2, MsgDeliveredNull) {
    int r = pny_msg_delivered(nullptr, 1);
    (void)r;
    SUCCEED();
}

/* pny_msg_serialize */
TEST(RuntimeCov2, MsgSerialize) {
    uint8_t buf[256];
    size_t out_size = 0;
    int r = pny_msg_serialize(nullptr, buf, sizeof(buf), &out_size);
    (void)r;
    SUCCEED();
}

/* pny_msg_deserialize */
TEST(RuntimeCov2, MsgDeserialize) {
    PnyMessage *msg = nullptr;
    int r = pny_msg_deserialize(nullptr, 0, &msg);
    (void)r;
    SUCCEED();
}

/* pny_actor_destroy */
TEST(RuntimeCov2, ActorDestroy) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "test", 64);
    ASSERT_NE(a, nullptr);
    ActorRef *ref = pny_actor_ref(a);
    pny_actor_destroy(rt, ref);
    pny_runtime_free(rt);
}

/* pny_actor_destroy null */
TEST(RuntimeCov2, ActorDestroyNull) {
    pny_actor_destroy(nullptr, nullptr);
    SUCCEED();
}

/* 多个 actor */
TEST(RuntimeCov2, MultipleActors) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    for (int i = 0; i < 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "actor%d", i);
        PnyActor *a = pny_actor_new(rt, name, 64);
        ASSERT_NE(a, nullptr);
    }
    pny_runtime_free(rt);
}

/* 消息池 */
TEST(RuntimeCov2, MessagePool) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a1 = pny_actor_new(rt, "sender", 64);
    PnyActor *a2 = pny_actor_new(rt, "receiver", 64);
    ASSERT_NE(a1, nullptr);
    ASSERT_NE(a2, nullptr);
    ActorRef *r1 = pny_actor_ref(a1);
    ActorRef *r2 = pny_actor_ref(a2);
    
    /* 发送多条消息 */
    for (int i = 0; i < 10; i++) {
        pny_actor_send(r1, r2, "test", nullptr, 0);
    }
    
    /* 调度处理 */
    pny_scheduler_tick(rt);
    
    pny_runtime_free(rt);
}

/* pny_actor_new 大 state */
TEST(RuntimeCov2, ActorLargeState) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "big", 65536);
    ASSERT_NE(a, nullptr);
    pny_runtime_free(rt);
}

/* pny_actor_new 零 state */
TEST(RuntimeCov2, ActorZeroState) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "empty", 0);
    if (a) {
        pny_runtime_free(rt);
    } else {
        pny_runtime_free(rt);
    }
    SUCCEED();
}
