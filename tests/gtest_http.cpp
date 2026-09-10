/*
 * gtest_http.cpp - HTTP客户端单元测试
 *
 * Phase 4: http.c 测试 (URL解析/请求构建/响应解析)
 */
#include <gtest/gtest.h>
extern "C" {
#include "ponypp/http.h"
}
#include <string.h>

/* ======================== URL 解析 ======================== */

TEST(HttpUrl, ParseBasic) {
    UrlParts parts;
    ASSERT_EQ(http_url_parse("http://example.com/path", &parts), 0);
    EXPECT_STREQ(parts.scheme, "http");
    EXPECT_STREQ(parts.host, "example.com");
    EXPECT_EQ(parts.port, 80);
    EXPECT_STREQ(parts.path, "/path");
}

TEST(HttpUrl, ParseWithPort) {
    UrlParts parts;
    ASSERT_EQ(http_url_parse("http://localhost:8080/api", &parts), 0);
    EXPECT_STREQ(parts.host, "localhost");
    EXPECT_EQ(parts.port, 8080);
    EXPECT_STREQ(parts.path, "/api");
}

TEST(HttpUrl, ParseWithQuery) {
    UrlParts parts;
    ASSERT_EQ(http_url_parse("http://example.com/search?q=pony&page=1", &parts), 0);
    EXPECT_STREQ(parts.path, "/search");
    EXPECT_STREQ(parts.query, "q=pony&page=1");
}

TEST(HttpUrl, ParseHttps) {
    UrlParts parts;
    ASSERT_EQ(http_url_parse("https://secure.example.com/", &parts), 0);
    EXPECT_STREQ(parts.scheme, "https");
    EXPECT_EQ(parts.port, 443);
}

TEST(HttpUrl, ParseNoPath) {
    UrlParts parts;
    ASSERT_EQ(http_url_parse("http://example.com", &parts), 0);
    EXPECT_STREQ(parts.path, "/");
}

TEST(HttpUrl, ParseInvalid) {
    UrlParts parts;
    EXPECT_EQ(http_url_parse(nullptr, &parts), -1);
    EXPECT_EQ(http_url_parse("", &parts), -1);
    EXPECT_EQ(http_url_parse("no-scheme", &parts), -1);
    EXPECT_EQ(http_url_parse("http://", &parts), 0); /* 空host但格式正确 */
}

/* ======================== 请求构建 ======================== */

TEST(HttpRequest, NewAndFree) {
    HttpRequest *req = http_request_new("GET", "http://example.com");
    ASSERT_NE(req, nullptr);
    EXPECT_STREQ(req->method, "GET");
    EXPECT_STREQ(req->url, "http://example.com");
    EXPECT_EQ(req->timeout_ms, 30000);
    http_request_free(req);
}

TEST(HttpRequest, NewNullArgs) {
    EXPECT_EQ(http_request_new(nullptr, "http://x"), nullptr);
    EXPECT_EQ(http_request_new("GET", nullptr), nullptr);
    http_request_free(nullptr);
}

TEST(HttpRequest, AddHeaders) {
    HttpRequest *req = http_request_new("GET", "http://example.com");
    ASSERT_NE(req, nullptr);
    
    EXPECT_EQ(http_request_add_header(req, "Accept", "application/json"), 0);
    EXPECT_EQ(http_request_add_header(req, "User-Agent", "Pony++/0.5"), 0);
    EXPECT_EQ(req->header_count, 2);
    
    /* NULL 参数 */
    EXPECT_EQ(http_request_add_header(req, nullptr, "v"), -1);
    EXPECT_EQ(http_request_add_header(req, "k", nullptr), -1);
    
    http_request_free(req);
}

TEST(HttpRequest, SetBody) {
    HttpRequest *req = http_request_new("POST", "http://example.com");
    ASSERT_NE(req, nullptr);
    
    EXPECT_EQ(http_request_set_body(req, "hello", 5), 0);
    EXPECT_EQ(req->body_len, (size_t)5);
    EXPECT_STREQ(req->body, "hello");
    
    /* 覆盖已有 body */
    EXPECT_EQ(http_request_set_body(req, "world!", 6), 0);
    EXPECT_STREQ(req->body, "world!");
    
    http_request_free(req);
}

TEST(HttpRequest, SetJson) {
    HttpRequest *req = http_request_new("POST", "http://example.com");
    ASSERT_NE(req, nullptr);
    
    EXPECT_EQ(http_request_set_json(req, "{\"key\":\"value\"}"), 0);
    EXPECT_STREQ(req->body, "{\"key\":\"value\"}");
    /* 应自动添加 Content-Type 头 */
    EXPECT_GE(req->header_count, 1);
    
    http_request_free(req);
}

TEST(HttpRequest, SetTimeout) {
    HttpRequest *req = http_request_new("GET", "http://example.com");
    ASSERT_NE(req, nullptr);
    
    http_request_set_timeout(req, 5000);
    EXPECT_EQ(req->timeout_ms, 5000);
    
    http_request_set_timeout(nullptr, 0); /* 不应崩溃 */
    
    http_request_free(req);
}

/* ======================== 响应解析 ======================== */

TEST(HttpResponse, FreeNull) {
    http_response_free(nullptr); /* 不应崩溃 */
}

TEST(HttpResponse, GetHeaderNull) {
    EXPECT_EQ(http_response_get_header(nullptr, "Content-Type"), nullptr);
    
    HttpResponse resp;
    memset(&resp, 0, sizeof(resp));
    EXPECT_EQ(http_response_get_header(&resp, "Content-Type"), nullptr);
}

TEST(HttpResponse, GetHeaderFound) {
    HttpResponse resp;
    memset(&resp, 0, sizeof(resp));
    strcpy(resp.headers[0], "Content-Type: application/json");
    strcpy(resp.headers[1], "Server: Pony++");
    resp.header_count = 2;
    
    const char *ct = http_response_get_header(&resp, "Content-Type");
    ASSERT_NE(ct, nullptr);
    EXPECT_STREQ(ct, "application/json");
    
    const char *srv = http_response_get_header(&resp, "Server");
    ASSERT_NE(srv, nullptr);
    EXPECT_STREQ(srv, "Pony++");
    
    /* 不存在的头 */
    EXPECT_EQ(http_response_get_header(&resp, "X-Custom"), nullptr);
}

/* ======================== 便捷函数 ======================== */

TEST(HttpConvenience, GetNullUrl) {
    /* NULL URL 应返回 NULL 而不是崩溃 */
    HttpResponse *resp = http_get(nullptr);
    EXPECT_EQ(resp, nullptr);
}

TEST(HttpConvenience, PostNullArgs) {
    EXPECT_EQ(http_post(nullptr, "body", "text/plain"), nullptr);
}
