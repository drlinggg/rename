#include "../../system.h"
#include "cmpswap.h"
#include "const_folding.h"
#include "jit.h"
#include "jit_types.h"
#include <stdio.h>
#include <stdlib.h>
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
    
    // 1. Constant folding
    FoldStats cf_stats;
    CodeObj* cf_optimized = jit_optimize_constant_folding(code, &cf_stats);
    if (cf_optimized && cf_stats.removed_instructions > 0) {
        DPRINT("[JIT] Applied constant folding: %zu optimizations\n",
               cf_stats.removed_instructions);
        optimized = cf_optimized;
    }
    
    // 2. Compare-and-swap
    CodeObj* cas_optimized = jit_optimize_compare_and_swap(jit, 
        optimized ? optimized : code);
    if (cas_optimized) {
        DPRINT("[JIT] Applied compare-and-swap\n");
        if (optimized && optimized != code) {
            free_code_obj(optimized);
        }
        optimized = cas_optimized;
    }
    
    if (optimized && optimized != code) {
        if (jit->cache_size >= jit->cache_capacity) {
            jit->cache_capacity = jit->cache_capacity ? jit->cache_capacity * 2 : 8;
            jit->compiled_cache = realloc(jit->compiled_cache,
                                         jit->cache_capacity * sizeof(CodeObj*));
        }
        jit->compiled_cache[jit->cache_size++] = optimized;
        jit->compiled_count++;
    }
    
    if (optimized) {
        bytecode_array_print(&optimized->code);
        return optimized;
    }
    DPRINT("[JIT] Returning original version\n");
    return code;
}
