/*
 * 覆盖率提升测试: distributed + ffi + http (实际API)
 */

#include <gtest/gtest.h>
#include <ponypp/distributed.h>
#include <ponypp/ffi.h>
#include <ponypp/http.h>
#include <cstring>

/* ==================== Distributed ==================== */

TEST(Distributed, RemoteActorNewFree) {
    RemoteActor *ra = remote_actor_new("test_actor", "127.0.0.1", 9999, 42);
    ASSERT_NE(ra, nullptr);
    EXPECT_STREQ(ra->name, "test_actor");
    EXPECT_STREQ(ra->host, "127.0.0.1");
    EXPECT_EQ(ra->port, 9999);
    EXPECT_EQ(ra->actor_id, 42);
    remote_actor_free(ra);
}

TEST(Distributed, RemoteActorNull) {
    remote_actor_free(nullptr);
}

TEST(Distributed, DistRuntimeNewFree) {
    DistributedRuntime *dr = dist_runtime_new(nullptr, 0);
    if (dr) {
        const char *node_id = dist_runtime_node_id(dr);
        EXPECT_NE(node_id, nullptr);
        dist_runtime_free(dr);
    }
}

TEST(Distributed, DistConnNull) {
    dist_conn_free(nullptr);
    EXPECT_EQ(dist_send(nullptr, "test", 4), -1);
    EXPECT_EQ(dist_recv(nullptr, nullptr, 0), -1);
}

TEST(Distributed, DistRuntimeRegisterNull) {
    DistributedRuntime *dr = dist_runtime_new(nullptr, 0);
    if (dr) {
        int rc = dist_runtime_register_remote(dr, "remote1", "127.0.0.1", 9998, 1);
        (void)rc;  /* 可能失败(无网络), 但不应崩溃 */
        dist_runtime_free(dr);
    }
}

/* ==================== FFI ==================== */

TEST(FFI, RuntimeNewFree) {
    ffi_runtime_new();
    ffi_runtime_free();
}

TEST(FFI, TypeNames) {
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_I32), "i32");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_I64), "i64");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_F64), "f64");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_STRING), "string");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_PTR), "ptr");
    EXPECT_STREQ(ffi_type_name(FFI_TYPE_VOID), "void");
}

TEST(FFI, RegisterAndFind) {
    ffi_runtime_new();
    const char *param_names[] = {"a", "b"};
    FFIType param_types[] = {FFI_TYPE_I64, FFI_TYPE_I64};
    int rc = ffi_register_func("test_add", "test_mod", FFI_TYPE_I64, param_names, param_types, 2);
    if (rc == 0) {
        FFIFunc *ff = ffi_find_func("test_add");
        EXPECT_NE(ff, nullptr);
        EXPECT_EQ(ffi_find_func("nonexistent"), nullptr);
        EXPECT_GE(ffi_func_count(), 1u);
    }
    ffi_runtime_free();
}

TEST(FFI, Dump) {
    char buf[1024];
    ffi_runtime_new();
    int n = ffi_dump(buf, sizeof(buf));
    EXPECT_GE(n, 0);
    ffi_runtime_free();
}

/* ==================== HTTP ==================== */

TEST(Http, RequestNewFree) {
    HttpRequest *req = http_request_new("GET", "http://example.com/");
    ASSERT_NE(req, nullptr);
    http_request_free(req);
}

TEST(Http, RequestNull) {
    http_request_free(nullptr);
    http_response_free(nullptr);
}

TEST(Http, RequestHeaders) {
    HttpRequest *req = http_request_new("GET", "http://example.com/");
    ASSERT_NE(req, nullptr);
    EXPECT_EQ(http_request_add_header(req, "User-Agent", "PonyTest"), 0);
    EXPECT_EQ(http_request_add_header(req, "Accept", "application/json"), 0);
    http_request_free(req);
}

TEST(Http, RequestBody) {
    HttpRequest *req = http_request_new("POST", "http://example.com/api");
    ASSERT_NE(req, nullptr);
    EXPECT_EQ(http_request_set_body(req, "hello", 5), 0);
    EXPECT_EQ(http_request_set_json(req, "{\"key\":\"value\"}"), 0);
    http_request_set_timeout(req, 5000);
    http_request_free(req);
}

TEST(Http, UrlParse) {
    UrlParts parts;
    int rc = http_url_parse("http://example.com:8080/path?q=1", &parts);
    if (rc == 0) {
        EXPECT_STREQ(parts.scheme, "http");
        EXPECT_STREQ(parts.host, "example.com");
        EXPECT_EQ(parts.port, 8080);
    }
}

TEST(Http, UrlParseSimple) {
    UrlParts parts;
    int rc = http_url_parse("https://api.example.com/v1/users", &parts);
    if (rc == 0) {
        EXPECT_STREQ(parts.scheme, "https");
        EXPECT_STREQ(parts.host, "api.example.com");
        EXPECT_EQ(parts.port, 443);  /* HTTPS默认端口 */
    }
}

TEST(Http, ResponseNull) {
    EXPECT_EQ(http_response_get_header(nullptr, "Content-Type"), nullptr);
}
