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
    /* 有符号 LEB128: 终止字节的 bit6 须与符号一致 (64 → C0 00, -64 → 40) */
    int more = 1;
    while (more) {
        uint8_t byte = (uint8_t)(v & 0x7F);
        v >>= 7; /* 算术右移 */
        if ((v == 0 && !(byte & 0x40)) || (v == -1 && (byte & 0x40))) {
            more = 0;
        } else {
            byte |= 0x80;
        }
        bv_write_u8(bv, byte);
    }
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
#define WASM_OPCODE_I32_GT_S  0x4A
#define WASM_OPCODE_I32_LE_S  0x4C
#define WASM_OPCODE_I32_GE_S  0x4E
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
/* W2: 字符串运行时所需 opcode (0.2.11: 修正 GT_S/LE_S/GE_S 旧定义错位) */
#define WASM_OPCODE_I32_LT_U    0x49
#define WASM_OPCODE_I32_GT_U    0x4B
#define WASM_OPCODE_I32_LE_U    0x4D
#define WASM_OPCODE_I32_GE_U    0x4F
#define WASM_OPCODE_I32_LOAD8_U 0x2D
#define WASM_OPCODE_I32_STORE8  0x3A
#define WASM_OPCODE_I32_REM_S   0x6F
#define WASM_OPCODE_I32_REM_U   0x70
#define WASM_OPCODE_I32_DIV_U   0x6E
#define WASM_OPCODE_GLOBAL_GET  0x23
#define WASM_OPCODE_GLOBAL_SET  0x24

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
    /* Bug#33: 局部变量表 (main 函数 locals, i32) */
    char local_names[64][64];
    int local_count;
    /* W2: 局部变量字符串标记 (1=String 指针) */
    char local_is_str[64];
    /* W2: 字符串运行时函数索引 */
    int32_t rt_alloc, rt_strlen, rt_concat, rt_itoa, rt_slice;
    int32_t rt_streq, rt_print_str, rt_find, rt_find_from, rt_field;
    /* W4: 内建全集 */
    int32_t rt_chr, rt_repl, rt_json;
    /* W4b: WASI 系统接口运行时 */
    int32_t rt_envget, rt_fexists, rt_fread, rt_fappend, rt_sysexec;
    /* W4b: WASI import 索引 (P2/P3 布局不同) */
    int32_t imp_env_sizes, imp_env_get, imp_path_open, imp_fd_close,
            imp_path_fstat, imp_fd_fstat, imp_fd_read;
    /* W4b(修复): preopen 绝对路径解析 (wasi-libc 同款) */
    int32_t imp_prestat_get, imp_prestat_name, imp_fd_seek, rt_resolve;
    /* W3: 当前方法所属类索引 (-1=无); 局部变量类名追踪 */
    int cur_class;
    char local_class[64][32];
} WasmGen;

/* ===== W3: 类系统表 ===== */
typedef struct {
    char name[64];
    int is_str;
    int32_t offset;   /* 字节偏移: 4 * 声明序 */
} WasmField;

typedef struct {
    char name[64];
    int32_t fn_idx;
    int nargs;        /* 不含 self */
    int ret_is_str;
    int is_ctor;
} WasmMethod;

typedef struct {
    char name[64];
    WasmField fields[32];
    int nfields;
    WasmMethod methods[32];
    int nmethods;
    int32_t ctor_fn;  /* 构造器函数索引, -1=无 */
} WasmClass;

static WasmClass g_wasm_classes[16];
static int g_wasm_nclasses = 0;

static WasmClass *w3_find_class(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < g_wasm_nclasses; i++)
        if (strcmp(g_wasm_classes[i].name, name) == 0) return &g_wasm_classes[i];
    return NULL;
}

static int w3_field_idx(WasmClass *c, const char *fname) {
    if (!c || !fname) return -1;
    for (int i = 0; i < c->nfields; i++)
        if (strcmp(c->fields[i].name, fname) == 0) return i;
    return -1;
}

static WasmMethod *w3_find_method(WasmClass *c, const char *mname) {
    if (!c || !mname) return NULL;
    for (int i = 0; i < c->nmethods; i++)
        if (strcmp(c->methods[i].name, mname) == 0) return &c->methods[i];
    return NULL;
}

/* 递归查找节点树中是否有 NODE_IDENT data==tname */
static int w3_tree_has_type(ASTNode *n, const char *tname) {
    if (!n) return 0;
    if (n->type == NODE_IDENT && n->data && strcmp((const char *)n->data, tname) == 0) return 1;
    for (size_t i = 0; i < n->child_count; i++)
        if (w3_tree_has_type(n->children[i], tname)) return 1;
    return 0;
}

/* W3: 收集 AST 中所有非 main 的 actor/class 为类定义; fn_base=首个可用函数索引 */
static void w3_collect_classes(ASTNode *ast, int32_t fn_base) {
    g_wasm_nclasses = 0;
    if (!ast) return;
    int32_t next_fn = fn_base;
    for (size_t i = 0; i < ast->child_count && g_wasm_nclasses < 16; i++) {
        ASTNode *ch = ast->children[i];
        if (!ch || ch->type != NODE_ACTOR || !ch->data) continue;
        if (strcmp((const char *)ch->data, "main") == 0) continue;  /* main actor 非类 */
        WasmClass *c = &g_wasm_classes[g_wasm_nclasses];
        memset(c, 0, sizeof(*c));
        snprintf(c->name, sizeof(c->name), "%s", (const char *)ch->data);
        c->ctor_fn = -1;
        c->nfields = 0;
        c->nmethods = 0;
        for (size_t j = 0; j < ch->child_count; j++) {
            ASTNode *m = ch->children[j];
            if (!m) continue;
            if ((m->type == NODE_VAR || m->type == NODE_LET) && m->data && c->nfields < 32) {
                WasmField *f = &c->fields[c->nfields];
                snprintf(f->name, sizeof(f->name), "%s", (const char *)m->data);
                f->is_str = w3_tree_has_type(m, "String");
                f->offset = (int32_t)c->nfields * 4;
                c->nfields++;
            } else if ((m->type == NODE_FUN || m->type == NODE_NEW || m->type == NODE_BE) && m->data && c->nmethods < 32) {
                WasmMethod *me = &c->methods[c->nmethods];
                snprintf(me->name, sizeof(me->name), "%s", (const char *)m->data);
                me->is_ctor = (m->type == NODE_NEW) ? 1 : 0;
                me->fn_idx = next_fn++;
                /* params 容器 + ret type 定位 */
                ASTNode *params = NULL;
                for (size_t k = 0; k < m->child_count; k++) {
                    ASTNode *cc2 = m->children[k];
                    if (cc2 && cc2->type == NODE_EMPTY && cc2->data &&
                        strcmp((const char *)cc2->data, "params") == 0) { params = cc2; break; }
                }
                me->nargs = params ? (int)params->child_count : 0;
                /* ret: params 后第一位 或 无 params 时的唯一前置 child (非 body=最后) */
                ASTNode *ret = NULL;
                if (params && m->child_count >= 3) ret = m->children[1];
                else if (!params && m->child_count >= 2) ret = m->children[0];
                me->ret_is_str = (ret && ret->type == NODE_IDENT && ret->data &&
                                  strcmp((const char *)ret->data, "String") == 0) ? 1 : 0;
                if (me->is_ctor && c->ctor_fn < 0) c->ctor_fn = me->fn_idx;
                c->nmethods++;
            }
        }
        g_wasm_nclasses++;
    }
}


static int wasm_local_lookup(WasmGen *wg, const char *name) {
    if (!name) return -1;
    for (int i = 0; i < wg->local_count; i++) {
        if (strcmp(wg->local_names[i], name) == 0) return i;
    }
    return -1;
}

static int wasm_local_ensure(WasmGen *wg, const char *name) {
    int idx = wasm_local_lookup(wg, name);
    if (idx >= 0) return idx;
    if (wg->local_count >= 64 || !name) return -1;
    snprintf(wg->local_names[wg->local_count], sizeof(wg->local_names[0]), "%s", name);
    return wg->local_count++;
}

/* 前向声明 */
static void emit_expr(WasmGen *wg, ASTNode *n);
static void emit_stmt(WasmGen *wg, ASTNode *n);
/* W3 前向声明 (定义在 wasm_try_builtin_call 之后) */
static int w3_split_dot(const char *name, char *recv, size_t rsz, char *meth, size_t msz);
static int w3_resolve_field(WasmGen *wg, const char *name,
                            const char **recv_out, int32_t *off_out, int *is_str_out);

/* W2: 表达式字符串类型判定 */
static int wasm_expr_is_str(WasmGen *wg, ASTNode *n) {
    if (!n || !wg) return 0;
    switch (n->type) {
        case NODE_STRING: return 1;
        case NODE_IDENT: {
            int li = n->data ? wasm_local_lookup(wg, (const char *)n->data) : -1;
            if (li >= 0 && li < 64 && wg->local_is_str[li]) return 1;
            /* W3: 字符串字段 (this.f / 裸字段 / c.f — cur_class 门槛由 resolve 内部判断) */
            if (n->data) {
                const char *recv = NULL; int32_t off = 0; int fs = 0;
                if (w3_resolve_field(wg, (const char *)n->data, &recv, &off, &fs)) return fs;
            }
            return 0;
        }
        case NODE_CALL: {
            const char *name = n->data ? (const char *)n->data : "";
            if (strcmp(name, "concat") == 0 || strcmp(name, "itoa") == 0 ||
                strcmp(name, "slice") == 0 || strcmp(name, "field") == 0 ||
                strcmp(name, "str_field") == 0 || strcmp(name, "str_from_char") == 0 ||
                strcmp(name, "str_replace_all") == 0 || strcmp(name, "json_raw_get") == 0 ||
                strcmp(name, "env_get") == 0 || strcmp(name, "file_read") == 0 ||
                strcmp(name, "sys_exec") == 0) return 1;
            /* W3: 方法返回 String */
            {
                char recv[128], meth[128];
                if (w3_split_dot(name, recv, sizeof(recv), meth, sizeof(meth))) {
                    WasmClass *c = NULL;
                    if (strcmp(recv, "this") == 0) {
                        if (wg->cur_class >= 0 && wg->cur_class < g_wasm_nclasses)
                            c = &g_wasm_classes[wg->cur_class];
                    } else {
                        int li = wasm_local_lookup(wg, recv);
                        if (li >= 0 && li < 64 && wg->local_class[li][0])
                            c = w3_find_class(wg->local_class[li]);
                    }
                    if (c) {
                        WasmMethod *m = w3_find_method(c, meth);
                        if (m && m->ret_is_str) return 1;
                    }
                }
            }
            return 0;
        }
        case NODE_EMPTY: {
            if (!n->data || n->child_count < 2) return 0;
            const char *d = (const char *)n->data;
            if (strcmp(d, "+") == 0)
                return wasm_expr_is_str(wg, n->children[0]) || wasm_expr_is_str(wg, n->children[1]);
            return 0;
        }
        default: return 0;
    }
}

