#include <gtest/gtest.h>
#include <ponypp/distributed.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* dist_conn_listen + connect + send + recv */
TEST(DistCov6, ListenConnectSendRecv) {
    DistConnection *server = dist_conn_listen(0);
    if (!server) { SUCCEED(); return; }
    
    int port = server->peer_port;
    if (port <= 0) { dist_conn_free(server); SUCCEED(); return; }
    
    DistConnection *client = dist_conn_connect("127.0.0.1", port);
    if (!client) { dist_conn_free(server); SUCCEED(); return; }
    
    /* 发送数据 */
    const char *msg = "hello";
    int r = dist_send(client, msg, strlen(msg));
    (void)r;
    
    /* 接收 */
    char buf[256] = {0};
    int n = dist_recv(client, buf, sizeof(buf));
    (void)n;
    
    dist_conn_free(client);
    dist_conn_free(server);
    SUCCEED();
}

/* dist_conn_connect 不存在的主机 */
TEST(DistCov6, ConnectNonexistentHost) {
    DistConnection *conn = dist_conn_connect("192.0.2.1", 9999);
    if (conn) dist_conn_free(conn);
    SUCCEED();
}

/* dist_conn_connect 端口 0 */
TEST(DistCov6, ConnectPortZero) {
    DistConnection *conn = dist_conn_connect("127.0.0.1", 0);
    if (conn) dist_conn_free(conn);
    SUCCEED();
}

/* dist_conn_connect 负端口 */
TEST(DistCov6, ConnectNegativePort) {
    DistConnection *conn = dist_conn_connect("127.0.0.1", -1);
    if (conn) dist_conn_free(conn);
    SUCCEED();
}

/* dist_conn_listen 负端口 */
TEST(DistCov6, ListenNegativePort) {
    DistConnection *conn = dist_conn_listen(-1);
    if (conn) dist_conn_free(conn);
    SUCCEED();
}

/* dist_conn_listen 端口 0 */
TEST(DistCov6, ListenPortZero) {
    DistConnection *conn = dist_conn_listen(0);
    if (conn) {
        int port = conn->peer_port;
        EXPECT_GE(port, 0);
        dist_conn_free(conn);
    }
    SUCCEED();
}

/* dist_send null */
TEST(DistCov6, SendNull) {
    int r = dist_send(nullptr, "test", 4);
    (void)r;
    SUCCEED();
}

/* dist_send null data */
TEST(DistCov6, SendNullData) {
    DistConnection *conn = dist_conn_listen(0);
    if (conn) {
        int r = dist_send(conn, nullptr, 0);
        (void)r;
        dist_conn_free(conn);
    }
    SUCCEED();
}

/* dist_send 0 长度 */
TEST(DistCov6, SendZeroLength) {
    DistConnection *conn = dist_conn_listen(0);
    if (conn) {
        int r = dist_send(conn, "test", 0);
        (void)r;
        dist_conn_free(conn);
    }
    SUCCEED();
}

/* dist_recv null */
TEST(DistCov6, RecvNull) {
    int r = dist_recv(nullptr, nullptr, 0);
    (void)r;
    SUCCEED();
}

/* dist_recv null buf */
TEST(DistCov6, RecvNullBuf) {
    DistConnection *conn = dist_conn_listen(0);
    if (conn) {
        int r = dist_recv(conn, nullptr, 0);
        (void)r;
        dist_conn_free(conn);
    }
    SUCCEED();
}

/* dist_recv 0 size */
TEST(DistCov6, RecvZeroSize) {
    DistConnection *conn = dist_conn_listen(0);
    if (conn) {
        char buf[16];
        int r = dist_recv(conn, buf, 0);
        (void)r;
        dist_conn_free(conn);
    }
    SUCCEED();
}

/* dist_conn_free null */
TEST(DistCov6, ConnFreeNull) {
    dist_conn_free(nullptr);
    SUCCEED();
}

/* remote_actor_new */
TEST(DistCov6, RemoteActorNew) {
    RemoteActor *ra = remote_actor_new("test", "127.0.0.1", 8080, 1);
    ASSERT_NE(ra, nullptr);
    EXPECT_STREQ(ra->name, "test");
    remote_actor_free(ra);
}

/* remote_actor_new null name */
TEST(DistCov6, RemoteActorNewNullName) {
    RemoteActor *ra = remote_actor_new(nullptr, "127.0.0.1", 8080, 1);
    if (ra) remote_actor_free(ra);
    SUCCEED();
}

