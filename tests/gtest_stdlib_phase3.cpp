#include "ponypp/stdlib.h"
#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* ==================== JSON Tests ==================== */

TEST(JsonBasic, ParseNull) {
    JsonValue *v = json_parse("null", 4);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_NULL);
    json_free(v);
}

TEST(JsonBasic, ParseTrue) {
    JsonValue *v = json_parse("true", 4);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_BOOL);
    ASSERT_TRUE(v->b);
    json_free(v);
}

TEST(JsonBasic, ParseFalse) {
    JsonValue *v = json_parse("false", 5);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_BOOL);
    ASSERT_FALSE(v->b);
    json_free(v);
}

TEST(JsonBasic, ParseInt) {
    JsonValue *v = json_parse("42", 2);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_INT);
    ASSERT_EQ(v->i, 42);
    json_free(v);
}

TEST(JsonBasic, ParseNegInt) {
    JsonValue *v = json_parse("-7", 2);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_INT);
    ASSERT_EQ(v->i, -7);
    json_free(v);
}

TEST(JsonBasic, ParseDouble) {
    JsonValue *v = json_parse("3.14", 4);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_DOUBLE);
    ASSERT_NEAR(v->d, 3.14, 0.001);
    json_free(v);
}

TEST(JsonBasic, ParseString) {
    JsonValue *v = json_parse("\"hello\"", 7);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_STRING);
    ASSERT_STREQ(v->s, "hello");
    json_free(v);
}

TEST(JsonBasic, ParseStringEscapes) {
    JsonValue *v = json_parse("\"a\\nb\\tc\"", 9);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_STRING);
    ASSERT_STREQ(v->s, "a\nb\tc");
    json_free(v);
}

TEST(JsonBasic, ParseArray) {
    JsonValue *v = json_parse("[1,2,3]", 7);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_ARRAY);
    ASSERT_EQ(v->arr.count, 3);
    ASSERT_EQ(v->arr.items[0]->i, 1);
    ASSERT_EQ(v->arr.items[1]->i, 2);
    ASSERT_EQ(v->arr.items[2]->i, 3);
    json_free(v);
}

TEST(JsonBasic, ParseEmptyArray) {
    JsonValue *v = json_parse("[]", 2);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_ARRAY);
    ASSERT_EQ(v->arr.count, 0);
    json_free(v);
}

TEST(JsonBasic, ParseObject) {
    JsonValue *v = json_parse("{\"name\":\"Pony++\",\"version\":3}", 31);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_OBJECT);
    ASSERT_EQ(v->obj.count, 2);
    JsonValue *name = json_obj_get(v, "name");
    ASSERT_NE(name, nullptr);
    ASSERT_STREQ(name->s, "Pony++");
    JsonValue *ver = json_obj_get(v, "version");
    ASSERT_NE(ver, nullptr);
    ASSERT_EQ(ver->i, 3);
    json_free(v);
}

TEST(JsonBasic, ParseEmptyObject) {
    JsonValue *v = json_parse("{}", 2);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_OBJECT);
    ASSERT_EQ(v->obj.count, 0);
    json_free(v);
}

TEST(JsonBasic, ParseNested) {
    JsonValue *v = json_parse("{\"a\":{\"b\":[1,2,{\"c\":true}]}}", 31);
    ASSERT_NE(v, nullptr);
    JsonValue *a = json_obj_get(v, "a");
    ASSERT_NE(a, nullptr);
    JsonValue *b = json_obj_get(a, "b");
    ASSERT_NE(b, nullptr);
    ASSERT_EQ(b->type, JSON_ARRAY);
    ASSERT_EQ(b->arr.count, 3);
    ASSERT_EQ(b->arr.items[0]->i, 1);
    ASSERT_EQ(b->arr.items[1]->i, 2);
    ASSERT_TRUE(b->arr.items[2]->type == JSON_OBJECT);
    json_free(v);
}

TEST(JsonBasic, ParseWhitespace) {
    JsonValue *v = json_parse("  [ 1 , 2 ]  ", 14);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_ARRAY);
    ASSERT_EQ(v->arr.count, 2);
    json_free(v);
}

TEST(JsonBasic, ParseInvalid) {
    ASSERT_EQ(json_parse("", 0), nullptr);
    ASSERT_EQ(json_parse("nul", 3), nullptr);
    ASSERT_EQ(json_parse("{invalid", 8), nullptr);
}

