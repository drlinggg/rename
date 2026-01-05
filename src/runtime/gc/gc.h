#pragma once

#include "../vm/object.h"

typedef struct GC GC;

GC* gc_create(void);
void gc_destroy(GC* gc);

void gc_incref(GC* gc, Object* o);
void gc_decref(GC* gc, Object* o);

void gc_collect(GC* gc);

