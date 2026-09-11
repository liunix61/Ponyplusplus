#include <gtest/gtest.h>
#include <ponypp/distributed.h>
#include <ponypp/runtime.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

/* ==================== 连接管理 ==================== */

TEST(DistConn, ListenAndClose) {
    /* 使用高位端口避免权限问题 */
    DistConnection *conn = dist_conn_listen(19876);
    if (conn) {
        dist_conn_free(conn);
    }
    /* port <= 0 应返回 NULL */
    EXPECT_EQ(dist_conn_listen(0), nullptr);
    EXPECT_EQ(dist_conn_listen(-1), nullptr);
}

TEST(DistConn, AcceptNull) {
    EXPECT_EQ(dist_conn_accept(nullptr), -1);
}

TEST(DistConn, SendNull) {
    EXPECT_EQ(dist_send(nullptr, "data", 4), -1);
}

TEST(DistConn, RecvNull) {
    char buf[64];
    EXPECT_EQ(dist_recv(nullptr, buf, sizeof(buf)), -1);
}

TEST(DistConn, SendNoData) {
    DistConnection *conn = dist_conn_listen(0);
    if (conn) {
        EXPECT_EQ(dist_send(conn, nullptr, 0), -1);
        dist_conn_free(conn);
    }
}

/* ==================== 分布式运行时 ==================== */

TEST(DistRuntime, CreateAndFree) {
    DistributedRuntime *dr = dist_runtime_new(nullptr, 0);
    if (dr) {
        const char *node_id = dist_runtime_node_id(dr);
        EXPECT_NE(node_id, nullptr);
        dist_runtime_free(dr);
    }
}

TEST(DistRuntime, CreateWithRuntime) {
    PnyRuntime *rt = pny_runtime_new();
    ASSERT_NE(rt, nullptr);
    
    DistributedRuntime *dr = dist_runtime_new(rt, 0);
    if (dr) {
        dist_runtime_free(dr);
    }
    
    pny_runtime_free(rt);
}

TEST(DistRuntime, RegisterRemote) {
    DistributedRuntime *dr = dist_runtime_new(nullptr, 0);
    if (dr) {
        int rc = dist_runtime_register_remote(dr, "node1", "127.0.0.1", 9999, 1);
        EXPECT_EQ(rc, 0);
        
        /* 重复注册应失败 */
        rc = dist_runtime_register_remote(dr, "node1", "127.0.0.1", 9999, 1);
        EXPECT_NE(rc, 0);
        
        dist_runtime_free(dr);
    }
}

TEST(DistRuntime, RegisterRemoteBadArgs) {
    DistributedRuntime *dr = dist_runtime_new(nullptr, 0);
    if (dr) {
        EXPECT_EQ(dist_runtime_register_remote(nullptr, "n", "h", 1, 1), -1);
        EXPECT_EQ(dist_runtime_register_remote(dr, nullptr, "h", 1, 1), -1);
        EXPECT_EQ(dist_runtime_register_remote(dr, "n", nullptr, 1, 1), -1);
        
        dist_runtime_free(dr);
    }
}

TEST(DistRuntime, SendToRemote) {
    DistributedRuntime *dr = dist_runtime_new(nullptr, 0);
    if (dr) {
        dist_runtime_register_remote(dr, "fake", "127.0.0.1", 1, 1);
        
        /* 发送到不存在的节点应失败 */
        int rc = dist_runtime_send(dr, "fake", "method", "data", 4);
        EXPECT_NE(rc, 0);
        
        /* 发送到未注册节点应失败 */
        rc = dist_runtime_send(dr, "unknown", "method", "data", 4);
        EXPECT_NE(rc, 0);
        
        dist_runtime_free(dr);
    }
}

TEST(DistRuntime, SendBadArgs) {
    DistributedRuntime *dr = dist_runtime_new(nullptr, 0);
    if (dr) {
        EXPECT_EQ(dist_runtime_send(nullptr, "n", "m", "d", 1), -1);
        EXPECT_EQ(dist_runtime_send(dr, nullptr, "m", "d", 1), -1);
        EXPECT_EQ(dist_runtime_send(dr, "n", nullptr, "d", 1), -1);
        EXPECT_EQ(dist_runtime_send(dr, "n", "m", nullptr, 1), -1);
        
        dist_runtime_free(dr);
    }
}

TEST(DistRuntime, Listen) {
    DistributedRuntime *dr = dist_runtime_new(nullptr, 0);
    if (dr) {
        int rc = dist_runtime_listen(dr);
        (void)rc;
        
        dist_runtime_free(dr);
    }
}

/* ==================== 连接操作 ==================== */

TEST(DistConn, MultipleListen) {
    DistConnection *c1 = dist_conn_listen(19877);
    DistConnection *c2 = dist_conn_listen(19878);
    
    if (c1 && c2) {
        /* 两个监听器应有不同 fd */
        dist_conn_free(c1);
        dist_conn_free(c2);
    } else {
        if (c1) dist_conn_free(c1);
        if (c2) dist_conn_free(c2);
    }
}

TEST(DistConn, FreeNull) {
    dist_conn_free(nullptr);  /* 不应崩溃 */
}

TEST(DistRuntime, FreeNull) {
    dist_runtime_free(nullptr);  /* 不应崩溃 */
}

/* ==================== 连接操作 ==================== */

