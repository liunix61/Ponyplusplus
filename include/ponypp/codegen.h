#ifndef PONYPP_CODEGEN_H
#define PONYPP_CODEGEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ponypp/ast.h"

typedef struct Codegen Codegen;

Codegen *codegen_new(FILE *out);
void codegen_free(Codegen *cg);
void codegen_program(Codegen *cg, ASTNode *ast);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_CODEGEN_H */
