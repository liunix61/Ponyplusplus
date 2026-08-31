#ifndef PONYPP_CODEGEN_H
#define PONYPP_CODEGEN_H

#include "ponypp/ast.h"

typedef struct Codegen Codegen;

Codegen *codegen_new(FILE *out);
void codegen_free(Codegen *cg);
void codegen_program(Codegen *cg, ASTNode *ast);

#endif