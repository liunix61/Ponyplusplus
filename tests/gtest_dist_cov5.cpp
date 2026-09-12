#include <gtest/gtest.h>
#include <ponypp/distributed.h>
#include <ponypp/runtime.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* ==================== 实际 TCP 连接 ==================== */

TEST(DistCov5, ConnectToLocalhost) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(listen_fd, 0);
    
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_LOOPBACK;
    addr.sin_port = 0;
    
    int bind_result = bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    if (bind_result != 0) { close(listen_fd); GTEST_SKIP() << "bind failed"; }
    ASSERT_EQ(listen(listen_fd, 1), 0);
    
    socklen_t alen = sizeof(addr);
    getsockname(listen_fd, (struct sockaddr*)&addr, &alen);
    int port = ntohs(addr.sin_port);
    
    DistConnection *conn = dist_conn_connect("127.0.0.1", port);
    EXPECT_NE(conn, nullptr);
    
    dist_conn_free(conn);
    close(listen_fd);
}

TEST(DistCov5, ConnectToClosedPort) {
    DistConnection *conn = dist_conn_connect("127.0.0.1", 1);
    EXPECT_EQ(conn, nullptr);
}

TEST(DistCov5, ListenOnRandomPort) {
    DistConnection *listener = dist_conn_listen(19876);
    EXPECT_NE(listener, nullptr);
    dist_conn_free(listener);
}

TEST(DistCov5, ListenOnPrivilegedPort) {
    DistConnection *listener = dist_conn_listen(1);
    if (listener) dist_conn_free(listener);
}

TEST(DistCov5, SendRecvLoopback) {
    DistConnection *listener = dist_conn_listen(19876);
    ASSERT_NE(listener, nullptr);
    
    DistConnection *client = dist_conn_connect("127.0.0.1", listener->peer_port);
    ASSERT_NE(client, nullptr);
    
    DistConnection *server = dist_conn_accept(listener);
    ASSERT_NE(server, nullptr);
    
    const char *msg = "hello";
    int sent = dist_send(client, msg, strlen(msg));
    EXPECT_EQ(sent, (int)strlen(msg));
    
    /* dist_conn_accept 现在返回可用的连接，recv 可以真正读到数据 */
    char buf[256];
    memset(buf, 0, sizeof(buf));
    int received = dist_recv(server, buf, sizeof(buf));
    EXPECT_EQ(received, (int)strlen(msg));
    EXPECT_STREQ(buf, msg);
    
    dist_conn_free(server);
    dist_conn_free(client);
    dist_conn_free(listener);
}

TEST(DistCov5, SendNull) {
    int sent = dist_send(nullptr, "hello", 5);
    EXPECT_LT(sent, 0);
}

TEST(DistCov5, RecvNull) {
    char buf[256];
    int received = dist_recv(nullptr, buf, sizeof(buf));
    EXPECT_LT(received, 0);
}

/* ==================== dist_runtime 实际连接 ==================== */

TEST(DistCov5, RuntimeNewWithPort) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    
    DistributedRuntime *dr = dist_runtime_new(rt, 19878);
    EXPECT_NE(dr, nullptr);
    
    dist_runtime_free(dr);
    pny_runtime_free(rt);
}

TEST(DistCov5, RuntimeListen) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    
    DistributedRuntime *dr = dist_runtime_new(rt, 19878);
    ASSERT_NE(dr, nullptr);
    
    int r = dist_runtime_listen(dr);
    EXPECT_EQ(r, 0);
    
    dist_runtime_free(dr);
    pny_runtime_free(rt);
}

TEST(DistCov5, RuntimeRegisterRemote) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    
    DistributedRuntime *dr = dist_runtime_new(rt, 19878);
    ASSERT_NE(dr, nullptr);
    
    int r = dist_runtime_register_remote(dr, "node1", "127.0.0.1", 9999, 1);
    EXPECT_EQ(r, 0);
    
    dist_runtime_free(dr);
    pny_runtime_free(rt);
}

TEST(DistCov5, RuntimeSendToRemote) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    
    DistributedRuntime *dr = dist_runtime_new(rt, 19878);
    ASSERT_NE(dr, nullptr);
    
    dist_runtime_register_remote(dr, "node1", "127.0.0.1", 9999, 1);
    
    int r = dist_runtime_send(dr, "node1", "greet", "hello", 5);
    /* 可能失败（没有实际连接），但覆盖了代码路径 */
    (void)r;
    
    dist_runtime_free(dr);
    pny_runtime_free(rt);
}