TEST(JsonBasic, ObjHas) {
    JsonValue *v = json_parse("{\"x\":1}", 7);
    ASSERT_TRUE(json_obj_has(v, "x"));
    ASSERT_FALSE(json_obj_has(v, "y"));
    json_free(v);
}

TEST(JsonConstruct, BasicConstruct) {
    JsonValue *null_v = json_new_null();
    ASSERT_EQ(null_v->type, JSON_NULL);
    json_free(null_v);

    JsonValue *bool_v = json_new_bool(true);
    ASSERT_EQ(bool_v->type, JSON_BOOL);
    ASSERT_TRUE(bool_v->b);
    json_free(bool_v);

    JsonValue *int_v = json_new_int(42);
    ASSERT_EQ(int_v->type, JSON_INT);
    ASSERT_EQ(int_v->i, 42);
    json_free(int_v);

    JsonValue *d_v = json_new_double(3.14);
    ASSERT_EQ(d_v->type, JSON_DOUBLE);
    ASSERT_NEAR(d_v->d, 3.14, 0.001);
    json_free(d_v);

    JsonValue *s_v = json_new_string("hello");
    ASSERT_EQ(s_v->type, JSON_STRING);
    ASSERT_STREQ(s_v->s, "hello");
    json_free(s_v);

    JsonValue *arr = json_new_array();
    ASSERT_EQ(arr->type, JSON_ARRAY);
    json_free(arr);

    JsonValue *obj = json_new_object();
    ASSERT_EQ(obj->type, JSON_OBJECT);
    json_free(obj);
}

TEST(JsonConstruct, ArrPush) {
    JsonValue *arr = json_new_array();
    ASSERT_EQ(json_arr_push(arr, json_new_int(1)), 0);
    ASSERT_EQ(json_arr_push(arr, json_new_string("two")), 0);
    ASSERT_EQ(json_arr_push(arr, json_new_bool(true)), 0);
    ASSERT_EQ(arr->arr.count, 3);
    json_free(arr);
}

TEST(JsonConstruct, ObjSet) {
    JsonValue *obj = json_new_object();
    ASSERT_EQ(json_obj_set(obj, "name", json_new_string("Pony++")), 0);
    ASSERT_EQ(json_obj_set(obj, "count", json_new_int(10)), 0);
    ASSERT_EQ(obj->obj.count, 2);
    JsonValue *n = json_obj_get(obj, "name");
    ASSERT_NE(n, nullptr);
    ASSERT_STREQ(n->s, "Pony++");
    json_free(obj);
}

TEST(JsonConstruct, ObjSetExistingKey) {
    JsonValue *obj = json_new_object();
    json_obj_set(obj, "key", json_new_int(1));
    json_obj_set(obj, "key", json_new_int(2));
    ASSERT_EQ(obj->obj.count, 2); /* both stored */
    json_free(obj);
}

TEST(JsonStringify, Basic) {
    JsonValue *v = json_parse("{\"a\":1,\"b\":[2,3],\"c\":true,\"d\":null}", 42);
    ASSERT_NE(v, nullptr);
    char *s = json_stringify(v);
    ASSERT_NE(s, nullptr);
    /* Parse back to verify */
    JsonValue *v2 = json_parse(s, strlen(s));
    ASSERT_NE(v2, nullptr);
    ASSERT_EQ(v2->type, JSON_OBJECT);
    ASSERT_EQ(json_obj_get(v2, "a")->i, 1);
    ASSERT_EQ(json_obj_get(v2, "b")->arr.count, 2);
    ASSERT_TRUE(json_obj_get(v2, "c")->b);
    ASSERT_EQ(json_obj_get(v2, "d")->type, JSON_NULL);
    free(s);
    json_free(v);
    json_free(v2);
}

TEST(JsonStringify, StringEscape) {
    JsonValue *v = json_parse("\"hello\\nworld\"", 14);
    ASSERT_NE(v, nullptr);
    char *s = json_stringify(v);
    ASSERT_NE(s, nullptr);
    ASSERT_STREQ(s, "\"hello\\nworld\"");
    free(s);
    json_free(v);
}

