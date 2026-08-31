#ifndef PONYPP_WIT_H
#define PONYPP_WIT_H

#include "ponypp.h"
#include "ponypp/ast.h"

/* WIT 写入器 */
WITWriter *wit_writer_new(const char *output_path);
void wit_writer_close(WITWriter *writer);

/* 写入程序 */
int wit_write_program(WITWriter *writer, const ASTNode *ast);

#endif /* PONYPP_WIT_H */
