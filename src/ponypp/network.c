/**
 * Pony++ Network - TCP socket 操作
 * 
 * 提供 TCP 客户端和服务端基本操作。
 * 实现: src/ponypp/network.c
 */
#include "ponypp/tool.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

typedef struct PnySocket {
    int fd;
    bool listening;
    bool connected;
} PnySocket;

PnySocket *pny_tcp_connect(const char *host, int port) {
    if (!host || port <= 0) return NULL;
    PnySocket *s = (PnySocket *)calloc(1, sizeof(PnySocket));
    if (!s) return NULL;
    s->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->fd < 0) { free(s); return NULL; }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        close(s->fd);
        free(s);
        return NULL;
    }
    if (connect(s->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(s->fd);
        free(s);
        return NULL;
    }
    s->connected = true;
    return s;
}

PnySocket *pny_tcp_listen(int port) {
    if (port <= 0) return NULL;
    PnySocket *s = (PnySocket *)calloc(1, sizeof(PnySocket));
    if (!s) return NULL;
    s->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->fd < 0) { free(s); return NULL; }
    int opt = 1;
    setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(s->fd);
        free(s);
        return NULL;
    }
    if (listen(s->fd, 16) < 0) {
        close(s->fd);
        free(s);
        return NULL;
    }
    s->listening = true;
    return s;
}

int pny_tcp_send(PnySocket *s, const void *data, size_t len) {
    if (!s || s->fd < 0 || !data || len == 0) return -1;
    ssize_t total = 0;
    const char *p = (const char *)data;
    while (total < (ssize_t)len) {
        ssize_t n = send(s->fd, p + total, len - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return (int)total;
}

int pny_tcp_recv(PnySocket *s, void *buf, size_t max_len) {
    if (!s || s->fd < 0 || !buf || max_len == 0) return -1;
    ssize_t n = recv(s->fd, buf, max_len, MSG_DONTWAIT);
    if (n < 0) return -1;
    return (int)n;
}

void pny_tcp_close(PnySocket *s) {
    if (!s) return;
    if (s->fd >= 0) close(s->fd);
    free(s);
}

bool pny_tcp_connected(const PnySocket *s) {
    return s && s->connected;
}
