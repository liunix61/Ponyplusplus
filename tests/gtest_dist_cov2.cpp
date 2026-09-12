#include <gtest/gtest.h>
#include <ponypp/distributed.h>
#include <ponypp/runtime.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

/* ==================== 连接管理 ==================== */

TEST(DistCov2, ConnConnectInvalidHost) {
    DistConnection *conn = dist_conn_connect("invalid_host_xyz", 12345);
    EXPECT_EQ(conn, nullptr);
}

TEST(DistCov2, ConnListenInvalidPort) {
    DistConnection *conn = dist_conn_listen(-1);
    EXPECT_EQ(conn, nullptr);
}

TEST(DistCov2, ConnListenValidPort) {
    DistConnection *conn = dist_conn_listen(0);  /* 随机端口 */
    if (conn) {
        dist_conn_free(conn);
    }
}

TEST(DistCov2, ConnAcceptNull) {
    EXPECT_EQ(dist_conn_accept(nullptr), (DistConnection *)nullptr);
}

TEST(DistCov2, ConnFreeNull) {
    dist_conn_free(nullptr);  /* 不崩溃 */
}

/* ==================== 数据传输 ==================== */

TEST(DistCov2, SendNullConn) {
    EXPECT_EQ(dist_send(nullptr, "data", 4), -1);
}

TEST(DistCov2, RecvNullConn) {
    char buf[16];
    EXPECT_EQ(dist_recv(nullptr, buf, 16), -1);
}

/* ==================== 分布式运行时 ==================== */

TEST(DistCov2, RuntimeNewNull) {
    DistributedRuntime *dr = dist_runtime_new(nullptr, 0);
    EXPECT_EQ(dr, nullptr);
}

TEST(DistCov2, RuntimeFreeNull) {
    dist_runtime_free(nullptr);  /* 不崩溃 */
}

TEST(DistCov2, RuntimeListenNull) {
    EXPECT_EQ(dist_runtime_listen(nullptr), -1);
}

/* ==================== 远程 Actor 管理 ==================== */

TEST(DistCov2, RemoteActorFreeNull) {
    remote_actor_free(nullptr);  /* 不崩溃 */
}

#ifdef PONYPP_USE_TLS
/* ==================== TLS ==================== */

TEST(DistCov2, TlsCtxFreeNull) {
    tls_ctx_free(nullptr);  /* 不崩溃 */
}

TEST(DistCov2, TlsCtxLoadCertNull) {
    EXPECT_EQ(tls_ctx_load_cert(nullptr, "cert.pem", "key.pem"), -1);
}

TEST(DistCov2, TlsCtxLoadCaNull) {
    EXPECT_EQ(tls_ctx_load_ca(nullptr, "ca.pem"), -1);
}

TEST(DistCov2, TlsWrapSocketNull) {
    EXPECT_EQ(tls_wrap_socket(nullptr, -1), -1);
}

TEST(DistCov2, TlsHandshakeNull) {
    EXPECT_EQ(tls_handshake(nullptr), -1);
}

TEST(DistCov2, TlsReadNull) {
    char buf[16];
    EXPECT_EQ(tls_read(nullptr, buf, 16), -1);
}

TEST(DistCov2, TlsWriteNull) {
    EXPECT_EQ(tls_write(nullptr, "data", 4), -1);
}

TEST(DistCov2, TlsCloseNull) {
    tls_close(nullptr);  /* 不崩溃 */
}

/* ==================== 注册远程 ==================== */

TEST(DistCov2, RegisterRemoteNull) {
    EXPECT_EQ(dist_runtime_register_remote(nullptr, "name", "host", 12345), -1);
}

TEST(DistCov2, SendRemoteNull) {
    EXPECT_EQ(dist_runtime_send(nullptr, "name", "data", 4), -1);
}
#endif
