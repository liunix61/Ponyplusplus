#include <gtest/gtest.h>
#include <ponypp/wamr.h>
#include <cstring>
#include <cstdlib>

/* 针对性覆盖 src/ponypp/wamr.c wasm_exec_func (261-555):
 * 手工构造最小 wasm 二进制, 通过 wamr_call_func 驱动解释器。
 *
 * wasm 模块格式:
 *   magic(4) + version(4)
 *   type section (id=1): () -> i32
 *   function section (id=3): 1 func
 *   export section (id=7): "main" func 0
 *   code section (id=10): body
 */

static WamrModule *make_mod(unsigned char *wasm, size_t sz) {
    WamrModule *mod = (WamrModule *)calloc(1, sizeof(WamrModule));
    mod->name = (char *)"t";
    mod->data = wasm;
    mod->size = sz;
    mod->mcu_type = MCU_GENERIC;
    return mod;
}

/* 最小模块: header + type + func + export + code section(任意 body) */
static size_t build_wasm(unsigned char *out, const unsigned char *body, size_t body_len) {
    size_t pos = 0;
    /* magic + version */
    unsigned char hdr[] = { 0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00 };
    memcpy(out + pos, hdr, 8); pos += 8;
    /* type section: id=1, size=5, count=1, ()->i32 */
    unsigned char type_sec[] = { 0x01, 0x05, 0x01, 0x60, 0x00, 0x01, 0x7f };
    memcpy(out + pos, type_sec, sizeof(type_sec)); pos += sizeof(type_sec);
    /* function section: id=3, size=2, count=1, type=0 */
    unsigned char func_sec[] = { 0x03, 0x02, 0x01, 0x00 };
    memcpy(out + pos, func_sec, sizeof(func_sec)); pos += sizeof(func_sec);
    /* export section: id=7, size=8, count=1, "main" func 0 */
    unsigned char exp_sec[] = { 0x07, 0x08, 0x01, 0x04, 'm','a','i','n', 0x00, 0x00 };
    memcpy(out + pos, exp_sec, sizeof(exp_sec)); pos += sizeof(exp_sec);
    /* code section: id=10, size=body_len+3, count=1, bodysize=body_len+1(含locals), locals=0 */
    out[pos++] = 0x0a;
    out[pos++] = (unsigned char)(body_len + 3);
    out[pos++] = 0x01;                    /* count */
    out[pos++] = (unsigned char)(body_len + 1); /* body size */
    out[pos++] = 0x00;                    /* locals count */
    memcpy(out + pos, body, body_len); pos += body_len;
    return pos;
}

static int run_wasm(const unsigned char *body, size_t body_len, void **result) {
    unsigned char wasm[256];
    size_t sz = build_wasm(wasm, body, body_len);
    WamrModule *mod = make_mod(wasm, sz);
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrInstance *inst = NULL;
    if (wamr_instance_create(mod, &cfg, &inst) != 0) { free(mod); return -1; }
    int rc = 0;
    int result_count = 0;
    rc = wamr_call_func(inst, "main", NULL, 0, result, &result_count);
    wamr_instance_free(inst);
    free(mod);
    return rc;
}

/* ==================== 基本指令 ==================== */

TEST(WamrCov4, I32ConstReturn) {
    /* i32.const 42; end */
    unsigned char body[] = { 0x41, 0x2a, 0x0b };
    void *result = NULL;
    EXPECT_EQ(run_wasm(body, sizeof(body), &result), 0);
}

TEST(WamrCov4, NopThenConst) {
    /* nop; i32.const 7; end */
    unsigned char body[] = { 0x01, 0x41, 0x07, 0x0b };
    void *result = NULL;
    EXPECT_EQ(run_wasm(body, sizeof(body), &result), 0);
}

TEST(WamrCov4, I64Const) {
    /* i64.const 100; drop? no drop — end */
    unsigned char body[] = { 0x42, 0x64, 0x0b };
    void *result = NULL;
    EXPECT_EQ(run_wasm(body, sizeof(body), &result), 0);
}

