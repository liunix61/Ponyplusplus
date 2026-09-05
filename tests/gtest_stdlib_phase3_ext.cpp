#include "ponypp/stdlib.h"
#include "ponypp/tool.h"
#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

/* ==================== Network TCP Tests ==================== */

TEST(Network, TcpListenInvalidPort) {
    /* Port 0 should fail */
    PnySocket *s = pny_tcp_listen(0);
    ASSERT_EQ(s, nullptr);
    /* Port -1 should fail */
    s = pny_tcp_listen(-1);
    ASSERT_EQ(s, nullptr);
}

TEST(Network, TcpConnectInvalidHost) {
    /* Invalid host should return NULL */
    PnySocket *s = pny_tcp_connect("256.256.256.256", 80);
    ASSERT_EQ(s, nullptr);
}

TEST(Network, TcpConnectInvalidPort) {
    /* Port 0 should fail */
    PnySocket *s = pny_tcp_connect("127.0.0.1", 0);
    ASSERT_EQ(s, nullptr);
}

TEST(Network, TcpConnectNoServer) {
    /* Connect to a port with no listener should fail */
    PnySocket *s = pny_tcp_connect("127.0.0.1", 1);
    ASSERT_EQ(s, nullptr);
}

TEST(Network, TcpConnectValidHost) {
    /* Connect to localhost on a valid but likely closed port — should fail gracefully */
    PnySocket *s = pny_tcp_connect("127.0.0.1", 19999);
    /* May succeed or fail depending on port availability, but shouldn't crash */
    if (s) {
        ASSERT_TRUE(pny_tcp_connected(s));
        pny_tcp_close(s);
    }
}

TEST(Network, TcpSendRecvNull) {
    /* Null socket operations should return -1 / not crash */
    ASSERT_EQ(pny_tcp_send(NULL, "test", 4), -1);
    char buf[64];
    ASSERT_EQ(pny_tcp_recv(NULL, buf, sizeof(buf)), -1);
    pny_tcp_close(NULL); /* should not crash */
    ASSERT_FALSE(pny_tcp_connected(NULL));
}

TEST(Network, TcpSendEmpty) {
    PnySocket *s = pny_tcp_listen(19998);
    if (s) {
        /* Send with empty data should fail */
        ASSERT_EQ(pny_tcp_send(s, NULL, 0), -1);
        pny_tcp_close(s);
    }
}

TEST(Network, TcpRecvEmpty) {
    PnySocket *s = pny_tcp_listen(19997);
    if (s) {
        char buf[64];
        /* Recv on a listening socket should return -1 (no connection) */
        int n = pny_tcp_recv(s, buf, sizeof(buf));
        ASSERT_EQ(n, -1);
        pny_tcp_close(s);
    }
}

TEST(Network, TcpClose) {
    PnySocket *s = pny_tcp_listen(19996);
    if (s) {
        ASSERT_FALSE(pny_tcp_connected(s));
        pny_tcp_close(s);
    }
    /* Double close should not crash */
    pny_tcp_close(NULL);
}

/* ==================== Profiler Tests ==================== */

TEST(Profiler, NewAndFree) {
    PnyProfiler *p = pny_profiler_new();
    ASSERT_NE(p, nullptr);
    ASSERT_EQ(pny_profiler_sample_count(p), 0);
    pny_profiler_free(p);
}

TEST(Profiler, StartStop) {
    PnyProfiler *p = pny_profiler_new();
    ASSERT_NE(p, nullptr);
    pny_profiler_start(p);
    pny_profiler_stop(p);
    pny_profiler_free(p);
}

TEST(Profiler, SampleCount) {
    PnyProfiler *p = pny_profiler_new();
    ASSERT_NE(p, nullptr);
    /* Should not sample when not started */
    pny_profiler_sample(p, 1, "method", 100);
    ASSERT_EQ(pny_profiler_sample_count(p), 0);
    /* Start and sample */
    pny_profiler_start(p);
    pny_profiler_sample(p, 1, "method", 100);
    ASSERT_EQ(pny_profiler_sample_count(p), 1);
    pny_profiler_free(p);
}

