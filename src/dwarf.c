/*
 * dwarf.c - DWARF 调试信息生成器
 *
 * 生成 DWARF v5 .debug_line section (行号程序)
 * 用于源码级调试: 将生成代码地址映射回源码位置
 *
 * 参考: DWARF Debugging Information Format v5, Section 6.2
 */

#include "ponypp/dwarf.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ==================== LEB128 编码 ==================== */

static size_t write_uleb128(uint8_t *buf, uint64_t val) {
    size_t n = 0;
    do {
        uint8_t byte = val & 0x7f;
        val >>= 7;
        if (val) byte |= 0x80;
        buf[n++] = byte;
    } while (val);
    return n;
}

static size_t write_sleb128(uint8_t *buf, int64_t val) {
    size_t n = 0;
    bool more = true;
    while (more) {
        uint8_t byte = val & 0x7f;
        val >>= 7;
        if ((val == 0 && !(byte & 0x40)) || (val == -1 && (byte & 0x40)))
            more = false;
        else
            byte |= 0x80;
        buf[n++] = byte;
    }
    return n;
}

/* ==================== DWARF 行号状态机 ==================== */

typedef struct {
    uint64_t address;
    uint32_t file;
    uint32_t line;
    uint32_t column;
    bool is_stmt;
    bool end_sequence;
} LineState;

struct DwarfLineProgram {
    /* 文件表 */
    char **files;
    uint32_t file_count;
    uint32_t file_cap;

    /* 目录表 */
    char **dirs;
    uint32_t dir_count;
    uint32_t dir_cap;

    /* 行号条目 */
    LineState *rows;
    uint32_t row_count;
    uint32_t row_cap;

    /* 当前状态 */
    uint64_t current_addr;
    uint32_t current_file;
    uint32_t current_line;
    uint32_t current_column;

    /* 标准参数 */
    uint8_t min_inst_len;
    uint8_t max_ops_per_inst;
    uint8_t default_is_stmt;
    int8_t line_base;
    uint8_t line_range;
    uint8_t opcode_base;

    char *comp_dir;
    char *comp_name;
};

DwarfLineProgram *dwarf_line_new(const char *comp_dir, const char *comp_name) {
    DwarfLineProgram *dp = (DwarfLineProgram *)calloc(1, sizeof(DwarfLineProgram));
    if (!dp) return NULL;

    dp->min_inst_len = 1;
    dp->max_ops_per_inst = 1;
    dp->default_is_stmt = 1;
    dp->line_base = -5;
    dp->line_range = 14;
    dp->opcode_base = 13;

    dp->file_cap = 16;
    dp->files = (char **)calloc(dp->file_cap, sizeof(char *));
    dp->dir_cap = 8;
    dp->dirs = (char **)calloc(dp->dir_cap, sizeof(char *));
    dp->row_cap = 256;
    dp->rows = (LineState *)calloc(dp->row_cap, sizeof(LineState));

    if (!dp->files || !dp->dirs || !dp->rows) {
        dwarf_line_free(dp);
        return NULL;
    }

    if (comp_dir) dp->comp_dir = strdup(comp_dir);
    if (comp_name) dp->comp_name = strdup(comp_name);

    dp->current_addr = 0;
    dp->current_file = 1;  /* DWARF文件索引从1开始 */
    dp->current_line = 1;
    dp->current_column = 0;

    return dp;
}

void dwarf_line_free(DwarfLineProgram *dp) {
    if (!dp) return;
    for (uint32_t i = 0; i < dp->file_count; i++) free(dp->files[i]);
    for (uint32_t i = 0; i < dp->dir_count; i++) free(dp->dirs[i]);
    free(dp->files);
    free(dp->dirs);
    free(dp->rows);
    free(dp->comp_dir);
    free(dp->comp_name);
    free(dp);
}

int dwarf_line_add_dir(DwarfLineProgram *dp, const char *dir) {
    if (!dp || !dir) return -1;
    if (dp->dir_count >= dp->dir_cap) {
        uint32_t nc = dp->dir_cap * 2;
        char **nd = (char **)realloc(dp->dirs, nc * sizeof(char *));
        if (!nd) return -2;
        dp->dirs = nd;
        dp->dir_cap = nc;
    }
    dp->dirs[dp->dir_count++] = strdup(dir);
    return (int)dp->dir_count;
}

int dwarf_line_add_file(DwarfLineProgram *dp, const char *filename, uint32_t dir_idx) {
    if (!dp || !filename) return -1;
    if (dp->file_count >= dp->file_cap) {
        uint32_t nc = dp->file_cap * 2;
        char **nf = (char **)realloc(dp->files, nc * sizeof(char *));
        if (!nf) return -2;
        dp->files = nf;
        dp->file_cap = nc;
    }
    /* 存储为 "dir_idx:filename" 格式, DWARF v5用目录索引 */
    size_t len = strlen(filename) + 16;
    char *entry = (char *)malloc(len);
    if (!entry) return -3;
    snprintf(entry, len, "%u:%s", dir_idx, filename);
    dp->files[dp->file_count++] = entry;
    return (int)(dp->file_count);  /* 1-based */
}

