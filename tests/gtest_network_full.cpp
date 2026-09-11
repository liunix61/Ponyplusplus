#include <gtest/gtest.h>
#include <ponypp/tool.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

/* ==================== TCP 连接 ==================== */

TEST(NetworkFull, TcpConnectNullHost) {
    PnySocket *s = pny_tcp_connect(nullptr, 80);
    EXPECT_EQ(s, nullptr);
}

TEST(NetworkFull, TcpConnectInvalidHost) {
    /* 使用不存在的主机名, 快速失败 */
    PnySocket *s = pny_tcp_connect("nonexistent.invalid", 80);
    EXPECT_EQ(s, nullptr);
}

TEST(NetworkFull, TcpListenInvalidPort) {
    /* 端口 0 由系统分配 */
    PnySocket *s = pny_tcp_listen(0);
    if (s) {
        pny_tcp_close(s);
    }
}

TEST(NetworkFull, TcpListenHighPort) {
    PnySocket *s = pny_tcp_listen(19876);
    if (s) {
        pny_tcp_close(s);
    }
}

/* ==================== 发送/接收 ==================== */

TEST(NetworkFull, TcpSendNull) {
    EXPECT_NE(pny_tcp_send(nullptr, "data", 4), 0);
}

TEST(NetworkFull, TcpRecvNull) {
    char buf[16];
    EXPECT_NE(pny_tcp_recv(nullptr, buf, sizeof(buf)), 0);
}

TEST(NetworkFull, TcpCloseNull) {
    pny_tcp_close(nullptr);  /* 不崩溃 */
}

TEST(NetworkFull, TcpConnectedNull) {
    EXPECT_FALSE(pny_tcp_connected(nullptr));
}

/* ==================== 本地回环测试 ==================== */

TEST(NetworkFull, TcpLoopback) {
    /* 创建监听 */
    PnySocket *server = pny_tcp_listen(19877);
    if (!server) {
        GTEST_SKIP() << "无法绑定端口";
    }

    /* 连接 */
    PnySocket *client = pny_tcp_connect("127.0.0.1", 19877);
    if (!client) {
        pny_tcp_close(server);
        GTEST_SKIP() << "无法连接";
    }

    EXPECT_TRUE(pny_tcp_connected(client));

    /* 发送 */
    const char *msg = "hello";
    int sent = pny_tcp_send(client, msg, strlen(msg));
    EXPECT_GT(sent, 0);

    pny_tcp_close(client);
    pny_tcp_close(server);
}
