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
DistributedRuntime *dist_runtime_new(void *local, int port);
void dist_runtime_free(DistributedRuntime *dr);
int dist_runtime_listen(DistributedRuntime *dr);
int dist_runtime_register_remote(DistributedRuntime *dr, const char *name,
                                  const char *host, int port, int actor_id);
int dist_runtime_send(DistributedRuntime *dr, const char *remote_name,
                       const char *method, const void *arg, size_t arg_size);
const char *dist_runtime_node_id(DistributedRuntime *dr);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_DISTRIBUTED_H */
