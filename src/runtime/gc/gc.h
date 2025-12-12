#pragma once

#include "../vm/object.h"

typedef struct GC GC;

GC* gc_create(void);
void gc_destroy(GC* gc);

// Hooked refcount helpers - currently simple wrappers/no-ops to be replaced by real GC
void gc_incref(GC* gc, Object* o);
void gc_decref(GC* gc, Object* o);

// Additional GC APIs to integrate later
void gc_collect(GC* gc);

