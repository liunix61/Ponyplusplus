#include <gtest/gtest.h>
#include <ponypp/wasm_gc.h>
#include <cstring>
#include <cstdlib>

/* ==================== 堆管理 ==================== */

TEST(WasmGC, CheneyHeapCreate) {
    WasmGCHeap *h = wgc_heap_new("cheney");
    ASSERT_NE(h, nullptr);
    EXPECT_STREQ(wgc_heap_backend(h), "cheney");
    wgc_heap_free(h);
}

TEST(WasmGC, DefaultHeap) {
    WasmGCHeap *h = wgc_heap_new(nullptr);
    ASSERT_NE(h, nullptr);
    EXPECT_STREQ(wgc_heap_backend(h), "cheney");
    wgc_heap_free(h);
}

TEST(WasmGC, HasWasmBackend) {
    /* Cheney-only 构建时返回 false */
    bool has = wgc_has_wasm_backend();
    (void)has; /* 不断言, 因为取决于编译开关 */
}

/* ==================== structref ==================== */

TEST(WasmGC, StructCreateGetSet) {
    WasmGCHeap *h = wgc_heap_new("cheney");
    ASSERT_NE(h, nullptr);

    WasmGCFieldDesc fields[] = {
        {"count", WGC_FIELD_I32, true},
        {"value", WGC_FIELD_I64, true},
        {"ratio", WGC_FIELD_F64, true},
    };

    WasmGCRef *ref = wgc_struct_new(h, fields, 3);
    ASSERT_NE(ref, nullptr);
    EXPECT_EQ(wgc_ref_type(ref), WGC_REF_STRUCT);
    EXPECT_FALSE(wgc_ref_is_null(ref));

    /* i32 */
    EXPECT_EQ(wgc_struct_set_i32(ref, 0, 42), 0);
    EXPECT_EQ(wgc_struct_get_i32(ref, 0), 42);

    /* i64 */
    EXPECT_EQ(wgc_struct_set_i64(ref, 1, 1234567890LL), 0);
    EXPECT_EQ(wgc_struct_get_i64(ref, 1), 1234567890LL);

    /* f64 */
    EXPECT_EQ(wgc_struct_set_f64(ref, 2, 3.14), 0);
    EXPECT_NEAR(wgc_struct_get_f64(ref, 2), 3.14, 0.001);

    wgc_ref_free(ref);
    wgc_heap_free(h);
}

TEST(WasmGC, StructImmutableField) {
    WasmGCHeap *h = wgc_heap_new("cheney");
    ASSERT_NE(h, nullptr);

    WasmGCFieldDesc fields[] = {
        {"val", WGC_FIELD_I32, false},  /* 不可变 */
    };

    WasmGCRef *ref = wgc_struct_new(h, fields, 1);
    ASSERT_NE(ref, nullptr);

    /* 不可变字段写入应失败 */
    EXPECT_EQ(wgc_struct_set_i32(ref, 0, 42), -2);

    wgc_ref_free(ref);
    wgc_heap_free(h);
}

TEST(WasmGC, StructRefField) {
    WasmGCHeap *h = wgc_heap_new("cheney");
    ASSERT_NE(h, nullptr);

    WasmGCFieldDesc outer_fields[] = {
        {"inner", WGC_FIELD_REF, true},
    };
    WasmGCFieldDesc inner_fields[] = {
        {"data", WGC_FIELD_I32, true},
    };

    WasmGCRef *inner = wgc_struct_new(h, inner_fields, 1);
    WasmGCRef *outer = wgc_struct_new(h, outer_fields, 1);
    ASSERT_NE(inner, nullptr);
    ASSERT_NE(outer, nullptr);

    wgc_struct_set_i32(inner, 0, 99);
    EXPECT_EQ(wgc_struct_set_ref(outer, 0, inner), 0);

    WasmGCRef *got = wgc_struct_get_ref(outer, 0);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(wgc_struct_get_i32(got, 0), 99);

    wgc_ref_free(outer);
    wgc_ref_free(inner);
    wgc_heap_free(h);
}

TEST(WasmGC, StructBoundsCheck) {
    WasmGCHeap *h = wgc_heap_new("cheney");
    ASSERT_NE(h, nullptr);

    WasmGCFieldDesc fields[] = {{"x", WGC_FIELD_I32, true}};
    WasmGCRef *ref = wgc_struct_new(h, fields, 1);
    ASSERT_NE(ref, nullptr);

    /* 越界访问 */
    EXPECT_EQ(wgc_struct_get_i32(ref, 5), 0);
    EXPECT_NE(wgc_struct_set_i32(ref, 5, 1), 0);

    /* NULL 安全 */
    EXPECT_EQ(wgc_struct_get_i32(nullptr, 0), 0);

    wgc_ref_free(ref);
    wgc_heap_free(h);
}

