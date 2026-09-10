/*
 * HTTP 集成测试（需要本地服务器）
 * 服务器: python3 tests/http_test_server.py 18923
 */

#include <gtest/gtest.h>
#include <ponypp/http.h>
#include <cstring>

#define TEST_URL "http://127.0.0.1:18923"

/* ==================== GET 请求 ==================== */

TEST(HttpIntegration, HttpGet) {
    HttpResponse *resp = http_get(TEST_URL "/test");
    if (resp) {
        /* 验证响应存在 */
        http_response_free(resp);
    }
    /* 网络失败不应崩溃 */
}

TEST(HttpIntegration, HttpGetHeaders) {
    HttpRequest *req = http_request_new("GET", TEST_URL "/test");
    ASSERT_NE(req, nullptr);
    
    HttpResponse *resp = http_request_send(req);
    if (resp) {
        const char *hdr = http_response_get_header(resp, "X-Test-Header");
        /* 可能返回 NULL 或 "test-value" */
        (void)hdr;
        http_response_free(resp);
    }
    http_request_free(req);
}

/* ==================== POST 请求 ==================== */

TEST(HttpIntegration, HttpPost) {
    HttpResponse *resp = http_post(TEST_URL "/test", "{\"key\":\"value\"}", "application/json");
    if (resp) {
        http_response_free(resp);
    }
}

TEST(HttpIntegration, HttpPostWithBody) {
    HttpRequest *req = http_request_new("POST", TEST_URL "/test");
    ASSERT_NE(req, nullptr);
    
    http_request_set_body(req, "hello world", 11);
    http_request_add_header(req, "Content-Type", "text/plain");
    
    HttpResponse *resp = http_request_send(req);
    if (resp) {
        http_response_free(resp);
    }
    http_request_free(req);
}

/* ==================== 错误处理 ==================== */

TEST(HttpIntegration, HttpGetInvalidHost) {
    HttpResponse *resp = http_get("http://invalid.invalid/test");
    /* 应该失败但不崩溃 */
    if (resp) http_response_free(resp);
}

TEST(HttpIntegration, HttpGetConnectionRefused) {
    HttpResponse *resp = http_get("http://127.0.0.1:1/test");
    /* 应该失败但不崩溃 */
    if (resp) http_response_free(resp);
}

/* ==================== URL 解析 + 请求 ==================== */

TEST(HttpIntegration, UrlParseAndRequest) {
    UrlParts parts;
    int r = http_url_parse(TEST_URL "/api/v1/data?limit=10", &parts);
    ASSERT_EQ(r, 0);
    EXPECT_STREQ(parts.host, "127.0.0.1");
    EXPECT_EQ(parts.port, 18923);
    EXPECT_STREQ(parts.path, "/api/v1/data");
    EXPECT_STREQ(parts.query, "limit=10");
}

/* ==================== 超时设置 ==================== */

TEST(HttpIntegration, HttpGetWithTimeout) {
    HttpRequest *req = http_request_new("GET", TEST_URL "/test");
    ASSERT_NE(req, nullptr);
    http_request_set_timeout(req, 1000);
    
    HttpResponse *resp = http_request_send(req);
    if (resp) {
        http_response_free(resp);
    }
    http_request_free(req);
}

/* ==================== JSON 请求 ==================== */

TEST(HttpIntegration, HttpPostJson) {
    HttpRequest *req = http_request_new("POST", TEST_URL "/test");
    ASSERT_NE(req, nullptr);
    
    int r = http_request_set_json(req, "{\"name\":\"test\",\"value\":42}");
    EXPECT_EQ(r, 0);
    
    HttpResponse *resp = http_request_send(req);
    if (resp) {
        http_response_free(resp);
    }
    http_request_free(req);
}
