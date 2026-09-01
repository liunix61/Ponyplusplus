#ifndef PONYPP_RUNTIME_H
#define PONYPP_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct PnyActor {
    const char *name;
    void *state;
    size_t state_size;
    struct PnyActor *next;
    struct PnyMessage *messages;
} PnyActor;

typedef struct PnyMessage {
    char *method;
    void *arg;
    struct PnyMessage *next;
} PnyMessage;

typedef struct PnyRuntime {
    PnyActor *actors;
    size_t actor_count;
} PnyRuntime;

extern PnyRuntime *pny_runtime_global;

PnyRuntime *pny_runtime_new(void);
void pny_runtime_free(PnyRuntime *r);
void pny_actor_register(PnyRuntime *r, PnyActor *a);
PnyMessage *pny_msg_new(const char *method, void *arg);
void pny_msg_free(PnyMessage *m);
void pny_actor_send(PnyActor *actor, const char *method, void *arg);
void pny_scheduler_tick(PnyRuntime *r);
void pny_gc_mark_root(PnyActor *a);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_RUNTIME_H */