void dwarf_line_set_addr(DwarfLineProgram *dp, uint64_t addr) {
    if (dp) dp->current_addr = addr;
}

void dwarf_line_set_file(DwarfLineProgram *dp, uint32_t file_idx) {
    if (dp) dp->current_file = file_idx;
}

void dwarf_line_set_line(DwarfLineProgram *dp, uint32_t line) {
    if (dp) dp->current_line = line;
}

void dwarf_line_set_column(DwarfLineProgram *dp, uint32_t col) {
    if (dp) dp->current_column = col;
}

int dwarf_line_emit_row(DwarfLineProgram *dp) {
    if (!dp) return -1;
    if (dp->row_count >= dp->row_cap) {
        uint32_t nc = dp->row_cap * 2;
        LineState *nr = (LineState *)realloc(dp->rows, nc * sizeof(LineState));
        if (!nr) return -2;
        dp->rows = nr;
        dp->row_cap = nc;
    }
    LineState *row = &dp->rows[dp->row_count++];
    row->address = dp->current_addr;
    row->file = dp->current_file;
    row->line = dp->current_line;
    row->column = dp->current_column;
    row->is_stmt = dp->default_is_stmt;
    row->end_sequence = false;
    return 0;
}

int dwarf_line_end_sequence(DwarfLineProgram *dp) {
    if (!dp) return -1;
    if (dp->row_count >= dp->row_cap) {
        uint32_t nc = dp->row_cap * 2;
        LineState *nr = (LineState *)realloc(dp->rows, nc * sizeof(LineState));
        if (!nr) return -2;
        dp->rows = nr;
        dp->row_cap = nc;
    }
    LineState *row = &dp->rows[dp->row_count++];
    row->address = dp->current_addr;
    row->file = 0;
    row->line = 0;
    row->column = 0;
    row->is_stmt = false;
    row->end_sequence = true;
    return 0;
}

/* ==================== 生成 .debug_line 字节码 ==================== */

static size_t emit_line_program_header(DwarfLineProgram *dp, uint8_t *buf) {
    size_t n = 0;

    /* unit_length (4字节, 后面填充) */
    uint32_t unit_length_pos = (uint32_t)n;
    buf[n] = 0; buf[n+1] = 0; buf[n+2] = 0; buf[n+3] = 0;
    n += 4;

    /* version (2字节) = 5 */
    buf[n++] = 5; buf[n++] = 0;

    /* address_size (1字节) = 8 */
    buf[n++] = 8;

    /* segment_selector_size (1字节) = 0 */
    buf[n++] = 0;

    /* header_length (4字节, 后面填充) */
    uint32_t header_length_pos = (uint32_t)n;
    buf[n] = 0; buf[n+1] = 0; buf[n+2] = 0; buf[n+3] = 0;
    n += 4;

    /* minimum_instruction_length */
    buf[n++] = dp->min_inst_len;

    /* maximum_operations_per_instruction */
    buf[n++] = dp->max_ops_per_inst;

    /* default_is_stmt */
    buf[n++] = dp->default_is_stmt;

    /* line_base (signed) */
    buf[n++] = (uint8_t)dp->line_base;

    /* line_range */
    buf[n++] = dp->line_range;

    /* opcode_base */
    buf[n++] = dp->opcode_base;

    /* standard_opcode_lengths[opcode_base - 1] */
    static const uint8_t std_op_lens[] = {0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1};
    for (size_t i = 0; i < sizeof(std_op_lens); i++)
        buf[n++] = std_op_lens[i];

    /* directory_entry_format_count = 1 (DWARF v5) */
    buf[n++] = 1;
    /* DW_LNCT_path (0x01), DW_FORM_line_strp (0x0f) */
    buf[n++] = 1; buf[n++] = 0x0f;

    /* directories_count */
    n += write_uleb128(buf + n, dp->dir_count);
    for (uint32_t i = 0; i < dp->dir_count; i++) {
        /* 字符串偏移(简化: 直接写字符串) */
        size_t slen = strlen(dp->dirs[i]) + 1;
        memcpy(buf + n, dp->dirs[i], slen);
        n += slen;
    }

    /* file_name_entry_format_count = 2 (DWARF v5) */
    buf[n++] = 2;
    /* DW_LNCT_path (0x01), DW_FORM_string (0x08) */
    buf[n++] = 1; buf[n++] = 0x08;
    /* DW_LNCT_directory_index (0x02), DW_FORM_udata (0x0f) */
    buf[n++] = 2; buf[n++] = 0x0f;

    /* file_names_count */
    n += write_uleb128(buf + n, dp->file_count);
    for (uint32_t i = 0; i < dp->file_count; i++) {
        /* 文件名(去掉dir_idx:前缀) */
        const char *fname = strchr(dp->files[i], ':');
        fname = fname ? fname + 1 : dp->files[i];
        size_t slen = strlen(fname) + 1;
        memcpy(buf + n, fname, slen);
        n += slen;
        /* 目录索引 */
        uint32_t dir_idx = 0;
        sscanf(dp->files[i], "%u:", &dir_idx);
        n += write_uleb128(buf + n, dir_idx);
    }

    /* 填充header_length */
    uint32_t header_length = (uint32_t)(n - header_length_pos - 4);
    buf[header_length_pos] = (uint8_t)(header_length);
    buf[header_length_pos + 1] = (uint8_t)(header_length >> 8);
    buf[header_length_pos + 2] = (uint8_t)(header_length >> 16);
    buf[header_length_pos + 3] = (uint8_t)(header_length >> 24);

    (void)unit_length_pos;
    return n;
}

