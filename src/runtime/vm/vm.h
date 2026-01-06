#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "heap.h"
#include "../../runtime/gc/gc.h"
#include "../../runtime/jit/jit.h"
#include "../../compiler/bytecode.h"
#include "../../compiler/value.h"

typedef struct Frame Frame;
typedef struct VM VM;

VM* vm_create(Heap* heap, size_t global_count);
void vm_register_builtins(VM* vm);
void vm_destroy(VM* vm);

Object* vm_execute(VM* vm, CodeObj* code);

void vm_set_global(VM* vm, size_t idx, Object* value);
Object* vm_get_global(VM* vm, size_t idx);

Frame* frame_create(VM* vm, CodeObj* code);
void frame_destroy(Frame* frame);
Object* frame_execute(Frame* frame);

GC* vm_get_gc(VM* vm);
JIT* vm_get_jit(VM* vm);
Heap* vm_get_heap(VM* vm);

Object* vm_get_none(VM* vm);
Object* vm_get_true(VM* vm);
Object* vm_get_false(VM* vm);

// GC интеграция
void vm_collect_garbage(VM* vm);
void vm_register_frame(VM* vm, Frame* frame);
void vm_unregister_frame(VM* vm, Frame* frame);