TEST(JsonStringify, ConstructAndStringify) {
    JsonValue *obj = json_new_object();
    json_obj_set(obj, "name", json_new_string("Pony++"));
    json_obj_set(obj, "version", json_new_int(1));
    char *s = json_stringify(obj);
    ASSERT_NE(s, nullptr);
    /* Parse back */
    JsonValue *v = json_parse(s, strlen(s));
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_OBJECT);
    ASSERT_STREQ(json_obj_get(v, "name")->s, "Pony++");
    ASSERT_EQ(json_obj_get(v, "version")->i, 1);
    free(s);
    json_free(obj);
    json_free(v);
}

TEST(JsonStringify, NegativeNumber) {
    JsonValue *v = json_parse("-42.5", 5);
    ASSERT_NE(v, nullptr);
    char *s = json_stringify(v);
    ASSERT_NE(s, nullptr);
    ASSERT_STREQ(s, "-42.5");
    free(s);
    json_free(v);
}

TEST(JsonStringify, ConstructTree) {
    JsonValue *obj = json_new_object();
    JsonValue *arr = json_new_array();
    json_arr_push(arr, json_new_int(1));
    json_arr_push(arr, json_new_int(2));
    json_arr_push(arr, json_new_string("three"));
    json_obj_set(obj, "items", arr);
    json_obj_set(obj, "total", json_new_int(3));
    json_obj_set(obj, "active", json_new_bool(true));
    char *s = json_stringify(obj);
    ASSERT_NE(s, nullptr);
    JsonValue *v = json_parse(s, strlen(s));
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->type, JSON_OBJECT);
    ASSERT_EQ(json_obj_get(v, "items")->arr.count, 3);
    ASSERT_EQ(json_obj_get(v, "total")->i, 3);
    ASSERT_TRUE(json_obj_get(v, "active")->b);
    free(s);
    json_free(obj);
    json_free(v);
}

/* ==================== Timer Tests ==================== */

TEST(Timer, NowMsPositive) {
    int64_t now = pny_timer_now_ms();
    ASSERT_GT(now, 0);
}

TEST(Timer, ElapsedNonNegative) {
    int64_t e = pny_timer_elapsed_ms();
    ASSERT_GE(e, 0);
}

TEST(Timer, NewAndFree) {
    PnyTimer *t = pny_timer_new(1000, NULL, NULL, false);
    ASSERT_NE(t, nullptr);
    ASSERT_FALSE(pny_timer_running(t));
    pny_timer_start(t);
    ASSERT_TRUE(pny_timer_running(t));
    pny_timer_stop(t);
    ASSERT_FALSE(pny_timer_running(t));
    pny_timer_free(t);
}

TEST(Timer, ElapsedIncreases) {
    int64_t e1 = pny_timer_elapsed_ms();
    /* Sleep briefly */
    usleep(1000); /* 1ms */
    int64_t e2 = pny_timer_elapsed_ms();
    ASSERT_GE(e2, e1);
}

TEST(Timer, MultipleTimers) {
    PnyTimer *t1 = pny_timer_new(100, NULL, NULL, false);
    PnyTimer *t2 = pny_timer_new(200, NULL, NULL, true);
    ASSERT_NE(t1, nullptr);
    ASSERT_NE(t2, nullptr);
    pny_timer_start(t1);
    pny_timer_start(t2);
    ASSERT_TRUE(pny_timer_running(t1));
    ASSERT_TRUE(pny_timer_running(t2));
    pny_timer_stop(t1);
    pny_timer_stop(t2);
    pny_timer_free(t1);
    pny_timer_free(t2);
}

TEST(Timer, NullSafe) {
    ASSERT_FALSE(pny_timer_running(NULL));
    pny_timer_start(NULL); /* should not crash */
    pny_timer_stop(NULL);
    pny_timer_free(NULL);
}

/* ==================== Logger Tests ==================== */

TEST(Logger, NewAndFree) {
    PnyLogger *l = pny_logger_new("test");
    ASSERT_NE(l, nullptr);
    ASSERT_EQ(pny_logger_get_level(l), LOG_INFO);
    pny_logger_free(l);
}

