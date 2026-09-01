/* Pony++ native backend generated code */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

static PnyRuntime *pny_runtime_global = NULL;

static PnyRuntime *pny_runtime_new(void) {
    PnyRuntime *r = (PnyRuntime *)calloc(1, sizeof(PnyRuntime));
    if (!r) return NULL;
    pny_runtime_global = r;
    return r;
}

static void pny_actor_register(PnyRuntime *r, PnyActor *a) {
    if (!r || !a) return;
    a->next = r->actors;
    r->actors = a;
}

static PnyMessage *pny_msg_new(const char *m, void *arg) {
    PnyMessage *msg = (PnyMessage *)malloc(sizeof(PnyMessage));
    if (!msg) return NULL;
    msg->method = m ? strdup(m) : NULL;
    msg->arg = arg;
    msg->next = NULL;
    return msg;
}

static void pny_actor_send(PnyActor *a, const char *m, void *arg) {
    if (!a) return;
    PnyMessage *msg = pny_msg_new(m, arg);
    if (!msg) return;
    if (a->messages) {
        PnyMessage *tail = a->messages;
        while (tail->next) tail = tail->next;
        tail->next = msg;
    } else { a->messages = msg; }
}

static void pny_runtime_free(PnyRuntime *r) {
    if (!r) return;
    PnyActor *a = r->actors;
    while (a) { PnyActor *n = a->next;
        PnyMessage *m = a->messages; while (m) { PnyMessage *mn = m->next; free(m->method); free(m); m = mn; }
        free(a->state); free(a); a = n;
    }
    free(r);
}

static PnyActor *pny_actor_new(const char *nm, size_t sz) {
    PnyActor *a = (PnyActor *)malloc(sizeof(PnyActor));
    if (!a) return NULL;
    a->name = nm;
    a->state = sz > 0 ? calloc(1, sz) : NULL;
    a->state_size = sz;
    a->next = NULL;
    a->messages = NULL;
    return a;
}

typedef struct {
} main_t;

static main_t main_create() {
  main_t self;
  memset(&self, 0, sizeof(self));
  /* stmt */printf("Hello, World!\n");
  return self;
}

int main(int argc, char *argv[]) {
  main_t __main_obj = main_create();
  PnyRuntime *r = pny_runtime_new();
  PnyActor *__actor = pny_actor_new("main", sizeof(main_t));
  if (__actor) { memcpy(__actor->state, &__main_obj, sizeof(main_t)); pny_actor_register(r, __actor); }
  (void)r; (void)__actor;
    return 0;
}
