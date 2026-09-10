#ifndef PONYPP_DISTRIBUTED_H
#define PONYPP_DISTRIBUTED_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "ponypp/runtime.h"

/* 远程 Actor */
typedef struct RemoteActor {
    char *name;
    char *host;
    int port;
    int actor_id;
    void *state;
    size_t state_size;
} RemoteActor;

/* 分布式连接 */
typedef struct DistConnection {
    int fd;
    char *peer_addr;
    int peer_port;
    bool connected;
    uint64_t msgs_sent;
    uint64_t msgs_recv;
    void *tls;  /* TlsContext*, 需PONYPP_USE_TLS */
} DistConnection;

/* 分布式运行时 */
typedef struct DistributedRuntime {
    DistConnection *self_conn;
    RemoteActor **remote_actors;
    size_t remote_count;
    size_t remote_cap;
    void *local_runtime;
    int port;
    char node_id[64];
} DistributedRuntime;

/* 连接管理 */
DistConnection *dist_conn_connect(const char *host, int port);
DistConnection *dist_conn_listen(int port);
int dist_conn_accept(DistConnection *listener);
void dist_conn_free(DistConnection *conn);

/* 网络 I/O */
int dist_send(DistConnection *conn, const void *data, size_t len);
int dist_recv(DistConnection *conn, void *buf, size_t buf_size);

/* 远程 Actor */
RemoteActor *remote_actor_new(const char *name, const char *host, int port, int actor_id);
void remote_actor_free(RemoteActor *ra);

/* 分布式运行时 */
DistributedRuntime *dist_runtime_new(PnyRuntime *local, int port);
void dist_runtime_free(DistributedRuntime *dr);
int dist_runtime_listen(DistributedRuntime *dr);
int dist_runtime_register_remote(DistributedRuntime *dr, const char *name,
                                  const char *host, int port, int actor_id);
int dist_runtime_send(DistributedRuntime *dr, const char *remote_name,
                       const char *method, const void *arg, size_t arg_size);
const char *dist_runtime_node_id(DistributedRuntime *dr);

/* ==================== TLS 支持 ==================== */
/* 编译时检测: 定义 PONYPP_USE_TLS 启用 */

#ifdef PONYPP_USE_TLS
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct {
    SSL_CTX *ctx;
    SSL *ssl;
    bool is_server;
    bool handshake_done;
} TlsContext;

/* TLS 上下文管理 */
TlsContext *tls_ctx_new(bool is_server);
void tls_ctx_free(TlsContext *tc);

/* 证书加载 */
int tls_ctx_load_cert(TlsContext *tc, const char *cert_path, const char *key_path);
int tls_ctx_load_ca(TlsContext *tc, const char *ca_path);

/* 连接包装 */
int tls_wrap_socket(TlsContext *tc, int fd);
int tls_handshake(TlsContext *tc);
int tls_read(TlsContext *tc, void *buf, size_t len);
int tls_write(TlsContext *tc, const void *data, size_t len);
void tls_close(TlsContext *tc);

/* 连接级TLS */
int dist_conn_enable_tls(DistConnection *conn, bool is_server, const char *cert_path, const char *key_path);
int dist_conn_tls_handshake(DistConnection *conn);

/* 自签名证书生成(测试用) */
int tls_generate_selfsigned(const char *cert_path, const char *key_path, int days);
#endif /* PONYPP_USE_TLS */

/* ==================== 分布式监督树 ==================== */
typedef enum {
    DIST_SUPERVISE_ONE_FOR_ONE = 0,   /* 只重启崩溃的 */
    DIST_SUPERVISE_ONE_FOR_ALL,       /* 重启全部 */
    DIST_SUPERVISE_REST_FOR_ONE       /* 重启崩溃的+后续 */
} DistSuperviseStrategy;

typedef enum {
    DIST_ACTOR_RUNNING = 0,
    DIST_ACTOR_STOPPED,
    DIST_ACTOR_CRASHED,
    DIST_ACTOR_RESTARTING
} DistActorState;

typedef struct {
    char name[64];
    char host[128];
    int port;
    int remote_actor_id;
    DistActorState state;
    int restart_count;
    int max_restarts;
    uint64_t last_heartbeat_ms;
} DistSupervisedActor;

typedef struct DistSupervisor {
    DistSupervisedActor *actors;
    size_t count;
    size_t cap;
    DistSuperviseStrategy strategy;
    int max_restarts;           /* 全局最大重启次数 */
    int global_restarts;
    char supervisor_id[64];
} DistSupervisor;

/* 创建/释放 */
DistSupervisor *dist_supervisor_new(const char *id, DistSuperviseStrategy strategy, int max_restarts);
void dist_supervisor_free(DistSupervisor *sup);

/* 注册远程Actor */
int dist_supervisor_register(DistSupervisor *sup, const char *name,
                              const char *host, int port, int remote_actor_id);

/* 心跳更新 */
int dist_supervisor_heartbeat(DistSupervisor *sup, const char *name);

/* 崩溃通知 -> 触发监督策略 */
int dist_supervisor_notify_crash(DistSupervisor *sup, const char *name);

/* 查询状态 */
DistActorState dist_supervisor_actor_state(DistSupervisor *sup, const char *name);
size_t dist_supervisor_count(DistSupervisor *sup);
int dist_supervisor_restart_count(DistSupervisor *sup, const char *name);

/* 超时检测 (返回超时未心跳的actor数量) */
int dist_supervisor_check_timeouts(DistSupervisor *sup, uint64_t timeout_ms);

/* 获取需要重启的actor列表 */
int dist_supervisor_pending_restarts(DistSupervisor *sup, char names[][64], int max);

/* ==================== 分布式监督树 ==================== */

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_DISTRIBUTED_H */