static size_t emit_line_program_body(DwarfLineProgram *dp, uint8_t *buf) {
    size_t n = 0;
    uint64_t prev_addr = 0;
    uint32_t prev_line = 1;
    uint32_t prev_file = 1;

    for (uint32_t i = 0; i < dp->row_count; i++) {
        LineState *row = &dp->rows[i];

        if (row->end_sequence) {
            /* DW_LNE_end_sequence */
            buf[n++] = 0;  /* extended */
            n += write_uleb128(buf + n, 1 + 8);  /* length */
            buf[n++] = 1;  /* DW_LNE_end_sequence */
            /* 地址(8字节) */
            for (int j = 0; j < 8; j++)
                buf[n++] = (uint8_t)(row->address >> (j * 8));
            prev_addr = row->address;
            prev_line = 1;
            prev_file = 1;
            continue;
        }

        /* 地址增量 */
        if (row->address > prev_addr) {
            uint64_t delta = row->address - prev_addr;
            /* DW_LNE_set_address */
            buf[n++] = 0;
            n += write_uleb128(buf + n, 1 + 8);
            buf[n++] = 2;  /* DW_LNE_set_address */
            for (int j = 0; j < 8; j++)
                buf[n++] = (uint8_t)(row->address >> (j * 8));
            prev_addr = row->address;
        }

        /* 文件变更 */
        if (row->file != prev_file) {
            buf[n++] = 4;  /* DW_LNS_set_file */
            n += write_uleb128(buf + n, row->file);
            prev_file = row->file;
        }

        /* 行号增量 -> 特殊操作码 */
        int64_t line_delta = (int64_t)row->line - (int64_t)prev_line;
        if (line_delta >= dp->line_base && line_delta < dp->line_base + dp->line_range) {
            /* 特殊操作码 */
            uint8_t opcode = (uint8_t)((line_delta - dp->line_base) + (dp->line_range * 1) + dp->opcode_base);
            buf[n++] = opcode;
        } else {
            /* DW_LNS_advance_line + DW_LNS_copy */
            buf[n++] = 3;  /* DW_LNS_advance_line */
            n += write_sleb128(buf + n, line_delta);
            buf[n++] = 1;  /* DW_LNS_copy */
        }
        prev_line = row->line;

        /* 列号 */
        if (row->column > 0) {
            buf[n++] = 5;  /* DW_LNS_set_column */
            n += write_uleb128(buf + n, row->column);
        }
    }

    /* 最终 end_sequence */
    buf[n++] = 0;
    n += write_uleb128(buf + n, 1 + 8);
    buf[n++] = 1;
    for (int j = 0; j < 8; j++)
        buf[n++] = (uint8_t)(prev_addr >> (j * 8));

    return n;
}

int dwarf_line_generate(DwarfLineProgram *dp, uint8_t *buf, size_t buf_len, size_t *out_len) {
    if (!dp || !buf || !out_len) return -1;

    size_t n = emit_line_program_header(dp, buf);
    n += emit_line_program_body(dp, buf + n);

    /* 填充unit_length (不含自身4字节) */
    uint32_t unit_length = (uint32_t)(n - 4);
    buf[0] = (uint8_t)(unit_length);
    buf[1] = (uint8_t)(unit_length >> 8);
    buf[2] = (uint8_t)(unit_length >> 16);
    buf[3] = (uint8_t)(unit_length >> 24);

    *out_len = n;
    return 0;
}

int dwarf_line_save(DwarfLineProgram *dp, const char *path) {
    if (!dp || !path) return -1;

    /* 估算大小: 每行约20字节 + 头部512字节 */
    size_t est = 512 + dp->row_count * 32;
    uint8_t *buf = (uint8_t *)malloc(est);
    if (!buf) return -2;

    size_t len = 0;
    if (dwarf_line_generate(dp, buf, est, &len) != 0) {
        free(buf);
        return -3;
    }

    FILE *f = fopen(path, "wb");
    if (!f) { free(buf); return -4; }
    fwrite(buf, 1, len, f);
    fclose(f);
    free(buf);
    return 0;
}

size_t dwarf_line_row_count(const DwarfLineProgram *dp) {
    return dp ? dp->row_count : 0;
}

uint32_t dwarf_line_file_count(const DwarfLineProgram *dp) {
    return dp ? dp->file_count : 0;
}
