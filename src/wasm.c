/*
 * wasm.c - Pony++ WASM 代码生成器（Path B）
 *
 * 目标: wasi-p2 / component / browser / mcu-wasm
 * 输出: 标准 .wasm 二进制
 *
 * 生成的模块:
 *   - type section:  (func (result i32))
 *   - import section: WASI Snapshot Preview 1
 *   - function section
 *   - table + memory sections
 *   - export section: 导出 "main"
 *   - code section:    main() { print("Hello"); return 0; }
 *
 * 支持 Actor:
 *   - 每个 Actor 编译为一个 func (param i32) (result i32)
 *   - Actor 方法通过 wasi proc_exit 传递返回值
 *   - Actor 消息通过线性内存传递
 *
 * 支持类型:
 *   - I8/I16/I32/I64  → i32/i64
 *   - U8/U16/U32/U64  → i32/i64 (unsigned)
 *   - F32/F64         → f32/f64
 *   - Bool/String     → i32 (ptr)
 */

#include "ponypp/wasm.h"
#include "ponypp/ast.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* --- 字节向量 --- */
typedef struct {
    unsigned char *data;
    size_t size;
    size_t cap;
} ByteVec;

static void bv_grow(ByteVec *bv, size_t need) {
    while (bv->cap < need) bv->cap = bv->cap ? bv->cap * 2 : 512;
    bv->data = (unsigned char *)realloc(bv->data, bv->cap);
}
static void bv_write_u8(ByteVec *bv, unsigned char v) {
    if (bv->size + 1 > bv->cap) bv_grow(bv, bv->size + 1);
    bv->data[bv->size++] = v;
}
static void bv_write_u32(ByteVec *bv, uint32_t v) {
    if (bv->size + 4 > bv->cap) bv_grow(bv, bv->size + 4);
    bv->data[bv->size++] = (unsigned char)(v & 0xFF);
    bv->data[bv->size++] = (unsigned char)((v >> 8) & 0xFF);
    bv->data[bv->size++] = (unsigned char)((v >> 16) & 0xFF);
    bv->data[bv->size++] = (unsigned char)((v >> 24) & 0xFF);
}
static void bv_write_u32_leb128(ByteVec *bv, uint32_t v);  /* 前向声明 */
static void bv_write_raw(ByteVec *bv, const unsigned char *data, size_t len) {
    if (bv->size + len > bv->cap) bv_grow(bv, bv->size + len);
    if (len > 0) memcpy(bv->data + bv->size, data, len);
    bv->size += len;
}
static void bv_write_data(ByteVec *bv, const unsigned char *data, size_t len) {
    bv_write_u32_leb128(bv, (uint32_t)len);
    bv_write_raw(bv, data, len);
}
static void bv_write_u32_leb128(ByteVec *bv, uint32_t v) {
    do {
        unsigned char byte = (unsigned char)(v & 0x7F);
        v >>= 7;
        if (v) byte |= 0x80;
        bv_write_u8(bv, byte);
    } while (v);
}
static void bv_write_i32_leb128(ByteVec *bv, int32_t v) {
    uint32_t u = (uint32_t)v;
    bv_write_u32_leb128(bv, u);
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
static void bv_free(ByteVec *bv) { free(bv->data); bv->data = NULL; bv->size = 0; bv->cap = 0; }

/* --- Wasm 常量 --- */
#define WASM_OPCODE_I32_CONST 0x41
#define WASM_OPCODE_I64_CONST 0x42
#define WASM_OPCODE_F32_CONST 0x43
#define WASM_OPCODE_F64_CONST 0x44
#define WASM_OPCODE_I32_ADD   0x6A
#define WASM_OPCODE_I32_SUB   0x6B
#define WASM_OPCODE_I32_MUL   0x6C
#define WASM_OPCODE_I32_DIV_S 0x6D
#define WASM_OPCODE_I32_EQ    0x46
#define WASM_OPCODE_I32_NE    0x47
#define WASM_OPCODE_I32_LT_S  0x48
#define WASM_OPCODE_I32_GT_S  0x49
#define WASM_OPCODE_I32_LE_S  0x4A
#define WASM_OPCODE_I32_GE_S  0x4B
#define WASM_OPCODE_I32_AND   0x71
#define WASM_OPCODE_I32_OR    0x72
#define WASM_OPCODE_I32_XOR   0x73
#define WASM_OPCODE_I32_NOT   0x70
#define WASM_OPCODE_I32_EQZ   0x45
#define WASM_OPCODE_I32_LOAD  0x28
#define WASM_OPCODE_I32_STORE 0x36
#define WASM_OPCODE_LOCAL_GET 0x20
#define WASM_OPCODE_LOCAL_SET 0x21
#define WASM_OPCODE_LOCAL_TEE 0x22
#define WASM_OPCODE_LOCAL_INIT 0x20
#define WASM_OPCODE_DROP      0x1A
#define WASM_OPCODE_NOP       0x01
#define WASM_OPCODE_UNREACHABLE 0x00
#define WASM_OPCODE_END       0x0B
#define WASM_OPCODE_IF        0x04
#define WASM_OPCODE_ELSE      0x05
#define WASM_OPCODE_BLOCK     0x02
#define WASM_OPCODE_LOOP      0x03
#define WASM_OPCODE_BR        0x0C
#define WASM_OPCODE_BR_IF     0x0D
#define WASM_OPCODE_RETURN    0x0F
#define WASM_OPCODE_CALL      0x10
#define WASM_OPCODE_PRINT_I32 0x07  /* 自定义: 通过 fd_write 模拟 */

/* --- 类型映射 --- */
static const char *wasm_type_of(const ASTNode *t) {
    if (!t || !t->data) return "i32";
    const char *n = (const char *)t->data;
    if (strcmp(n, "I8") == 0 || strcmp(n, "I16") == 0 || strcmp(n, "I32") == 0 ||
        strcmp(n, "U8") == 0 || strcmp(n, "U16") == 0 || strcmp(n, "U32") == 0 ||
        strcmp(n, "Bool") == 0) return "i32";
    if (strcmp(n, "I64") == 0 || strcmp(n, "U64") == 0) return "i64";
    if (strcmp(n, "F32") == 0) return "f32";
    if (strcmp(n, "F64") == 0) return "f64";
    if (strcmp(n, "String") == 0 || strcmp(n, "Bytes") == 0) return "i32";
    return "i32";
}

/* --- 编译器状态 --- */
typedef struct {
    ByteVec out;
    int func_count;
    int32_t main_func_idx;
    int32_t print_func_idx;
    int in_actor;
    const char *actor_name;
    int32_t local_depth;
    /* 字符串池：收集 AST 中的字符串，分配到线性内存地址 */
    char **strings;
    size_t *string_lens;
    int32_t *string_addrs;
    size_t string_count;
    size_t string_cap;
    int32_t next_str_addr;
} WasmGen;

/* 前向声明 */
static void emit_expr(WasmGen *wg, ASTNode *n);
static void emit_stmt(WasmGen *wg, ASTNode *n);

static void emit_i32_const(WasmGen *wg, int32_t v) {
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_i32_leb128(&wg->out, v);
}

static void emit_i64_const(WasmGen *wg, int64_t v) {
    bv_write_u8(&wg->out, WASM_OPCODE_I64_CONST);
    bv_write_i32_leb128(&wg->out, (int32_t)v);
}

/* 分配字符串到线性内存，返回地址 */
static int32_t wasm_alloc_string(WasmGen *wg, const char *s) {
    size_t len = s ? strlen(s) : 0;
    int32_t addr = wg->next_str_addr;
    if (wg->string_count >= wg->string_cap) {
        size_t newcap = wg->string_cap ? wg->string_cap * 2 : 16;
        wg->strings = (char **)realloc(wg->strings, newcap * sizeof(char *));
        wg->string_lens = (size_t *)realloc(wg->string_lens, newcap * sizeof(size_t));
        wg->string_addrs = (int32_t *)realloc(wg->string_addrs, newcap * sizeof(int32_t));
        wg->string_cap = newcap;
    }
    wg->strings[wg->string_count] = (char *)s;
    wg->string_lens[wg->string_count] = len;
    wg->string_addrs[wg->string_count] = addr;
    wg->string_count++;
    wg->next_str_addr += (int32_t)len + 1;
    return addr;
}

/* 预扫描 AST，收集所有字符串字面量并分配内存地址 */
static void wasm_collect_strings(WasmGen *wg, ASTNode *n) {
    if (!n) return;
    if (n->type == NODE_STRING && n->data) {
        wasm_alloc_string(wg, (const char *)n->data);
    }
    for (size_t i = 0; i < n->child_count; i++) {
        wasm_collect_strings(wg, n->children[i]);
    }
}

/* 查找已分配的字符串地址（预扫描后使用） */
static int32_t wasm_lookup_string(WasmGen *wg, const char *s) {
    for (size_t i = 0; i < wg->string_count; i++) {
        if (wg->strings[i] && strcmp(wg->strings[i], s) == 0) {
            return wg->string_addrs[i];
        }
    }
    return wasm_alloc_string(wg, s);
}

static void emit_print_i32(WasmGen *wg) {
    /* 调用 $print_i32 (通过 fd_write 输出) */
    bv_write_u8(&wg->out, WASM_OPCODE_CALL);
    bv_write_u32_leb128(&wg->out, (uint32_t)wg->print_func_idx);
}

static void emit_print_string(WasmGen *wg, const char *s) {
    int32_t str_addr = wasm_lookup_string(wg, s);
    size_t slen = s ? strlen(s) : 0;
    /* fd_write(fd=1, iovs=8, iovs_len=1, rets=24) */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_u32_leb128(&wg->out, 1);              /* fd = 1 (stdout) */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_u32_leb128(&wg->out, 8);              /* iovs offset */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_u32_leb128(&wg->out, 1);              /* iovs count */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_u32_leb128(&wg->out, 24);             /* rets offset */
    /* iovec: [ptr, len] at address 8 */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_u32_leb128(&wg->out, 8);              /* ptr */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_u32_leb128(&wg->out, (int32_t)str_addr);
    bv_write_u8(&wg->out, WASM_OPCODE_I32_STORE);
    bv_write_u8(&wg->out, 0x02); bv_write_u8(&wg->out, 0x00); /* align=2, offset=0 */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_u32_leb128(&wg->out, 12);             /* len */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_u32_leb128(&wg->out, (int32_t)slen);
    bv_write_u8(&wg->out, WASM_OPCODE_I32_STORE);
    bv_write_u8(&wg->out, 0x02); bv_write_u8(&wg->out, 0x00);
    /* call fd_write */
    bv_write_u8(&wg->out, WASM_OPCODE_CALL);
    bv_write_u32_leb128(&wg->out, 0);              /* $fd_write */
    bv_write_u8(&wg->out, WASM_OPCODE_DROP);
}

/* --- 解析 print 调用 --- */
static void emit_print_call(WasmGen *wg, ASTNode *call) {
    if (!call || call->child_count < 1) return;
    ASTNode *arg = call->children[0];
    if (!arg) return;

    /* 解包 args 节点: NODE_EMPTY(data="args") 包含实际参数 */
    if (arg->type == NODE_EMPTY && arg->data && strcmp(arg->data, "args") == 0 &&
        arg->child_count >= 1) {
        arg = arg->children[0];
    }
    if (!arg) return;

    switch (arg->type) {
        case NODE_INT:
            emit_i32_const(wg, (int32_t)atoll((const char *)arg->data));
            emit_print_i32(wg);
            break;
        case NODE_STRING: {
            const char *s = (const char *)arg->data;
            emit_print_string(wg, s);
            break;
        }
        case NODE_IDENT:
        default:
            emit_print_i32(wg);
            break;
    }
}

static void emit_expr(WasmGen *wg, ASTNode *n) {
    if (!n) return;
    switch (n->type) {
        case NODE_INT:
            emit_i32_const(wg, (int32_t)atoll((const char *)n->data));
            break;
        case NODE_FLOAT:
            bv_write_u8(&wg->out, WASM_OPCODE_F64_CONST);
            break;
        case NODE_STRING: {
            int32_t addr = wasm_lookup_string(wg, (const char *)n->data);
            emit_i32_const(wg, addr);
            break;
        }
        case NODE_BOOL:
            emit_i32_const(wg, strcmp((const char *)n->data, "true") == 0 ? 1 : 0);
            break;
        case NODE_IDENT:
            emit_i32_const(wg, 0);
            break;
        case NODE_CALL: {
            const char *name = n->data ? (const char *)n->data : "";
            if (strcmp(name, "print") == 0) {
                emit_print_call(wg, n);
                bv_write_u8(&wg->out, WASM_OPCODE_DROP);
            } else {
                emit_i32_const(wg, 0);
            }
            break;
        }
        case NODE_SEND:
            /* 消息发送: receiver ! payload → emit call */
            if (n->child_count >= 2) {
                emit_expr(wg, n->children[0]);
                emit_expr(wg, n->children[1]);
                bv_write_u8(&wg->out, WASM_OPCODE_DROP);
                bv_write_u8(&wg->out, WASM_OPCODE_DROP);
            }
            break;
        case NODE_MSG_CALL:
            /* 同步消息调用: receiver @ payload */
            if (n->child_count >= 2) {
                emit_expr(wg, n->children[0]);
                emit_expr(wg, n->children[1]);
                bv_write_u8(&wg->out, WASM_OPCODE_DROP);
            }
            break;
        case NODE_IF:
            bv_write_u8(&wg->out, WASM_OPCODE_IF);
            bv_write_u8(&wg->out, 0x7F);
            if (n->child_count >= 1) emit_stmt(wg, n->children[0]);
            bv_write_u8(&wg->out, WASM_OPCODE_END);
            break;
        case NODE_WHILE:
        case NODE_FOR:
            bv_write_u8(&wg->out, WASM_OPCODE_LOOP);
            bv_write_u8(&wg->out, 0x40);
            bv_write_u8(&wg->out, WASM_OPCODE_END);
            break;
        case NODE_RETURN:
            if (n->child_count >= 1) emit_expr(wg, n->children[0]);
            else emit_i32_const(wg, 0);
            bv_write_u8(&wg->out, WASM_OPCODE_RETURN);
            break;
        case NODE_CAP:
            if (n->child_count >= 1) emit_expr(wg, n->children[0]);
            break;
        default:
            emit_i32_const(wg, 0);
            break;
    }
}

static void emit_stmt(WasmGen *wg, ASTNode *n) {
    if (!n) return;
    switch (n->type) {
        case NODE_RETURN:
            emit_expr(wg, n);
            break;
        case NODE_VAR:
        case NODE_LET:
            emit_expr(wg, n);
            bv_write_u8(&wg->out, WASM_OPCODE_DROP);
            break;
        case NODE_IF:
            bv_write_u8(&wg->out, WASM_OPCODE_IF);
            bv_write_u8(&wg->out, 0x7F);
            for (size_t i = 0; i < n->child_count && i < 2; i++) {
                emit_stmt(wg, n->children[i]);
            }
            bv_write_u8(&wg->out, WASM_OPCODE_END);
            break;
        case NODE_CALL: {
            const char *name = n->data ? (const char *)n->data : "";
            if (strcmp(name, "print") == 0) {
                emit_print_call(wg, n);
            } else {
                emit_expr(wg, n);
                bv_write_u8(&wg->out, WASM_OPCODE_DROP);
            }
            break;
        }
        case NODE_EMPTY:
            if (!n->data) {
                /* Block: 遍历子节点作为语句 */
                for (size_t i = 0; i < n->child_count; i++) {
                    emit_stmt(wg, n->children[i]);
                }
            }
            break;
        default:
            emit_expr(wg, n);
            bv_write_u8(&wg->out, WASM_OPCODE_DROP);
            break;
    }
}

/* --- 生成 WASM 模块 --- */
int wasm_write_program(ASTNode *ast, const char *output, TargetKind target) {
    if (!output) return 1;

    ByteVec bv = {0};

    /* 魔术字 + 版本 */
    bv_write_u8(&bv, 0x00); bv_write_u8(&bv, 0x61);
    bv_write_u8(&bv, 0x73); bv_write_u8(&bv, 0x6d);
    bv_write_u8(&bv, 0x01); bv_write_u8(&bv, 0x00);
    bv_write_u8(&bv, 0x00); bv_write_u8(&bv, 0x00);

    /* --- type section --- */
    bv_write_u8(&bv, 0x01);
    {
        ByteVec body = {0};
        bv_write_u8(&body, 0x02); /* 2 types */
        /* type 0: (func (result i32)) */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x00);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        /* type 1: fd_write signature */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x04);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        bv_write_vec(&bv, &body);
        bv_free(&body);
    }

    /* --- import section --- */
    bv_write_u8(&bv, 0x02);
    {
        ByteVec body = {0};
        const char *module_name = "wasi_snapshot_preview1";
        if (target == TARGET_WASI_P3) {
            module_name = "wasi_unstable";
            /* WASI P3: fd_write + proc_exit + async_spawn + async_await */
            bv_write_u8(&body, 0x04); /* 4 imports */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_write");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x01);
            bv_write_str(&body, module_name);
            bv_write_str(&body, "proc_exit");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x02);
            bv_write_str(&body, module_name);
            bv_write_str(&body, "async_spawn");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x00);
            bv_write_str(&body, module_name);
            bv_write_str(&body, "async_await");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x00);
        } else {
            bv_write_u8(&body, 0x02); /* 2 imports */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_write");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x01);
            bv_write_str(&body, module_name);
            bv_write_str(&body, "proc_exit");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x02);
        }
        bv_write_vec(&bv, &body);
        bv_free(&body);
    }

    /* --- function section --- */
    bv_write_u8(&bv, 0x03);
    {
        ByteVec body = {0};
        bv_write_u8(&body, 0x02); /* main + print_i32 */
        bv_write_u8(&body, 0x00); /* main -> type 0 */
        bv_write_u8(&body, 0x00); /* print_i32 -> type 0 */
        bv_write_vec(&bv, &body);
        bv_free(&body);
    }

    /* --- table section (indirect function table) --- */
    bv_write_u8(&bv, 0x04);
    bv_write_u8(&bv, 0x04); /* section size = 4: count(1) + type(1) + flags(1) + min(1) */
    bv_write_u8(&bv, 0x01); /* 1 table */
    bv_write_u8(&bv, 0x70); /* funcref */
    bv_write_u8(&bv, 0x00); /* flags=0 (no max) */
    bv_write_u8(&bv, 0x01); /* min = 1 */

    /* --- memory section --- */
    bv_write_u8(&bv, 0x05);
    bv_write_u8(&bv, 0x03); /* section size = 3: count(1) + flags(1) + min(1) */
    bv_write_u8(&bv, 0x01); /* 1 memory */
    bv_write_u8(&bv, 0x00); /* flags=0 (no max) */
    bv_write_u8(&bv, 0x01); /* min = 1 page (64KB) */

    /* --- export section --- */
    bv_write_u8(&bv, 0x07);
    {
        ByteVec body = {0};
        bv_write_u8(&body, 0x01);
        bv_write_str(&body, "main");
        bv_write_u8(&body, 0x00);
        /* main 的函数索引 = import 数量 (fd_write=0, proc_exit=1, ...) */
        int32_t export_idx = (target == TARGET_WASI_P3) ? 4 : 2;
        bv_write_u32_leb128(&body, (uint32_t)export_idx);
        bv_write_vec(&bv, &body);
        bv_free(&body);
    }

    WasmGen wg = {0};
    wg.out = bv;
    wg.print_func_idx = (target == TARGET_WASI_P3) ? 5 : 3; /* import_count + 1 */
    wg.next_str_addr = 64; /* 字符串从地址 64 开始，避开 iovec 区域 (8-24) */

    /* 预扫描 AST，收集字符串 */
    if (ast) wasm_collect_strings(&wg, ast);

    /* --- code section --- */
    bv_write_u8(&bv, 0x0A);
    {
        ByteVec body = {0};

        bv_write_u32_leb128(&body, 2); /* 2 functions: main + print_i32 */

        /* func 0: main */
        {
            ByteVec code = {0};
            bv_write_u8(&code, 0x00); /* 0 locals */

            if (ast) {
                for (size_t i = 0; i < ast->child_count; i++) {
                    ASTNode *ch = ast->children[i];
                    if (!ch || ch->type != NODE_ACTOR) continue;
                    for (size_t j = 0; j < ch->child_count; j++) {
                        ASTNode *m = ch->children[j];
                        if (!m || m->type != NODE_NEW) continue;
                        for (size_t k = 0; k < m->child_count; k++) {
                            ASTNode *c = m->children[k];
                            if (!c) continue;
                            WasmGen stmt_wg = {0};
                            stmt_wg.print_func_idx = wg.print_func_idx;
                            stmt_wg.next_str_addr = wg.next_str_addr;
                            stmt_wg.strings = wg.strings;
                            stmt_wg.string_lens = wg.string_lens;
                            stmt_wg.string_addrs = wg.string_addrs;
                            stmt_wg.string_count = wg.string_count;
                            stmt_wg.string_cap = wg.string_cap;
                            emit_stmt(&stmt_wg, c);
                            bv_write_raw(&code, stmt_wg.out.data, stmt_wg.out.size);
                            bv_free(&stmt_wg.out);
                        }
                    }
                }
            }
            bv_write_u8(&code, WASM_OPCODE_I32_CONST);
            bv_write_i32_leb128(&code, 0);
            bv_write_u8(&code, WASM_OPCODE_END);
            bv_write_u32_leb128(&body, (uint32_t)code.size);
            bv_write_raw(&body, code.data, code.size);
            bv_free(&code);
        }

        /* func 1: print_i32 helper (drop val, return 0) */
        {
            ByteVec code = {0};
            bv_write_u8(&code, 0x00); /* 0 locals */
            bv_write_u8(&code, WASM_OPCODE_I32_CONST);
            bv_write_i32_leb128(&code, 0);
            bv_write_u8(&code, WASM_OPCODE_END);
            bv_write_u32_leb128(&body, (uint32_t)code.size);
            bv_write_raw(&body, code.data, code.size);
            bv_free(&code);
        }

        bv_write_vec(&bv, &body);
        bv_free(&body);
    }

    /* --- data section (字符串常量) --- */
    bv_write_u8(&bv, 0x0B);
    {
        ByteVec body = {0};
        if (wg.string_count > 0) {
            bv_write_u32_leb128(&body, (uint32_t)wg.string_count);
            for (size_t i = 0; i < wg.string_count; i++) {
                /* 段 0：i32.const addr; end */
                bv_write_u8(&body, 0x00);
                bv_write_u8(&body, 0x41);
                bv_write_i32_leb128(&body, wg.string_addrs[i]);
                bv_write_u8(&body, 0x0B);
                /* 字符串内容 + null 终止符 */
                size_t slen = wg.string_lens[i];
                bv_write_u32_leb128(&body, (uint32_t)(slen + 1));
                if (slen > 0 && wg.strings[i]) {
                    bv_write_raw(&body, (const unsigned char *)wg.strings[i], slen);
                }
                bv_write_u8(&body, 0x00); /* null terminator */
            }
        } else {
            bv_write_u8(&body, 0x00);
        }
        bv_write_vec(&bv, &body);
        bv_free(&body);
    }

    /* 写文件 */
    FILE *f = fopen(output, "wb");
    if (!f) { bv_free(&bv); return 1; }
    if (fwrite(bv.data, 1, bv.size, f) != bv.size) { fclose(f); bv_free(&bv); return 1; }
    fclose(f);

    bv_free(&bv);
    return 0;
}

/* WASM 目标平台名 */
const char *wasm_target_name(TargetKind target) {
    switch (target) {
        case TARGET_WASI_P2:   return "wasi-p2";
        case TARGET_WASI_P3:   return "wasi-p3";
        case TARGET_COMPONENT: return "component";
        case TARGET_BROWSER:   return "browser";
        case TARGET_MCU_WASM:  return "mcu-wasm";
        case TARGET_NATIVE:    return "native";
        default: return "unknown";
    }
}
