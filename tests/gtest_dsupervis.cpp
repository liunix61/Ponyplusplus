/*
 * 分布式监督树测试
 */

#include <gtest/gtest.h>
#include <ponypp/distributed.h>
#include <cstring>

TEST(DSupervis, NewFree) {
    DistSupervisor *sup = dist_supervisor_new("sup1", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    EXPECT_EQ(dist_supervisor_count(sup), 0u);
    dist_supervisor_free(sup);
}

TEST(DSupervis, Register) {
    DistSupervisor *sup = dist_supervisor_new("sup1", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    EXPECT_EQ(dist_supervisor_register(sup, "worker1", "127.0.0.1", 9001, 1), 0);
    EXPECT_EQ(dist_supervisor_register(sup, "worker2", "127.0.0.1", 9002, 2), 0);
    EXPECT_EQ(dist_supervisor_register(sup, "worker1", "127.0.0.1", 9001, 1), -2);  /* 重复 */
    EXPECT_EQ(dist_supervisor_count(sup), 2u);
    EXPECT_EQ(dist_supervisor_actor_state(sup, "worker1"), DIST_ACTOR_RUNNING);
    EXPECT_EQ(dist_supervisor_actor_state(sup, "nonexistent"), DIST_ACTOR_STOPPED);
    dist_supervisor_free(sup);
}

TEST(DSupervis, Heartbeat) {
    DistSupervisor *sup = dist_supervisor_new("sup1", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    dist_supervisor_register(sup, "w1", "127.0.0.1", 9001, 1);
    EXPECT_EQ(dist_supervisor_heartbeat(sup, "w1"), 0);
    EXPECT_EQ(dist_supervisor_heartbeat(sup, "nonexistent"), -2);
    dist_supervisor_free(sup);
}

TEST(DSupervis, CrashOneForOne) {
    DistSupervisor *sup = dist_supervisor_new("sup1", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    dist_supervisor_register(sup, "w1", "127.0.0.1", 9001, 1);
    dist_supervisor_register(sup, "w2", "127.0.0.1", 9002, 2);

    /* w1崩溃 */
    EXPECT_EQ(dist_supervisor_notify_crash(sup, "w1"), 0);
    EXPECT_EQ(dist_supervisor_actor_state(sup, "w1"), DIST_ACTOR_RESTARTING);
    EXPECT_EQ(dist_supervisor_actor_state(sup, "w2"), DIST_ACTOR_RUNNING);  /* 不受影响 */
    EXPECT_EQ(dist_supervisor_restart_count(sup, "w1"), 1);
    dist_supervisor_free(sup);
}

TEST(DSupervis, CrashOneForAll) {
    DistSupervisor *sup = dist_supervisor_new("sup1", DIST_SUPERVISE_ONE_FOR_ALL, 3);
    dist_supervisor_register(sup, "w1", "127.0.0.1", 9001, 1);
    dist_supervisor_register(sup, "w2", "127.0.0.1", 9002, 2);
    dist_supervisor_register(sup, "w3", "127.0.0.1", 9003, 3);

    /* w2崩溃 -> 全部重启 */
    EXPECT_EQ(dist_supervisor_notify_crash(sup, "w2"), 0);
    EXPECT_EQ(dist_supervisor_actor_state(sup, "w1"), DIST_ACTOR_RESTARTING);
    EXPECT_EQ(dist_supervisor_actor_state(sup, "w2"), DIST_ACTOR_RESTARTING);
    EXPECT_EQ(dist_supervisor_actor_state(sup, "w3"), DIST_ACTOR_RESTARTING);
    dist_supervisor_free(sup);
}

TEST(DSupervis, CrashRestForOne) {
    DistSupervisor *sup = dist_supervisor_new("sup1", DIST_SUPERVISE_REST_FOR_ONE, 3);
    dist_supervisor_register(sup, "w1", "127.0.0.1", 9001, 1);
    dist_supervisor_register(sup, "w2", "127.0.0.1", 9002, 2);
    dist_supervisor_register(sup, "w3", "127.0.0.1", 9003, 3);

    /* w2崩溃 -> w2和w3重启, w1不受影响 */
    EXPECT_EQ(dist_supervisor_notify_crash(sup, "w2"), 0);
    EXPECT_EQ(dist_supervisor_actor_state(sup, "w1"), DIST_ACTOR_RUNNING);
    EXPECT_EQ(dist_supervisor_actor_state(sup, "w2"), DIST_ACTOR_RESTARTING);
    EXPECT_EQ(dist_supervisor_actor_state(sup, "w3"), DIST_ACTOR_RESTARTING);
    dist_supervisor_free(sup);
}

TEST(DSupervis, MaxRestartsExceeded) {
    DistSupervisor *sup = dist_supervisor_new("sup1", DIST_SUPERVISE_ONE_FOR_ONE, 2);
    dist_supervisor_register(sup, "w1", "127.0.0.1", 9001, 1);

    /* 崩溃3次(超过max_restarts=2) */
    EXPECT_EQ(dist_supervisor_notify_crash(sup, "w1"), 0);  /* 第1次 */
    EXPECT_EQ(dist_supervisor_notify_crash(sup, "w1"), 0);  /* 第2次 */
    EXPECT_EQ(dist_supervisor_notify_crash(sup, "w1"), 1);  /* 第3次: 超限 */
    EXPECT_EQ(dist_supervisor_actor_state(sup, "w1"), DIST_ACTOR_CRASHED);
    dist_supervisor_free(sup);
}

TEST(DSupervis, HeartbeatRecovers) {
    DistSupervisor *sup = dist_supervisor_new("sup1", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    dist_supervisor_register(sup, "w1", "127.0.0.1", 9001, 1);

    /* 崩溃 -> 重启中 */
    dist_supervisor_notify_crash(sup, "w1");
    EXPECT_EQ(dist_supervisor_actor_state(sup, "w1"), DIST_ACTOR_RESTARTING);

    /* 心跳恢复 -> RUNNING */
    dist_supervisor_heartbeat(sup, "w1");
    EXPECT_EQ(dist_supervisor_actor_state(sup, "w1"), DIST_ACTOR_RUNNING);
    dist_supervisor_free(sup);
}

TEST(DSupervis, TimeoutDetection) {
    DistSupervisor *sup = dist_supervisor_new("sup1", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    dist_supervisor_register(sup, "w1", "127.0.0.1", 9001, 1);

    /* 立即检查: 不应超时 */
    EXPECT_EQ(dist_supervisor_check_timeouts(sup, 10000), 0);
    dist_supervisor_free(sup);
}

TEST(DSupervis, PendingRestarts) {
    DistSupervisor *sup = dist_supervisor_new("sup1", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    dist_supervisor_register(sup, "w1", "127.0.0.1", 9001, 1);
    dist_supervisor_register(sup, "w2", "127.0.0.1", 9002, 2);

    dist_supervisor_notify_crash(sup, "w1");

    char names[8][64];
    int n = dist_supervisor_pending_restarts(sup, names, 8);
    EXPECT_EQ(n, 1);
    EXPECT_STREQ(names[0], "w1");
    dist_supervisor_free(sup);
}
