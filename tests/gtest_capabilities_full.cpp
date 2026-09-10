/*
 * gtest_capabilities_full.cpp - 完整6种引用能力测试
 *
 * P0修复: 引用能力从3种扩展到6种
 */
#include <gtest/gtest.h>
extern "C" {
#include "ponypp/runtime.h"
}
#include <string.h>

/* ======================== 6种能力定义 ======================== */

TEST(Capabilities, SixCapsDefined) {
    EXPECT_EQ(PNY_CAP_ISO, 0x01);
    EXPECT_EQ(PNY_CAP_TRN, 0x02);
    EXPECT_EQ(PNY_CAP_REF, 0x03);
    EXPECT_EQ(PNY_CAP_VAL, 0x04);
    EXPECT_EQ(PNY_CAP_BOX, 0x05);
    EXPECT_EQ(PNY_CAP_TAG, 0x06);
    EXPECT_EQ(PNY_CAP_ERR, 0xFF);
}

/* ======================== Sendable 判断 ======================== */

TEST(Capabilities, SendableRules) {
    /* 可发送: iso, val, tag */
    EXPECT_TRUE(pny_cap_is_sendable(PNY_CAP_ISO));
    EXPECT_FALSE(pny_cap_is_sendable(PNY_CAP_TRN));
    EXPECT_FALSE(pny_cap_is_sendable(PNY_CAP_REF));
    EXPECT_TRUE(pny_cap_is_sendable(PNY_CAP_VAL));
    EXPECT_FALSE(pny_cap_is_sendable(PNY_CAP_BOX));
    EXPECT_TRUE(pny_cap_is_sendable(PNY_CAP_TAG));
    EXPECT_FALSE(pny_cap_is_sendable(PNY_CAP_ERR));
}

/* ======================== 发送后转换 ======================== */

TEST(Capabilities, AfterSendConversion) {
    /* iso → box (消费语义) */
    EXPECT_EQ(pny_cap_after_send(PNY_CAP_ISO), PNY_CAP_BOX);
    /* val → val (值复制) */
    EXPECT_EQ(pny_cap_after_send(PNY_CAP_VAL), PNY_CAP_VAL);
    /* tag → tag (引用句柄) */
    EXPECT_EQ(pny_cap_after_send(PNY_CAP_TAG), PNY_CAP_TAG);
    /* 不可发送类型 → ERR */
    EXPECT_EQ(pny_cap_after_send(PNY_CAP_TRN), PNY_CAP_ERR);
    EXPECT_EQ(pny_cap_after_send(PNY_CAP_REF), PNY_CAP_ERR);
    EXPECT_EQ(pny_cap_after_send(PNY_CAP_BOX), PNY_CAP_ERR);
}

/* ======================== 能力名称 ======================== */

TEST(Capabilities, CapNames) {
    EXPECT_STREQ(pny_cap_name(PNY_CAP_ISO), "iso");
    EXPECT_STREQ(pny_cap_name(PNY_CAP_TRN), "trn");
    EXPECT_STREQ(pny_cap_name(PNY_CAP_REF), "ref");
    EXPECT_STREQ(pny_cap_name(PNY_CAP_VAL), "val");
    EXPECT_STREQ(pny_cap_name(PNY_CAP_BOX), "box");
    EXPECT_STREQ(pny_cap_name(PNY_CAP_TAG), "tag");
    EXPECT_STREQ(pny_cap_name(PNY_CAP_ERR), "err");
}

/* ======================== 序列化中的能力标记 ======================== */

TEST(Capabilities, SerializationCapMark) {
    PnyMessage *m = pny_msg_new("test", nullptr, 0);
    ASSERT_NE(m, nullptr);
    
    /* 设置不同能力标记并序列化 */
    PnyCapMark caps[] = {PNY_CAP_ISO, PNY_CAP_TRN, PNY_CAP_REF, PNY_CAP_VAL, PNY_CAP_BOX, PNY_CAP_TAG};
    for (int i = 0; i < 6; i++) {
        m->cap_mark = caps[i];
        uint8_t buf[256];
        size_t out_len = 0;
        int len = pny_msg_serialize(m, buf, sizeof(buf), &out_len);
        ASSERT_EQ(len, 0);
        ASSERT_GT(out_len, 0u);
        
        /* 能力标记在序列化后可恢复 */
        PnyMessage *out = nullptr;
        int dret = pny_msg_deserialize(buf, out_len, &out);
        ASSERT_EQ(dret, 0);
        ASSERT_NE(out, nullptr);
        EXPECT_EQ(out->cap_mark, caps[i]) << "cap=" << pny_cap_name(caps[i]);
        pny_msg_free(out);
    }
    
    pny_msg_free(m);
}

/* ======================== CRASHED 状态 ======================== */

TEST(ActorState, CrashedState) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    PnyActor *a = pny_actor_new(r, "crash_test", 0);
    ASSERT_NE(a, nullptr);
    ActorRef *ref = pny_actor_ref(a);
    
    /* 正常状态可接收消息 */
    a->actor_state = ACTOR_STATE_RUNNING;
    EXPECT_EQ(pny_actor_send(nullptr, ref, "m", nullptr, 0), 0);
    
    /* CRASHED 状态不可接收 */
    a->actor_state = ACTOR_STATE_CRASHED;
    EXPECT_EQ(pny_actor_send(nullptr, ref, "m", nullptr, 0), -7);
    
    pny_runtime_free(r);
}

/* ======================== simple_one_for_one 监督策略 ======================== */

TEST(Supervision, SimpleOneForOne) {
    PnyRuntime *r = pny_runtime_new();
    ASSERT_NE(r, nullptr);
    
    /* 创建 supervisor */
    PnyActor *sup = pny_actor_new(r, "supervisor", 0);
    ASSERT_NE(sup, nullptr);
    
    /* 创建多个动态子Actor */
    PnyActor *child1 = pny_actor_new(r, "worker1", 0);
    PnyActor *child2 = pny_actor_new(r, "worker2", 0);
    ASSERT_NE(child1, nullptr);
    ASSERT_NE(child2, nullptr);
    
    ActorRef *c1_ref = pny_actor_ref(child1);
    ActorRef *c2_ref = pny_actor_ref(child2);
    
    /* 注册 simple_one_for_one 监督 */
    pny_supervise_register(sup, c1_ref, SUPERVISE_SIMPLE_ONE_FOR_ONE, 3);
    pny_supervise_register(sup, c2_ref, SUPERVISE_SIMPLE_ONE_FOR_ONE, 3);
    
    /* 模拟 child1 崩溃 */
    child1->actor_state = ACTOR_STATE_CRASHED;
    pny_supervisor_handle_crash(r, c1_ref);
    
    /* child1 应该被标记为 RESTARTING */
    EXPECT_EQ(child1->actor_state, ACTOR_STATE_RESTARTING);
    
    /* child2 不应受影响 (simple_one_for_one 只重启崩溃的那个) */
    EXPECT_NE(child2->actor_state, ACTOR_STATE_RESTARTING);
    
    pny_runtime_free(r);
}
