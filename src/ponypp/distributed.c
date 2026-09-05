/**
 * Pony++ Distributed - 跨网络 Actor 通信
 *
 * 基于 TCP + 序列化协议，实现分布式 Actor 模型。
 * - RemoteActor: 远程 Actor 引用
 * - DistributedRuntime: 分布式运行时
 * - Actor 消息通过网络发送/接收
 */
#include "ponypp/tool.h"
#include "ponypp/runtime.h"
#include "ponypp/util.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

/* 分布式消息包: magic(4) + len(4) + payload */
#define PNY_DIST_MAGIC 0x504E5944  /* "PNYD" */
#define PNY_DIST_MAX_PACKET 65536

typedef struct RemoteActor {
    char *name;
    char *host;
    int port;
    int actor_id;
    void *state;
    size_t state_size;
} RemoteActor;

typedef struct DistConnection {
    int fd;
    char *peer_addr;
    int peer_port;
    bool connected;
    uint64_t msgs_sent;
    uint64_t msgs_recv;
} DistConnection;

typedef struct DistributedRuntime {
    DistConnection *self_conn;     /* 作为 server 的监听连接 */
    RemoteActor **remote_actors;
    size_t remote_count;
    size_t remote_cap;
    PnyRuntime *local_runtime;
    int port;
    char node_id[64];
} DistributedRuntime;

/* ======================== 连接管理 ======================== */