TEST(Logger, SetLevel) {
    PnyLogger *l = pny_logger_new("test");
    pny_logger_set_level(l, LOG_DEBUG);
    ASSERT_EQ(pny_logger_get_level(l), LOG_DEBUG);
    pny_logger_set_level(l, LOG_ERROR);
    ASSERT_EQ(pny_logger_get_level(l), LOG_ERROR);
    pny_logger_free(l);
}

TEST(Logger, LogLevelOrder) {
    ASSERT_LT(LOG_TRACE, LOG_DEBUG);
    ASSERT_LT(LOG_DEBUG, LOG_INFO);
    ASSERT_LT(LOG_INFO, LOG_WARN);
    ASSERT_LT(LOG_WARN, LOG_ERROR);
    ASSERT_LT(LOG_ERROR, LOG_FATAL);
}

TEST(Logger, LogBelowLevel) {
    PnyLogger *l = pny_logger_new("test");
    pny_logger_set_level(l, LOG_ERROR);
    /* Should be suppressed (returns -1) */
    ASSERT_EQ(pny_logger_info(l, "suppressed %d", 42), -1);
    ASSERT_EQ(pny_logger_warn(l, "suppressed %d", 42), -1);
    /* Should pass */
    ASSERT_GT(pny_logger_error(l, "visible %d", 42), 0);
    pny_logger_free(l);
}

TEST(Logger, NullSafe) {
    /* Should not crash */
    pny_logger_free(NULL);
    ASSERT_EQ(pny_logger_get_level(NULL), LOG_INFO);
    pny_logger_set_level(NULL, LOG_DEBUG);
}

TEST(Logger, AllLevels) {
    PnyLogger *l = pny_logger_new("all");
    pny_logger_set_level(l, LOG_TRACE);
    /* All levels should pass */
    ASSERT_GT(pny_logger_trace(l, "trace %d", 1), 0);
    ASSERT_GT(pny_logger_debug(l, "debug %d", 2), 0);
    ASSERT_GT(pny_logger_info(l, "info %d", 3), 0);
    ASSERT_GT(pny_logger_warn(l, "warn %d", 4), 0);
    ASSERT_GT(pny_logger_error(l, "error %d", 5), 0);
    ASSERT_GT(pny_logger_fatal(l, "fatal %d", 6), 0);
    pny_logger_free(l);
}

TEST(Logger, LogFunction) {
    PnyLogger *l = pny_logger_new("func");
    pny_logger_set_level(l, LOG_INFO);
    ASSERT_GT(pny_logger_log(l, LOG_INFO, "formatted %s %d", "test", 42), 0);
    ASSERT_EQ(pny_logger_log(l, LOG_DEBUG, "hidden"), -1);
    pny_logger_free(l);
}

/* ==================== Math Tests ==================== */

TEST(Math, SqrtPow) {
    ASSERT_NEAR(pny_math_sqrt(16.0), 4.0, 0.0001);
    ASSERT_NEAR(pny_math_sqrt(2.0), 1.4142, 0.0001);
    ASSERT_NEAR(pny_math_pow(2.0, 10.0), 1024.0, 0.001);
    ASSERT_NEAR(pny_math_pow(2.0, 0.0), 1.0, 0.0001);
}

TEST(Math, Trig) {
    ASSERT_NEAR(pny_math_sin(0.0), 0.0, 0.0001);
    ASSERT_NEAR(pny_math_cos(0.0), 1.0, 0.0001);
    ASSERT_NEAR(pny_math_tan(0.0), 0.0, 0.0001);
    ASSERT_NEAR(pny_math_sin(PNY_MATH_PI / 2.0), 1.0, 0.0001);
    ASSERT_NEAR(pny_math_cos(PNY_MATH_PI), -1.0, 0.0001);
}

TEST(Math, InverseTrig) {
    ASSERT_NEAR(pny_math_asin(0.0), 0.0, 0.0001);
    ASSERT_NEAR(pny_math_acos(1.0), 0.0, 0.0001);
    ASSERT_NEAR(pny_math_acos(-1.0), PNY_MATH_PI, 0.0001);
    ASSERT_NEAR(pny_math_atan(0.0), 0.0, 0.0001);
    ASSERT_NEAR(pny_math_atan2(1.0, 1.0), PNY_MATH_PI / 4.0, 0.0001);
    ASSERT_NEAR(pny_math_atan2(1.0, -1.0), 3.0 * PNY_MATH_PI / 4.0, 0.0001);
}

