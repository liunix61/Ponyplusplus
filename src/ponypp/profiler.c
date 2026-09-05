/**
 * Pony++ Profiler - 性能采样
 *
 * 实现: src/ponypp/profiler.c
 */
#include "ponypp/tool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#define PROFILER_MAX_SAMPLES 10000
#define PROFILER_INITIAL_CAP 1024

typedef struct PnyProfiler {
    ProfSample *samples;
    int count;
    int cap;
    bool running;
    int64_t start_time;
} PnyProfiler;

static int64_t profiler_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

PnyProfiler *pny_profiler_new(void) {
    PnyProfiler *p = (PnyProfiler *)calloc(1, sizeof(PnyProfiler));
    if (!p) return NULL;
    p->cap = PROFILER_INITIAL_CAP;
    p->samples = (ProfSample *)calloc((size_t)p->cap, sizeof(ProfSample));
    if (!p->samples) { free(p); return NULL; }
    p->count = 0;
    p->running = false;
    return p;
}

void pny_profiler_free(PnyProfiler *p) {
    if (!p) return;
    free(p->samples);
    free(p);
}

void pny_profiler_start(PnyProfiler *p) {
    if (!p) return;
    p->running = true;
    p->start_time = profiler_now_ns();
}

void pny_profiler_stop(PnyProfiler *p) {
    if (!p) return;
    p->running = false;
}

void pny_profiler_sample(PnyProfiler *p, int actor_id, const char *method, int64_t dur_ns) {
    if (!p || !p->running) return;
    if (p->count >= PROFILER_MAX_SAMPLES) return;
    if (p->count >= p->cap) {
        int new_cap = p->cap * 2;
        if (new_cap > PROFILER_MAX_SAMPLES) new_cap = PROFILER_MAX_SAMPLES;
        ProfSample *new_samples = (ProfSample *)realloc(p->samples, (size_t)new_cap * sizeof(ProfSample));
        if (!new_samples) return;
        p->samples = new_samples;
        p->cap = new_cap;
    }
    ProfSample *s = &p->samples[p->count];
    s->ts = profiler_now_ns();
    s->actor_id = actor_id;
    s->method = method ? method : "<unknown>";
    s->dur_ns = dur_ns;
    p->count++;
}

int pny_profiler_export(PnyProfiler *p, const char *path) {
    if (!p || !path) return -1;
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "{\n");
    fprintf(f, "  \"total_samples\": %d,\n", p->count);
    fprintf(f, "  \"duration_ns\": %lld,\n", (long long)(profiler_now_ns() - p->start_time));
    fprintf(f, "  \"samples\": [\n");
    for (int i = 0; i < p->count; i++) {
        ProfSample *s = &p->samples[i];
        fprintf(f, "    {\"ts\": %lld, \"actor_id\": %d, \"method\": \"%s\", \"dur_ns\": %lld}%s\n",
                (long long)s->ts, s->actor_id, s->method, (long long)s->dur_ns,
                (i + 1 < p->count) ? "," : "");
    }
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);
    return 0;
}

int pny_profiler_sample_count(const PnyProfiler *p) {
    return p ? p->count : 0;
}
