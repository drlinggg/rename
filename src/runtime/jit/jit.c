#include "jit.h"
#include <stdlib.h>
#include <stdio.h>

struct JIT {
    // placeholder for JIT internal state
    size_t compiled_count;
};

JIT* jit_create(void) {
    JIT* j = malloc(sizeof(JIT));
    if (!j) return NULL;
    j->compiled_count = 0;
    return j;
}

void jit_destroy(JIT* jit) {
    if (!jit) return;
    free(jit);
}

void* jit_compile_function(JIT* jit, CodeObj* code) {
    (void)jit;
    (void)code;
    // placeholder - returns NULL meaning no compiled representation available
    fprintf(stderr, "[JIT] compile called but not implemented\n");
    return NULL;
}