/* ==================== arrayref ==================== */

TEST(WasmGC, ArrayCreateGetSet) {
    WasmGCHeap *h = wgc_heap_new("cheney");
    ASSERT_NE(h, nullptr);

    WasmGCRef *arr = wgc_array_new(h, WGC_FIELD_I32, 8);
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(wgc_ref_type(arr), WGC_REF_ARRAY);
    EXPECT_EQ(wgc_array_length(arr), 8);

    for (uint32_t i = 0; i < 8; i++)
        EXPECT_EQ(wgc_array_set_i32(arr, i, (int32_t)(i * 10)), 0);

    for (uint32_t i = 0; i < 8; i++)
        EXPECT_EQ(wgc_array_get_i32(arr, i), (int32_t)(i * 10));

    wgc_ref_free(arr);
    wgc_heap_free(h);
}

TEST(WasmGC, ArrayBounds) {
    WasmGCHeap *h = wgc_heap_new("cheney");
    ASSERT_NE(h, nullptr);

    WasmGCRef *arr = wgc_array_new(h, WGC_FIELD_I32, 4);
    ASSERT_NE(arr, nullptr);

    EXPECT_EQ(wgc_array_get_i32(arr, 10), 0);
    EXPECT_NE(wgc_array_set_i32(arr, 10, 1), 0);
    EXPECT_EQ(wgc_array_length(nullptr), 0);

    wgc_ref_free(arr);
    wgc_heap_free(h);
}

/* ==================== i31ref ==================== */

TEST(WasmGC, I31CreateGet) {
    WasmGCHeap *h = wgc_heap_new("cheney");
    ASSERT_NE(h, nullptr);

    WasmGCRef *ref = wgc_i31_new(h, 42);
    ASSERT_NE(ref, nullptr);
    EXPECT_EQ(wgc_ref_type(ref), WGC_REF_I31);
    EXPECT_EQ(wgc_i31_get(ref), 42);

    /* 负数 */
    WasmGCRef *neg = wgc_i31_new(h, -100);
    ASSERT_NE(neg, nullptr);
    EXPECT_EQ(wgc_i31_get(neg), -100);

    wgc_ref_free(neg);
    wgc_ref_free(ref);
    wgc_heap_free(h);
}

TEST(WasmGC, I31NullSafety) {
    EXPECT_EQ(wgc_i31_get(nullptr), 0);
}

/* ==================== externref ==================== */

TEST(WasmGC, ExternCreateGet) {
    WasmGCHeap *h = wgc_heap_new("cheney");
    ASSERT_NE(h, nullptr);

    int host_data = 123;
    WasmGCRef *ref = wgc_extern_new(h, &host_data);
    ASSERT_NE(ref, nullptr);
    EXPECT_EQ(wgc_ref_type(ref), WGC_REF_EXTERN);
    EXPECT_FALSE(wgc_ref_is_null(ref));
    EXPECT_EQ(wgc_extern_get(ref), &host_data);

    wgc_ref_free(ref);
    wgc_heap_free(h);
}

TEST(WasmGC, ExternNull) {
    WasmGCHeap *h = wgc_heap_new("cheney");
    ASSERT_NE(h, nullptr);

    WasmGCRef *ref = wgc_extern_new(h, nullptr);
    ASSERT_NE(ref, nullptr);
    EXPECT_TRUE(wgc_ref_is_null(ref));
    EXPECT_EQ(wgc_extern_get(ref), nullptr);

    wgc_ref_free(ref);
    wgc_heap_free(h);
}

/* ==================== 引用操作 ==================== */

TEST(WasmGC, RefNull) {
    EXPECT_TRUE(wgc_ref_is_null(nullptr));
    EXPECT_EQ(wgc_ref_type(nullptr), WGC_REF_NULL);
}

TEST(WasmGC, RefEq) {
    WasmGCHeap *h = wgc_heap_new("cheney");
    ASSERT_NE(h, nullptr);

    WasmGCFieldDesc fields[] = {{"x", WGC_FIELD_I32, true}};
    WasmGCRef *a = wgc_struct_new(h, fields, 1);
    WasmGCRef *b = wgc_struct_new(h, fields, 1);

    EXPECT_TRUE(wgc_ref_eq(a, a));
    EXPECT_FALSE(wgc_ref_eq(a, b));
    EXPECT_FALSE(wgc_ref_eq(a, nullptr));

    wgc_ref_free(a);
    wgc_ref_free(b);
    wgc_heap_free(h);
}