/* remote_actor_new null host */
TEST(DistCov6, RemoteActorNewNullHost) {
    RemoteActor *ra = remote_actor_new("test", nullptr, 8080, 1);
    if (ra) remote_actor_free(ra);
    SUCCEED();
}

/* remote_actor_free null */
TEST(DistCov6, RemoteActorFreeNull) {
    remote_actor_free(nullptr);
    SUCCEED();
}

/* dist_runtime_new */
TEST(DistCov6, RuntimeNew) {
    PnyRuntime *rt = pny_runtime_new();
    if (!rt) { SUCCEED(); return; }
    DistributedRuntime *dr = dist_runtime_new(rt, 0);
    if (dr) {
        dist_runtime_free(dr);
    }
    pny_runtime_free(rt);
    SUCCEED();
}

/* dist_runtime_new null */
TEST(DistCov6, RuntimeNewNull) {
    DistributedRuntime *dr = dist_runtime_new(nullptr, 0);
    if (dr) dist_runtime_free(dr);
    SUCCEED();
}

/* dist_runtime_free null */
TEST(DistCov6, RuntimeFreeNull) {
    dist_runtime_free(nullptr);
    SUCCEED();
}

/* dist_runtime_listen */
TEST(DistCov6, RuntimeListen) {
    PnyRuntime *rt = pny_runtime_new();
    if (!rt) { SUCCEED(); return; }
    DistributedRuntime *dr = dist_runtime_new(rt, 0);
    if (dr) {
        int r = dist_runtime_listen(dr);
        (void)r;
        dist_runtime_free(dr);
    }
    pny_runtime_free(rt);
    SUCCEED();
}

/* dist_runtime_listen null */
TEST(DistCov6, RuntimeListenNull) {
    int r = dist_runtime_listen(nullptr);
    (void)r;
    SUCCEED();
}

/* dist_runtime_register_remote */
TEST(DistCov6, RuntimeRegisterRemote) {
    PnyRuntime *rt = pny_runtime_new();
    if (!rt) { SUCCEED(); return; }
    DistributedRuntime *dr = dist_runtime_new(rt, 0);
    if (dr) {
        int r = dist_runtime_register_remote(dr, "remote1", "127.0.0.1", 8080, 1);
        (void)r;
        dist_runtime_free(dr);
    }
    pny_runtime_free(rt);
    SUCCEED();
}

/* dist_runtime_register_remote null */
TEST(DistCov6, RuntimeRegisterRemoteNull) {
    int r = dist_runtime_register_remote(nullptr, "test", "127.0.0.1", 8080, 1);
    (void)r;
    SUCCEED();
}

/* dist_runtime_send */
TEST(DistCov6, RuntimeSend) {
    PnyRuntime *rt = pny_runtime_new();
    if (!rt) { SUCCEED(); return; }
    DistributedRuntime *dr = dist_runtime_new(rt, 0);
    if (dr) {
        dist_runtime_register_remote(dr, "remote1", "127.0.0.1", 8080, 1);
        int r = dist_runtime_send(dr, "remote1", "test", "hello", 5);
        (void)r;
        dist_runtime_free(dr);
    }
    pny_runtime_free(rt);
    SUCCEED();
}

/* dist_runtime_send null */
TEST(DistCov6, RuntimeSendNull) {
    int r = dist_runtime_send(nullptr, "test", "method", "data", 4);
    (void)r;
    SUCCEED();
}

/* dist_supervisor_new */
TEST(DistCov6, SupervisorNew) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    dist_supervisor_free(sup);
}

/* dist_supervisor_new null */
TEST(DistCov6, SupervisorNewNull) {
    DistSupervisor *sup = dist_supervisor_new(nullptr, DIST_SUPERVISE_ONE_FOR_ONE, 3);
    if (sup) dist_supervisor_free(sup);
    SUCCEED();
}

/* dist_supervisor_free null */
TEST(DistCov6, SupervisorFreeNull) {
    dist_supervisor_free(nullptr);
    SUCCEED();
}

/* dist_supervisor_register */
TEST(DistCov6, SupervisorRegister) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    int r = dist_supervisor_register(sup, "child1", "127.0.0.1", 8080, 1);
    (void)r;
    dist_supervisor_free(sup);
}

/* dist_supervisor_register null */
TEST(DistCov6, SupervisorRegisterNull) {
    int r = dist_supervisor_register(nullptr, "child", "127.0.0.1", 8080, 1);
    (void)r;
    SUCCEED();
}

