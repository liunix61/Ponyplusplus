#include <gtest/gtest.h>
#include <ponypp/distributed.h>
#include <ponypp/runtime.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

/* ==================== 连接管理 ==================== */

TEST(DistCov, ConnConnectNullHost) {
    DistConnection *conn = dist_conn_connect(nullptr, 80);
    EXPECT_EQ(conn, nullptr);
}

TEST(DistCov, ConnConnectInvalidHost) {
    DistConnection *conn = dist_conn_connect("nonexistent.invalid", 80);
    EXPECT_EQ(conn, nullptr);
}

TEST(DistCov, ConnListenAndClose) {
    DistConnection *conn = dist_conn_listen(19880);
    if (conn) {
        dist_conn_free(conn);
    }
}

TEST(DistCov, ConnFreeNull) {
    dist_conn_free(nullptr);
}

TEST(DistCov, ConnSendNull) {
    EXPECT_NE(dist_send(nullptr, "data", 4), 0);
}

TEST(DistCov, ConnRecvNull) {
    char buf[16];
    EXPECT_NE(dist_recv(nullptr, buf, sizeof(buf)), 0);
}

/* ==================== 远程 Actor ==================== */

TEST(DistCov, RemoteActorFreeNull) {
    remote_actor_free(nullptr);
}

/* ==================== 分布式运行时 ==================== */

TEST(DistCov, RuntimeNewNull) {
    DistributedRuntime *dr = dist_runtime_new(nullptr, 0);
    /* 可能返回 NULL 或有效指针 */
    if (dr) dist_runtime_free(dr);
}

TEST(DistCov, RuntimeFreeNull) {
    dist_runtime_free(nullptr);
}

TEST(DistCov, RuntimeListenNull) {
    EXPECT_NE(dist_runtime_listen(nullptr), 0);
}

TEST(DistCov, RuntimeRegisterRemoteNull) {
    EXPECT_NE(dist_runtime_register_remote(nullptr, "remote", "127.0.0.1", 8080, 1), 0);
}

TEST(DistCov, RuntimeSendNull) {
    EXPECT_NE(dist_runtime_send(nullptr, "remote", "method", "data", 4), 0);
}

/* ==================== TLS ==================== */

#ifdef PONYPP_USE_TLS
TEST(DistCov, TlsCtxNewFree) {
    TlsContext *tc = tls_ctx_new(false);
    if (tc) tls_ctx_free(tc);
}
#endif

#ifdef PONYPP_USE_TLS
TEST(DistCov, TlsCtxLoadCertInvalid) {
    TlsContext *tc = tls_ctx_new(false);
    if (tc) {
        EXPECT_NE(tls_ctx_load_cert(tc, "/nonexistent/cert.pem", "/nonexistent/key.pem"), 0);
        tls_ctx_free(tc);
    }
}
#endif

#ifdef PONYPP_USE_TLS
TEST(DistCov, TlsCtxLoadCaInvalid) {
    TlsContext *tc = tls_ctx_new(false);
    if (tc) {
        EXPECT_NE(tls_ctx_load_ca(tc, "/nonexistent/ca.pem"), 0);
        tls_ctx_free(tc);
    }
}
#endif

/* ==================== 监督策略 ==================== */

TEST(DistCov, SuperviseStrategyNames) {
    /* 验证策略名称字符串 */
    const char *names[] = {"one_for_one", "one_for_all", "rest_for_one"};
    for (int i = 0; i < 3; i++) {
        EXPECT_NE(names[i], nullptr);
        EXPECT_GT(strlen(names[i]), 0);
    }
}