/* ==================== GC 操作 ==================== */

TEST(WasmGC, CollectAndStats) {
    WasmGCHeap *h = wgc_heap_new("cheney");
    ASSERT_NE(h, nullptr);

    WasmGCFieldDesc fields[] = {{"x", WGC_FIELD_I32, true}};

    /* 创建对象并注册为 root */
    WasmGCRef *ref = wgc_struct_new(h, fields, 1);
    ASSERT_NE(ref, nullptr);
    EXPECT_EQ(wgc_add_root(h, ref), 0);

    /* 创建更多对象 (不注册 root, 会被回收) */
    for (int i = 0; i < 10; i++) {
        WasmGCRef *tmp = wgc_struct_new(h, fields, 1);
        if (tmp) wgc_ref_free(tmp);
    }

    WasmGCStats stats;
    wgc_stats(h, &stats);
    EXPECT_GT(stats.total_alloc, 0);
    EXPECT_GT(stats.live_objects, 0);

    /* 回收 */
    wgc_collect(h);

    wgc_stats(h, &stats);
    EXPECT_GE(stats.collections, 1);

    /* root 对象应存活 */
    EXPECT_EQ(wgc_struct_get_i32(ref, 0), 0);

    wgc_remove_root(h, ref);
    wgc_ref_free(ref);
    wgc_heap_free(h);
}

TEST(WasmGC, RootManagement) {
    WasmGCHeap *h = wgc_heap_new("cheney");
    ASSERT_NE(h, nullptr);

    WasmGCFieldDesc fields[] = {{"x", WGC_FIELD_I32, true}};
    WasmGCRef *ref = wgc_struct_new(h, fields, 1);
    ASSERT_NE(ref, nullptr);

    EXPECT_EQ(wgc_add_root(h, ref), 0);
    EXPECT_EQ(wgc_remove_root(h, ref), 0);
    EXPECT_NE(wgc_remove_root(h, ref), 0);  /* 已移除 */

    wgc_ref_free(ref);
    wgc_heap_free(h);
}

/* ==================== Pony++ 能力映射 ==================== */

TEST(WasmGC, CapMapping) {
    /* iso -> structref */
    EXPECT_EQ(wgc_cap_to_ref_type(0x01), WGC_REF_STRUCT);
    /* trn -> structref */
    EXPECT_EQ(wgc_cap_to_ref_type(0x02), WGC_REF_STRUCT);
    /* ref -> structref */
    EXPECT_EQ(wgc_cap_to_ref_type(0x03), WGC_REF_STRUCT);
    /* val -> i31ref */
    EXPECT_EQ(wgc_cap_to_ref_type(0x04), WGC_REF_I31);
    /* box -> externref */
    EXPECT_EQ(wgc_cap_to_ref_type(0x05), WGC_REF_EXTERN);
    /* tag -> externref */
    EXPECT_EQ(wgc_cap_to_ref_type(0x06), WGC_REF_EXTERN);
}

TEST(WasmGC, CapSendable) {
    EXPECT_TRUE(wgc_cap_sendable(0x01));   /* iso */
    EXPECT_FALSE(wgc_cap_sendable(0x02));  /* trn */
    EXPECT_FALSE(wgc_cap_sendable(0x03));  /* ref */
    EXPECT_TRUE(wgc_cap_sendable(0x04));   /* val */
    EXPECT_FALSE(wgc_cap_sendable(0x05));  /* box */
    EXPECT_TRUE(wgc_cap_sendable(0x06));   /* tag */
}

TEST(WasmGC, CapConsume) {
    /* iso 消费后降级为 box */
    EXPECT_EQ(wgc_cap_consume(0x01), 0x05);
    /* val 不变 */
    EXPECT_EQ(wgc_cap_consume(0x04), 0x04);
    /* tag 不变 */
    EXPECT_EQ(wgc_cap_consume(0x06), 0x06);
    /* trn/ref/box 不变 */
    EXPECT_EQ(wgc_cap_consume(0x02), 0x02);
    EXPECT_EQ(wgc_cap_consume(0x03), 0x03);
    EXPECT_EQ(wgc_cap_consume(0x05), 0x05);
}

/* ==================== NULL 安全 ==================== */

TEST(WasmGC, NullSafety) {
    wgc_heap_free(nullptr);
    wgc_collect(nullptr);
    wgc_ref_free(nullptr);

    WasmGCStats stats;
    wgc_stats(nullptr, &stats);

    EXPECT_EQ(wgc_add_root(nullptr, nullptr), -1);
    EXPECT_EQ(wgc_remove_root(nullptr, nullptr), -1);
    EXPECT_STREQ(wgc_heap_backend(nullptr), "null");
}
