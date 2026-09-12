#include <gtest/gtest.h>
#include <ponypp/runtime.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

/* 针对性覆盖 src/ponypp/runtime.c 未覆盖行:
 * - 90-98:   pny_actor_register 重复注册路径
 * - 143-147: msg_pool_get 消息池复用
 * - 231-233: send 时从消息池取
 * - 353-356: 热代码升级 (pny_hot_upgrade + tick)
 * - 990-1009: pny_mn_start/pny_mn_stop 多节点工作线程
 * - 1037-1071: 跨组件监督 register/state
 * - 1101-1109: 跨组件监督 notify_crash 多次/上限
 */

static void echo_behavior(PnyActor *self, PnyMessage *msg) {
    (void)self; (void)msg;
}
static void echo_behavior_v2(PnyActor *self, PnyMessage *msg) {
    (void)self; (void)msg;
}

TEST(RuntimeCov4, ActorRegisterAgain) {
    /* 90-98: actor 已在链表中, a->next != NULL -> 走 !a->next false 路径 */
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "reg", 0);
    ASSERT_NE(a, nullptr);
    /* 再次注册: a->next 已被 pny_actor_new 设置 */
    pny_actor_register(rt, a);
    pny_actor_register(rt, nullptr);   /* NULL guard */
    pny_actor_register(nullptr, a);    /* NULL guard */
    pny_runtime_free(rt);
}

TEST(RuntimeCov4, ActorRegisterDetached) {
    /* 完全脱离链表+注册表后重新入链 (93-98) */
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "detached", 0);
    ASSERT_NE(a, nullptr);
    PnyActor *b = pny_actor_new(rt, "head", 0);  /* 头插: b 是 head, b->next == a */
    ASSERT_NE(b, nullptr);

    /* 从链表摘除 a: b->next = NULL; 清注册表槽位 */
    b->next = NULL;
    a->prev = NULL;
    size_t idx = (size_t)a->id - 1;
    if (idx < rt->scheduler.max_actors) rt->scheduler.registry[idx] = nullptr;

    /* 重新注册: 走入链路径 */
    pny_actor_register(rt, a);
    EXPECT_EQ(rt->scheduler.actors, a);
    EXPECT_EQ(a->next, b);
    pny_runtime_free(rt);
}

TEST(RuntimeCov4, MsgPoolReuse) {
    /* send -> tick (消息回收进池) -> 再 send (从池取, 143-147/231-233) */
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "pool", 0);
    ASSERT_NE(a, nullptr);
    a->behavior = echo_behavior;
    ActorRef *ref = pny_actor_ref(a);
    ASSERT_NE(ref, nullptr);

    char buf[4] = "abc";
    EXPECT_EQ(pny_actor_send(ref, ref, "echo", buf, sizeof(buf)), 0);
    pny_scheduler_tick(rt);  /* 处理 -> msg_pool_put */
    /* 第二次 send 应复用池中消息 */
    EXPECT_EQ(pny_actor_send(ref, ref, "echo2", buf, sizeof(buf)), 0);
    pny_scheduler_tick(rt);
    pny_runtime_free(rt);
}

TEST(RuntimeCov4, HotUpgrade) {
    /* 353-356: pny_hot_upgrade 后 tick 切换 behavior */
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "hot", 0);
    ASSERT_NE(a, nullptr);
    a->behavior = echo_behavior;
    ActorRef *ref = pny_actor_ref(a);

    pny_hot_upgrade(a, echo_behavior_v2);
    char buf[2] = "x";
    pny_actor_send(ref, ref, "m", buf, sizeof(buf));
    pny_scheduler_tick(rt);  /* tick: behavior = new_behavior */
    EXPECT_EQ(a->behavior, echo_behavior_v2);
    EXPECT_EQ(a->new_behavior, nullptr);
    pny_runtime_free(rt);
}

TEST(RuntimeCov4, MultiNodeStartStop) {
    /* 990-1009: pny_mn_start/stop */
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    pny_mn_start(rt);
    usleep(20 * 1000);  /* 让 worker 线程跑起来 */
    pny_mn_stop(rt);
    pny_runtime_free(rt);
}

TEST(RuntimeCov4, CrossSuperviseLifecycle) {
    /* 1037-1071: cross_supervise new/register/state/free */
    CrossComponentSupervisor *cs = pny_cross_supervise_new();
    ASSERT_NE(cs, nullptr);

    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "child1", 0);
    ASSERT_NE(a, nullptr);
    ActorRef *ref = pny_actor_ref(a);

    EXPECT_EQ(pny_cross_supervise_register(cs, ref, "child1"), 0);
    const char *st = pny_cross_supervise_state(cs, 0);
    EXPECT_NE(st, nullptr);

    pny_cross_supervise_free(cs);
    pny_runtime_free(rt);
}

TEST(RuntimeCov4, CrossSuperviseCrashEscalation) {
    /* 1101-1109: notify_crash 多次 -> 状态升级 */
    CrossComponentSupervisor *cs = pny_cross_supervise_new();
    ASSERT_NE(cs, nullptr);

    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    PnyActor *a = pny_actor_new(rt, "crashy", 0);
    ASSERT_NE(a, nullptr);
    ActorRef *ref = pny_actor_ref(a);
    ASSERT_EQ(pny_cross_supervise_register(cs, ref, "crashy"), 0);

    /* 连续 crash 上报直到触发升级 */
    for (int i = 0; i < 6; i++) {
        pny_cross_supervise_notify_crash(cs, 0);
    }
    const char *st = pny_cross_supervise_state(cs, 0);
    EXPECT_NE(st, nullptr);

    pny_cross_supervise_free(cs);
    pny_runtime_free(rt);
}

TEST(RuntimeCov4, CrossSuperviseBounds) {
    /* 越界 index 安全 */
    CrossComponentSupervisor *cs = pny_cross_supervise_new();
    ASSERT_NE(cs, nullptr);
    EXPECT_LT(pny_cross_supervise_notify_crash(cs, 99), 0);
    EXPECT_STREQ(pny_cross_supervise_state(cs, 99), "unknown");
    pny_cross_supervise_free(cs);
}
