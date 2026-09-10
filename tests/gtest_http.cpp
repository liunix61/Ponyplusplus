/*
 * HTTP 模块测试
 * URL解析、请求构建、头部管理（不需要网络）
 */

#include <gtest/gtest.h>
#include <ponypp/http.h>
#include <cstring>

/* ==================== URL 解析 ==================== */

TEST(HttpTest, UrlParseBasic) {
    UrlParts parts;
    int r = http_url_parse("http://example.com/path", &parts);
    ASSERT_EQ(r, 0);
    EXPECT_STREQ(parts.scheme, "http");
    EXPECT_STREQ(parts.host, "example.com");
    EXPECT_STREQ(parts.path, "/path");
    EXPECT_EQ(parts.port, 80);
}

TEST(HttpTest, UrlParseHttps) {
    UrlParts parts;
    int r = http_url_parse("https://example.com/secure", &parts);
    ASSERT_EQ(r, 0);
    EXPECT_STREQ(parts.scheme, "https");
    EXPECT_EQ(parts.port, 443);
}

TEST(HttpTest, UrlParsePort) {
    UrlParts parts;
    int r = http_url_parse("http://example.com:8080/api", &parts);
    ASSERT_EQ(r, 0);
    EXPECT_EQ(parts.port, 8080);
    EXPECT_STREQ(parts.host, "example.com");
}

TEST(HttpTest, UrlParseQuery) {
    UrlParts parts;
    int r = http_url_parse("http://example.com/search?q=test&page=1", &parts);
    ASSERT_EQ(r, 0);
    EXPECT_STREQ(parts.path, "/search");
    EXPECT_STREQ(parts.query, "q=test&page=1");
}

TEST(HttpTest, UrlParseRootPath) {
    UrlParts parts;
    int r = http_url_parse("http://example.com", &parts);
    ASSERT_EQ(r, 0);
    EXPECT_STREQ(parts.host, "example.com");
}

TEST(HttpTest, UrlParseNull) {
    UrlParts parts;
    int r = http_url_parse(nullptr, &parts);
    EXPECT_NE(r, 0);
}

TEST(HttpTest, UrlParseEmpty) {
    UrlParts parts;
    int r = http_url_parse("", &parts);
    EXPECT_NE(r, 0);
}

/* ==================== 请求构建 ==================== */

TEST(HttpTest, RequestNew) {
    HttpRequest *req = http_request_new("GET", "http://example.com");
    ASSERT_NE(req, nullptr);
    http_request_free(req);
}

TEST(HttpTest, RequestAddHeader) {
    HttpRequest *req = http_request_new("GET", "http://example.com");
    ASSERT_NE(req, nullptr);
    int r = http_request_add_header(req, "Content-Type", "application/json");
    EXPECT_EQ(r, 0);
    http_request_free(req);
}

TEST(HttpTest, RequestSetBody) {
    HttpRequest *req = http_request_new("POST", "http://example.com");
    ASSERT_NE(req, nullptr);
    int r = http_request_set_body(req, "hello", 5);
    EXPECT_EQ(r, 0);
    http_request_free(req);
}

TEST(HttpTest, RequestSetJson) {
    HttpRequest *req = http_request_new("POST", "http://example.com");
    ASSERT_NE(req, nullptr);
    int r = http_request_set_json(req, "{\"key\": \"value\"}");
    EXPECT_EQ(r, 0);
    http_request_free(req);
}

TEST(HttpTest, RequestSetTimeout) {
    HttpRequest *req = http_request_new("GET", "http://example.com");
    ASSERT_NE(req, nullptr);
    http_request_set_timeout(req, 5000);
    http_request_free(req);
}

TEST(HttpTest, RequestNullArgs) {
    HttpRequest *req = http_request_new(nullptr, nullptr);
    /* 可能返回NULL或有效指针 */
    if (req) http_request_free(req);
}

/* ==================== 响应处理 ==================== */

TEST(HttpTest, ResponseFreeNull) {
    http_response_free(nullptr);
    /* 不应崩溃 */
}

TEST(HttpTest, ResponseGetHeaderNull) {
    const char *h = http_response_get_header(nullptr, "Content-Type");
    EXPECT_EQ(h, nullptr);
}

/* ==================== 便捷函数 ==================== */

TEST(HttpTest, HttpGetNull) {
    HttpResponse *resp = http_get(nullptr);
    /* 网络调用可能失败，但不应崩溃 */
    if (resp) http_response_free(resp);
}

TEST(HttpTest, HttpPostNull) {
    HttpResponse *resp = http_post(nullptr, nullptr, nullptr);
    if (resp) http_response_free(resp);
}

/* ==================== 边界情况 ==================== */

TEST(HttpTest, UrlParseLongUrl) {
    UrlParts parts;
    char long_url[2048];
    snprintf(long_url, sizeof(long_url), "http://example.com/%s", "a");
    for (int i = 0; i < 100; i++) {
        strcat(long_url, "/segment");
    }
    int r = http_url_parse(long_url, &parts);
    /* 可能成功或失败，但不应崩溃 */
    (void)r;
}

TEST(HttpTest, UrlParseSpecialChars) {
    UrlParts parts;
    int r = http_url_parse("http://example.com/path%20with%20spaces", &parts);
    ASSERT_EQ(r, 0);
    EXPECT_STREQ(parts.host, "example.com");
}

TEST(HttpTest, UrlParseFragment) {
    UrlParts parts;
    int r = http_url_parse("http://example.com/page#section", &parts);
    ASSERT_EQ(r, 0);
    EXPECT_STREQ(parts.host, "example.com");
}