/* W2: 发射字符串内建调用; 返回 1=已处理 */
static int wasm_try_builtin_call(WasmGen *wg, ASTNode *n) {
    if (!n || n->type != NODE_CALL || !n->data) return 0;
    const char *name = (const char *)n->data;
    /* Bug#46 W2: 解析器把调用实参打包进单个 NODE_EMPTY 容器 — 展开后再分派 */
    ASTNode *args = n;
    if (n->child_count == 1 && n->children[0] &&
        n->children[0]->type == NODE_EMPTY && n->children[0]->child_count > 0) {
        args = n->children[0];
    }
    struct { const char *nm; int nargs; int32_t fn; } tbl[] = {
        {"concat", 2, wg->rt_concat}, {"itoa", 1, wg->rt_itoa},
        {"slice", 3, wg->rt_slice}, {"len", 1, wg->rt_strlen},
        {"length", 1, wg->rt_strlen}, {"streq", 2, wg->rt_streq},
        {"find", 2, wg->rt_find}, {"find_from", 3, wg->rt_find_from},
        {"field", 3, wg->rt_field}, {"str_field", 3, wg->rt_field},
        {"str_from_char", 1, wg->rt_chr},
        {"str_replace_all", 3, wg->rt_repl},
        {"json_raw_get", 2, wg->rt_json},
        /* W4b: WASI 系统接口 */
        {"env_get", 1, wg->rt_envget}, {"file_exists", 1, wg->rt_fexists},
        {"file_read", 1, wg->rt_fread}, {"file_append", 2, wg->rt_fappend},
        {"sys_exec", 1, wg->rt_sysexec},
    };
    for (size_t i = 0; i < sizeof(tbl)/sizeof(tbl[0]); i++) {
        if (strcmp(name, tbl[i].nm) == 0 && (int)args->child_count >= tbl[i].nargs) {
            for (int a = 0; a < tbl[i].nargs; a++) emit_expr(wg, args->children[a]);
            bv_write_u8(&wg->out, WASM_OPCODE_CALL);
            bv_write_u32_leb128(&wg->out, (uint32_t)tbl[i].fn);
            return 1;
        }
    }
    return 0;
}

/* ===== W3: 类系统发射 helper ===== */
static void emit_i32_const(WasmGen *wg, int32_t v);
/* 拆 "recv.meth" → recv/meth; 返回 1=有点 */
static int w3_split_dot(const char *name, char *recv, size_t rsz, char *meth, size_t msz) {
    const char *dot = name ? strchr(name, '.') : NULL;
    if (!dot || dot == name) return 0;
    size_t rl = (size_t)(dot - name);
    if (rl >= rsz) rl = rsz - 1;
    memcpy(recv, name, rl); recv[rl] = 0;
    snprintf(meth, msz, "%s", dot + 1);
    return 1;
}

/* 发射 self/receiver 指针: this→local0; 局部变量→local.get; 否则 const 0 */
static void w3_emit_recv(WasmGen *wg, const char *recv) {
    if (strcmp(recv, "this") == 0) {
        bv_write_u8(&wg->out, WASM_OPCODE_LOCAL_GET);
        bv_write_u32_leb128(&wg->out, 0);
        return;
    }
    int li = wasm_local_lookup(wg, recv);
    if (li >= 0) {
        bv_write_u8(&wg->out, WASM_OPCODE_LOCAL_GET);
        bv_write_u32_leb128(&wg->out, (uint32_t)li);
    } else {
        emit_i32_const(wg, 0);
    }
}

/* i32.load offset=N align=2 — 栈顶 ptr → 值 */
static void w3_emit_load(WasmGen *wg, int32_t offset) {
    bv_write_u8(&wg->out, WASM_OPCODE_I32_LOAD);
    bv_write_u8(&wg->out, 0x02); /* align=2 */
    bv_write_u32_leb128(&wg->out, (uint32_t)offset); /* offset */
}

/* i32.store offset=N align=2 — 栈顶 [ptr, val] */
static void w3_emit_store(WasmGen *wg, int32_t offset) {
    bv_write_u8(&wg->out, WASM_OPCODE_I32_STORE);
    bv_write_u8(&wg->out, 0x02);
    bv_write_u32_leb128(&wg->out, (uint32_t)offset);
}

/* "this.f"/"c.f"/裸字段名(方法体内) → 解析为 (recv_name, field_idx); 返回 1=成功 */
static char w3_recv_buf[128];
static int w3_resolve_field(WasmGen *wg, const char *name,
                            const char **recv_out, int32_t *off_out, int *is_str_out) {
    if (!name) return 0;
    WasmClass *c = NULL;
    char fld[128];
    if (strncmp(name, "this.", 5) == 0) {
        if (wg->cur_class < 0 || wg->cur_class >= g_wasm_nclasses) return 0;
        c = &g_wasm_classes[wg->cur_class];
        snprintf(w3_recv_buf, sizeof(w3_recv_buf), "this");
        snprintf(fld, sizeof(fld), "%s", name + 5);
    } else if (strchr(name, '.')) {
        /* c.f: 接收者是持有实例的局部变量 (main 或方法体内均适用) */
        char recv[128];
        if (!w3_split_dot(name, recv, sizeof(recv), fld, sizeof(fld))) return 0;
        int li = wasm_local_lookup(wg, recv);
        if (li < 0 || li >= 64 || !wg->local_class[li][0]) return 0;
        c = w3_find_class(wg->local_class[li]);
        if (!c) return 0;
        snprintf(w3_recv_buf, sizeof(w3_recv_buf), "%s", recv);
    } else {
        /* 裸字段名: 仅方法体内、且不在局部表时按 self 字段解析 */
        if (wg->cur_class < 0 || wg->cur_class >= g_wasm_nclasses) return 0;
        if (wasm_local_lookup(wg, name) >= 0) return 0;
        c = &g_wasm_classes[wg->cur_class];
        snprintf(w3_recv_buf, sizeof(w3_recv_buf), "this");
        snprintf(fld, sizeof(fld), "%s", name);
    }
    int fi = w3_field_idx(c, fld);
    if (fi < 0) return 0;
    *recv_out = w3_recv_buf;
    *off_out = c->fields[fi].offset;
    *is_str_out = c->fields[fi].is_str;
    return 1;
}

/* W3: expr 层的类方法/构造分派; 返回 1=已处理 */
static int w3_try_class_call(WasmGen *wg, ASTNode *n) {
    if (!n || n->type != NODE_CALL || !n->data) return 0;
    const char *name = (const char *)n->data;
    ASTNode *args = n;
    /* 解析器把实参打包进单个 NODE_EMPTY("args") 容器 — 空参也要展开 */
    if (n->child_count == 1 && n->children[0] &&
        n->children[0]->type == NODE_EMPTY && n->children[0]->data &&
        strcmp((const char *)n->children[0]->data, "args") == 0) {
        args = n->children[0];
    }
    /* 构造: "Cls" 或 "Cls.create" */
    {
        WasmClass *c = w3_find_class(name);
        char recv[128], meth[128];
        if (!c && w3_split_dot(name, recv, sizeof(recv), meth, sizeof(meth))) {
            WasmClass *c2 = w3_find_class(recv);
            if (c2 && w3_find_method(c2, meth) && w3_find_method(c2, meth)->is_ctor) c = c2;
        }
        if (c && c->ctor_fn >= 0) {
            int32_t sz = c->nfields > 0 ? c->nfields * 4 : 4;
            emit_i32_const(wg, sz);
            bv_write_u8(&wg->out, WASM_OPCODE_CALL);
            bv_write_u32_leb128(&wg->out, (uint32_t)wg->rt_alloc);
            /* alloc 返回 ptr, 直接作为 self 传入 ctor → 返回 self */
            bv_write_u8(&wg->out, WASM_OPCODE_CALL);
            bv_write_u32_leb128(&wg->out, (uint32_t)c->ctor_fn);
            return 1;
        }
    }
    /* 方法: "this.m" / "recv.m" */
    char recv[128], meth[128];
    if (!w3_split_dot(name, recv, sizeof(recv), meth, sizeof(meth))) return 0;
    WasmClass *c = NULL;
    if (strcmp(recv, "this") == 0) {
        if (wg->cur_class >= 0 && wg->cur_class < g_wasm_nclasses)
            c = &g_wasm_classes[wg->cur_class];
    } else {
        int li = wasm_local_lookup(wg, recv);
        if (li >= 0 && li < 64 && wg->local_class[li][0])
            c = w3_find_class(wg->local_class[li]);
    }
    if (!c) return 0;
    WasmMethod *m = w3_find_method(c, meth);
    if (!m || m->is_ctor) return 0;
    w3_emit_recv(wg, recv);
    for (size_t a = 0; a < args->child_count; a++) emit_expr(wg, args->children[a]);
    bv_write_u8(&wg->out, WASM_OPCODE_CALL);
    bv_write_u32_leb128(&wg->out, (uint32_t)m->fn_idx);
    return 1;
}

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
    /* 调用 $print_i32 (通过 fd_write 输出); 返回值 DROP 保持栈平衡 */
    bv_write_u8(&wg->out, WASM_OPCODE_CALL);
    bv_write_u32_leb128(&wg->out, (uint32_t)wg->print_func_idx);
    bv_write_u8(&wg->out, WASM_OPCODE_DROP);
}

