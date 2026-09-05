#ifndef PONYPP_WIT_H
#define PONYPP_WIT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "ponypp.h"
#include "ponypp/ast.h"

int wit_write_program(ASTNode *ast, const char *output, TargetKind target);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_WIT_H */