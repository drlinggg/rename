#pragma once

#include "../vm/object.h"

typedef struct JIT JIT;

JIT* jit_create(void);
void jit_destroy(JIT* jit);

// placeholder - compile a CodeObj into optimized representation
void* jit_compile_function(JIT* jit, CodeObj* code);

