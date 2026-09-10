/*
 * http.c - Pony++ HTTP 客户端实现
 *
 * Phase 4: 生态 — HTTP/1.1 客户端 (POSIX sockets)
 */

#include "ponypp/http.h"
#include "ponypp/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

/* ======================== URL 解析 ======================== */

int http_url_parse(const char *url, UrlParts *out) {
    if (!url || !out) return -1;
    memset(out, 0, sizeof(UrlParts));
    
    const char *p = url;
    
    /* scheme */
    const char *colon = strstr(p, "://");
    if (!colon) return -1;
    size_t scheme_len = colon - p;
    if (scheme_len >= sizeof(out->scheme)) return -1;
    memcpy(out->scheme, p, scheme_len);
    out->scheme[scheme_len] = '\0';
    p = colon + 3;
    
    /* host[:port][/path][?query] */
    const char *host_end = p;
    while (*host_end && *host_end != '/' && *host_end != '?' && *host_end != ':') host_end++;
    
    size_t host_len = host_end - p;
    if (host_len >= sizeof(out->host)) return -1;
    memcpy(out->host, p, host_len);
    out->host[host_len] = '\0';
    p = host_end;
    
    /* port */
    if (*p == ':') {
        p++;
        out->port = atoi(p);
        while (*p && *p != '/' && *p != '?') p++;
    } else {
        out->port = (strcmp(out->scheme, "https") == 0) ? 443 : 80;
    }
    
    /* path */
    if (*p == '/') {
        const char *path_start = p;
        while (*p && *p != '?') p++;
        size_t path_len = p - path_start;
        if (path_len >= sizeof(out->path)) path_len = sizeof(out->path) - 1;
        memcpy(out->path, path_start, path_len);
        out->path[path_len] = '\0';
    } else {
        strcpy(out->path, "/");
    }
    
    /* query */
    if (*p == '?') {
        p++;
        strncpy(out->query, p, sizeof(out->query) - 1);
    }
    
    return 0;
}

/* ======================== 请求管理 ======================== */

HttpRequest *http_request_new(const char *method, const char *url) {
    if (!method || !url) return NULL;
    
    HttpRequest *req = (HttpRequest *)calloc(1, sizeof(HttpRequest));
    if (!req) return NULL;
    
    strncpy(req->method, method, sizeof(req->method) - 1);
    strncpy(req->url, url, sizeof(req->url) - 1);
    req->timeout_ms = 30000; /* 默认 30 秒 */
    
    return req;
}

void http_request_free(HttpRequest *req) {
    if (!req) return;
    if (req->body) s_free(req->body);
    s_free(req);
}

int http_request_add_header(HttpRequest *req, const char *key, const char *value) {
    if (!req || !key || !value || req->header_count >= 16) return -1;
    
    snprintf(req->headers[req->header_count], sizeof(req->headers[0]),
             "%s: %s", key, value);
    req->header_count++;
    return 0;
}

int http_request_set_body(HttpRequest *req, const char *body, size_t len) {
    if (!req) return -1;
    if (req->body) s_free(req->body);
    
    req->body = (char *)s_malloc(len + 1);
    if (!req->body) return -1;
    
    if (body && len > 0) memcpy(req->body, body, len);
    req->body[len] = '\0';
    req->body_len = len;
    
    return 0;
}

int http_request_set_json(HttpRequest *req, const char *json) {
    if (!req || !json) return -1;
    
    http_request_add_header(req, "Content-Type", "application/json");
    return http_request_set_body(req, json, strlen(json));
}

void http_request_set_timeout(HttpRequest *req, int timeout_ms) {
    if (req) req->timeout_ms = timeout_ms;
}

/* ======================== 响应管理 ======================== */

void http_response_free(HttpResponse *resp) {
    if (!resp) return;
    if (resp->body) s_free(resp->body);
    s_free(resp);
}

const char *http_response_get_header(const HttpResponse *resp, const char *key) {
    if (!resp || !key) return NULL;
    
    size_t key_len = strlen(key);
    for (int i = 0; i < resp->header_count; i++) {
        if (strncasecmp(resp->headers[i], key, key_len) == 0 &&
            resp->headers[i][key_len] == ':') {
            const char *val = resp->headers[i] + key_len + 1;
            while (*val == ' ') val++;
            return val;
        }
    }
    return NULL;
}

/* ======================== HTTP 发送 ======================== */

static int http_connect(const UrlParts *parts) {
    struct addrinfo hints, *result = NULL;
    char port_str[16];
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    snprintf(port_str, sizeof(port_str), "%d", parts->port);
    
    if (getaddrinfo(parts->host, port_str, &hints, &result) != 0) return -1;
    
    int sock = -1;
    for (struct addrinfo *rp = result; rp; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) continue;
        
        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) break;
        
        close(sock);
        sock = -1;
    }
    
    freeaddrinfo(result);
    return sock;
}

