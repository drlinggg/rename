#pragma once

#include "object.h"
#include "../../compiler/value.h"

typedef struct HeapHeap Heap;

Heap* heap_create(void);
void heap_destroy(Heap* heap);

// allocate functions
Object* heap_alloc_int(Heap* heap, int64_t v);
Object* heap_alloc_bool(Heap* heap, bool b);
Object* heap_alloc_none(Heap* heap);
Object* heap_alloc_code(Heap* heap, CodeObj* code);
Object* heap_alloc_function(Heap* heap, CodeObj* code);
Object* heap_alloc_array(Heap* heap);
Object* heap_alloc_array_with_size(Heap* heap, size_t size);

typedef Object* (*NativeCFunc)(VM* vm, int arg_count, Object** args);
Object* heap_alloc_native_function(Heap* heap, NativeCFunc func, const char* name);

// Convert compile-time Value to runtime Object
Object* heap_from_value(Heap* heap, Value val);

// helpful debug
size_t heap_live_objects(Heap* heap);

