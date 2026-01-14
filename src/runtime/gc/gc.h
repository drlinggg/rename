#pragma once

#include "../vm/object.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct GC GC;

typedef void (*GC_ObjectCallback)(void* user_data, Object* obj);

typedef void (*GC_ObjectIterator)(void* iterator_data, GC_ObjectCallback callback, void* callback_data);

GC* gc_create(void);
void gc_destroy(GC* gc);

void gc_incref(GC* gc, Object* o);
void gc_decref(GC* gc, Object* o);

void gc_collect(GC* gc, Object** roots, size_t roots_count, 
                GC_ObjectIterator iterate_all_objects, void* iterator_data);

void gc_collect_simple(GC* gc, Object** roots, size_t roots_count);

void gc_add_root(GC* gc, Object* root);
void gc_remove_root(GC* gc, Object* root);
void gc_clear_roots(GC* gc);

size_t gc_get_allocated_count(GC* gc);
size_t gc_get_marked_count(GC* gc);
size_t gc_get_collected_count(GC* gc);