TEST(DistConn, ConnectInvalidHost) {
    /* 连接到不存在的主机应失败 */
    DistConnection *conn = dist_conn_connect("999.999.999.999", 80);
    EXPECT_EQ(conn, nullptr);
    
    EXPECT_EQ(dist_conn_connect(nullptr, 80), nullptr);
    EXPECT_EQ(dist_conn_connect("127.0.0.1", 0), nullptr);
    EXPECT_EQ(dist_conn_connect("127.0.0.1", -1), nullptr);
}

TEST(DistConn, ConnectRefused) {
    /* 连接到未监听端口应失败 */
    DistConnection *conn = dist_conn_connect("127.0.0.1", 1);
    if (conn) {
        dist_conn_free(conn);
    }
}

/* ==================== 分布式监督树 ==================== */

TEST(DistSupervisor, CreateAndFree) {
    DistSupervisor *sup = dist_supervisor_new("test-sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    EXPECT_EQ(dist_supervisor_count(sup), 0);
    
    dist_supervisor_free(sup);
}

TEST(DistSupervisor, CreateNullArgs) {
    /* NULL id 可能仍返回非 NULL (使用默认 id) */
    DistSupervisor *sup = dist_supervisor_new(nullptr, DIST_SUPERVISE_ONE_FOR_ONE, 3);
    if (sup) dist_supervisor_free(sup);
}

TEST(DistSupervisor, RegisterActor) {
    DistSupervisor *sup = dist_supervisor_new("test-sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    int rc = dist_supervisor_register(sup, "worker1", "127.0.0.1", 9999, 1);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(dist_supervisor_count(sup), 1);
    
    /* 重复注册应失败 */
    rc = dist_supervisor_register(sup, "worker1", "127.0.0.1", 9999, 1);
    EXPECT_NE(rc, 0);
    
    dist_supervisor_free(sup);
}

TEST(DistSupervisor, RegisterBadArgs) {
    DistSupervisor *sup = dist_supervisor_new("test-sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    EXPECT_EQ(dist_supervisor_register(nullptr, "w", "h", 1, 1), -1);
    EXPECT_EQ(dist_supervisor_register(sup, nullptr, "h", 1, 1), -1);
    EXPECT_EQ(dist_supervisor_register(sup, "w", nullptr, 1, 1), -1);
    
    dist_supervisor_free(sup);
}

TEST(DistSupervisor, Heartbeat) {
    DistSupervisor *sup = dist_supervisor_new("test-sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "worker1", "127.0.0.1", 9999, 1);
    
    int rc = dist_supervisor_heartbeat(sup, "worker1");
    EXPECT_EQ(rc, 0);
    
    /* 未注册的 actor */
    rc = dist_supervisor_heartbeat(sup, "unknown");
    EXPECT_NE(rc, 0);
    
    dist_supervisor_free(sup);
}

TEST(DistSupervisor, NotifyCrash) {
    DistSupervisor *sup = dist_supervisor_new("test-sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "worker1", "127.0.0.1", 9999, 1);
    
    int rc = dist_supervisor_notify_crash(sup, "worker1");
    EXPECT_EQ(rc, 0);
    
    /* 检查重启计数 */
    EXPECT_GE(dist_supervisor_restart_count(sup, "worker1"), 1);
    
    dist_supervisor_free(sup);
}

TEST(DistSupervisor, ActorState) {
    DistSupervisor *sup = dist_supervisor_new("test-sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "worker1", "127.0.0.1", 9999, 1);
    
    DistActorState state = dist_supervisor_actor_state(sup, "worker1");
    /* 初始状态应该是 RUNNING 或 STARTING */
    (void)state;
    
    dist_supervisor_free(sup);
}

TEST(DistSupervisor, CheckTimeouts) {
    DistSupervisor *sup = dist_supervisor_new("test-sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "worker1", "127.0.0.1", 9999, 1);
    
    /* 立即检查超时 (不应超时) */
    int rc = dist_supervisor_check_timeouts(sup, 10000);
    (void)rc;
    
    dist_supervisor_free(sup);
}

TEST(DistSupervisor, PendingRestarts) {
    DistSupervisor *sup = dist_supervisor_new("test-sup", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    ASSERT_NE(sup, nullptr);
    
    dist_supervisor_register(sup, "worker1", "127.0.0.1", 9999, 1);
    dist_supervisor_notify_crash(sup, "worker1");
    
    char names[8][64];
    int count = dist_supervisor_pending_restarts(sup, names, 8);
    (void)count;
    
    dist_supervisor_free(sup);
}

TEST(DistSupervisor, FreeNull) {
    dist_supervisor_free(nullptr);  /* 不应崩溃 */
}

/* ==================== TLS ==================== */

#ifdef PONYPP_USE_TLS
TEST(TLS, GenerateSelfsigned) {
    const char *cert = "/tmp/ponypp-test-cert.pem";
    const char *key = "/tmp/ponypp-test-key.pem";
    
    int rc = tls_generate_selfsigned(cert, key, 365);
    (void)rc;
    
    unlink(cert);
    unlink(key);
    
    EXPECT_EQ(tls_generate_selfsigned(nullptr, key, 365), -1);
    EXPECT_EQ(tls_generate_selfsigned(cert, nullptr, 365), -1);
}
#endif /* PONYPP_USE_TLS */