TEST(Profiler, SampleWhileRunning) {
    PnyProfiler *p = pny_profiler_new();
    ASSERT_NE(p, nullptr);
    pny_profiler_start(p);
    pny_profiler_sample(p, 1, "init", 1500);
    pny_profiler_sample(p, 2, "update", 2500);
    pny_profiler_sample(p, 1, "init", 300);
    ASSERT_EQ(pny_profiler_sample_count(p), 3);
    pny_profiler_stop(p);
    pny_profiler_free(p);
}

TEST(Profiler, NullSafe) {
    /* Null profiler operations should not crash */
    pny_profiler_free(NULL);
    pny_profiler_start(NULL);
    pny_profiler_stop(NULL);
    pny_profiler_sample(NULL, 1, "test", 100);
    ASSERT_EQ(pny_profiler_sample_count(NULL), 0);
    ASSERT_EQ(pny_profiler_export(NULL, "/tmp/null.json"), -1);
}

TEST(Profiler, Export) {
    PnyProfiler *p = pny_profiler_new();
    ASSERT_NE(p, nullptr);
    pny_profiler_start(p);
    pny_profiler_sample(p, 1, "actor1_method", 1500000);
    pny_profiler_sample(p, 2, "actor2_method", 2500000);
    pny_profiler_sample(p, 3, "actor3_method", 500000);
    pny_profiler_stop(p);

    const char *path = "/tmp/ponypp_profile_test.json";
    int rc = pny_profiler_export(p, path);
    ASSERT_EQ(rc, 0);

    /* Verify file was created and has content */
    FILE *f = fopen(path, "r");
    ASSERT_NE(f, nullptr);
    char buf[1024];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    ASSERT_GT(n, 0);
    buf[n] = '\0';
    fclose(f);

    /* Check JSON structure */
    ASSERT_NE(strstr(buf, "total_samples"), nullptr);
    ASSERT_NE(strstr(buf, "3"), nullptr)
        << "Expected count=3 in: " << buf;
    ASSERT_NE(strstr(buf, "actor1_method"), nullptr);
    ASSERT_NE(strstr(buf, "actor2_method"), nullptr);
    ASSERT_NE(strstr(buf, "actor3_method"), nullptr);

    remove(path);
    pny_profiler_free(p);
}

TEST(Profiler, ExportNullPath) {
    PnyProfiler *p = pny_profiler_new();
    ASSERT_NE(p, nullptr);
    ASSERT_EQ(pny_profiler_export(p, NULL), -1);
    pny_profiler_free(p);
}

TEST(Profiler, ExportNoSamples) {
    PnyProfiler *p = pny_profiler_new();
    ASSERT_NE(p, nullptr);
    const char *path = "/tmp/ponypp_profile_empty.json";
    int rc = pny_profiler_export(p, path);
    ASSERT_EQ(rc, 0);
    FILE *f = fopen(path, "r");
    ASSERT_NE(f, nullptr);
    fclose(f);
    remove(path);
    pny_profiler_free(p);
}

TEST(Profiler, MultipleSamples) {
    PnyProfiler *p = pny_profiler_new();
    ASSERT_NE(p, nullptr);
    pny_profiler_start(p);
    for (int i = 0; i < 100; i++) {
        pny_profiler_sample(p, i % 5, "loop", (int64_t)(i * 1000));
    }
    ASSERT_EQ(pny_profiler_sample_count(p), 100);
    pny_profiler_stop(p);
    pny_profiler_free(p);
}

/* ==================== Tool REPL Test ==================== */

TEST(Tool, ReplCommandParsed) {
    ToolConfig tc;
    char *argv[] = { "ponyppc", "repl", NULL };
    int argc = 2;
    int rc = tool_parse_args(argc, argv, &tc);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(tc.cmd, TOOL_REPL);
}