/* ==================== 局部变量 ==================== */

/* 带 locals 的模块需要自定义 build */
static size_t build_wasm_locals(unsigned char *out, const unsigned char *body, size_t body_len) {
    size_t pos = 0;
    unsigned char hdr[] = { 0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00 };
    memcpy(out + pos, hdr, 8); pos += 8;
    unsigned char type_sec[] = { 0x01, 0x05, 0x01, 0x60, 0x00, 0x01, 0x7f };
    memcpy(out + pos, type_sec, sizeof(type_sec)); pos += sizeof(type_sec);
    unsigned char func_sec[] = { 0x03, 0x02, 0x01, 0x00 };
    memcpy(out + pos, func_sec, sizeof(func_sec)); pos += sizeof(func_sec);
    unsigned char exp_sec[] = { 0x07, 0x08, 0x01, 0x04, 'm','a','i','n', 0x00, 0x00 };
    memcpy(out + pos, exp_sec, sizeof(exp_sec)); pos += sizeof(exp_sec);
    /* body 内容包含 locals 声明, 直接由调用方给出完整 body */
    out[pos++] = 0x0a;
    out[pos++] = (unsigned char)(body_len + 2);
    out[pos++] = 0x01;
    out[pos++] = (unsigned char)body_len;
    memcpy(out + pos, body, body_len); pos += body_len;
    return pos;
}

static int run_wasm_raw(const unsigned char *body, size_t body_len, void **result) {
    unsigned char wasm[256];
    size_t sz = build_wasm_locals(wasm, body, body_len);
    WamrModule *mod = make_mod(wasm, sz);
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrInstance *inst = NULL;
    if (wamr_instance_create(mod, &cfg, &inst) != 0) { free(mod); return -1; }
    int rc = wamr_call_func(inst, "main", NULL, 0, result, NULL);
    wamr_instance_free(inst);
    free(mod);
    return rc;
}

TEST(WamrCov4, LocalGetSet) {
    /* locals: 1 xi32; i32.const 5; local.set 0; local.get 0; end */
    unsigned char body[] = {
        0x01, 0x01, 0x7f,        /* 1 local group: 1 x i32 */
        0x41, 0x05,              /* i32.const 5 */
        0x21, 0x00,              /* local.set 0 */
        0x20, 0x00,              /* local.get 0 */
        0x0b                     /* end */
    };
    void *result = NULL;
    EXPECT_EQ(run_wasm_raw(body, sizeof(body), &result), 0);
}

TEST(WamrCov4, LocalTee) {
    /* locals: 1 xi32; i32.const 9; local.tee 0; end */
    unsigned char body[] = {
        0x01, 0x01, 0x7f,
        0x41, 0x09,
        0x22, 0x00,              /* local.tee 0 */
        0x0b
    };
    void *result = NULL;
    EXPECT_EQ(run_wasm_raw(body, sizeof(body), &result), 0);
}

/* ==================== 算术/比较 ==================== */

TEST(WamrCov4, I32Add) {
    /* i32.const 3; i32.const 4; i32.add; end */
    unsigned char body[] = { 0x41, 0x03, 0x41, 0x04, 0x6a, 0x0b };
    void *result = NULL;
    EXPECT_EQ(run_wasm(body, sizeof(body), &result), 0);
}

TEST(WamrCov4, I32CmpOps) {
    /* i32.const 1; i32.const 2; i32.lt_s; end */
    unsigned char body[] = { 0x41, 0x01, 0x41, 0x02, 0x48, 0x0b };
    void *result = NULL;
    EXPECT_EQ(run_wasm(body, sizeof(body), &result), 0);
}

TEST(WamrCov4, I32Eqz) {
    /* i32.const 0; i32.eqz; end */
    unsigned char body[] = { 0x41, 0x00, 0x45, 0x0b };
    void *result = NULL;
    EXPECT_EQ(run_wasm(body, sizeof(body), &result), 0);
}

/* ==================== 控制流 ==================== */

