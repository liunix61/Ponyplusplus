#ifndef PONYPP_WASM_H
#define PONYPP_WASM_H

#include "ponypp.h"
#include "ponypp/ast.h"

int wasm_write_program(ASTNode *ast, const char *output);
const char *wasm_target_name(TargetKind target);

#endif /* PONYPP_WASM_H */