DistConnection *dist_conn_connect(const char *host, int port) {
    if (!host || port <= 0) return NULL;
    DistConnection *conn = (DistConnection *)calloc(1, sizeof(DistConnection));
    if (!conn) return NULL;

    conn->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->fd < 0) { free(conn); return NULL; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        close(conn->fd);
        free(conn);
        return NULL;
    }

    /* 设置超时 */
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(conn->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(conn->fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(conn->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(conn->fd);
        free(conn);
        return NULL;
    }

    conn->peer_addr = strdup(host);
    conn->peer_port = port;
    conn->connected = true;
    return conn;
}

DistConnection *dist_conn_listen(int port) {
    if (port <= 0) return NULL;
    DistConnection *conn = (DistConnection *)calloc(1, sizeof(DistConnection));
    if (!conn) return NULL;

    conn->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->fd < 0) { free(conn); return NULL; }

    int opt = 1;
    setsockopt(conn->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(conn->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(conn->fd);
        free(conn);
        return NULL;
    }
    if (listen(conn->fd, 16) < 0) {
        close(conn->fd);
        free(conn);
        return NULL;
    }

    conn->peer_port = port;
    conn->connected = true;
    return conn;
}

int dist_conn_accept(DistConnection *listener) {
    if (!listener || listener->fd < 0) return -1;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int fd = accept(listener->fd, (struct sockaddr *)&addr, &len);
    if (fd < 0) return -1;

    DistConnection *conn = (DistConnection *)calloc(1, sizeof(DistConnection));
    if (!conn) { close(fd); return -2; }
    conn->fd = fd;
    conn->peer_addr = strdup(inet_ntoa(addr.sin_addr));
    conn->peer_port = ntohs(addr.sin_port);
    conn->connected = true;
    return 0;
}

void dist_conn_free(DistConnection *conn) {
    if (!conn) return;
    if (conn->fd >= 0) close(conn->fd);
    free(conn->peer_addr);
    free(conn);
}

/* ======================== 网络发送/接收 ======================== */

int dist_send(DistConnection *conn, const void *data, size_t len) {
    if (!conn || !conn->connected || conn->fd < 0) return -1;
    if (len > PNY_DIST_MAX_PACKET) return -2;

    uint32_t magic = PNY_DIST_MAGIC;
    uint32_t payload_len = (uint32_t)len;

    /* 发送 header */
    if (send(conn->fd, &magic, 4, MSG_NOSIGNAL) != 4) return -3;
    if (send(conn->fd, &payload_len, 4, MSG_NOSIGNAL) != 4) return -4;

    /* 发送 payload */
    size_t total_sent = 0;
    while (total_sent < len) {
        ssize_t sent = send(conn->fd, (const char *)data + total_sent,
                           len - total_sent, MSG_NOSIGNAL);
        if (sent <= 0) return -5;
        total_sent += sent;
    }

    conn->msgs_sent++;
    return (int)len;
}

int dist_recv(DistConnection *conn, void *buf, size_t buf_size) {
    if (!conn || !conn->connected || conn->fd < 0) return -1;

    /* 接收 header */
    uint32_t magic, payload_len;
    if (recv(conn->fd, &magic, 4, MSG_WAITALL) != 4) return -2;
    if (magic != PNY_DIST_MAGIC) return -3;
    if (recv(conn->fd, &payload_len, 4, MSG_WAITALL) != 4) return -4;

    if (payload_len > buf_size) return -5;

    /* 接收 payload */
    size_t total_recv = 0;
    while (total_recv < payload_len) {
        ssize_t r = recv(conn->fd, (char *)buf + total_recv,
                        payload_len - total_recv, MSG_WAITALL);
        if (r <= 0) return -6;
        total_recv += r;
    }

    conn->msgs_recv++;
    return (int)payload_len;
}

/* ======================== 远程 Actor ======================== */

RemoteActor *remote_actor_new(const char *name, const char *host, int port, int actor_id) {
    if (!name || !host) return NULL;
    RemoteActor *ra = (RemoteActor *)calloc(1, sizeof(RemoteActor));
    if (!ra) return NULL;
    ra->name = strdup(name);
    ra->host = strdup(host);
    ra->port = port;
    ra->actor_id = actor_id;
    return ra;
}

void remote_actor_free(RemoteActor *ra) {
    if (!ra) return;
    free(ra->name);
    free(ra->host);
    free(ra->state);
    free(ra);
}

/* ======================== 分布式运行时 ======================== */

DistributedRuntime *dist_runtime_new(PnyRuntime *local, int port) {
    if (!local) return NULL;
    DistributedRuntime *dr = (DistributedRuntime *)calloc(1, sizeof(DistributedRuntime));
    if (!dr) return NULL;
    dr->local_runtime = local;
    dr->port = port;
    snprintf(dr->node_id, sizeof(dr->node_id), "node-%d", (int)time(NULL));
    dr->remote_cap = 8;
    dr->remote_actors = (RemoteActor **)calloc(dr->remote_cap, sizeof(RemoteActor *));
    if (!dr->remote_actors) { free(dr); return NULL; }
    return dr;
}

void dist_runtime_free(DistributedRuntime *dr) {
    if (!dr) return;
    if (dr->self_conn) dist_conn_free(dr->self_conn);
    for (size_t i = 0; i < dr->remote_count; i++) {
        remote_actor_free(dr->remote_actors[i]);
    }
    free(dr->remote_actors);
    free(dr);
}

int dist_runtime_listen(DistributedRuntime *dr) {
    if (!dr || dr->port <= 0) return -1;
    dr->self_conn = dist_conn_listen(dr->port);
    return dr->self_conn ? 0 : -1;
}

int dist_runtime_register_remote(DistributedRuntime *dr, const char *name,
                                  const char *host, int port, int actor_id) {
    if (!dr || !name || !host) return -1;
    if (dr->remote_count >= dr->remote_cap) {
        dr->remote_cap *= 2;
        dr->remote_actors = (RemoteActor **)realloc(dr->remote_actors,
                                                    dr->remote_cap * sizeof(RemoteActor *));
        if (!dr->remote_actors) return -2;
    }
    RemoteActor *ra = remote_actor_new(name, host, port, actor_id);
    if (!ra) return -3;
    dr->remote_actors[dr->remote_count++] = ra;
    return 0;
}

int dist_runtime_send(DistributedRuntime *dr, const char *remote_name,
                       const char *method, const void *arg, size_t arg_size) {
    if (!dr || !remote_name || !method) return -1;

    /* 查找远程 Actor */
    RemoteActor *ra = NULL;
    for (size_t i = 0; i < dr->remote_count; i++) {
        if (strcmp(dr->remote_actors[i]->name, remote_name) == 0) {
            ra = dr->remote_actors[i];
            break;
        }
    }
    if (!ra) return -2;

    /* 建立连接 */
    DistConnection *conn = dist_conn_connect(ra->host, ra->port);
    if (!conn) return -3;

    /* 序列化消息 */
    PnyMessage msg_storage;
    memset(&msg_storage, 0, sizeof(msg_storage));
    msg_storage.method = (char *)method;
    msg_storage.arg = (void *)arg;
    msg_storage.arg_size = arg_size;

    uint8_t *buf = (uint8_t *)malloc(PNY_DIST_MAX_PACKET);
    if (!buf) { dist_conn_free(conn); return -4; }

    size_t serialized_size = 0;
    int rc = pny_msg_serialize(&msg_storage, buf, PNY_DIST_MAX_PACKET, &serialized_size);
    if (rc != 0) { free(buf); dist_conn_free(conn); return -5; }

    int sent = dist_send(conn, buf, serialized_size);
    free(buf);
    dist_conn_free(conn);
    return sent;
}

const char *dist_runtime_node_id(DistributedRuntime *dr) {
    if (!dr) return NULL;
    return dr->node_id;
}
