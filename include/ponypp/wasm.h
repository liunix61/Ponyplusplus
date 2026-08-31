#ifndef PONYPP_WASM_H
#define PONYPP_WASM_H

#include "ponypp.h"
#include "ponypp/ast.h"

/* Wasm 写入器 */
WasmWriter *wasm_writer_new(const char *output_path);
void wasm_writer_close(WasmWriter *writer);

/* 写入程序 */
int wasm_write_program(WasmWriter *writer, ASTNode *ast, const CompilerConfig *cfg);

#endif /* PONYPP_WASM_H */
