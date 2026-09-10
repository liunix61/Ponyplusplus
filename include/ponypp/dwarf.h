/*
 * dwarf.h - DWARF 调试信息生成器
 *
 * 生成 DWARF v5 .debug_line section
 * 用于源码级调试
 */
#ifndef PONYPP_DWARF_H
#define PONYPP_DWARF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct DwarfLineProgram DwarfLineProgram;

/* 创建/释放 */
DwarfLineProgram *dwarf_line_new(const char *comp_dir, const char *comp_name);
void dwarf_line_free(DwarfLineProgram *dp);

/* 构建行号表 */
int dwarf_line_add_dir(DwarfLineProgram *dp, const char *dir);
int dwarf_line_add_file(DwarfLineProgram *dp, const char *filename, uint32_t dir_idx);

/* 设置当前状态 */
void dwarf_line_set_addr(DwarfLineProgram *dp, uint64_t addr);
void dwarf_line_set_file(DwarfLineProgram *dp, uint32_t file_idx);
void dwarf_line_set_line(DwarfLineProgram *dp, uint32_t line);
void dwarf_line_set_column(DwarfLineProgram *dp, uint32_t col);

/* 发射行号条目 */
int dwarf_line_emit_row(DwarfLineProgram *dp);
int dwarf_line_end_sequence(DwarfLineProgram *dp);

/* 生成 .debug_line 字节码 */
int dwarf_line_generate(DwarfLineProgram *dp, uint8_t *buf, size_t buf_len, size_t *out_len);
int dwarf_line_save(DwarfLineProgram *dp, const char *path);

/* 查询 */
size_t dwarf_line_row_count(const DwarfLineProgram *dp);
uint32_t dwarf_line_file_count(const DwarfLineProgram *dp);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_DWARF_H */
