#ifndef PONYPP_WASM_H
#define PONYPP_WASM_H
#ifdef __cplusplus
extern "C" {
#endif

#include "ponypp.h"
#include "ponypp/ast.h"

int wasm_write_program(ASTNode *ast, const char *output);
const char *wasm_target_name(TargetKind target);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_WASM_H */