static void emit_print_string(WasmGen *wg, const char *s) {
    int32_t str_addr = wasm_lookup_string(wg, s);
    size_t slen = s ? strlen(s) : 0;
    /* fd_write(fd=1, iovs=8, iovs_len=1, rets=24)
       Bug#32: i32.const 操作数必须 signed LEB128 — 64 (0x40) 被 sLEB 解码为 -64 */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_i32_leb128(&wg->out, 1);              /* fd = 1 (stdout) */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_i32_leb128(&wg->out, 8);              /* iovs offset */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_i32_leb128(&wg->out, 1);              /* iovs count */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_i32_leb128(&wg->out, 24);             /* rets offset */
    /* iovec: [ptr, len] at address 8 */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_i32_leb128(&wg->out, 8);              /* ptr */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_i32_leb128(&wg->out, (int32_t)str_addr);
    bv_write_u8(&wg->out, WASM_OPCODE_I32_STORE);
    bv_write_u8(&wg->out, 0x02); bv_write_u8(&wg->out, 0x00); /* align=2, offset=0 */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_i32_leb128(&wg->out, 12);             /* len */
    bv_write_u8(&wg->out, WASM_OPCODE_I32_CONST);
    bv_write_i32_leb128(&wg->out, (int32_t)slen);
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

    /* W2: 真实 print — str 走 print_str, int 走 itoa+print_str
       (此前 print(变量/表达式) 是静默桩: helper 无参数无输出) */
    if (wasm_expr_is_str(wg, arg)) {
        emit_expr(wg, arg);
        bv_write_u8(&wg->out, WASM_OPCODE_CALL);
        bv_write_u32_leb128(&wg->out, (uint32_t)wg->rt_print_str);
        bv_write_u8(&wg->out, WASM_OPCODE_DROP);
    } else {
        emit_expr(wg, arg);
        bv_write_u8(&wg->out, WASM_OPCODE_CALL);
        bv_write_u32_leb128(&wg->out, (uint32_t)wg->rt_itoa);
        bv_write_u8(&wg->out, WASM_OPCODE_CALL);
        bv_write_u32_leb128(&wg->out, (uint32_t)wg->rt_print_str);
        bv_write_u8(&wg->out, WASM_OPCODE_DROP);
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
        case NODE_IDENT: {
            /* Bug#33: 局部变量真实访问 */
            const char *vn = n->data ? (const char *)n->data : "";
            int li = wasm_local_lookup(wg, vn);
            if (li >= 0) {
                bv_write_u8(&wg->out, WASM_OPCODE_LOCAL_GET);
                bv_write_u32_leb128(&wg->out, (uint32_t)li);
                break;
            }
            /* W3: 字段加载 this.f / c.f / 裸字段名(方法体内) */
            {
                const char *recv = NULL; int32_t off = 0; int fs = 0;
                if (w3_resolve_field(wg, vn, &recv, &off, &fs)) {
                    w3_emit_recv(wg, recv);
                    w3_emit_load(wg, off);
                    break;
                }
            }
            emit_i32_const(wg, 0);
            break;
        }
        case NODE_CALL: {
            const char *name = n->data ? (const char *)n->data : "";
            if (strcmp(name, "print") == 0) {
                emit_print_call(wg, n); /* 已内部平衡, 无需 DROP */
                break;
            }
            /* W2: 字符串内建 (concat/itoa/slice/len/streq/find/find_from/field) */
            if (wasm_try_builtin_call(wg, n)) break;
            /* W3: 类方法/构造分派 */
            if (w3_try_class_call(wg, n)) break;
            /* Bug#33: 二元运算符真实语义 */
            {
                unsigned char opc = 0;
                if (strcmp(name, "+") == 0) opc = WASM_OPCODE_I32_ADD;
                else if (strcmp(name, "-") == 0) opc = WASM_OPCODE_I32_SUB;
                else if (strcmp(name, "*") == 0) opc = WASM_OPCODE_I32_MUL;
                else if (strcmp(name, "/") == 0) opc = WASM_OPCODE_I32_DIV_S;
                else if (strcmp(name, "==") == 0) opc = WASM_OPCODE_I32_EQ;
                else if (strcmp(name, "!=") == 0) opc = WASM_OPCODE_I32_NE;
                else if (strcmp(name, "<") == 0) opc = WASM_OPCODE_I32_LT_S;
                else if (strcmp(name, ">") == 0) opc = WASM_OPCODE_I32_GT_S;
                else if (strcmp(name, "<=") == 0) opc = WASM_OPCODE_I32_LE_S;
                else if (strcmp(name, ">=") == 0) opc = WASM_OPCODE_I32_GE_S;
                else if (strcmp(name, "and") == 0) opc = WASM_OPCODE_I32_AND;
                else if (strcmp(name, "or") == 0) opc = WASM_OPCODE_I32_OR;
                if (opc && n->child_count >= 2) {
                    emit_expr(wg, n->children[0]);
                    emit_expr(wg, n->children[1]);
                    bv_write_u8(&wg->out, opc);
                    break;
                }
            }
            emit_i32_const(wg, 0);
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
        case NODE_EMPTY:
            /* Bug#51: parser 的 return 语句是 NODE_EMPTY(data="return") 而非 NODE_RETURN */
            if (n->data && strcmp((const char *)n->data, "return") == 0) {
                if (n->child_count >= 1) emit_expr(wg, n->children[0]);
                else emit_i32_const(wg, 0);
                bv_write_u8(&wg->out, WASM_OPCODE_RETURN);
                break;
            }
            /* W1: 一元 not/neg */
            if (n->data && n->child_count == 1) {
                const char *u = (const char *)n->data;
                if (strcmp(u, "not") == 0) {
                    emit_expr(wg, n->children[0]);
                    bv_write_u8(&wg->out, WASM_OPCODE_I32_EQZ);
                    break;
                }
                if (strcmp(u, "neg") == 0) {
                    emit_i32_const(wg, 0);
                    emit_expr(wg, n->children[0]);
                    bv_write_u8(&wg->out, WASM_OPCODE_I32_SUB);
                    break;
                }
            }
            /* Bug#33b: 二元运算符节点是 NODE_EMPTY(data=op) 而非 NODE_CALL */
            if (n->data && n->child_count >= 2) {
                const char *name = (const char *)n->data;
                /* W2: 字符串运算符重载 */
                if (strcmp(name, "+") == 0 &&
                    (wasm_expr_is_str(wg, n->children[0]) || wasm_expr_is_str(wg, n->children[1]))) {
                    emit_expr(wg, n->children[0]);
                    if (!wasm_expr_is_str(wg, n->children[0])) {
                        bv_write_u8(&wg->out, WASM_OPCODE_CALL);
                        bv_write_u32_leb128(&wg->out, (uint32_t)wg->rt_itoa);
                    }
                    emit_expr(wg, n->children[1]);
                    if (!wasm_expr_is_str(wg, n->children[1])) {
                        bv_write_u8(&wg->out, WASM_OPCODE_CALL);
                        bv_write_u32_leb128(&wg->out, (uint32_t)wg->rt_itoa);
                    }
                    bv_write_u8(&wg->out, WASM_OPCODE_CALL);
                    bv_write_u32_leb128(&wg->out, (uint32_t)wg->rt_concat);
                    break;
                }
                if ((strcmp(name, "==") == 0 || strcmp(name, "!=") == 0) &&
                    (wasm_expr_is_str(wg, n->children[0]) || wasm_expr_is_str(wg, n->children[1]))) {
                    emit_expr(wg, n->children[0]);
                    emit_expr(wg, n->children[1]);
                    bv_write_u8(&wg->out, WASM_OPCODE_CALL);
                    bv_write_u32_leb128(&wg->out, (uint32_t)wg->rt_streq);
                    if (strcmp(name, "!=") == 0) bv_write_u8(&wg->out, WASM_OPCODE_I32_EQZ);
                    break;
                }
                unsigned char opc = 0;
                if (strcmp(name, "+") == 0) opc = WASM_OPCODE_I32_ADD;
                else if (strcmp(name, "-") == 0) opc = WASM_OPCODE_I32_SUB;
                else if (strcmp(name, "*") == 0) opc = WASM_OPCODE_I32_MUL;
                else if (strcmp(name, "/") == 0) opc = WASM_OPCODE_I32_DIV_S;
                else if (strcmp(name, "==") == 0) opc = WASM_OPCODE_I32_EQ;
                else if (strcmp(name, "!=") == 0) opc = WASM_OPCODE_I32_NE;
                else if (strcmp(name, "<") == 0) opc = WASM_OPCODE_I32_LT_S;
                else if (strcmp(name, ">") == 0) opc = WASM_OPCODE_I32_GT_S;
                else if (strcmp(name, "<=") == 0) opc = WASM_OPCODE_I32_LE_S;
                else if (strcmp(name, ">=") == 0) opc = WASM_OPCODE_I32_GE_S;
                else if (strcmp(name, "and") == 0) opc = WASM_OPCODE_I32_AND;
                else if (strcmp(name, "or") == 0) opc = WASM_OPCODE_I32_OR;
                if (opc) {
                    emit_expr(wg, n->children[0]);
                    emit_expr(wg, n->children[1]);
                    bv_write_u8(&wg->out, opc);
                    break;
                }
            }
            emit_i32_const(wg, 0);
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
        case NODE_LET: {
            /* Bug#33: var x: T = expr → expr; local.set x */
            ASTNode *init = NULL;
            if (n->child_count > 1) init = n->children[1];
            else if (n->child_count > 0 && n->children[0]->type != NODE_CAP) init = n->children[0];
            int li = n->data ? wasm_local_ensure(wg, (const char *)n->data) : -1;
            /* W2: String 类型标记 */
            if (li >= 0 && li < 64) {
                for (size_t ci = 0; ci < n->child_count; ci++) {
                    ASTNode *t = n->children[ci];
                    if (t && t->data && strcmp((const char *)t->data, "String") == 0) {
                        wg->local_is_str[li] = 1;
                        break;
                    }
                }
                if (init && wasm_expr_is_str(wg, init)) wg->local_is_str[li] = 1;
                /* W3: 类实例追踪 — 类型标注或构造调用 */
                if (li < 64) {
                    for (size_t ci = 0; ci < n->child_count; ci++) {
                        ASTNode *t = n->children[ci];
                        if (t && t->type == NODE_CAP && t->data &&
                            strcmp((const char *)t->data, "type") == 0 && t->child_count > 0 &&
                            t->children[0] && t->children[0]->data &&
                            w3_find_class((const char *)t->children[0]->data)) {
                            snprintf(wg->local_class[li], sizeof(wg->local_class[0]), "%s",
                                     (const char *)t->children[0]->data);
                            break;
                        }
                    }
                    if (!wg->local_class[li][0] && init && init->type == NODE_CALL && init->data) {
                        const char *cn = (const char *)init->data;
                        WasmClass *c = w3_find_class(cn);
                        char r2[128], m2[128];
                        if (!c && w3_split_dot(cn, r2, sizeof(r2), m2, sizeof(m2))) c = w3_find_class(r2);
                        if (c) snprintf(wg->local_class[li], sizeof(wg->local_class[0]), "%s", c->name);
                    }
                }
            }
            if (init) {
                emit_expr(wg, init);
                if (li >= 0) {
                    bv_write_u8(&wg->out, WASM_OPCODE_LOCAL_SET);
                    bv_write_u32_leb128(&wg->out, (uint32_t)li);
                } else {
                    bv_write_u8(&wg->out, WASM_OPCODE_DROP);
                }
            }
            break;
        }
        case NODE_IF:
            /* Bug#33: children[0]=条件, [1]=then块, [2]=else块; blocktype=void */
            if (n->child_count > 0) emit_expr(wg, n->children[0]);
            else emit_i32_const(wg, 0);
            bv_write_u8(&wg->out, WASM_OPCODE_IF);
            bv_write_u8(&wg->out, 0x40);
            if (n->child_count > 1) emit_stmt(wg, n->children[1]);
            if (n->child_count > 2 && n->children[2]) {
                bv_write_u8(&wg->out, WASM_OPCODE_ELSE);
                emit_stmt(wg, n->children[2]);
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
        case NODE_WHILE: {
            /* block { loop { cond; i32.eqz; br_if 1; body; br 0 } } */
            bv_write_u8(&wg->out, WASM_OPCODE_BLOCK);
            bv_write_u8(&wg->out, 0x40);
            bv_write_u8(&wg->out, WASM_OPCODE_LOOP);
            bv_write_u8(&wg->out, 0x40);
            if (n->child_count > 0) emit_expr(wg, n->children[0]);
            else emit_i32_const(wg, 1);
            bv_write_u8(&wg->out, WASM_OPCODE_I32_EQZ);
            bv_write_u8(&wg->out, WASM_OPCODE_BR_IF);
            bv_write_u8(&wg->out, 0x01);
            if (n->child_count > 1) emit_stmt(wg, n->children[1]);
            bv_write_u8(&wg->out, WASM_OPCODE_BR);
            bv_write_u8(&wg->out, 0x00);
            bv_write_u8(&wg->out, WASM_OPCODE_END);
            bv_write_u8(&wg->out, WASM_OPCODE_END);
            break;
        }
        case NODE_EMPTY:
            if (!n->data) {
                /* Block: 遍历子节点作为语句 */
                for (size_t i = 0; i < n->child_count; i++) {
                    emit_stmt(wg, n->children[i]);
                }
            } else if (strcmp((const char *)n->data, "assign") == 0 && n->child_count >= 2) {
                /* W1: x = expr → expr; local.set x (此前静默丢弃) */
                ASTNode *lhs = n->children[0];
                /* W3: 字段赋值 this.f = v / c.f = v / 裸字段 = v */
                if (lhs && lhs->type == NODE_IDENT && lhs->data) {
                    const char *recv = NULL; int32_t off = 0; int fs = 0;
                    const char *lname = (const char *)lhs->data;
                    /* c.f 赋值: 接收者是局部变量持有的实例 */
                    if (strchr(lname, '.') && strncmp(lname, "this.", 5) != 0) {
                        char r2[128], f2[128];
                        if (w3_split_dot(lname, r2, sizeof(r2), f2, sizeof(f2))) {
                            int li2 = wasm_local_lookup(wg, r2);
                            if (li2 >= 0 && li2 < 64 && wg->local_class[li2][0]) {
                                WasmClass *c2 = w3_find_class(wg->local_class[li2]);
                                int fi2 = c2 ? w3_field_idx(c2, f2) : -1;
                                if (fi2 >= 0) {
                                    w3_emit_recv(wg, r2);
                                    emit_expr(wg, n->children[1]);
                                    w3_emit_store(wg, c2->fields[fi2].offset);
                                    break;
                                }
                            }
                        }
                    }
                    if (w3_resolve_field(wg, lname, &recv, &off, &fs)) {
                        w3_emit_recv(wg, recv);
                        emit_expr(wg, n->children[1]);
                        w3_emit_store(wg, off);
                        break;
                    }
                }
                emit_expr(wg, n->children[1]);
                if (lhs && lhs->type == NODE_IDENT && lhs->data) {
                    int li = wasm_local_ensure(wg, (const char *)lhs->data);
                    if (li >= 0) {
                        bv_write_u8(&wg->out, WASM_OPCODE_LOCAL_SET);
                        bv_write_u32_leb128(&wg->out, (uint32_t)li);
                    } else {
                        bv_write_u8(&wg->out, WASM_OPCODE_DROP);
                    }
                } else {
                    bv_write_u8(&wg->out, WASM_OPCODE_DROP);
                }
            } else if ((strcmp((const char *)n->data, "add-assign") == 0 ||
                        strcmp((const char *)n->data, "sub-assign") == 0 ||
                        strcmp((const char *)n->data, "mul-assign") == 0 ||
                        strcmp((const char *)n->data, "div-assign") == 0) && n->child_count >= 2) {
                /* W1: 复合赋值 x += e → x = x + e */
                ASTNode *lhs = n->children[0];
                if (lhs && lhs->type == NODE_IDENT && lhs->data) {
                    int li = wasm_local_ensure(wg, (const char *)lhs->data);
                    if (li >= 0) {
                        bv_write_u8(&wg->out, WASM_OPCODE_LOCAL_GET);
                        bv_write_u32_leb128(&wg->out, (uint32_t)li);
                        emit_expr(wg, n->children[1]);
                        const char *d = (const char *)n->data;
                        unsigned char opc = WASM_OPCODE_I32_ADD;
                        if (d[0] == 's') opc = WASM_OPCODE_I32_SUB;
                        else if (d[0] == 'm') opc = WASM_OPCODE_I32_MUL;
                        else if (d[0] == 'd') opc = WASM_OPCODE_I32_DIV_S;
                        bv_write_u8(&wg->out, opc);
                        bv_write_u8(&wg->out, WASM_OPCODE_LOCAL_SET);
                        bv_write_u32_leb128(&wg->out, (uint32_t)li);
                    }
                }
            } else {
                /* 未知 NODE_EMPTY: 作为表达式求值并丢弃 */
                emit_expr(wg, n);
                bv_write_u8(&wg->out, WASM_OPCODE_DROP);
            }
            break;
        default:
            emit_expr(wg, n);
            bv_write_u8(&wg->out, WASM_OPCODE_DROP);
            break;
    }
}


/* ===== W3: 类方法体发射 =====
 * 每个方法是独立 wasm 函数: local0=self, local1..N=参数, 其后为方法体局部变量。
 * 参数来自函数类型 (不占 locals 声明); 额外 locals 从 nparam+1 起声明。
 * 返回值: 显式 return / 尾表达式 / ctor 返回 self / 兜底 0。
 */
static int w3_is_value_expr(ASTNode *n) {
    if (!n) return 0;
    switch (n->type) {
        case NODE_INT: case NODE_FLOAT: case NODE_STRING: case NODE_BOOL:
        case NODE_CHAR: case NODE_IDENT: case NODE_CALL: case NODE_INDEX_ACCESS:
            return 1;
        case NODE_EMPTY: {
            if (!n->data) return 0;
            const char *d = (const char *)n->data;
            static const char *ops[] = {"+","-","*","/","%","==","!=","<",">","<=",">=",
                                        "and","or","not","neg", NULL};
            for (int i = 0; ops[i]; i++) if (strcmp(d, ops[i]) == 0) return 1;
            return 0;
        }
        default: return 0;
    }
}

static void w3_emit_method_body(ByteVec *body, WasmGen *wg, int ci, int mi,
                                ASTNode *mnode) {
    WasmClass *c = &g_wasm_classes[ci];
    WasmMethod *me = &c->methods[mi];
    WasmGen fw = *wg;
    fw.out = (ByteVec){0};
    fw.cur_class = ci;
    memset(fw.local_is_str, 0, sizeof(fw.local_is_str));
    memset(fw.local_class, 0, sizeof(fw.local_class));

    /* 参数预注册: self=0, 参数=1..N (来自函数类型, 不声明 locals) */
    int nparams = me->nargs + 1;
    wasm_local_ensure(&fw, "this"); /* local 0 = self */
    /* 找 params 容器注册参数名 + String 标记 */
    if (mnode) {
        for (size_t k = 0; k < mnode->child_count; k++) {
            ASTNode *cc = mnode->children[k];
            if (cc && cc->type == NODE_EMPTY && cc->data &&
                strcmp((const char *)cc->data, "params") == 0) {
                for (size_t pi = 0; pi < cc->child_count && pi < 4; pi++) {
                    ASTNode *pm = cc->children[pi];
                    if (!pm || !pm->data) continue;
                    int li = wasm_local_ensure(&fw, (const char *)pm->data);
                    if (li >= 0 && li < 64 && pm->child_count > 0 &&
                        pm->children[0] && pm->children[0]->data &&
                        strcmp((const char *)pm->children[0]->data, "String") == 0) {
                        fw.local_is_str[li] = 1;
                    }
                }
                break;
            }
        }
    }

    /* 方法体: 除最后子节点外全部 stmt; 最后子节点按值/stmt 分派 */
    ASTNode *body_node = NULL;
    if (mnode) {
        /* body = 最后一个非 params/非返回类型 child */
        for (size_t k = mnode->child_count; k > 0; k--) {
            ASTNode *cc = mnode->children[k - 1];
            if (!cc) continue;
            if (cc->type == NODE_EMPTY && cc->data &&
                strcmp((const char *)cc->data, "params") == 0) continue;
            if (cc->type == NODE_IDENT) continue; /* 返回类型节点 */
            body_node = cc;
            break;
        }
    }
    if (body_node) {
        /* NODE_EMPTY(data=NULL) 即语句块: 逐语句; 单表达式体: 直接发射 */
        int blocklike = (body_node->type == NODE_EMPTY && !body_node->data);
        size_t nstmt = blocklike ? body_node->child_count : 1;
        ASTNode **stmts = blocklike ? body_node->children : &body_node;
        for (size_t k = 0; k < nstmt; k++) {
            ASTNode *st = stmts[k];
            if (!st) continue;
            if (k + 1 == nstmt && w3_is_value_expr(st)) {
                emit_expr(&fw, st); /* 尾表达式: 留栈为返回值 */
            } else {
                emit_stmt(&fw, st);
            }
        }
    }
    /* 尾部返回值判定: 非值尾部补 0; ctor 补 self */
    {
        int tail_is_value = 0;
        if (body_node) {
            int blocklike = (body_node->type == NODE_EMPTY && !body_node->data);
            ASTNode *last = NULL;
            if (blocklike && body_node->child_count > 0) last = body_node->children[body_node->child_count - 1];
            else if (!blocklike) last = body_node;
            tail_is_value = w3_is_value_expr(last);
            /* ctor: 返回 self 而非尾值 */
            if (me->is_ctor) tail_is_value = 0;
        }
        if (!tail_is_value) {
            if (me->is_ctor) {
                /* local.get 0 → 返回 self 指针 */
                bv_write_u8(&fw.out, WASM_OPCODE_LOCAL_GET);
                bv_write_u32_leb128(&fw.out, 0);
            } else {
                emit_i32_const(&fw, 0);
            }
        }
    }

    /* 函数体编码: params 占 local 0..nparams-1, 额外 locals 声明 */
    ByteVec code = {0};
    int extra = fw.local_count - nparams;
    if (extra > 0) {
        bv_write_u8(&code, 0x01);
        bv_write_u32_leb128(&code, (uint32_t)extra);
        bv_write_u8(&code, 0x7F);
    } else {
        bv_write_u8(&code, 0x00);
    }
    bv_write_raw(&code, fw.out.data, fw.out.size);
    bv_free(&fw.out);
    bv_write_u8(&code, WASM_OPCODE_END);
    bv_write_u32_leb128(body, (uint32_t)code.size);
    bv_write_raw(body, code.data, code.size);
    bv_free(&code);
    (void)nparams;
}

/* ===== W2: 字符串运行时 (手写 wasm 函数体) =====
 * 函数索引布局 (wasip2): main=5 print_i32=6 alloc=7 strlen=8 concat=9 itoa=10
 * slice=11 streq=12 print_str=13 find=14 find_from=15 field=16 (p3 各减 1)
 * 堆: global 0 (mut i32) bump 分配器, main 开头初始化为字面量区末尾 8 对齐 */
static void w8(ByteVec *v, unsigned char x) { bv_write_u8(v, x); }
static void wu32(ByteVec *v, uint32_t x) { bv_write_u32_leb128(v, x); }
static void wi32(ByteVec *v, int32_t x) { w8(v, 0x41); bv_write_i32_leb128(v, x); }
static void wl(ByteVec *v, unsigned char op, int idx) { w8(v, op); wu32(v, (uint32_t)idx); }
static void wfn(ByteVec *v, int idx) { w8(v, 0x10); wu32(v, (uint32_t)idx); }
static void wload8(ByteVec *v)  { w8(v, 0x2D); w8(v, 0x00); w8(v, 0x00); } /* align=0 off=0 */
static void wstore8(ByteVec *v) { w8(v, 0x3A); w8(v, 0x00); w8(v, 0x00); }
static void wstore32(ByteVec *v){ w8(v, 0x36); w8(v, 0x02); w8(v, 0x00); }
static void wload32(ByteVec *v) { w8(v, 0x28); w8(v, 0x02); w8(v, 0x00); } /* align=2 off=0 */
static void wi64(ByteVec *v, int64_t x) { w8(v, 0x42); bv_write_i32_leb128(v, (int32_t)x); }

/* 写一个函数体: size 前缀 + locals + 字节码 */
static void wfn_begin(ByteVec *body, ByteVec *code, int nlocals) {
    bv_free(code); *code = (ByteVec){0};
    if (nlocals > 0) { w8(code, 0x01); wu32(code, (uint32_t)nlocals); w8(code, 0x7F); }
    else w8(code, 0x00);
}
static void wfn_end(ByteVec *body, ByteVec *code) {
    w8(code, 0x0B); /* end */
    wu32(body, (uint32_t)code->size);
    bv_write_raw(body, code->data, code->size);
    bv_free(code); *code = (ByteVec){0};
}

static void w2_emit_runtime_bodies(ByteVec *body, WasmGen *wg) {
    ByteVec c = {0};

    /* alloc(n): n=(n+7)&~8; ptr=global0; global0+=n; return ptr */
    wfn_begin(body, &c, 0);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wi32(&c, 7); w8(&c, WASM_OPCODE_I32_ADD);
    wi32(&c, -8); w8(&c, WASM_OPCODE_I32_AND); wl(&c, WASM_OPCODE_LOCAL_SET, 0);
    wl(&c, WASM_OPCODE_GLOBAL_GET, 0);
    wl(&c, WASM_OPCODE_GLOBAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 0);
    w8(&c, WASM_OPCODE_I32_ADD); wl(&c, WASM_OPCODE_GLOBAL_SET, 0);
    wfn_end(body, &c); /* 栈上已有首个 global.get 留下的 ptr 作返回值 */

    /* strlen(p) */
    wfn_begin(body, &c, 1);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 1);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40);
    w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 1);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c);
    w8(&c, WASM_OPCODE_I32_EQZ); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 1);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1);
    wfn_end(body, &c);

    /* concat(a,b): la=2 lb=3 dst=4 i=5 */
    wfn_begin(body, &c, 4);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wfn(&c, wg->rt_strlen); wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wfn(&c, wg->rt_strlen); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wl(&c, WASM_OPCODE_LOCAL_GET, 3);
    w8(&c, WASM_OPCODE_I32_ADD); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wl(&c, WASM_OPCODE_LOCAL_GET, 2);
    w8(&c, WASM_OPCODE_I32_GE_U); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wl(&c, WASM_OPCODE_LOCAL_GET, 5); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 5); w8(&c, WASM_OPCODE_I32_ADD);
    wload8(&c); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wl(&c, WASM_OPCODE_LOCAL_GET, 3);
    w8(&c, WASM_OPCODE_I32_GE_U); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wl(&c, WASM_OPCODE_LOCAL_GET, 2); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wl(&c, WASM_OPCODE_LOCAL_GET, 5); w8(&c, WASM_OPCODE_I32_ADD);
    wload8(&c); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wl(&c, WASM_OPCODE_LOCAL_GET, 2); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); w8(&c, WASM_OPCODE_I32_ADD);
    wi32(&c, 0); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4);
    wfn_end(body, &c);

    /* itoa(v): p=1 i=2 neg=3 (INT_MIN 溢出边界文档标注) */
    wfn_begin(body, &c, 3);
    wi32(&c, 12); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 1);
    wi32(&c, 11); wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wl(&c, WASM_OPCODE_LOCAL_GET, 2); w8(&c, WASM_OPCODE_I32_ADD);
    wi32(&c, 0); wstore8(&c);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wi32(&c, 0); w8(&c, WASM_OPCODE_I32_LT_S);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 1); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 0); w8(&c, WASM_OPCODE_I32_SUB);
    wl(&c, WASM_OPCODE_LOCAL_SET, 0);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_SUB);
    wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wl(&c, WASM_OPCODE_LOCAL_GET, 2); w8(&c, WASM_OPCODE_I32_ADD);
    wi32(&c, 48); wstore8(&c);
    w8(&c, WASM_OPCODE_ELSE);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_SUB);
    wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wl(&c, WASM_OPCODE_LOCAL_GET, 2); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wi32(&c, 10); w8(&c, WASM_OPCODE_I32_REM_U);
    wi32(&c, 48); w8(&c, WASM_OPCODE_I32_ADD); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wi32(&c, 10); w8(&c, WASM_OPCODE_I32_DIV_U);
    wl(&c, WASM_OPCODE_LOCAL_SET, 0);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); w8(&c, WASM_OPCODE_I32_EQZ); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_SUB);
    wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wl(&c, WASM_OPCODE_LOCAL_GET, 2); w8(&c, WASM_OPCODE_I32_ADD);
    wi32(&c, 45); wstore8(&c);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wl(&c, WASM_OPCODE_LOCAL_GET, 2); w8(&c, WASM_OPCODE_I32_ADD);
    wfn_end(body, &c);

    

    /* slice(s,st,en): la=3 len=4 dst=5 i=6 — pony 语义: 越界截断, en<0→la */
    wfn_begin(body, &c, 4);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wfn(&c, wg->rt_strlen); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wi32(&c, 0); w8(&c, WASM_OPCODE_I32_LT_S);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 1); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wl(&c, WASM_OPCODE_LOCAL_GET, 3); w8(&c, WASM_OPCODE_I32_GT_S);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); wl(&c, WASM_OPCODE_LOCAL_SET, 1); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wi32(&c, 0); w8(&c, WASM_OPCODE_I32_LT_S);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); wl(&c, WASM_OPCODE_LOCAL_SET, 2); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wl(&c, WASM_OPCODE_LOCAL_GET, 3); w8(&c, WASM_OPCODE_I32_GT_S);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); wl(&c, WASM_OPCODE_LOCAL_SET, 2); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wl(&c, WASM_OPCODE_LOCAL_GET, 1); w8(&c, WASM_OPCODE_I32_SUB);
    w8(&c, WASM_OPCODE_LOCAL_TEE); wu32(&c, 4);
    wi32(&c, 0); w8(&c, WASM_OPCODE_I32_LT_S);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 4); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wl(&c, WASM_OPCODE_LOCAL_GET, 4);
    w8(&c, WASM_OPCODE_I32_GE_U); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wl(&c, WASM_OPCODE_LOCAL_GET, 6); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); w8(&c, WASM_OPCODE_I32_ADD);
    wload8(&c); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wl(&c, WASM_OPCODE_LOCAL_GET, 4); w8(&c, WASM_OPCODE_I32_ADD);
    wi32(&c, 0); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5);
    wfn_end(body, &c);

    /* streq(a,b): i=2 ca=3 cb=4 */
    wfn_begin(body, &c, 3);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 2); w8(&c, WASM_OPCODE_I32_ADD);
    wload8(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wl(&c, WASM_OPCODE_LOCAL_GET, 2); w8(&c, WASM_OPCODE_I32_ADD);
    wload8(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); wl(&c, WASM_OPCODE_LOCAL_GET, 4); w8(&c, WASM_OPCODE_I32_NE);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 0); w8(&c, WASM_OPCODE_RETURN); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 1); w8(&c, WASM_OPCODE_RETURN); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wi32(&c, 1);
    wfn_end(body, &c);

    /* print_str(s): fd_write(1, iov@8, 1, ret@24) */
    wfn_begin(body, &c, 0);
    wi32(&c, 1); wi32(&c, 8); wi32(&c, 1); wi32(&c, 24);
    wi32(&c, 8); wl(&c, WASM_OPCODE_LOCAL_GET, 0); wstore32(&c);
    wi32(&c, 12); wl(&c, WASM_OPCODE_LOCAL_GET, 0); wfn(&c, wg->rt_strlen);
    wstore32(&c);
    wfn(&c, 0); /* call fd_write (import 0) */
    w8(&c, WASM_OPCODE_DROP);
    wi32(&c, 0);
    wfn_end(body, &c);

    /* find(s,sub) = find_from(s,sub,0) */
    wfn_begin(body, &c, 0);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 1);
    wi32(&c, 0); wfn(&c, wg->rt_find_from);
    wfn_end(body, &c);

    /* find_from(s,sub,from): ls=3 lsub=4 i=5 j=6 ok=7 */
    wfn_begin(body, &c, 5);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wfn(&c, wg->rt_strlen); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wfn(&c, wg->rt_strlen); wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wi32(&c, 0); w8(&c, WASM_OPCODE_I32_LT_S);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 2); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wl(&c, WASM_OPCODE_LOCAL_GET, 3); w8(&c, WASM_OPCODE_I32_GT_U);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, -1); w8(&c, WASM_OPCODE_RETURN); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); w8(&c, WASM_OPCODE_RETURN); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wl(&c, WASM_OPCODE_LOCAL_GET, 4); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); w8(&c, WASM_OPCODE_I32_GT_U);
    w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    wi32(&c, 1); wl(&c, WASM_OPCODE_LOCAL_SET, 7);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wl(&c, WASM_OPCODE_LOCAL_GET, 4);
    w8(&c, WASM_OPCODE_I32_GE_U); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 5); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); w8(&c, WASM_OPCODE_I32_ADD); wload8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wl(&c, WASM_OPCODE_LOCAL_GET, 6); w8(&c, WASM_OPCODE_I32_ADD);
    wload8(&c);
    w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 1); /* if 块内 label0=if, label1=内层 loop → continue */
    w8(&c, WASM_OPCODE_END);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 7);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 1);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); w8(&c, WASM_OPCODE_RETURN); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wi32(&c, -1);
    wfn_end(body, &c);

    /* field(s,idx,sep): ls=3 lsep=4 cur=5 p=6 e=7 dst=8 len=9 — pny_str_field 语义 */
    wfn_begin(body, &c, 7);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wfn(&c, wg->rt_strlen); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wfn(&c, wg->rt_strlen); wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wl(&c, WASM_OPCODE_LOCAL_GET, 1);
    w8(&c, WASM_OPCODE_I32_GE_U); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 2);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wfn(&c, wg->rt_find_from);
    wl(&c, WASM_OPCODE_LOCAL_SET, 7);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, -1); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 1); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 8);
    wl(&c, WASM_OPCODE_LOCAL_GET, 8); wi32(&c, 0); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 8); w8(&c, WASM_OPCODE_RETURN);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wl(&c, WASM_OPCODE_LOCAL_GET, 4); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 2);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wfn(&c, wg->rt_find_from);
    wl(&c, WASM_OPCODE_LOCAL_SET, 7);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); wl(&c, WASM_OPCODE_LOCAL_GET, 6); w8(&c, WASM_OPCODE_I32_SUB);
    wl(&c, WASM_OPCODE_LOCAL_SET, 9);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, -1); w8(&c, WASM_OPCODE_I32_NE);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wl(&c, WASM_OPCODE_LOCAL_GET, 6); w8(&c, WASM_OPCODE_I32_SUB);
    wl(&c, WASM_OPCODE_LOCAL_SET, 9);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 9); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 8);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wl(&c, WASM_OPCODE_LOCAL_GET, 9);
    w8(&c, WASM_OPCODE_I32_GE_U); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 8); wl(&c, WASM_OPCODE_LOCAL_GET, 5); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 6); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); w8(&c, WASM_OPCODE_I32_ADD);
    wload8(&c); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 8); wl(&c, WASM_OPCODE_LOCAL_GET, 9); w8(&c, WASM_OPCODE_I32_ADD);
    wi32(&c, 0); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 8);
    wfn_end(body, &c);

    /* W4: chr(c): alloc(2); p[0]=c; p[1]=0; return p — local 1 */
    wfn_begin(body, &c, 1);
    wi32(&c, 2); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wl(&c, WASM_OPCODE_LOCAL_GET, 0); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wi32(&c, 0); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1);
    wfn_end(body, &c);

    /* W4: repl(s,old,new): res=3 pos=4 p=5 — 组合 find_from/slice/concat */
    wfn_begin(body, &c, 3);
    wi32(&c, 1); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); wi32(&c, 0); wstore8(&c);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wfn(&c, wg->rt_find_from);
    wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wi32(&c, -1); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 4);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wfn(&c, wg->rt_strlen);
    wfn(&c, wg->rt_slice); wfn(&c, wg->rt_concat); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 2);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 4);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wfn(&c, wg->rt_slice);
    wfn(&c, wg->rt_concat); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); wl(&c, WASM_OPCODE_LOCAL_GET, 2);
    wfn(&c, wg->rt_concat); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wl(&c, WASM_OPCODE_LOCAL_GET, 1);
    wfn(&c, wg->rt_strlen); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3);
    wfn_end(body, &c);

    /* W4: json(s,key): pat=2 pos=3 p=4 after=5 i=6 ch=7 depth=8 in_str=9 esc=10 c=11 q=12
     * pny_json_raw_get 语义: 找 "key": 后按 {[/引号/标量 提取原始值 */
    wfn_begin(body, &c, 11);
    /* pat = q + (key + q), q=chr(34) */
    wi32(&c, 34); wfn(&c, wg->rt_chr); wl(&c, WASM_OPCODE_LOCAL_SET, 12);
    wl(&c, WASM_OPCODE_LOCAL_GET, 12);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wl(&c, WASM_OPCODE_LOCAL_GET, 12);
    wfn(&c, wg->rt_concat); wfn(&c, wg->rt_concat);
    wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    /* 搜索循环: find_from 直到后随 ':' */
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 2);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); wfn(&c, wg->rt_find_from);
    wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wi32(&c, -1); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 1); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_TEE, 12);
    wi32(&c, 0); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 12); w8(&c, WASM_OPCODE_RETURN);
    w8(&c, WASM_OPCODE_END);
    /* after = p + len(pat); 跳空白 */
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wl(&c, WASM_OPCODE_LOCAL_GET, 2);
    wfn(&c, wg->rt_strlen); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 5);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 7);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 32); w8(&c, WASM_OPCODE_I32_EQ);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 9); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 10); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 13); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    w8(&c, WASM_OPCODE_I32_EQZ); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    /* s[after]==':' → 跳出搜索循环 */
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 5);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c); wi32(&c, 58);
    w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 2);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    /* p = after+1; 再跳空白 (含前导冒号后的空白) */
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 4);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 7);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 32); w8(&c, WASM_OPCODE_I32_EQ);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 9); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 10); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 13); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    w8(&c, WASM_OPCODE_I32_EQZ); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    /* c = s[p] */
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 4);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 11);
    /* 分支1: { 或 [ → 深度扫描 */
    wl(&c, WASM_OPCODE_LOCAL_GET, 11); wi32(&c, 123); w8(&c, WASM_OPCODE_I32_EQ);
    wl(&c, WASM_OPCODE_LOCAL_GET, 11); wi32(&c, 91); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 8);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 9);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 10);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 6);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 7);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 1); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_TEE, 12);
    wi32(&c, 0); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 12); w8(&c, WASM_OPCODE_RETURN);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 9);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 10);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 10);
    w8(&c, WASM_OPCODE_ELSE);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 92); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 1); wl(&c, WASM_OPCODE_LOCAL_SET, 10);
    w8(&c, WASM_OPCODE_ELSE);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 34); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 9);
    w8(&c, WASM_OPCODE_END);
    w8(&c, WASM_OPCODE_END);
    w8(&c, WASM_OPCODE_END);
    w8(&c, WASM_OPCODE_ELSE);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 34); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 1); wl(&c, WASM_OPCODE_LOCAL_SET, 9);
    w8(&c, WASM_OPCODE_ELSE);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 123); w8(&c, WASM_OPCODE_I32_EQ);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 91); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 8); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 8);
    w8(&c, WASM_OPCODE_ELSE);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 125); w8(&c, WASM_OPCODE_I32_EQ);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 93); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 8); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_SUB);
    wl(&c, WASM_OPCODE_LOCAL_SET, 8);
    wl(&c, WASM_OPCODE_LOCAL_GET, 8); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 4);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wfn(&c, wg->rt_slice); w8(&c, WASM_OPCODE_RETURN);
    w8(&c, WASM_OPCODE_END);
    w8(&c, WASM_OPCODE_END);
    w8(&c, WASM_OPCODE_END);
    w8(&c, WASM_OPCODE_END);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    w8(&c, WASM_OPCODE_END); /* 分支1 END */
    /* 分支2: 引号串 */
    wl(&c, WASM_OPCODE_LOCAL_GET, 11); wi32(&c, 34); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 6);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 7);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 2);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 92); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 34); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 4);
    wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6);
    wfn(&c, wg->rt_slice); w8(&c, WASM_OPCODE_RETURN);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    w8(&c, WASM_OPCODE_END); /* 分支2 END */
    /* 分支3: 标量 — 扫到 , } ] 或 NUL, 尾部去空白 */
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 6);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 7);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 44); w8(&c, WASM_OPCODE_I32_EQ);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 125); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 93); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 2);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    /* 尾部去空白: while i>p && s[i-1]∈空白: i-- */
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wl(&c, WASM_OPCODE_LOCAL_GET, 4);
    w8(&c, WASM_OPCODE_I32_GT_S); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 6);
    wi32(&c, 1); w8(&c, WASM_OPCODE_I32_SUB); w8(&c, WASM_OPCODE_I32_ADD);
    wload8(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 7);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 32); w8(&c, WASM_OPCODE_I32_EQ);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 9); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 10); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 13); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_I32_OR);
    w8(&c, WASM_OPCODE_I32_EQZ); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_SUB);
    wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 4);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6);
    wfn(&c, wg->rt_slice); w8(&c, WASM_OPCODE_RETURN);
    wfn_end(body, &c);

    /* ===== W4b: WASI 系统接口运行时 (scratch: 28/32/36, iov 8/12/24) ===== */

    /* envget(name): locals 1=cnt 2=ptrs 3=buf 4=i 5=p 6=j 7=ok 8=c */
    wfn_begin(body, &c, 8);
    wi32(&c, 28); wi32(&c, 32); wfn(&c, wg->imp_env_sizes); w8(&c, WASM_OPCODE_DROP);
    wi32(&c, 28); wload32(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wi32(&c, 4); w8(&c, WASM_OPCODE_I32_MUL);
    wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    wi32(&c, 32); wload32(&c); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wl(&c, WASM_OPCODE_LOCAL_GET, 3);
    wfn(&c, wg->imp_env_get); w8(&c, WASM_OPCODE_DROP);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wl(&c, WASM_OPCODE_LOCAL_GET, 1);
    w8(&c, WASM_OPCODE_I32_GE_U); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wl(&c, WASM_OPCODE_LOCAL_GET, 4);
    wi32(&c, 4); w8(&c, WASM_OPCODE_I32_MUL); w8(&c, WASM_OPCODE_I32_ADD);
    wload32(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    wi32(&c, 1); wl(&c, WASM_OPCODE_LOCAL_SET, 7);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 6);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 8);
    wl(&c, WASM_OPCODE_LOCAL_GET, 8); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wl(&c, WASM_OPCODE_LOCAL_GET, 6);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 8); w8(&c, WASM_OPCODE_I32_NE);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 7);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 2);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wl(&c, WASM_OPCODE_LOCAL_GET, 6);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c); wi32(&c, 61); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wl(&c, WASM_OPCODE_LOCAL_GET, 6);
    w8(&c, WASM_OPCODE_I32_ADD); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    w8(&c, WASM_OPCODE_RETURN);
    w8(&c, WASM_OPCODE_END);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wi32(&c, 1); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wi32(&c, 0); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5);
    wfn_end(body, &c);

    /* fexists(path): resolve → preopen fd+relpath; path_filestat_get errno==0 → 1 */
    wfn_begin(body, &c, 2);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wi32(&c, 48);
    wfn(&c, wg->rt_resolve); wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 0); w8(&c, WASM_OPCODE_RETURN);
    w8(&c, WASM_OPCODE_END);
    wi32(&c, 64); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 1);
    wi32(&c, 48); wload32(&c); wi32(&c, 0);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wl(&c, WASM_OPCODE_LOCAL_GET, 2);
    wfn(&c, wg->rt_strlen); wl(&c, WASM_OPCODE_LOCAL_GET, 1);
    wfn(&c, wg->imp_path_fstat);
    wi32(&c, 0); w8(&c, WASM_OPCODE_I32_EQ);
    wfn_end(body, &c);

    /* fread(path): locals 1=fd 2=sb 3=sz 4=buf 5=total 6=rel — 循环 fd_read 防短读 */
    wfn_begin(body, &c, 6);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wi32(&c, 48);
    wfn(&c, wg->rt_resolve); wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 1); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wi32(&c, 0); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); w8(&c, WASM_OPCODE_RETURN);
    w8(&c, WASM_OPCODE_END);
    wi32(&c, 48); wload32(&c); wi32(&c, 0);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wl(&c, WASM_OPCODE_LOCAL_GET, 6);
    wfn(&c, wg->rt_strlen); wi32(&c, 0);
    wi64(&c, 0x200026); /* FD_READ|FD_SEEK|FD_TELL|FD_FILESTAT_GET */
    wi64(&c, 0);
    wi32(&c, 0); wi32(&c, 36);
    wfn(&c, wg->imp_path_open);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 1); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wi32(&c, 0); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); w8(&c, WASM_OPCODE_RETURN);
    w8(&c, WASM_OPCODE_END);
    wi32(&c, 36); wload32(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 1);
    wi32(&c, 72); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wl(&c, WASM_OPCODE_LOCAL_GET, 2);
    wfn(&c, wg->imp_fd_fstat); w8(&c, WASM_OPCODE_DROP);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wi32(&c, 32); w8(&c, WASM_OPCODE_I32_ADD);
    wload32(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); wi32(&c, 1); w8(&c, WASM_OPCODE_I32_ADD);
    wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40); w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wl(&c, WASM_OPCODE_LOCAL_GET, 3);
    w8(&c, WASM_OPCODE_I32_GE_U); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wi32(&c, 8); wl(&c, WASM_OPCODE_LOCAL_GET, 4); wl(&c, WASM_OPCODE_LOCAL_GET, 5);
    w8(&c, WASM_OPCODE_I32_ADD); wstore32(&c);
    wi32(&c, 12); wl(&c, WASM_OPCODE_LOCAL_GET, 3); wl(&c, WASM_OPCODE_LOCAL_GET, 5);
    w8(&c, WASM_OPCODE_I32_SUB); wstore32(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wi32(&c, 8); wi32(&c, 1); wi32(&c, 24);
    wfn(&c, wg->imp_fd_read); w8(&c, WASM_OPCODE_DROP);
    wi32(&c, 24); wload32(&c); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wi32(&c, 24); wload32(&c);
    w8(&c, WASM_OPCODE_I32_ADD); wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wl(&c, WASM_OPCODE_LOCAL_GET, 5);
    w8(&c, WASM_OPCODE_I32_ADD); wi32(&c, 0); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wfn(&c, wg->imp_fd_close);
    w8(&c, WASM_OPCODE_DROP);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4);
    wfn_end(body, &c);

    /* fappend(path, data): resolve → O_CREAT 打开 → fd_seek(END) → 1/0
     * (wasmtime 25 的 fdflags=APPEND 不生效，写从 0 开始覆盖——fd_seek 官方语义兜底) */
    wfn_begin(body, &c, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wi32(&c, 48);
    wfn(&c, wg->rt_resolve); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 0); w8(&c, WASM_OPCODE_RETURN);
    w8(&c, WASM_OPCODE_END);
    wi32(&c, 48); wload32(&c); wi32(&c, 0);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); wl(&c, WASM_OPCODE_LOCAL_GET, 3);
    wfn(&c, wg->rt_strlen); wi32(&c, 1);
    wi64(&c, 0x200048); /* FD_WRITE|FD_SEEK|FD_FILESTAT_GET */
    wi64(&c, 0);
    wi32(&c, 0); wi32(&c, 36); /* fdflags=0 (APPEND 在 wasmtime25 不生效, 下面 fd_seek) */
    wfn(&c, wg->imp_path_open);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 0); w8(&c, WASM_OPCODE_RETURN);
    w8(&c, WASM_OPCODE_END);
    wi32(&c, 36); wload32(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    /* fd_seek(fd, 0, SEEK_END=2, &newoff@32) → errno */
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wi64(&c, 0); wi32(&c, 2); wi32(&c, 32);
    wfn(&c, wg->imp_fd_seek);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wfn(&c, wg->imp_fd_close);
    w8(&c, WASM_OPCODE_DROP);
    wi32(&c, 0); w8(&c, WASM_OPCODE_RETURN);
    w8(&c, WASM_OPCODE_END);
    wi32(&c, 8); wl(&c, WASM_OPCODE_LOCAL_GET, 1); wstore32(&c);
    wi32(&c, 12); wl(&c, WASM_OPCODE_LOCAL_GET, 1); wfn(&c, wg->rt_strlen);
    wstore32(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wi32(&c, 8); wi32(&c, 1); wi32(&c, 24);
    wfn(&c, 0); /* fd_write */
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wfn(&c, wg->imp_fd_close);
    w8(&c, WASM_OPCODE_DROP);
    wi32(&c, 0); w8(&c, WASM_OPCODE_I32_EQ);
    wfn_end(body, &c);

    /* sysexec(cmd): wasm 端安全返回 "" (ponydb printenv 有 fallback) */
    wfn_begin(body, &c, 1);
    wi32(&c, 1); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wi32(&c, 0); wstore8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1);
    wfn_end(body, &c);

    /* ===== W4b-fix: resolve(path, out_fd_ptr) → rel_ptr | 0 =====
     * preopen 最长前缀匹配 (wasi-libc __wasilibc_find_relpath 同款)
     * locals: 2=fd 3=best_fd 4=best_len 5=best_rel 6=nm 7=nlen 8=saved_hp 9=tmp 10=ok
     * arena 技巧: 保存/恢复堆指针 → namebuf 零泄漏 */
    wfn_begin(body, &c, 9);
    wl(&c, WASM_OPCODE_GLOBAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 8);
    wi32(&c, 256); wfn(&c, wg->rt_alloc); wl(&c, WASM_OPCODE_LOCAL_SET, 6);
    wi32(&c, 3); wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40);
    w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wi32(&c, 40);
    w8(&c, WASM_OPCODE_I32_GT_U); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wi32(&c, 40);
    wfn(&c, wg->imp_prestat_get); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40);
    wi32(&c, 40); wload8(&c); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 0);
    wi32(&c, 44); wload32(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 7);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 0);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wi32(&c, 256);
    w8(&c, WASM_OPCODE_I32_GE_U); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 0);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wl(&c, WASM_OPCODE_LOCAL_GET, 6);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wfn(&c, wg->imp_prestat_name);
    w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 0);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 9);
    w8(&c, WASM_OPCODE_BLOCK); w8(&c, 0x40);
    w8(&c, WASM_OPCODE_LOOP); w8(&c, 0x40);
    wl(&c, WASM_OPCODE_LOCAL_GET, 9); wl(&c, WASM_OPCODE_LOCAL_GET, 7);
    w8(&c, WASM_OPCODE_I32_GE_U); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 1);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 9);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wl(&c, WASM_OPCODE_LOCAL_GET, 9);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c);
    w8(&c, WASM_OPCODE_I32_NE); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 2);
    wl(&c, WASM_OPCODE_LOCAL_GET, 9); wi32(&c, 1);
    w8(&c, WASM_OPCODE_I32_ADD); wl(&c, WASM_OPCODE_LOCAL_SET, 9);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 7);
    w8(&c, WASM_OPCODE_I32_ADD); wload8(&c); wl(&c, WASM_OPCODE_LOCAL_SET, 9);
    wi32(&c, 0); wl(&c, WASM_OPCODE_LOCAL_SET, 10);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wl(&c, WASM_OPCODE_LOCAL_GET, 7);
    wi32(&c, 1); w8(&c, WASM_OPCODE_I32_SUB); w8(&c, WASM_OPCODE_I32_ADD);
    wload8(&c); wi32(&c, 47); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 1); wl(&c, WASM_OPCODE_LOCAL_SET, 10);
    w8(&c, WASM_OPCODE_ELSE);
    wl(&c, WASM_OPCODE_LOCAL_GET, 9); wi32(&c, 47); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 1); wl(&c, WASM_OPCODE_LOCAL_SET, 10);
    w8(&c, WASM_OPCODE_ELSE);
    wl(&c, WASM_OPCODE_LOCAL_GET, 9); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 1); wl(&c, WASM_OPCODE_LOCAL_SET, 10);
    w8(&c, WASM_OPCODE_END);
    w8(&c, WASM_OPCODE_END);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 10); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 0);
    wl(&c, WASM_OPCODE_LOCAL_GET, 4); wl(&c, WASM_OPCODE_LOCAL_GET, 7);
    w8(&c, WASM_OPCODE_I32_GT_U); w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 0);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wl(&c, WASM_OPCODE_LOCAL_SET, 3);
    wl(&c, WASM_OPCODE_LOCAL_GET, 7); wl(&c, WASM_OPCODE_LOCAL_SET, 4);
    wl(&c, WASM_OPCODE_LOCAL_GET, 0); wl(&c, WASM_OPCODE_LOCAL_GET, 7);
    w8(&c, WASM_OPCODE_I32_ADD); wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    wl(&c, WASM_OPCODE_LOCAL_GET, 6); wl(&c, WASM_OPCODE_LOCAL_GET, 7);
    wi32(&c, 1); w8(&c, WASM_OPCODE_I32_SUB); w8(&c, WASM_OPCODE_I32_ADD);
    wload8(&c); wi32(&c, 47); w8(&c, WASM_OPCODE_I32_EQ);
    w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 0);
    wl(&c, WASM_OPCODE_LOCAL_GET, 9); wi32(&c, 47); w8(&c, WASM_OPCODE_I32_NE);
    w8(&c, WASM_OPCODE_BR_IF); wu32(&c, 0);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5); wi32(&c, 1);
    w8(&c, WASM_OPCODE_I32_ADD); wl(&c, WASM_OPCODE_LOCAL_SET, 5);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 2); wi32(&c, 1);
    w8(&c, WASM_OPCODE_I32_ADD); wl(&c, WASM_OPCODE_LOCAL_SET, 2);
    w8(&c, WASM_OPCODE_BR); wu32(&c, 0);
    w8(&c, WASM_OPCODE_END); w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 8); wl(&c, WASM_OPCODE_GLOBAL_SET, 0);
    wl(&c, WASM_OPCODE_LOCAL_GET, 3); w8(&c, WASM_OPCODE_I32_EQZ);
    w8(&c, WASM_OPCODE_IF); w8(&c, 0x40);
    wi32(&c, 0); w8(&c, WASM_OPCODE_RETURN);
    w8(&c, WASM_OPCODE_END);
    wl(&c, WASM_OPCODE_LOCAL_GET, 1); wl(&c, WASM_OPCODE_LOCAL_GET, 3);
    wstore32(&c);
    wl(&c, WASM_OPCODE_LOCAL_GET, 5);
    wfn_end(body, &c);

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

    /* W3: 类收集必须先于 type/func 段发射 (段计数依赖类方法数) */
    {
        int32_t b = 15; /* W4b-fix: 14 imports → 统一 print=15 */
        w3_collect_classes(ast, b + 20);
    }

    /* --- type section --- */
    bv_write_u8(&bv, 0x01);
    {
        ByteVec body = {0};
        bv_write_u8(&body, 0x0C); /* 12 types (0-4 + W2: 5/6/7 + W3: 8 + W4b: 9/10 + fd_seek:11) */
        /* type 0: (func (result i32)) */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x00);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        /* type 1: fd_write/fd_read/random_get signature (i32,i32,i32,i32)->i32 */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x04);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        /* type 2: proc_exit signature (i32)->void */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x00);
        /* type 3: clock_time_get signature (i32,i64,i32)->i32 */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x03);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7E); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        /* type 4: random_get signature (i32,i32)->i32 */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x02);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        /* W2: type 5 = (i32)->i32 [alloc/strlen/itoa/print_str] */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        /* W2: type 6 = (i32,i32)->i32 [concat/streq/find] */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x02);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        /* W2: type 7 = (i32,i32,i32)->i32 [slice/find_from/field] */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x03);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        /* W3: type 8 = (i32,i32,i32,i32)->i32 [3参方法 + self] */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x04);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        /* W4b: type 9 = (i32,i32,i32,i32,i32,i64,i64,i32,i32)->i32 [path_open] */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x09);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x7E); bv_write_u8(&body, 0x7E);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        /* W4b: type 10 = (i32,i32,i32,i32,i32)->i32 [path_filestat_get] */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x05);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7F);
        bv_write_u8(&body, 0x01);
        bv_write_u8(&body, 0x7F);
        /* W4b-fix: type 11 = (i32,i64,i32,i32)->i32 [fd_seek] */
        bv_write_u8(&body, 0x60);
        bv_write_u8(&body, 0x04);
        bv_write_u8(&body, 0x7F); bv_write_u8(&body, 0x7E);
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
            /* WASI P3: fd_write + proc_exit + async_spawn + async_await
             * W4b: + fd_read + 6 系统接口 (与 P2 统一 b=12) */
            bv_write_u8(&body, 0x0E); /* 14 imports */
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
            /* W4b: P3 补 fd_read + 系统接口 (索引布局与 P2 对齐, fd_read=2 位置不同) */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_read");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x01);
            bv_write_str(&body, module_name);
            bv_write_str(&body, "environ_sizes_get");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x04);
            bv_write_str(&body, module_name);
            bv_write_str(&body, "environ_get");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x04);
            bv_write_str(&body, module_name);
            bv_write_str(&body, "path_open");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x09);
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_close");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x05);
            bv_write_str(&body, module_name);
            bv_write_str(&body, "path_filestat_get");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x0A);
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_filestat_get");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x06);
            /* W4b-fix: preopen 绝对路径解析 (wasi-libc 同款) */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_prestat_get");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x04); /* (i32,i32)->i32 */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_prestat_dir_name");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x07); /* (i32,i32,i32)->i32 */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_seek");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x0B); /* type 11: (i32,i64,i32,i32)->i32 */
        } else {
            /* WASI Preview 1 (P2): fd_write + proc_exit + fd_read + clock_time_get + random_get
             * W4b: + environ_sizes_get/environ_get/path_open/fd_close/path_filestat_get/fd_filestat_get */
            bv_write_u8(&body, 0x0E); /* 14 imports */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_write");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x01);
            bv_write_str(&body, module_name);
            bv_write_str(&body, "proc_exit");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x02);
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_read");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x01); /* same sig as fd_write: (i32,i32,i32,i32)->i32 */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "clock_time_get");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x03); /* (i32,i64,i32)->i32 */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "random_get");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x04); /* type 4: (i32,i32)->i32 */
            /* W4b: WASI 系统接口 import (索引 5-10) */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "environ_sizes_get");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x04); /* type 4: (i32,i32)->i32 */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "environ_get");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x04); /* type 4: (i32,i32)->i32 */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "path_open");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x09); /* W4b type 9: (5×i32,2×i64,2×i32)->i32 */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_close");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x05); /* type 5: (i32)->i32 */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "path_filestat_get");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x0A); /* W4b type 10: (5×i32)->i32 */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_filestat_get");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x06); /* type 6: (i32,i32)->i32 */
            /* W4b-fix: preopen 绝对路径解析 (wasi-libc 同款), 索引 11/12 */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_prestat_get");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x04); /* type 4: (i32,i32)->i32 */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_prestat_dir_name");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x07); /* type 7: (i32,i32,i32)->i32 */
            bv_write_str(&body, module_name);
            bv_write_str(&body, "fd_seek");
            bv_write_u8(&body, 0x00);
            bv_write_u8(&body, 0x0B); /* type 11 */
        }
        bv_write_vec(&bv, &body);
        bv_free(&body);
    }

    /* --- function section --- */
    bv_write_u8(&bv, 0x03);
    {
        ByteVec body = {0};
        /* W2: main + print_i32 + 10 字符串运行时函数; W4: +3; W4b: +5; W3: + 类方法 */
        {
            int ncf = 0;
            for (int ci = 0; ci < g_wasm_nclasses; ci++) ncf += g_wasm_classes[ci].nmethods;
            bv_write_u32_leb128(&body, (uint32_t)(21 + ncf));
        }
        bv_write_u8(&body, 0x00); /* main -> type 0 */
        bv_write_u8(&body, 0x00); /* print_i32 -> type 0 */
        bv_write_u8(&body, 0x05); /* alloc -> type 5 */
        bv_write_u8(&body, 0x05); /* strlen -> type 5 */
        bv_write_u8(&body, 0x06); /* concat -> type 6 */
        bv_write_u8(&body, 0x05); /* itoa -> type 5 */
        bv_write_u8(&body, 0x07); /* slice -> type 7 */
        bv_write_u8(&body, 0x06); /* streq -> type 6 */
        bv_write_u8(&body, 0x05); /* print_str -> type 5 */
        bv_write_u8(&body, 0x06); /* find -> type 6 */
        bv_write_u8(&body, 0x07); /* find_from -> type 7 */
        bv_write_u8(&body, 0x07); /* field -> type 7 */
        bv_write_u8(&body, 0x05); /* chr -> type 5 */
        bv_write_u8(&body, 0x07); /* repl -> type 7 */
        bv_write_u8(&body, 0x06); /* json -> type 6 */
        /* W4b: 系统接口运行时 */
        bv_write_u8(&body, 0x05); /* envget -> type 5 (i32)->i32 */
        bv_write_u8(&body, 0x05); /* fexists -> type 5 */
        bv_write_u8(&body, 0x05); /* fread -> type 5 */
        bv_write_u8(&body, 0x06); /* fappend -> type 6 (i32,i32)->i32 */
        bv_write_u8(&body, 0x05); /* sysexec -> type 5 (安全返回 "") */
        bv_write_u8(&body, 0x06); /* resolve -> type 6 (i32,i32)->i32 */
        /* W3: 类方法类型 (self+0..3 参数) → type 5/6/7/8 */
        for (int ci = 0; ci < g_wasm_nclasses; ci++) {
            for (int mi = 0; mi < g_wasm_classes[ci].nmethods; mi++) {
                int np = g_wasm_classes[ci].methods[mi].nargs; /* 不含 self */
                static const unsigned char ty[4] = {0x05, 0x06, 0x07, 0x08};
                bv_write_u8(&body, ty[np < 3 ? np : 3]);
            }
        }
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
    bv_write_u8(&bv, 0x10); /* W2: min = 16 pages (1MB, bump 堆) */

    /* --- global section (W2: 堆指针 global 0, mut i32, init 0) --- */
    bv_write_u8(&bv, 0x06);
    bv_write_u8(&bv, 0x06); /* section size */
    bv_write_u8(&bv, 0x01); /* 1 global */
    bv_write_u8(&bv, 0x7F); /* i32 */
    bv_write_u8(&bv, 0x01); /* mut */
    bv_write_u8(&bv, WASM_OPCODE_I32_CONST);
    bv_write_i32_leb128(&bv, 0);
    bv_write_u8(&bv, WASM_OPCODE_END);

    /* --- export section --- */
    /* Bug#31: WASI fd_write 要求 memory export ("missing required memory export") */
    bv_write_u8(&bv, 0x07);
    {
        ByteVec body = {0};
        bv_write_u8(&body, 0x03); /* 3 exports: memory + main + _start (Bug#46: wasmtime run 需 _start) */
        bv_write_str(&body, "memory");
        bv_write_u8(&body, 0x02); /* export kind: memory */
        bv_write_u8(&body, 0x00); /* memory index 0 */
        bv_write_str(&body, "main");
        bv_write_u8(&body, 0x00);
        /* main 的函数索引 = import 数量 (fd_write=0, proc_exit=1, ...; W4b: 11 imports) */
        int32_t export_idx = 14;
        bv_write_u32_leb128(&body, (uint32_t)export_idx);
        /* Bug#46: _start 别名 — wasmtime run 的命令入口 */
        bv_write_str(&body, "_start");
        bv_write_u8(&body, 0x00);
        bv_write_u32_leb128(&body, (uint32_t)export_idx);
        bv_write_vec(&bv, &body);
        bv_free(&body);
    }

    WasmGen wg = {0};
    wg.cur_class = -1;
    wg.out = bv;
    wg.print_func_idx = 15; /* W4b-fix: 14 imports → main=14, print=15; P2/P3 统一 */
    /* W4b: WASI 系统接口 import 索引 (P2/P3 共用 5-10; fd_read 位置不同) */
    wg.imp_env_sizes = 5; wg.imp_env_get = 6; wg.imp_path_open = 7;
    wg.imp_fd_close = 8; wg.imp_path_fstat = 9; wg.imp_fd_fstat = 10;
    wg.imp_fd_read = (target == TARGET_WASI_P3) ? 4 : 2;
    /* W4b-fix: preopen 解析 import (索引 11/12, 两 target 统一) */
    wg.imp_prestat_get = 11; wg.imp_prestat_name = 12; wg.imp_fd_seek = 13;
    wg.next_str_addr = 64; /* 字符串从地址 64 开始，避开 iovec 区域 (8-24) */
    /* W2: 运行时函数索引 (main=base, print_i32=base+1, 之后 10 个) */
    {
        int32_t b = wg.print_func_idx; /* print_i32; main=b-1, alloc=b+1 ... */
        wg.rt_alloc = b + 1; wg.rt_strlen = b + 2; wg.rt_concat = b + 3;
        wg.rt_itoa = b + 4; wg.rt_slice = b + 5; wg.rt_streq = b + 6;
        wg.rt_print_str = b + 7; wg.rt_find = b + 8; wg.rt_find_from = b + 9;
        wg.rt_field = b + 10;
        /* W4: chr/repl/json */
        wg.rt_chr = b + 11; wg.rt_repl = b + 12; wg.rt_json = b + 13;
        /* W4b: WASI 系统接口运行时 (b+14..b+18) */
        wg.rt_envget = b + 14; wg.rt_fexists = b + 15; wg.rt_fread = b + 16;
        wg.rt_fappend = b + 17; wg.rt_sysexec = b + 18;
        /* W4b-fix: preopen 绝对路径解析 (b+19; 类方法从 b+20 起) */
        wg.rt_resolve = b + 19;
    }

    /* 预扫描 AST，收集字符串 */
    if (ast) wasm_collect_strings(&wg, ast);

    /* --- code section --- */
    bv_write_u8(&bv, 0x0A);
    {
        ByteVec body = {0};

        {
            int ncf = 0;
            for (int ci = 0; ci < g_wasm_nclasses; ci++) ncf += g_wasm_classes[ci].nmethods;
            bv_write_u32_leb128(&body, (uint32_t)(21 + ncf)); /* W2:12 + W4a:3 + W4b:5 + W3: 类方法 */
        }

        /* func 0: main (Bug#33: 单 WasmGen 累积 locals + locals 声明头) */
        {
            WasmGen fw = wg;
            fw.out = (ByteVec){0};
            fw.local_count = 0;
            fw.cur_class = -1; /* W3: main 不属于任何类 */
            memset(fw.local_is_str, 0, sizeof(fw.local_is_str));
            memset(fw.local_class, 0, sizeof(fw.local_class));

            /* W2: 堆指针初始化 — global 0 = 字面量末尾 8 对齐 (main 体第一条指令) */
            {
                int32_t heap_base = (int32_t)(((uint32_t)fw.next_str_addr + 7u) & ~(uint32_t)7);
                emit_i32_const(&fw, heap_base);
                bv_write_u8(&fw.out, WASM_OPCODE_GLOBAL_SET);
                bv_write_u32_leb128(&fw.out, 0);
            }

            if (ast) {
                for (size_t i = 0; i < ast->child_count; i++) {
                    ASTNode *ch = ast->children[i];
                    if (!ch || ch->type != NODE_ACTOR) continue;
                    for (size_t j = 0; j < ch->child_count; j++) {
                        ASTNode *m = ch->children[j];
                        if (!m) continue;
                        if (m->type != NODE_NEW && m->type != NODE_FUN && m->type != NODE_BE) continue;
                        if (!m->data || strcmp((const char *)m->data, "main") != 0) continue;
                        for (size_t k = 0; k < m->child_count; k++) {
                            ASTNode *c = m->children[k];
                            if (!c) continue;
                            emit_stmt(&fw, c);
                        }
                    }
                }
            }

            ByteVec code = {0};
            if (fw.local_count > 0) {
                /* 1 组 locals: N × i32 */
                bv_write_u8(&code, 0x01);
                bv_write_u32_leb128(&code, (uint32_t)fw.local_count);
                bv_write_u8(&code, 0x7F);
            } else {
                bv_write_u8(&code, 0x00);
            }
            bv_write_raw(&code, fw.out.data, fw.out.size);
            bv_free(&fw.out);
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

        /* W2: func 2-11 字符串运行时 */
        w2_emit_runtime_bodies(&body, &wg);

        /* W3: 类方法函数体 (func 12+) — 需与 AST 方法节点按收集顺序对应 */
        if (ast) {
            for (size_t i = 0; i < ast->child_count; i++) {
                ASTNode *ch = ast->children[i];
                if (!ch || ch->type != NODE_ACTOR || !ch->data) continue;
                if (strcmp((const char *)ch->data, "main") == 0) continue;
                WasmClass *c = w3_find_class((const char *)ch->data);
                if (!c) continue;
                int mi = 0;
                for (size_t j = 0; j < ch->child_count && mi < c->nmethods; j++) {
                    ASTNode *m = ch->children[j];
                    if (!m) continue;
                    if (m->type != NODE_FUN && m->type != NODE_NEW && m->type != NODE_BE) continue;
                    int ci = (int)(c - g_wasm_classes);
                    w3_emit_method_body(&body, &wg, ci, mi, m);
                    mi++;
                }
            }
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