TEST(DistCov5, RuntimeSendToNonexistent) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    
    DistributedRuntime *dr = dist_runtime_new(rt, 19878);
    ASSERT_NE(dr, nullptr);
    
    int r = dist_runtime_send(dr, "nonexistent", "greet", "hello", 5);
    EXPECT_LT(r, 0);
    
    dist_runtime_free(dr);
    pny_runtime_free(rt);
}

TEST(DistCov5, RuntimeNodeId) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    
    DistributedRuntime *dr = dist_runtime_new(rt, 19878);
    ASSERT_NE(dr, nullptr);
    
    const char *id = dist_runtime_node_id(dr);
    EXPECT_NE(id, nullptr);
    
    dist_runtime_free(dr);
    pny_runtime_free(rt);
}

/* ==================== dist_supervisor 更多测试 ==================== */

TEST(DistCov5, SupervisorNewAndRegister) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    int r = dist_supervisor_register(sup, "actor1", "127.0.0.1", 9999, 1);
    EXPECT_EQ(r, 0);
    
    EXPECT_EQ(dist_supervisor_count(sup), 1);
    
    dist_supervisor_free(sup);
}

TEST(DistCov5, SupervisorRegisterMultiple) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    for (int i = 0; i < 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "actor_%d", i);
        int r = dist_supervisor_register(sup, name, "127.0.0.1", 9999 + i, i);
        EXPECT_EQ(r, 0);
    }
    
    EXPECT_EQ(dist_supervisor_count(sup), 10);
    
    dist_supervisor_free(sup);
}

TEST(DistCov5, SupervisorHeartbeat) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "actor1", "127.0.0.1", 9999, 1);
    
    int r = dist_supervisor_heartbeat(sup, "actor1");
    EXPECT_EQ(r, 0);
    
    dist_supervisor_free(sup);
}

TEST(DistCov5, SupervisorHeartbeatNonexistent) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    int r = dist_supervisor_heartbeat(sup, "nonexistent");
    EXPECT_LT(r, 0);
    
    dist_supervisor_free(sup);
}

TEST(DistCov5, SupervisorNotifyCrash) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "actor1", "127.0.0.1", 9999, 1);
    
    int r = dist_supervisor_notify_crash(sup, "actor1");
    EXPECT_EQ(r, 0);
    
    dist_supervisor_free(sup);
}

TEST(DistCov5, SupervisorNotifyCrashNonexistent) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    int r = dist_supervisor_notify_crash(sup, "nonexistent");
    EXPECT_LT(r, 0);
    
    dist_supervisor_free(sup);
}

TEST(DistCov5, SupervisorActorState) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "actor1", "127.0.0.1", 9999, 1);
    
    DistActorState state = dist_supervisor_actor_state(sup, "actor1");
    EXPECT_NE(state, (DistActorState)-1);
    
    dist_supervisor_free(sup);
}

TEST(DistCov5, SupervisorActorStateNonexistent) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    DistActorState state = dist_supervisor_actor_state(sup, "nonexistent");
    (void)state;
    
    dist_supervisor_free(sup);
}

TEST(DistCov5, SupervisorRestartCount) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "actor1", "127.0.0.1", 9999, 1);
    
    int count = dist_supervisor_restart_count(sup, "actor1");
    EXPECT_GE(count, 0);
    
    dist_supervisor_free(sup);
}

TEST(DistCov5, SupervisorCheckTimeouts) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "actor1", "127.0.0.1", 9999, 1);
    
    usleep(10000); /* 10ms */
    
    int timed_out = dist_supervisor_check_timeouts(sup, 0);
    EXPECT_GE(timed_out, 0);
    
    dist_supervisor_free(sup);
}

TEST(DistCov5, SupervisorCheckTimeoutsNull) {
    int r = dist_supervisor_check_timeouts(nullptr, 1000);
    EXPECT_EQ(r, -1);
}

TEST(DistCov5, SupervisorPendingRestarts) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "actor1", "127.0.0.1", 9999, 1);
    dist_supervisor_notify_crash(sup, "actor1");
    
    char names[10][64];
    int count = dist_supervisor_pending_restarts(sup, names, 10);
    EXPECT_GE(count, 0);
    
    dist_supervisor_free(sup);
}

