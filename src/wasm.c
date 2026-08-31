/*
 * wasm.c - Pony++ Wasm 代码生成器
 *
 * 生成最小有效的 Wasm 模块 (Phase 1)
 * Wasm 二进制格式: magic (0x00 0x61 0x73 0x6d) + version (0x01 0x00 0x00 0x00)
 */

#include "ponypp/wasm.h"
#include "ponypp/util.h"
#include <errno.h>

struct WasmWriter {
    char *path;
    unsigned char *buf;
    size_t len;
    size_t cap;
};

WasmWriter *wasm_writer_new(const char *output_path) {
    WasmWriter *w = (WasmWriter *)calloc(1, sizeof(WasmWriter));
    if (!w) return NULL;
    w->path = s_strdup(output_path);
    w->cap = 256;
    w->buf = (unsigned char *)malloc(w->cap);
    if (!w->buf) {
        free(w->path);
        free(w);
        return NULL;
    }
    return w;
}

static void wasm_ensure(WasmWriter *w, size_t extra) {
    if (w->len + extra > w->cap) {
        while (w->len + extra > w->cap) {
            w->cap *= 2;
        }
        w->buf = (unsigned char *)realloc(w->buf, w->cap);
    }
}

static void wasm_write_bytes(WasmWriter *w, const unsigned char *data, size_t len) {
    wasm_ensure(w, len);
    memcpy(w->buf + w->len, data, len);
    w->len += len;
}

static void wasm_write_u8(WasmWriter *w, unsigned char b) {
    wasm_write_bytes(w, &b, 1);
}

static void wasm_write_u32(WasmWriter *w, uint32_t v) {
    unsigned char b[4] = {
        (unsigned char)(v & 0xff),
        (unsigned char)((v >> 8) & 0xff),
        (unsigned char)((v >> 16) & 0xff),
        (unsigned char)((v >> 24) & 0xff),
    };
    wasm_write_bytes(w, b, 4);
}

void wasm_writer_close(WasmWriter *w) {
    if (!w) return;
    if (w->path) {
        s_file_write(w->path, (const char *)w->buf, w->len);
    }
    if (w->buf) free(w->buf);
    if (w->path) free(w->path);
    free(w);
}

int wasm_write_program(WasmWriter *w, ASTNode *ast, const CompilerConfig *cfg) {
    (void)ast;
    (void)cfg;
    if (!w) return -1;

    /* Wasm magic number + version */
    unsigned char magic[4] = { 0x00, 0x61, 0x73, 0x6d }; /* \0asm */
    wasm_write_bytes(w, magic, 4);
    wasm_write_u32(w, 1); /* version 1 */

    /* Type section (section 1, empty) */
    wasm_write_u8(w, 0x01); /* type section */
    wasm_write_u8(w, 0x00); /* size 0 */

    /* Function section (section 3, empty) */
    wasm_write_u8(w, 0x03); /* function section */
    wasm_write_u8(w, 0x00); /* size 0 */

    /* Code section (section 10, empty) */
    wasm_write_u8(w, 0x0a); /* code section */
    wasm_write_u8(w, 0x00); /* size 0 */

    return 0;
}
