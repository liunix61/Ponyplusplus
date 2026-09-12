/**
 * Pony++ Distributed - 跨网络 Actor 通信
 *
 * 基于 TCP + 序列化协议，实现分布式 Actor 模型。
 * - RemoteActor: 远程 Actor 引用
 * - DistributedRuntime: 分布式运行时
 * - Actor 消息通过网络发送/接收
 */
#define _POSIX_C_SOURCE 200809L
#include "ponypp/distributed.h"
#include "ponypp/tool.h"
#include "ponypp/runtime.h"
#include "ponypp/util.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

/* 分布式消息包: magic(4) + len(4) + payload */
#define PNY_DIST_MAGIC 0x504E5944  /* "PNYD" */
#define PNY_DIST_MAX_PACKET 65536

/* 结构体定义在 ponypp/distributed.h 中 */

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

DistConnection *dist_conn_accept(DistConnection *listener) {
    if (!listener || listener->fd < 0) return NULL;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int fd = accept(listener->fd, (struct sockaddr *)&addr, &len);
    if (fd < 0) return NULL;

    DistConnection *conn = (DistConnection *)calloc(1, sizeof(DistConnection));
    if (!conn) { close(fd); return NULL; }
    conn->fd = fd;
    conn->peer_addr = strdup(inet_ntoa(addr.sin_addr));
    conn->peer_port = ntohs(addr.sin_port);
    conn->connected = true;
    return conn;
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

/* ==================== TLS 支持 ==================== */
#ifdef PONYPP_USE_TLS

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>

TlsContext *tls_ctx_new(bool is_server) {
    TlsContext *tc = (TlsContext *)calloc(1, sizeof(TlsContext));
    if (!tc) return NULL;

    SSL_library_init();
    SSL_load_error_strings();

    const SSL_METHOD *method = is_server ? TLS_server_method() : TLS_client_method();
    tc->ctx = SSL_CTX_new(method);
    if (!tc->ctx) { free(tc); return NULL; }

    /* 最低TLS 1.2 */
    SSL_CTX_set_min_proto_version(tc->ctx, TLS1_2_VERSION);
    tc->is_server = is_server;
    return tc;
}

void tls_ctx_free(TlsContext *tc) {
    if (!tc) return;
    if (tc->ssl) SSL_free(tc->ssl);
    if (tc->ctx) SSL_CTX_free(tc->ctx);
    free(tc);
}

int tls_ctx_load_cert(TlsContext *tc, const char *cert_path, const char *key_path) {
    if (!tc || !cert_path || !key_path) return -1;
    if (SSL_CTX_use_certificate_file(tc->ctx, cert_path, SSL_FILETYPE_PEM) != 1)
        return -2;
    if (SSL_CTX_use_PrivateKey_file(tc->ctx, key_path, SSL_FILETYPE_PEM) != 1)
        return -3;
    if (SSL_CTX_check_private_key(tc->ctx) != 1) return -4;
    return 0;
}

int tls_ctx_load_ca(TlsContext *tc, const char *ca_path) {
    if (!tc || !ca_path) return -1;
    if (SSL_CTX_load_verify_locations(tc->ctx, ca_path, NULL) != 1) return -2;
    return 0;
}

int tls_wrap_socket(TlsContext *tc, int fd) {
    if (!tc || fd < 0) return -1;
    tc->ssl = SSL_new(tc->ctx);
    if (!tc->ssl) return -2;
    if (SSL_set_fd(tc->ssl, fd) != 1) return -3;
    return 0;
}

int tls_handshake(TlsContext *tc) {
    if (!tc || !tc->ssl) return -1;
    int rc = tc->is_server ? SSL_accept(tc->ssl) : SSL_connect(tc->ssl);
    if (rc != 1) return -2;
    tc->handshake_done = true;
    return 0;
}

int tls_read(TlsContext *tc, void *buf, size_t len) {
    if (!tc || !tc->ssl || !buf) return -1;
    int n = SSL_read(tc->ssl, buf, (int)len);
    if (n <= 0) {
        int err = SSL_get_error(tc->ssl, n);
        if (err == SSL_ERROR_WANT_READ) return 0;  /* 非阻塞 */
        return -2;
    }
    return n;
}

int tls_write(TlsContext *tc, const void *data, size_t len) {
    if (!tc || !tc->ssl || !data) return -1;
    int n = SSL_write(tc->ssl, data, (int)len);
    if (n <= 0) return -2;
    return n;
}

void tls_close(TlsContext *tc) {
    if (!tc || !tc->ssl) return;
    SSL_shutdown(tc->ssl);
}

int dist_conn_enable_tls(DistConnection *conn, bool is_server, const char *cert_path, const char *key_path) {
    if (!conn || !conn->connected) return -1;
    TlsContext *tc = tls_ctx_new(is_server);
    if (!tc) return -2;
    if (cert_path && key_path) {
        int rc = tls_ctx_load_cert(tc, cert_path, key_path);
        if (rc < 0) { tls_ctx_free(tc); return rc; }
    }
    int rc = tls_wrap_socket(tc, conn->fd);
    if (rc < 0) { tls_ctx_free(tc); return rc; }
    conn->tls = tc;
    return 0;
}

int dist_conn_tls_handshake(DistConnection *conn) {
    if (!conn || !conn->tls) return -1;
    return tls_handshake(conn->tls);
}

int tls_generate_selfsigned(const char *cert_path, const char *key_path, int days) {
    if (!cert_path || !key_path || days <= 0) return -1;

    /* 生成RSA 2048密钥对 */
    EVP_PKEY *pkey = EVP_RSA_gen(2048);
    if (!pkey) return -2;

    /* 创建自签名证书 */
    X509 *x509 = X509_new();
    if (!x509) { EVP_PKEY_free(pkey); return -3; }

    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), (long)days * 24 * 3600);
    X509_set_pubkey(x509, pkey);

    X509_NAME *name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                               (const unsigned char *)"Pony++ Test", -1, -1, 0);
    X509_set_issuer_name(x509, name);
    X509_sign(x509, pkey, EVP_sha256());

    /* 写入文件 */
    FILE *f = fopen(cert_path, "w");
    if (!f) { X509_free(x509); EVP_PKEY_free(pkey); return -4; }
    PEM_write_X509(f, x509);
    fclose(f);

    f = fopen(key_path, "w");
    if (!f) { X509_free(x509); EVP_PKEY_free(pkey); return -5; }
    PEM_write_PrivateKey(f, pkey, NULL, NULL, 0, NULL, NULL);
    fclose(f);

    X509_free(x509);
    EVP_PKEY_free(pkey);
    return 0;
}