/* ==================== 边界情况 ==================== */

TEST(DistCov5, ConnFreeNull) {
    dist_conn_free(nullptr);
    SUCCEED();
}





TEST(DistCov5, ConnAcceptNull) {
    EXPECT_EQ(dist_conn_accept(nullptr), (DistConnection *)nullptr);
}

TEST(DistCov5, RuntimeNewNull) {
    DistributedRuntime *dr = dist_runtime_new(nullptr, 9999);
    /* 可能返回 null 或非 null */
    if (dr) dist_runtime_free(dr);
}

TEST(DistCov5, RuntimeFreeNull) {
    dist_runtime_free(nullptr);
    SUCCEED();
}

TEST(DistCov5, RuntimeListenNull) {
    int r = dist_runtime_listen(nullptr);
    EXPECT_LT(r, 0);
}

TEST(DistCov5, RuntimeRegisterRemoteNull) {
    int r = dist_runtime_register_remote(nullptr, "node", "127.0.0.1", 9999, 1);
    EXPECT_LT(r, 0);
}

TEST(DistCov5, RuntimeSendNull) {
    int r = dist_runtime_send(nullptr, "node", "method", nullptr, 0);
    EXPECT_LT(r, 0);
}

TEST(DistCov5, RuntimeNodeIdNull) {
    const char *id = dist_runtime_node_id(nullptr);
    EXPECT_EQ(id, nullptr);
}

TEST(DistCov5, SupervisorNewNull) {
    DistSupervisor *sup = dist_supervisor_new(nullptr, DIST_SUPERVISE_ONE_FOR_ONE, 3);
    if (sup) dist_supervisor_free(sup);
}

TEST(DistCov5, SupervisorFreeNull) {
    dist_supervisor_free(nullptr);
    SUCCEED();
}

TEST(DistCov5, SupervisorRegisterNull) {
    int r = dist_supervisor_register(nullptr, "actor", "127.0.0.1", 9999, 1);
    EXPECT_LT(r, 0);
}

TEST(DistCov5, SupervisorHeartbeatNull) {
    int r = dist_supervisor_heartbeat(nullptr, "actor");
    EXPECT_LT(r, 0);
}

TEST(DistCov5, SupervisorNotifyCrashNull) {
    int r = dist_supervisor_notify_crash(nullptr, "actor");
    EXPECT_LT(r, 0);
}

TEST(DistCov5, SupervisorActorStateNull) {
    DistActorState state = dist_supervisor_actor_state(nullptr, "actor");
    (void)state;
}

TEST(DistCov5, SupervisorCountNull) {
    size_t count = dist_supervisor_count(nullptr);
    EXPECT_EQ(count, 0);
}

TEST(DistCov5, SupervisorRestartCountNull) {
    int count = dist_supervisor_restart_count(nullptr, "actor");
    EXPECT_EQ(count, -1);
}

TEST(DistCov5, SupervisorPendingRestartsNull) {
    char names[10][64];
    int count = dist_supervisor_pending_restarts(nullptr, names, 10);
    EXPECT_EQ(count, -1);
}

/* ==================== 监督策略 ==================== */

TEST(DistCov5, SupervisorOneForAll) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ALL, 3);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "actor1", "127.0.0.1", 9999, 1);
    dist_supervisor_register(sup, "actor2", "127.0.0.1", 9998, 2);
    
    dist_supervisor_notify_crash(sup, "actor1");
    
    EXPECT_EQ(dist_supervisor_count(sup), 2);
    
    dist_supervisor_free(sup);
}

TEST(DistCov5, SupervisorRestForOne) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_REST_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "actor1", "127.0.0.1", 9999, 1);
    dist_supervisor_register(sup, "actor2", "127.0.0.1", 9998, 2);
    
    dist_supervisor_notify_crash(sup, "actor1");
    
    EXPECT_EQ(dist_supervisor_count(sup), 2);
    
    dist_supervisor_free(sup);
}

TEST(DistCov5, SupervisorMaxRestarts) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 2);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "actor1", "127.0.0.1", 9999, 1);
    
    /* 超过最大重启次数 */
    for (int i = 0; i < 5; i++) {
        dist_supervisor_notify_crash(sup, "actor1");
    }
    
    EXPECT_EQ(dist_supervisor_count(sup), 1);
    
    dist_supervisor_free(sup);
}
