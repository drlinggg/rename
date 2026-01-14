#ifndef JIT_H
#define JIT_H

typedef struct JIT JIT;

JIT* jit_create(void);
void jit_destroy(JIT* jit);
void* jit_compile_function(JIT* jit, void* code);

#endif
