/* Pony++ native backend generated code */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <"ponypp/runtime.h">

typedef struct {
  unsigned int count;} main_t;

static main_t main_create() {
  main_t self;
  memset(&self, 0, sizeof(self));
  /* stmt */42;
  return self;
}

static void main_run(main_t *self) {
  /* stmt */printf("Hello from Pony++ native (real backend)\n");
  /* stmt */printf("0\n", count);
}

int main(int argc, char *argv[]) {
  main_t __main_obj = main_create();
  main_run(&__main_obj);
  (void)__main_obj;
    return 0;
}
