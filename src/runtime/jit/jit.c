#include "jit_types.h"
#include "cmpswap.h"
#include "jit.h"
#include "../../system.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

JIT* jit_create(void) {
    JIT* jit = malloc(sizeof(JIT));
    if (!jit) return NULL;
    
    jit->compiled_count = 0;
    jit->compiled_cache = NULL;
    jit->cache_size = 0;
    jit->cache_capacity = 0;
    
    return jit;
}

void jit_destroy(JIT* jit) {
    if (!jit) return;
    
    // Очищаем кэш
    for (size_t i = 0; i < jit->cache_size; i++) {
        if (jit->compiled_cache[i]) {
            // Освобождаем CodeObj из кэша
            CodeObj* code = jit->compiled_cache[i];
            if (code->name) free(code->name);
            if (code->constants) free(code->constants);
            if (code->code.bytecodes) free(code->code.bytecodes);
            free(code);
        }
    }
    
    if (jit->compiled_cache) {
        free(jit->compiled_cache);
    }
    
    free(jit);
}

void* jit_compile_function(JIT* jit, void* code_ptr) {
    if (!jit || !code_ptr) {
        return code_ptr;
    }
    
    CodeObj* code = (CodeObj*)code_ptr;
    
    // Check cache
    for (size_t i = 0; i < jit->cache_size; i++) {
        if (jit->compiled_cache[i] == code) {
            DPRINT("[JIT] Function found in cache\n");
            return jit->compiled_cache[i];
        }
    }

    CodeObj* optimized = NULL;
    
    // Try to optimize with cmpswap
    CodeObj* optimized_cmp_swap = jit_optimize_compare_and_swap(jit, code);
    if (optimized_cmp_swap) {
        DPRINT("[JIT] Optimized cmp&swap\n");
        optimized = optimized_cmp_swap;
        
        // Добавляем в кэш
        if (jit->cache_size >= jit->cache_capacity) {
            jit->cache_capacity = jit->cache_capacity ? jit->cache_capacity * 2 : 8;
            jit->compiled_cache = realloc(jit->compiled_cache, 
                                         jit->cache_capacity * sizeof(CodeObj*));
        }
        
        jit->compiled_cache[jit->cache_size++] = optimized;
        jit->compiled_count++;
    }

    if (optimized) {
        return optimized;
    }
    
    DPRINT("[JIT] Returning original version\n");
    return code;
}