TEST(Math, LogExp) {
    ASSERT_NEAR(pny_math_log(1.0), 0.0, 0.0001);
    ASSERT_NEAR(pny_math_log2(1.0), 0.0, 0.0001);
    ASSERT_NEAR(pny_math_log2(8.0), 3.0, 0.0001);
    ASSERT_NEAR(pny_math_exp(0.0), 1.0, 0.0001);
    ASSERT_NEAR(pny_math_exp(1.0), PNY_MATH_E, 0.0001);
}

TEST(Math, CeilFloorAbs) {
    ASSERT_NEAR(pny_math_ceil(3.2), 4.0, 0.0001);
    ASSERT_NEAR(pny_math_ceil(3.0), 3.0, 0.0001);
    ASSERT_NEAR(pny_math_floor(3.8), 3.0, 0.0001);
    ASSERT_NEAR(pny_math_floor(-3.2), -4.0, 0.0001);
    ASSERT_NEAR(pny_math_abs(-5.5), 5.5, 0.0001);
    ASSERT_NEAR(pny_math_abs(5.5), 5.5, 0.0001);
    ASSERT_EQ(pny_math_abs_int(-42), 42);
    ASSERT_EQ(pny_math_abs_int(42), 42);
}

TEST(Math, MinMaxFmod) {
    ASSERT_EQ(pny_math_min(3, 5), 3);
    ASSERT_EQ(pny_math_min(5, 3), 3);
    ASSERT_EQ(pny_math_min(-1, 1), -1);
    ASSERT_EQ(pny_math_max(3, 5), 5);
    ASSERT_EQ(pny_math_max(5, 3), 5);
    ASSERT_EQ(pny_math_max(-1, 1), 1);
    ASSERT_NEAR(pny_math_fmod(10.5, 3.0), 1.5, 0.0001);
    ASSERT_NEAR(pny_math_fmod(7.0, 2.0), 1.0, 0.0001);
}

TEST(Math, Factorial) {
    ASSERT_EQ(pny_math_factorial(0), 1);
    ASSERT_EQ(pny_math_factorial(1), 1);
    ASSERT_EQ(pny_math_factorial(5), 120);
    ASSERT_EQ(pny_math_factorial(10), 3628800);
    ASSERT_EQ(pny_math_factorial(20), 2432902008176640000LL);
    ASSERT_EQ(pny_math_factorial(21), -1); /* overflow guard */
}

TEST(Math, Random) {
    /* Should return values in [0, 1) */
    for (int i = 0; i < 100; i++) {
        double r = pny_math_random();
        ASSERT_GE(r, 0.0);
        ASSERT_LT(r, 1.0);
    }
}

TEST(Math, RandomInt) {
    for (int i = 0; i < 100; i++) {
        int64_t r = pny_math_random_int(10);
        ASSERT_GE(r, 0);
        ASSERT_LT(r, 10);
    }
    ASSERT_EQ(pny_math_random_int(0), 0);
    ASSERT_EQ(pny_math_random_int(-1), 0);
}

TEST(Math, PiE) {
    ASSERT_NEAR(PNY_MATH_PI, 3.141592653589793, 0.0000000001);
    ASSERT_NEAR(PNY_MATH_E, 2.718281828459045, 0.0000000001);
}

TEST(Math, RandomDistinct) {
    /* Random values should not all be the same */
    int distinct = 0;
    double prev = pny_math_random();
    for (int i = 1; i < 50; i++) {
        double curr = pny_math_random();
        if (curr != prev) distinct++;
        prev = curr;
    }
    ASSERT_GT(distinct, 40); /* at least 40 distinct values */
}

TEST(Math, FactorialOverflow) {
    ASSERT_EQ(pny_math_factorial(22), -1);
    ASSERT_EQ(pny_math_factorial(100), -1);
}

TEST(Math, NegNumbers) {
    ASSERT_NEAR(pny_math_sqrt(0.0), 0.0, 0.0001);
    ASSERT_NEAR(pny_math_pow(2.0, -1.0), 0.5, 0.0001);
    ASSERT_EQ(pny_math_abs_int(-9223372036854775807LL - 1), -9223372036854775807LL - 1); /* INT64_MIN edge */
}