#include "heap.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct HeapHeap {
    Object** objects;
    size_t count;
    size_t capacity;
};

Heap* heap_create(void) {
    Heap* h = malloc(sizeof(Heap));
    h->objects = NULL;
    h->count = 0;
    h->capacity = 0;
    return h;
}

void heap_destroy(Heap* heap) {
    if (!heap) return;
    for (size_t i = 0; i < heap->count; i++) {
        object_decref(heap->objects[i]);
        // object_decref will free if ref_count reaches 0
    }
    free(heap->objects);
    free(heap);
}

static void heap_track_object(Heap* heap, Object* o) {
    if (!heap || !o) return;
    if (heap->count + 1 > heap->capacity) {
        size_t new_capacity = heap->capacity == 0 ? 8 : heap->capacity * 2;
        heap->objects = realloc(heap->objects, new_capacity * sizeof(Object*));
        heap->capacity = new_capacity;
    }
    heap->objects[heap->count++] = o;
}

Object* heap_alloc_int(Heap* heap, int64_t v) {
    Object* o = object_new_int(v);
    heap_track_object(heap, o);
    return o;
}

Object* heap_alloc_bool(Heap* heap, bool b) {
    Object* o = object_new_bool(b);
    heap_track_object(heap, o);
    return o;
}

Object* heap_alloc_none(Heap* heap) {
    Object* o = object_new_none();
    heap_track_object(heap, o);
    return o;
}

Object* heap_alloc_code(Heap* heap, CodeObj* code) {
    Object* o = object_new_code(code);
    heap_track_object(heap, o);
    return o;
}

Object* heap_alloc_function(Heap* heap, CodeObj* code) {
    Object* o = object_new_function(code);
    heap_track_object(heap, o);
    return o;
}

Object* heap_alloc_array(Heap* heap) {
    Object* o = object_new_array();
    heap_track_object(heap, o);
    return o;
}

Object* heap_from_value(Heap* heap, Value val) {
    switch (val.type) {
        case VAL_INT:
            return heap_alloc_int(heap, val.int_val);
        /*
        case VAL_BOOL:
            return heap_alloc_bool(heap, val.bool_val);
        */
        case VAL_CODE:
            return heap_alloc_code(heap, val.code_val);
        case VAL_FUNCTION:
            return heap_alloc_function(heap, val.code_val);
        /*
        case VAL_NONE:
        */
        default:
            return heap_alloc_none(heap);
    }
}

size_t heap_live_objects(Heap* heap) {
    if (!heap) return 0;
    return heap->count;
}
