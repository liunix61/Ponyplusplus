/*
 * wasm.c - Pony++ Wasm 代码生成器（Phase 1）
 *
 * 生成最小可执行 Wasm 模块：
 *   - 魔术字 + 版本号
 *   - type section:  (func (result i32)) 用于 main
 *   - function section
 *   - export section: 导出 "main"
 *   - code section:   main() { return 42; }
 */

#include "ponypp/wasm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 字节向量 */
typedef struct {
    unsigned char *data;
    size_t size;
    size_t cap;
} ByteVec;

static void bv_grow(ByteVec *bv, size_t need) {
    while (bv->cap < need) bv->cap = bv->cap ? bv->cap * 2 : 256;
    bv->data = (unsigned char *)realloc(bv->data, bv->cap);
}
static void bv_write_u8(ByteVec *bv, unsigned char v) {
    if (bv->size + 1 > bv->cap) bv_grow(bv, bv->size + 1);
    bv->data[bv->size++] = v;
}
static void bv_write_i32_le(ByteVec *bv, uint32_t v) {
    if (bv->size + 4 > bv->cap) bv_grow(bv, bv->size + 4);
    bv->data[bv->size++] = v & 0xFF;
    bv->data[bv->size++] = (v >> 8) & 0xFF;
    bv->data[bv->size++] = (v >> 16) & 0xFF;
    bv->data[bv->size++] = (v >> 24) & 0xFF;
}
static void bv_write_u32_leb128(ByteVec *bv, uint32_t v) {
    do {
        unsigned char byte = v & 0x7F;
        v >>= 7;
        if (v) byte |= 0x80;
        bv_write_u8(bv, byte);
    } while (v);
}
static void bv_write_str(ByteVec *bv, const char *s) {
    size_t len = s ? strlen(s) : 0;
    bv_write_u32_leb128(bv, (uint32_t)len);
    if (len && bv->size + len > bv->cap) bv_grow(bv, bv->size + len);
    memcpy(bv->data + bv->size, s, len);
    bv->size += len;
}
static void bv_write_vec(ByteVec *bv, const ByteVec *inner) {
    bv_write_u32_leb128(bv, (uint32_t)inner->size);
    if (inner->size && bv->size + inner->size > bv->cap) bv_grow(bv, bv->size + inner->size);
    memcpy(bv->data + bv->size, inner->data, inner->size);
    bv->size += inner->size;
}

int wasm_write_program(ASTNode *ast, const char *output) {
    (void)ast;
    if (!output) return 1;

    ByteVec bv = {0};

    /* 魔术字 + 版本 */
    bv_write_u8(&bv, 0x00); bv_write_u8(&bv, 0x61);
    bv_write_u8(&bv, 0x73); bv_write_u8(&bv, 0x6d);
    bv_write_u8(&bv, 0x01); bv_write_u8(&bv, 0x00);
    bv_write_u8(&bv, 0x00); bv_write_u8(&bv, 0x00);

    /* type section: (func (result i32)) */
    bv_write_u8(&bv, 0x01); /* section id */
    {
        ByteVec body = {0};
        bv_write_u8(&body, 0x01); /* 1 type */
        bv_write_u8(&body, 0x60); /* func type */
        bv_write_u8(&body, 0x00); /* no params */
        bv_write_u8(&body, 0x01); /* 1 result */
        bv_write_u8(&body, 0x7F); /* i32 */
        bv_write_u32_leb128(&bv, (uint32_t)body.size);
        bv_write_vec(&bv, &body);
        free(body.data);
    }

    /* function section: func 0 uses type 0 */
    bv_write_u8(&bv, 0x03);
    {
        ByteVec body = {0};
        bv_write_u8(&body, 0x01); /* 1 func */
        bv_write_u8(&body, 0x00); /* type idx 0 */
        bv_write_u32_leb128(&bv, (uint32_t)body.size);
        bv_write_vec(&bv, &body);
        free(body.data);
    }

    /* table section */
    bv_write_u8(&bv, 0x04);
    bv_write_u8(&bv, 0x04);
    bv_write_u8(&bv, 0x70);
    bv_write_u8(&bv, 0x01);
    bv_write_u8(&bv, 0x00);

    /* memory section */
    bv_write_u8(&bv, 0x05);
    bv_write_u8(&bv, 0x03);
    bv_write_u8(&bv, 0x01);
    bv_write_u8(&bv, 0x00);
    bv_write_u8(&bv, 0x00);

    /* export section: export "main" func 0 */
    bv_write_u8(&bv, 0x07);
    {
        ByteVec body = {0};
        bv_write_u8(&body, 0x01); /* 1 export */
        bv_write_str(&body, "main");
        bv_write_u8(&body, 0x00); /* func */
        bv_write_u8(&body, 0x00); /* idx 0 */
        bv_write_u32_leb128(&bv, (uint32_t)body.size);
        bv_write_vec(&bv, &body);
        free(body.data);
    }

    /* code section: (func (i32.const 42) (end)) */
    bv_write_u8(&bv, 0x0a);
    {
        ByteVec body = {0};
        bv_write_u8(&body, 0x01); /* 1 func */
        {
            ByteVec code = {0};
            bv_write_u8(&code, 0x41); /* i32.const */
            bv_write_u8(&code, 0x2A); /* 42 */
            bv_write_u8(&code, 0x0B); /* end */
            bv_write_u32_leb128(&body, (uint32_t)code.size);
            bv_write_vec(&body, &code);
            free(code.data);
        }
        bv_write_u32_leb128(&bv, (uint32_t)body.size);
        bv_write_vec(&bv, &body);
        free(body.data);
    }

    /* Write to file */
    FILE *f = fopen(output, "wb");
    if (!f) return 1;
    if (fwrite(bv.data, 1, bv.size, f) != bv.size) { fclose(f); free(bv.data); return 1; }
    fclose(f);

    free(bv.data);
    return 0;
}

const char *wasm_target_name(TargetKind target) {
    switch (target) {
        case TARGET_WASI_P2:   return "wasi-p2";
        case TARGET_COMPONENT: return "component";
        case TARGET_BROWSER:   return "browser";
        case TARGET_MCU_WASM:  return "mcu-wasm";
        case TARGET_NATIVE:    return "native";
        default: return "unknown";
    }
}