#endif /* PONYPP_USE_TLS */

/* ==================== 分布式监督树 ==================== */

#include <sys/time.h>

static uint64_t dsup_now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

DistSupervisor *dist_supervisor_new(const char *id, DistSuperviseStrategy strategy, int max_restarts) {
    DistSupervisor *sup = (DistSupervisor *)calloc(1, sizeof(DistSupervisor));
    if (!sup) return NULL;
    if (id) strncpy(sup->supervisor_id, id, sizeof(sup->supervisor_id) - 1);
    sup->strategy = strategy;
    sup->max_restarts = max_restarts > 0 ? max_restarts : 3;
    sup->cap = 8;
    sup->actors = (DistSupervisedActor *)calloc(sup->cap, sizeof(DistSupervisedActor));
    if (!sup->actors) { free(sup); return NULL; }
    return sup;
}

void dist_supervisor_free(DistSupervisor *sup) {
    if (!sup) return;
    free(sup->actors);
    free(sup);
}

int dist_supervisor_register(DistSupervisor *sup, const char *name,
                              const char *host, int port, int remote_actor_id) {
    if (!sup || !name || !host) return -1;
    /* 检查重复 */
    for (size_t i = 0; i < sup->count; i++) {
        if (strcmp(sup->actors[i].name, name) == 0) return -2;
    }
    /* 扩容 */
    if (sup->count >= sup->cap) {
        size_t nc = sup->cap * 2;
        DistSupervisedActor *na = (DistSupervisedActor *)realloc(
            sup->actors, nc * sizeof(DistSupervisedActor));
        if (!na) return -3;
        memset(na + sup->cap, 0, (nc - sup->cap) * sizeof(DistSupervisedActor));
        sup->actors = na;
        sup->cap = nc;
    }
    DistSupervisedActor *a = &sup->actors[sup->count++];
    memset(a, 0, sizeof(*a));
    strncpy(a->name, name, sizeof(a->name) - 1);
    strncpy(a->host, host, sizeof(a->host) - 1);
    a->port = port;
    a->remote_actor_id = remote_actor_id;
    a->state = DIST_ACTOR_RUNNING;
    a->max_restarts = sup->max_restarts;
    a->last_heartbeat_ms = dsup_now_ms();
    return 0;
}

