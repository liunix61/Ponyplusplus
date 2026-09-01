#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "ponypp/runtime.h"

PnyRuntime *pny_runtime_global = NULL;

PnyRuntime *pny_runtime_new(void) {
    PnyRuntime *r = (PnyRuntime *)calloc(1, sizeof(PnyRuntime));
    if (!r) return NULL;
    pny_runtime_global = r;
    return r;
}

void pny_runtime_free(PnyRuntime *r) {
    if (!r) return;
    PnyActor *a = r->actors;
    while (a) {
        PnyActor *next = a->next;
        PnyMessage *m = a->messages;
        while (m) {
            PnyMessage *mn = m->next;
            free(m);
            m = mn;
        }
        free(a);
        a = next;
    }
    free(r);
}

void pny_actor_register(PnyRuntime *r, PnyActor *a) {
    if (!r || !a) return;
    a->next = r->actors;
    r->actors = a;
}

PnyMessage *pny_msg_new(const char *method, void *arg) {
    PnyMessage *m = (PnyMessage *)malloc(sizeof(PnyMessage));
    if (!m) return NULL;
    m->method = method ? strdup(method) : NULL;
    m->arg = arg;
    m->next = NULL;
    return m;
}

void pny_msg_free(PnyMessage *m) {
    if (!m) return;
    free(m->method);
    free(m);
}

void pny_actor_send(PnyActor *actor, const char *method, void *arg) {
    if (!actor) return;
    PnyMessage *m = pny_msg_new(method, arg);
    if (!m) return;
    if (actor->messages) {
        PnyMessage *tail = actor->messages;
        while (tail->next) tail = tail->next;
        tail->next = m;
    } else {
        actor->messages = m;
    }
}

void pny_scheduler_tick(PnyRuntime *r) {
    if (!r) return;
    PnyActor *a = r->actors;
    while (a) {
        if (a->messages && a->messages->next == NULL) {
            PnyMessage *m = a->messages;
            a->messages = m->next;
            pny_msg_free(m);
        }
        a = a->next;
    }
}

void pny_gc_mark_root(PnyActor *a) {
    (void)a;
}