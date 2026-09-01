#ifndef PONYPP_CAPABILITIES_H
#define PONYPP_CAPABILITIES_H
#ifdef __cplusplus
extern "C" {
#endif

#include "ponypp.h"
#include "ponypp/ast.h"

typedef struct {
    bool ok;
    int error_count;
    const char **errors;
} CapCheckResult;

int capabilities_check_program(ASTNode *ast, CapCheckResult *result);
void cap_check_free_result(CapCheckResult *result);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_CAPABILITIES_H */