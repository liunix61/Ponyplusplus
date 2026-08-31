/*
 * wit.c - Pony++ WIT 接口生成器
 */

#include "ponypp/wit.h"
#include "ponypp/util.h"
#include <errno.h>

struct WITWriter {
    char *path;
    FILE *file;
};

WITWriter *wit_writer_new(const char *output_path) {
    WITWriter *w = (WITWriter *)calloc(1, sizeof(WITWriter));
    if (!w) return NULL;
    w->path = s_strdup(output_path);
    w->file = fopen(output_path, "w");
    if (!w->file) {
        free(w->path);
        free(w);
        return NULL;
    }
    return w;
}

void wit_writer_close(WITWriter *w) {
    if (!w) return;
    if (w->file) fclose(w->file);
    if (w->path) free(w->path);
    free(w);
}

static void wit_write_actor(WITWriter *w, const ASTNode *actor) {
    const char *name = (const char *)actor->data;
    fprintf(w->file, "resource %s {\n", name);

    size_t child_idx = 0;
    for (size_t i = 0; i < actor->child_count; i++) {
        const ASTNode *child = actor->children[i];
        if (!child) continue;
        switch (child->type) {
            case NODE_BE: {
                const char *method = (const char *)child->data;
                fprintf(w->file, "  %s:\n", method);
                /* 参数 */
                if (child->child_count > 0 && child->children[0]) {
                    const ASTNode *params = child->children[0];
                    fprintf(w->file, "    {\n");
                    for (size_t j = 0; j < params->child_count; j++) {
                        if (params->children[j] && params->children[j]->data) {
                            fprintf(w->file, "      %s,\n",
                                    (const char *)params->children[j]->data);
                        }
                    }
                    fprintf(w->file, "    }\n");
                } else {
                    fprintf(w->file, "    {}\n");
                }
                break;
            }
            case NODE_FUN: {
                const char *method = (const char *)child->data;
                fprintf(w->file, "  %s:\n", method);
                if (child->child_count > 0 && child->children[0]) {
                    const ASTNode *params = child->children[0];
                    fprintf(w->file, "    {\n");
                    for (size_t j = 0; j < params->child_count; j++) {
                        if (params->children[j] && params->children[j]->data) {
                            fprintf(w->file, "      %s,\n",
                                    (const char *)params->children[j]->data);
                        }
                    }
                    fprintf(w->file, "    }\n");
                } else {
                    fprintf(w->file, "    {}\n");
                }
                break;
            }
            default:
                child_idx++;
                break;
        }
    }
    (void)child_idx;
    fprintf(w->file, "}\n\n");
}

int wit_write_program(WITWriter *w, const ASTNode *ast) {
    if (!w || !ast) return -1;

    /* WIT 文件头 */
    fprintf(w->file, "package ponypp:program\n\n");

    for (size_t i = 0; i < ast->child_count; i++) {
        const ASTNode *child = ast->children[i];
        if (!child) continue;
        switch (child->type) {
            case NODE_ACTOR:
                wit_write_actor(w, child);
                break;
            default:
                break;
        }
    }

    return 0;
}