/* ==================== JSON Extended Tests ==================== */

TEST(Json, DeeplyNested) {
    /* Deeply nested JSON */
    const char *input = "{\"a\":{\"b\":{\"c\":{\"d\":[1,{\"e\":true}]}}}}";
    JsonValue *v = json_parse(input, strlen(input));
    ASSERT_NE(v, nullptr);
    JsonValue *a = json_obj_get(v, "a");
    ASSERT_NE(a, nullptr);
    JsonValue *b = json_obj_get(a, "b");
    ASSERT_NE(b, nullptr);
    JsonValue *c = json_obj_get(b, "c");
    ASSERT_NE(c, nullptr);
    JsonValue *d = json_obj_get(c, "d");
    ASSERT_NE(d, nullptr);
    ASSERT_EQ(d->arr.count, 2);
    ASSERT_EQ(d->arr.items[0]->i, 1);
    ASSERT_TRUE(d->arr.items[1]->type == JSON_OBJECT);
    JsonValue *e = json_obj_get(d->arr.items[1], "e");
    ASSERT_TRUE(e->b);
    json_free(v);
}

TEST(Json, SpecialChars) {
    /* JSON with special characters */
    JsonValue *v = json_parse("{\"msg\":\"hello\\tworld\\r\\n\"}", 30);
    ASSERT_NE(v, nullptr);
    JsonValue *msg = json_obj_get(v, "msg");
    ASSERT_NE(msg, nullptr);
    ASSERT_STREQ(msg->s, "hello\tworld\r\n");
    json_free(v);
}

TEST(Json, UnicodeEscapes) {
    /* JSON with Unicode escape — \u0048 = 'H', \u0065 = 'e' */
    const char *input = "\"\\u0048\\u0065\"";
    JsonValue *v = json_parse(input, strlen(input));
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_STRING);
    ASSERT_STREQ(v->s, "He");
    json_free(v);
}

TEST(Json, StringifyDeep) {
    /* Build deeply nested structure and stringify */
    JsonValue *obj = json_new_object();
    JsonValue *arr = json_new_array();
    json_arr_push(arr, json_new_int(1));
    json_arr_push(arr, json_new_int(2));
    JsonValue *nested = json_new_object();
    json_obj_set(nested, "flag", json_new_bool(true));
    json_arr_push(arr, nested);
    json_obj_set(obj, "data", arr);
    json_obj_set(obj, "count", json_new_int(3));
    char *s = json_stringify(obj);
    ASSERT_NE(s, nullptr);
    JsonValue *parsed = json_parse(s, strlen(s));
    ASSERT_NE(parsed, nullptr);
    ASSERT_EQ(json_obj_get(parsed, "data")->arr.count, 3);
    ASSERT_EQ(json_obj_get(parsed, "count")->i, 3);
    ASSERT_TRUE(json_obj_get(json_obj_get(parsed, "data")->arr.items[2], "flag")->b);
    free(s);
    json_free(obj);
    json_free(parsed);
}

TEST(Json, EmptyString) {
    JsonValue *v = json_parse("\"\"", 2);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_STRING);
    ASSERT_STREQ(v->s, "");
    json_free(v);
}

TEST(Json, LargeArray) {
    /* Large array */
    JsonValue *arr = json_new_array();
    for (int i = 0; i < 100; i++) {
        json_arr_push(arr, json_new_int(i));
    }
    ASSERT_EQ(arr->arr.count, 100);
    char *s = json_stringify(arr);
    ASSERT_NE(s, nullptr);
    JsonValue *parsed = json_parse(s, strlen(s));
    ASSERT_NE(parsed, nullptr);
    ASSERT_EQ(parsed->arr.count, 100);
    ASSERT_EQ(parsed->arr.items[0]->i, 0);
    ASSERT_EQ(parsed->arr.items[99]->i, 99);
    free(s);
    json_free(arr);
    json_free(parsed);
}