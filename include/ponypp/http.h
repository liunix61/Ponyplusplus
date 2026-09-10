/*
 * http.h - Pony++ HTTP 客户端模块
 *
 * Phase 4: 生态 — HTTP/1.1 客户端
 *
 * 基于 POSIX sockets, 支持 GET/POST/PUT/DELETE。
 * 不依赖第三方库。
 */
#ifndef PONYPP_HTTP_H
#define PONYPP_HTTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

/* ======================== HTTP 请求/响应 ======================== */

typedef struct HttpRequest {
    char method[16];       /* GET/POST/PUT/DELETE */
    char url[2048];
    char headers[16][256]; /* 自定义请求头 */
    int header_count;
    char *body;            /* 请求体 */
    size_t body_len;
    int timeout_ms;        /* 超时(毫秒), 0=默认30秒 */
} HttpRequest;

typedef struct HttpResponse {
    int status_code;       /* 200, 404, 500... */
    char status_text[64];  /* "OK", "Not Found"... */
    char headers[32][256]; /* 响应头 */
    int header_count;
    char *body;            /* 响应体 */
    size_t body_len;
    bool chunked;          /* 是否 chunked 编码 */
} HttpResponse;

/* ======================== API ======================== */

/* 创建请求 */
HttpRequest *http_request_new(const char *method, const char *url);
void http_request_free(HttpRequest *req);

/* 设置请求头 */
int http_request_add_header(HttpRequest *req, const char *key, const char *value);

/* 设置请求体 */
int http_request_set_body(HttpRequest *req, const char *body, size_t len);
int http_request_set_json(HttpRequest *req, const char *json);

/* 设置超时 */
void http_request_set_timeout(HttpRequest *req, int timeout_ms);

/* 执行请求 (阻塞式) */
HttpResponse *http_request_send(HttpRequest *req);

/* 释放响应 */
void http_response_free(HttpResponse *resp);

/* 获取响应头 */
const char *http_response_get_header(const HttpResponse *resp, const char *key);

/* 便捷函数 */
HttpResponse *http_get(const char *url);
HttpResponse *http_post(const char *url, const char *body, const char *content_type);

/* URL 解析 */
typedef struct UrlParts {
    char scheme[16];   /* http/https */
    char host[256];
    int port;          /* 默认 80/443 */
    char path[1024];
    char query[1024];
} UrlParts;

int http_url_parse(const char *url, UrlParts *out);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_HTTP_H */
