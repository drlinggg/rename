#include "gc.h"
#include <stdlib.h>
#include <stdio.h>

struct GC {
    size_t allocated;
};

GC* gc_create(void) {
    GC* g = malloc(sizeof(GC));
    g->allocated = 0;
    return g;
}

void gc_destroy(GC* gc) {
    if (!gc) return;
    free(gc);
}

void gc_incref(GC* gc, Object* o) {
    (void)gc;
    if (!o) return;
    extern void object_incref(Object* o);
    object_incref(o);
}

void gc_decref(GC* gc, Object* o) {
    (void)gc;
    if (!o) return;
    extern void object_decref(Object* o);
    object_decref(o);
}

void gc_collect(GC* gc) {
    (void)gc;
    fprintf(stderr, "[GC] gc_collect called (noop)\n");
}
