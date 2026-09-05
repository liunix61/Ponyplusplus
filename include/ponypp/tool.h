/**
 * ponyppc-tool.h — Phase 3 Toolchain: build, run, test, pkg
 */
#ifndef PNY_TOOL_H
#define PNY_TOOL_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TOOL_BUILD,     /* ponyppc build hello.pny */
    TOOL_RUN,       /* ponyppc run hello.pny */
    TOOL_TEST,      /* ponyppc test */
    TOOL_PKG,       /* ponyppc pkg new/init/add/build */
    TOOL_FMT,       /* ponyppc fmt file.pny */
    TOOL_DOCS,      /* ponyppc docs */
    TOOL_HELP,
    TOOL_REPL       /* ponyppc repl - 交互式 REPL */
} ToolCommand;

typedef struct {
    ToolCommand cmd;
    const char *subcmd;    /* for pkg: new/init/add */
    const char *input;     /* source file */
    const char *output;    /* -o output */
    const char *target;    /* --target=... */
    const char *platform;  /* --platform=stm32|esp32 */
    const char *olevel;    /* -O level */
    bool debug;            /* -g */
    bool ast_dump;         /* --ast */
    bool pretty;           /* --pretty */
    bool wit_only;         /* --wit-only */
    bool run_after;        /* -run */
    bool test_verbose;     /* -v for test */
    char **extra_args;     /* remaining */
    int extra_count;
} ToolConfig;

/* Parse CLI into ToolConfig. Returns 0 on success, -1 on error. */
int tool_parse_args(int argc, char **argv, ToolConfig *tc);

/* Execute the parsed command. Returns exit code. */
int tool_execute(ToolConfig *tc);

/* Build: compile .pny -> native binary (or wasm) */
int tool_build(const char *input, const char *output, const char *target,
               const char *platform, const char *olevel, bool debug);

/* Run: compile and execute */
int tool_run(const char *input, const char *target, const char *olevel);

/* Test: find and run all *_test.pny files in current directory */
int tool_test(bool verbose);

/* Format: pretty-print .pny file */
int tool_fmt(const char *input);

/* Package operations */
int tool_pkg_new(const char *name);
int tool_pkg_add(const char *dep);

/* REPL: ponyppc repl */
int tool_repl(void);

/* Network: TCP socket operations */
typedef struct PnySocket PnySocket;
PnySocket *pny_tcp_connect(const char *host, int port);
PnySocket *pny_tcp_listen(int port);
int pny_tcp_send(PnySocket *s, const void *data, size_t len);
int pny_tcp_recv(PnySocket *s, void *buf, size_t max_len);
void pny_tcp_close(PnySocket *s);
bool pny_tcp_connected(const PnySocket *s);

/* Profiler: performance sampling */
typedef struct PnyProfiler PnyProfiler;
typedef struct { int64_t ts; int actor_id; const char *method; int64_t dur_ns; } ProfSample;
PnyProfiler *pny_profiler_new(void);
void pny_profiler_free(PnyProfiler *p);
void pny_profiler_start(PnyProfiler *p);
void pny_profiler_stop(PnyProfiler *p);
void pny_profiler_sample(PnyProfiler *p, int actor_id, const char *method, int64_t dur_ns);
int pny_profiler_export(PnyProfiler *p, const char *path);
int pny_profiler_sample_count(const PnyProfiler *p);

#ifdef __cplusplus
}
#endif

#endif /* PNY_TOOL_H */