HttpResponse *http_request_send(HttpRequest *req) {
    if (!req) return NULL;
    
    UrlParts parts;
    if (http_url_parse(req->url, &parts) != 0) return NULL;
    
    /* TODO: HTTPS 需要 TLS, 当前仅支持 HTTP */
    if (strcmp(parts.scheme, "https") == 0) {
        fprintf(stderr, "[http] HTTPS 暂不支持\n");
        return NULL;
    }
    
    int sock = http_connect(&parts);
    if (sock < 0) return NULL;
    
    /* 构建请求 */
    char request[8192];
    int offset = 0;
    
    /* 请求行 */
    const char *path = parts.path[0] ? parts.path : "/";
    if (parts.query[0]) {
        offset += snprintf(request + offset, sizeof(request) - offset,
                          "%s %s?%s HTTP/1.1\r\n", req->method, path, parts.query);
    } else {
        offset += snprintf(request + offset, sizeof(request) - offset,
                          "%s %s HTTP/1.1\r\n", req->method, path);
    }
    
    /* Host 头 */
    offset += snprintf(request + offset, sizeof(request) - offset,
                      "Host: %s:%d\r\n", parts.host, parts.port);
    
    /* Connection: close (简化: 不支持 keep-alive) */
    offset += snprintf(request + offset, sizeof(request) - offset,
                      "Connection: close\r\n");
    
    /* 自定义头 */
    for (int i = 0; i < req->header_count && offset < (int)sizeof(request) - 256; i++) {
        offset += snprintf(request + offset, sizeof(request) - offset,
                          "%s\r\n", req->headers[i]);
    }
    
    /* Content-Length */
    if (req->body && req->body_len > 0) {
        offset += snprintf(request + offset, sizeof(request) - offset,
                          "Content-Length: %zu\r\n", req->body_len);
    }
    
    /* 头部结束 */
    offset += snprintf(request + offset, sizeof(request) - offset, "\r\n");
    
    /* 发送请求头 */
    if (send(sock, request, offset, 0) < 0) {
        close(sock);
        return NULL;
    }
    
    /* 发送请求体 */
    if (req->body && req->body_len > 0) {
        if (send(sock, req->body, req->body_len, 0) < 0) {
            close(sock);
            return NULL;
        }
    }
    
    /* 读取响应 */
    HttpResponse *resp = (HttpResponse *)calloc(1, sizeof(HttpResponse));
    if (!resp) { close(sock); return NULL; }
    
    char buf[4096];
    char *response_data = NULL;
    size_t response_len = 0;
    size_t response_cap = 0;
    
    ssize_t n;
    while ((n = recv(sock, buf, sizeof(buf), 0)) > 0) {
        if (response_len + n > response_cap) {
            response_cap = (response_cap + n) * 2;
            response_data = (char *)s_realloc(response_data, response_cap);
            if (!response_data) { close(sock); http_response_free(resp); return NULL; }
        }
        memcpy(response_data + response_len, buf, n);
        response_len += n;
    }
    
    close(sock);
    
    if (!response_data || response_len == 0) {
        s_free(response_data);
        http_response_free(resp);
        return NULL;
    }
    
    response_data[response_len < response_cap ? response_len : response_cap - 1] = '\0';
    
    /* 解析状态行 */
    char *p = response_data;
    char *line_end = strstr(p, "\r\n");
    if (!line_end) { s_free(response_data); http_response_free(resp); return NULL; }
    
    /* HTTP/1.1 200 OK */
    char *space1 = strchr(p, ' ');
    if (space1) {
        resp->status_code = atoi(space1 + 1);
        char *space2 = strchr(space1 + 1, ' ');
        if (space2) {
            size_t text_len = line_end - space2 - 1;
            if (text_len >= sizeof(resp->status_text)) text_len = sizeof(resp->status_text) - 1;
            memcpy(resp->status_text, space2 + 1, text_len);
            resp->status_text[text_len] = '\0';
        }
    }
    
    /* 解析头部 */
    p = line_end + 2;
    while (p < response_data + response_len) {
        line_end = strstr(p, "\r\n");
        if (!line_end || line_end == p) break; /* 空行 = 头部结束 */
        
        if (resp->header_count < 32) {
            size_t hlen = line_end - p;
            if (hlen >= sizeof(resp->headers[0])) hlen = sizeof(resp->headers[0]) - 1;
            memcpy(resp->headers[resp->header_count], p, hlen);
            resp->headers[resp->header_count][hlen] = '\0';
            resp->header_count++;
        }
        
        p = line_end + 2;
    }
    
    /* 提取响应体 */
    if (line_end) {
        p = line_end + 2;
        size_t body_len = response_data + response_len - p;
        resp->body = (char *)s_malloc(body_len + 1);
        if (resp->body) {
            memcpy(resp->body, p, body_len);
            resp->body[body_len] = '\0';
            resp->body_len = body_len;
        }
    }
    
    s_free(response_data);
    return resp;
}

/* ======================== 便捷函数 ======================== */

HttpResponse *http_get(const char *url) {
    HttpRequest *req = http_request_new("GET", url);
    if (!req) return NULL;
    
    HttpResponse *resp = http_request_send(req);
    http_request_free(req);
    return resp;
}

HttpResponse *http_post(const char *url, const char *body, const char *content_type) {
    HttpRequest *req = http_request_new("POST", url);
    if (!req) return NULL;
    
    if (content_type) {
        http_request_add_header(req, "Content-Type", content_type);
    }
    if (body) {
        http_request_set_body(req, body, strlen(body));
    }
    
    HttpResponse *resp = http_request_send(req);
    http_request_free(req);
    return resp;
}