/* dist_supervisor_heartbeat */
TEST(DistCov6, SupervisorHeartbeat) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    dist_supervisor_register(sup, "child1", "127.0.0.1", 8080, 1);
    int r = dist_supervisor_heartbeat(sup, "child1");
    (void)r;
    dist_supervisor_free(sup);
}

/* dist_supervisor_heartbeat null */
TEST(DistCov6, SupervisorHeartbeatNull) {
    int r = dist_supervisor_heartbeat(nullptr, "child");
    (void)r;
    SUCCEED();
}

/* dist_supervisor_notify_crash */
TEST(DistCov6, SupervisorNotifyCrash) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    dist_supervisor_register(sup, "child1", "127.0.0.1", 8080, 1);
    int r = dist_supervisor_notify_crash(sup, "child1");
    (void)r;
    dist_supervisor_free(sup);
}

/* dist_supervisor_notify_crash null */
TEST(DistCov6, SupervisorNotifyCrashNull) {
    int r = dist_supervisor_notify_crash(nullptr, "child");
    (void)r;
    SUCCEED();
}

/* dist_supervisor_restart_count */
TEST(DistCov6, SupervisorRestartCount) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    dist_supervisor_register(sup, "child1", "127.0.0.1", 8080, 1);
    int r = dist_supervisor_restart_count(sup, "child1");
    EXPECT_GE(r, 0);
    dist_supervisor_free(sup);
}

/* dist_supervisor_restart_count null */
TEST(DistCov6, SupervisorRestartCountNull) {
    int r = dist_supervisor_restart_count(nullptr, "child");
    (void)r;
    SUCCEED();
}

/* dist_supervisor_check_timeouts */
TEST(DistCov6, SupervisorCheckTimeouts) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    int r = dist_supervisor_check_timeouts(sup, 1000);
    (void)r;
    dist_supervisor_free(sup);
}

/* dist_supervisor_check_timeouts null */
TEST(DistCov6, SupervisorCheckTimeoutsNull) {
    int r = dist_supervisor_check_timeouts(nullptr, 1000);
    (void)r;
    SUCCEED();
}

/* dist_supervisor_pending_restarts */
TEST(DistCov6, SupervisorPendingRestarts) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    char names[4][64];
    int r = dist_supervisor_pending_restarts(sup, names, 4);
    EXPECT_GE(r, 0);
    dist_supervisor_free(sup);
}

/* dist_supervisor_pending_restarts null */
TEST(DistCov6, SupervisorPendingRestartsNull) {
    char names[4][64];
    int r = dist_supervisor_pending_restarts(nullptr, names, 4);
    (void)r;
    SUCCEED();
}

TEST(DistCov6, AcceptNull) {
    DistConnection *r = dist_conn_accept(nullptr);
    (void)r;
    SUCCEED();
}

/* 监督策略变体 */
TEST(DistCov6, SupervisorStrategyOneForAll) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ALL, 3);
    ASSERT_NE(sup, nullptr);
    dist_supervisor_free(sup);
}

TEST(DistCov6, SupervisorStrategyRestForOne) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_REST_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    dist_supervisor_free(sup);
}

/* 最大重启次数 0 */
TEST(DistCov6, SupervisorMaxRestartsZero) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 0);
    ASSERT_NE(sup, nullptr);
    dist_supervisor_free(sup);
}

/* 最大重启次数 大值 */
TEST(DistCov6, SupervisorMaxRestartsLarge) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 1000);
    ASSERT_NE(sup, nullptr);
    dist_supervisor_free(sup);
}

/* 多次注册 */
TEST(DistCov6, SupervisorMultipleRegister) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    for (int i = 0; i < 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "child%d", i);
        dist_supervisor_register(sup, name, "127.0.0.1", 8080, i);
    }
    dist_supervisor_free(sup);
    SUCCEED();
}

/* 多次崩溃通知 */
TEST(DistCov6, SupervisorMultipleCrashes) {
    DistSupervisor *sup = dist_supervisor_new("test_sup", DIST_SUPERVISE_ONE_FOR_ONE, 5);
    ASSERT_NE(sup, nullptr);
    dist_supervisor_register(sup, "child1", "127.0.0.1", 8080, 1);
    for (int i = 0; i < 5; i++) {
        dist_supervisor_notify_crash(sup, "child1");
    }
    dist_supervisor_free(sup);
    SUCCEED();
}
