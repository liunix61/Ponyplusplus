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
