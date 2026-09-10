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

/* Source Map: 输出行 -> 源码位置映射 */
typedef struct {
    int generated_line;
    int source_line;
    int source_col;
    char source_file[256];
} SourceMapEntry;

typedef struct {
    SourceMapEntry *entries;
    size_t count;
    size_t cap;
} SourceMap;

SourceMap *sourcemap_new(void);
void sourcemap_free(SourceMap *sm);
int sourcemap_add(SourceMap *sm, int gen_line, const char *file, int src_line, int src_col);
int sourcemap_lookup(const SourceMap *sm, int gen_line, SourceMapEntry *out);
int sourcemap_save_json(const SourceMap *sm, const char *path);
SourceMap *sourcemap_load_json(const char *path);
size_t sourcemap_count(const SourceMap *sm);

/* Codegen with source map */
Codegen *codegen_new_with_map(FILE *out, SourceMap *sm);
void codegen_set_source_file(Codegen *cg, const char *filename);
void codegen_emit_line_directive(Codegen *cg, int src_line);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_CODEGEN_H */
