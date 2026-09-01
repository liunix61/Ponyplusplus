#ifndef PONYPP_TYPECHECK_H
#define PONYPP_TYPECHECK_H

#include "ponypp.h"
#include "ponypp/ast.h"

typedef struct {
    bool ok;
    int error_count;
    const char **errors;
} TypeCheckResult;

int typecheck_program(ASTNode *ast, TypeCheckResult *result);
void typecheck_free_result(TypeCheckResult *result);

#endif /* PONYPP_TYPECHECK_H */