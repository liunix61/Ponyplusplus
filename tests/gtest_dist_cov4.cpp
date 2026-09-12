#include <gtest/gtest.h>
#include <ponypp/distributed.h>
#include <ponypp/runtime.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

/* ==================== remote_actor_new 更多情况 ==================== */

TEST(DistCov4, RemoteActorNewValid) {
    RemoteActor *ra = remote_actor_new("actor1", "localhost", 12345, 1);
    EXPECT_NE(ra, nullptr);
    if (ra) {
        remote_actor_free(ra);
    }
}

TEST(DistCov4, RemoteActorNewNullName) {
    RemoteActor *ra = remote_actor_new(nullptr, "localhost", 12345, 1);
    EXPECT_EQ(ra, nullptr);
}

TEST(DistCov4, RemoteActorNewNullHost) {
    RemoteActor *ra = remote_actor_new("actor1", nullptr, 12345, 1);
    EXPECT_EQ(ra, nullptr);
}

/* ==================== 更多连接管理组合 ==================== */

TEST(DistCov4, ConnConnectInvalidPort) {
    DistConnection *conn = dist_conn_connect("localhost", -1);
    EXPECT_EQ(conn, nullptr);
}

TEST(DistCov4, ConnListenZeroPort) {
    DistConnection *conn = dist_conn_listen(0);
    if (conn) {
        dist_conn_free(conn);
    }
}

TEST(DistCov4, ConnAcceptInvalidListener) {
    EXPECT_EQ(dist_conn_accept(nullptr), (DistConnection *)nullptr);
}

/* ==================== 更多数据传输组合 ==================== */

TEST(DistCov4, SendNullData) {
    EXPECT_EQ(dist_send(nullptr, nullptr, 0), -1);
}

TEST(DistCov4, RecvNullBuf) {
    EXPECT_EQ(dist_recv(nullptr, nullptr, 0), -1);
}

/* ==================== 更多分布式运行时组合 ==================== */

TEST(DistCov4, RuntimeNewValid) {
    PnyRuntime *local = pny_runtime_new();
    if (local) {
        DistributedRuntime *dr = dist_runtime_new(local, 0);
        if (dr) {
            dist_runtime_free(dr);
        }
        pny_runtime_free(local);
    }
}

TEST(DistCov4, RuntimeListenValid) {
    PnyRuntime *local = pny_runtime_new();
    if (local) {
        DistributedRuntime *dr = dist_runtime_new(local, 0);
        if (dr) {
            int ret = dist_runtime_listen(dr);
            (void)ret;
            dist_runtime_free(dr);
        }
        pny_runtime_free(local);
    }
}

/* ==================== 更多远程注册组合 ==================== */

TEST(DistCov4, RegisterRemoteValid) {
    PnyRuntime *local = pny_runtime_new();
    if (local) {
        DistributedRuntime *dr = dist_runtime_new(local, 0);
        if (dr) {
            int ret = dist_runtime_register_remote(dr, "remote1", "localhost", 12345, 1);
            (void)ret;
            dist_runtime_free(dr);
        }
        pny_runtime_free(local);
    }
}

TEST(DistCov4, SendRemoteValid) {
    PnyRuntime *local = pny_runtime_new();
    if (local) {
        DistributedRuntime *dr = dist_runtime_new(local, 0);
        if (dr) {
            dist_runtime_register_remote(dr, "remote1", "localhost", 12345, 1);
            int ret = dist_runtime_send(dr, "remote1", "method", "data", 4);
            (void)ret;
            dist_runtime_free(dr);
        }
        pny_runtime_free(local);
    }
}

/* ==================== 更多 TLS 组合 ==================== */

#ifdef PONYPP_USE_TLS
TEST(DistCov4, TlsGenerateSelfsigned) {
    int ret = tls_generate_selfsigned("/tmp/test_cert_cov4.pem", "/tmp/test_key_cov4.pem", 365);
    unlink("/tmp/test_cert_cov4.pem");
    unlink("/tmp/test_key_cov4.pem");
    (void)ret;
}

TEST(DistCov4, TlsConnEnable) {
    EXPECT_EQ(dist_conn_enable_tls(nullptr, false, nullptr, nullptr), -1);
}

TEST(DistCov4, TlsConnHandshake) {
    EXPECT_EQ(dist_conn_tls_handshake(nullptr), -1);
}
#endif

/* ==================== 更多监督树组合 ==================== */

TEST(DistCov4, SupervisorNewValid) {
    DistSupervisor *sup = dist_supervisor_new("sup1", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    EXPECT_NE(sup, nullptr);
    if (sup) {
        dist_supervisor_free(sup);
    }
}

TEST(DistCov4, SupervisorNewNullId) {
    DistSupervisor *sup = dist_supervisor_new(nullptr, DIST_SUPERVISE_ONE_FOR_ONE, 3);
    if (sup) dist_supervisor_free(sup);
}

TEST(DistCov4, SupervisorNewOneForAll) {
    DistSupervisor *sup = dist_supervisor_new("sup2", DIST_SUPERVISE_ONE_FOR_ALL, 3);
    EXPECT_NE(sup, nullptr);
    if (sup) {
        dist_supervisor_free(sup);
    }
}

TEST(DistCov4, SupervisorNewRestForOne) {
    DistSupervisor *sup = dist_supervisor_new("sup3", DIST_SUPERVISE_REST_FOR_ONE, 3);
    EXPECT_NE(sup, nullptr);
    if (sup) {
        dist_supervisor_free(sup);
    }
}

/* ==================== 更多空指针安全组合 ==================== */

TEST(DistCov4, AllNullSafety) {
    remote_actor_free(nullptr);
    dist_runtime_free(nullptr);
    dist_supervisor_free(nullptr);
    dist_conn_free(nullptr);
    
    EXPECT_EQ(dist_conn_accept(nullptr), (DistConnection *)nullptr);
    EXPECT_EQ(dist_send(nullptr, nullptr, 0), -1);
    EXPECT_EQ(dist_recv(nullptr, nullptr, 0), -1);
    EXPECT_EQ(dist_runtime_listen(nullptr), -1);
    EXPECT_EQ(dist_runtime_register_remote(nullptr, "name", "host", 12345, 1), -1);
    EXPECT_EQ(dist_runtime_send(nullptr, "name", "method", "data", 4), -1);
}
