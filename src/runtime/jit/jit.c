#include "../../system.h"
#include "cmpswap.h"
#include "const_folding.h"
#include "dce.h"
#include "jit.h"
#include "jit_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t cache_size;
    size_t cache_capacity;
    size_t compiled_count;
} JITStats;


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
    
    for (size_t i = 0; i < jit->cache_size; i++) {
        CodeObj* code = jit->compiled_cache[i];
        if (code) {
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

static CodeObj* jit_find_in_cache(JIT* jit, CodeObj* code) {
    if (!jit || !jit->compiled_cache || !code) return NULL;
    
    for (size_t i = 0; i < jit->cache_size; i++) {
        CodeObj* cached = jit->compiled_cache[i];
        if (cached && cached->name && code->name) {
            if (strcmp(cached->name, code->name) == 0) {
                DPRINT("[JIT] Function '%s' found in cache\n", code->name);
                return cached;
            }
        }
    }
    return NULL;
}

static void jit_add_to_cache(JIT* jit, CodeObj* optimized_code) {
    if (!jit || !optimized_code) return;
    
    if (jit->cache_size >= jit->cache_capacity) {
        size_t new_capacity = jit->cache_capacity ? jit->cache_capacity * 2 : 8;
        CodeObj** new_cache = realloc(jit->compiled_cache, 
                                     new_capacity * sizeof(CodeObj*));
        if (!new_cache) {
            DPRINT("[JIT] Failed to expand cache\n");
            return;
        }
        jit->compiled_cache = new_cache;
        jit->cache_capacity = new_capacity;
    }
    
    jit->compiled_cache[jit->cache_size++] = optimized_code;
    jit->compiled_count++;
    
    DPRINT("[JIT] Added function '%s' to cache (cache size: %zu/%zu)\n",
           optimized_code->name ? optimized_code->name : "anonymous",
           jit->cache_size, jit->cache_capacity);
}

void* jit_compile_function(JIT* jit, void* code_ptr) {
    if (!jit || !code_ptr) {
        DPRINT("[JIT] Invalid parameters\n");
        return code_ptr;
    }
    
    CodeObj* original = (CodeObj*)code_ptr;
    
    DPRINT("[JIT] Compiling function: %s\n",
           original->name ? original->name : "anonymous");
    
    CodeObj* cached = jit_find_in_cache(jit, original);
    if (cached) {
        return cached;
    }
    
    CodeObj* optimized = original;
    bool was_optimized = false;
    
    FoldStats cf_stats;
    CodeObj* cf_result = jit_optimize_constant_folding(original, &cf_stats);
    
    if (cf_result && cf_result != original) {
        DPRINT("[JIT] Constant folding: folded %zu constants, removed %zu instructions\n",
               cf_stats.folded_constants, cf_stats.removed_instructions);
        optimized = cf_result;
        was_optimized = true;
    }
    
    CmpswapStats cas_stats;
    CodeObj* cas_result = jit_optimize_cmpswap(optimized, &cas_stats);
    
    if (cas_result && cas_result != optimized) {
        DPRINT("[JIT] Compare-and-swap: optimized %zu patterns\n",
               cas_stats.optimized_patterns);
        
        if (was_optimized && optimized != original) {
            free_code_obj(optimized);
        }
        
        optimized = cas_result;
        was_optimized = true;
    }
    
    DCEStats dce_stats;
    CodeObj* dce_result = jit_optimize_dce(optimized, &dce_stats);

    if (dce_result && dce_result != optimized) {
        DPRINT("[JIT-DCE] Dead Code Elimination: removed %zu function calls, %zu total instructions\n",
               dce_stats.removed_calls, dce_stats.removed_instructions);

        optimized = dce_result;
        was_optimized = true;
    } else if (dce_result == optimized) {
        DPRINT("[JIT-DCE] No DCE changes applied\n");
    }
    
    if (was_optimized && optimized) {
        jit_add_to_cache(jit, optimized);
        
        DPRINT("[JIT] Optimized bytecode for '%s':\n",
               optimized->name ? optimized->name : "anonymous");
        
        return optimized;
    }
    
    CodeObj* copy = deep_copy_codeobj(original);
    if (copy) {
        jit_add_to_cache(jit, copy);
        DPRINT("[JIT] Added '%s' to cache without optimizations\n",
               copy->name ? copy->name : "anonymous");
    }
    
    DPRINT("[JIT] No optimizations applied for '%s', returning original\n",
           original->name ? original->name : "anonymous");
    
    return original;
}

void jit_clear_cache(JIT* jit) {
    if (!jit) return;
    
    for (size_t i = 0; i < jit->cache_size; i++) {
        CodeObj* code = jit->compiled_cache[i];
        if (code) {
            free_code_obj(code);
        }
        jit->compiled_cache[i] = NULL;
    }
    
    jit->cache_size = 0;
    jit->compiled_count = 0;
    
    DPRINT("[JIT] Cache cleared\n");
}

JITStats jit_get_stats(JIT* jit) {
    JITStats stats = {0};
    
    if (jit) {
        stats.cache_size = jit->cache_size;
        stats.cache_capacity = jit->cache_capacity;
        stats.compiled_count = jit->compiled_count;
    }
    return stats;
}

void jit_print_cache(JIT* jit) {
    if (!jit) {
        printf("[JIT] Cache: (null)\n");
        return;
    }
    
    printf("[JIT] Cache statistics:\n");
    printf("  Size: %zu/%zu\n", jit->cache_size, jit->cache_capacity);
    printf("  Compiled functions: %zu\n", jit->compiled_count);
    
    if (jit->cache_size > 0) {
        printf("  Cached functions:\n");
        for (size_t i = 0; i < jit->cache_size; i++) {
            CodeObj* code = jit->compiled_cache[i];
            if (code) {
                printf("    [%zu] %s (bytecode size: %zu)\n",
                       i,
                       code->name ? code->name : "anonymous",
                       code->code.count);
            } else {
                printf("    [%zu] (null)\n", i);
            }
        }
    }
}