TEST(WamrCov4, IfElse) {
    /* i32.const 1; if (empty) end; i32.const 0; end */
    unsigned char body[] = { 0x41, 0x01, 0x04, 0x40, 0x0b, 0x41, 0x00, 0x0b };
    void *result = NULL;
    EXPECT_EQ(run_wasm(body, sizeof(body), &result), 0);
}

TEST(WamrCov4, Block) {
    /* block; end; i32.const 1; end */
    unsigned char body[] = { 0x02, 0x40, 0x0b, 0x41, 0x01, 0x0b };
    void *result = NULL;
    EXPECT_EQ(run_wasm(body, sizeof(body), &result), 0);
}

TEST(WamrCov4, LoopBrIf) {
    /* loop; i32.const 0; br_if 0 (不跳); end; i32.const 1; end */
    unsigned char body[] = { 0x03, 0x40, 0x41, 0x00, 0x0d, 0x00, 0x0b, 0x41, 0x01, 0x0b };
    void *result = NULL;
    EXPECT_EQ(run_wasm(body, sizeof(body), &result), 0);
}

TEST(WamrCov4, ReturnEarly) {
    /* i32.const 42; return; end */
    unsigned char body[] = { 0x41, 0x2a, 0x0f, 0x0b };
    void *result = NULL;
    EXPECT_EQ(run_wasm(body, sizeof(body), &result), 0);
}

TEST(WamrCov4, Unreachable) {
    /* unreachable — 解释器应安全退出 */
    unsigned char body[] = { 0x00 };
    void *result = NULL;
    run_wasm(body, sizeof(body), &result); /* 不检查返回值, 只要不 crash */
    SUCCEED();
}

/* ==================== 内存访问 ==================== */

TEST(WamrCov4, LoadStore) {
    /* i32.const 0; i32.const 99; i32.store; i32.const 0; i32.load; end */
    unsigned char body[] = {
        0x41, 0x00,              /* addr */
        0x41, 0x63,              /* value 99 */
        0x36, 0x02, 0x00,        /* i32.store align=2 offset=0 */
        0x41, 0x00,              /* addr */
        0x28, 0x02, 0x00,        /* i32.load */
        0x0b
    };
    void *result = NULL;
    EXPECT_EQ(run_wasm(body, sizeof(body), &result), 0);
}

TEST(WamrCov4, Store8Load8U) {
    unsigned char body[] = {
        0x41, 0x10,              /* addr 16 */
        0x41, 0x7f,              /* value 127 */
        0x3a, 0x00, 0x00,        /* i32.store8 */
        0x41, 0x10,
        0x2d, 0x00, 0x00,        /* i32.load8_u */
        0x0b
    };
    void *result = NULL;
    EXPECT_EQ(run_wasm(body, sizeof(body), &result), 0);
}

/* ==================== 错误路径 ==================== */

TEST(WamrCov4, CallNonexistentFunc) {
    unsigned char wasm[256];
    unsigned char body[] = { 0x41, 0x00, 0x0b };
    size_t sz = build_wasm(wasm, body, sizeof(body));
    WamrModule *mod = make_mod(wasm, sz);
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrInstance *inst = NULL;
    ASSERT_EQ(wamr_instance_create(mod, &cfg, &inst), 0);
    void *result = NULL;
    int rc = wamr_call_func(inst, "nonexistent", NULL, 0, &result, NULL);
    (void)rc; /* 找不到函数: 返回非0或0均可, 不能 crash */
    wamr_instance_free(inst);
    free(mod);
    SUCCEED();
}

TEST(WamrCov4, TruncatedWasm) {
    /* 太小的模块 */
    unsigned char tiny[] = { 0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00 };
    WamrModule *mod = make_mod(tiny, sizeof(tiny));
    WamrConfig cfg = wamr_config_default(MCU_GENERIC);
    WamrInstance *inst = NULL;
    ASSERT_EQ(wamr_instance_create(mod, &cfg, &inst), 0);
    void *result = NULL;
    wamr_call_func(inst, "main", NULL, 0, &result, NULL);
    wamr_instance_free(inst);
    free(mod);
    SUCCEED();
}
