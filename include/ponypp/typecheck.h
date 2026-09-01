#ifndef PONYPP_TYPECHECK_H
#define PONYPP_TYPECHECK_H
#ifdef __cplusplus
extern "C" {
#endif

#include "ponypp.h"
#include "ponypp/ast.h"

typedef struct {
    bool ok;
    int error_count;
    const char **errors;
} TypeCheckResult;

int typecheck_program(ASTNode *ast, TypeCheckResult *result);
void typecheck_free_result(TypeCheckResult *result);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_TYPECHECK_H */