int dist_supervisor_heartbeat(DistSupervisor *sup, const char *name) {
    if (!sup || !name) return -1;
    for (size_t i = 0; i < sup->count; i++) {
        if (strcmp(sup->actors[i].name, name) == 0) {
            sup->actors[i].last_heartbeat_ms = dsup_now_ms();
            if (sup->actors[i].state == DIST_ACTOR_RESTARTING)
                sup->actors[i].state = DIST_ACTOR_RUNNING;
            return 0;
        }
    }
    return -2;
}

int dist_supervisor_notify_crash(DistSupervisor *sup, const char *name) {
    if (!sup || !name) return -1;

    /* 找到崩溃的actor */
    size_t crash_idx = (size_t)-1;
    for (size_t i = 0; i < sup->count; i++) {
        if (strcmp(sup->actors[i].name, name) == 0) {
            crash_idx = i;
            break;
        }
    }
    if (crash_idx == (size_t)-1) return -2;

    DistSupervisedActor *crashed = &sup->actors[crash_idx];
    crashed->state = DIST_ACTOR_CRASHED;
    crashed->restart_count++;

    /* 检查是否超过重启限制 */
    if (crashed->restart_count > crashed->max_restarts) {
        /* 超限: 保持CRASHED状态, 不再重启 */
        return 1;  /* 表示超限 */
    }

    sup->global_restarts++;

    /* 应用监督策略 */
    switch (sup->strategy) {
        case DIST_SUPERVISE_ONE_FOR_ONE:
            /* 只重启崩溃的 */
            crashed->state = DIST_ACTOR_RESTARTING;
            break;

        case DIST_SUPERVISE_ONE_FOR_ALL:
            /* 重启全部 */
            for (size_t i = 0; i < sup->count; i++) {
                sup->actors[i].state = DIST_ACTOR_RESTARTING;
                if (i != crash_idx) sup->actors[i].restart_count++;
            }
            break;

        case DIST_SUPERVISE_REST_FOR_ONE:
            /* 重启崩溃的+后续所有 */
            for (size_t i = crash_idx; i < sup->count; i++) {
                sup->actors[i].state = DIST_ACTOR_RESTARTING;
                if (i != crash_idx) sup->actors[i].restart_count++;
            }
            break;
    }
    return 0;
}

DistActorState dist_supervisor_actor_state(DistSupervisor *sup, const char *name) {
    if (!sup || !name) return DIST_ACTOR_STOPPED;
    for (size_t i = 0; i < sup->count; i++) {
        if (strcmp(sup->actors[i].name, name) == 0)
            return sup->actors[i].state;
    }
    return DIST_ACTOR_STOPPED;
}

size_t dist_supervisor_count(DistSupervisor *sup) {
    return sup ? sup->count : 0;
}

int dist_supervisor_restart_count(DistSupervisor *sup, const char *name) {
    if (!sup || !name) return -1;
    for (size_t i = 0; i < sup->count; i++) {
        if (strcmp(sup->actors[i].name, name) == 0)
            return sup->actors[i].restart_count;
    }
    return -1;
}

int dist_supervisor_check_timeouts(DistSupervisor *sup, uint64_t timeout_ms) {
    if (!sup) return -1;
    uint64_t now = dsup_now_ms();
    int timed_out = 0;
    for (size_t i = 0; i < sup->count; i++) {
        if (sup->actors[i].state == DIST_ACTOR_RUNNING) {
            if (now - sup->actors[i].last_heartbeat_ms > timeout_ms) {
                sup->actors[i].state = DIST_ACTOR_CRASHED;
                timed_out++;
            }
        }
    }
    return timed_out;
}

int dist_supervisor_pending_restarts(DistSupervisor *sup, char names[][64], int max) {
    if (!sup || !names || max <= 0) return -1;
    int count = 0;
    for (size_t i = 0; i < sup->count && count < max; i++) {
        if (sup->actors[i].state == DIST_ACTOR_RESTARTING) {
            strncpy(names[count], sup->actors[i].name, 63);
            names[count][63] = ' ';
            count++;
        }
    }
    return count;
}
