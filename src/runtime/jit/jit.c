#include "../../system.h"
#include "cmpswap.h"
#include "const_folding.h"
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
    
    // Очищаем кэш
    for (size_t i = 0; i < jit->cache_size; i++) {
        CodeObj* code = jit->compiled_cache[i];
        if (code) {
            // Освобождаем CodeObj из кэша
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

// Проверяет, есть ли функция уже в кэше
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

// Добавляет функцию в кэш
static void jit_add_to_cache(JIT* jit, CodeObj* optimized_code) {
    if (!jit || !optimized_code) return;
    
    // Проверяем, не переполнен ли кэш
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

// Вспомогательная функция для создания глубокой копии
static CodeObj* deep_copy_codeobj(CodeObj* original) {
    if (!original) return NULL;
    
    CodeObj* copy = malloc(sizeof(CodeObj));
    if (!copy) return NULL;
    
    copy->name = original->name ? strdup(original->name) : NULL;
    copy->local_count = original->local_count;
    copy->arg_count = original->arg_count;
    copy->constants_count = original->constants_count;
    
    if (original->constants_count > 0 && original->constants) {
        copy->constants = malloc(original->constants_count * sizeof(Value));
        if (!copy->constants) {
            free(copy->name);
            free(copy);
            return NULL;
        }
        memcpy(copy->constants, original->constants,
               original->constants_count * sizeof(Value));
    } else {
        copy->constants = NULL;
    }
    
    copy->code.count = original->code.count;
    copy->code.capacity = original->code.count;
    copy->code.bytecodes = malloc(original->code.count * sizeof(bytecode));
    if (!copy->code.bytecodes) {
        free(copy->constants);
        free(copy->name);
        free(copy);
        return NULL;
    }
    memcpy(copy->code.bytecodes, original->code.bytecodes,
           original->code.count * sizeof(bytecode));
    
    return copy;
}

// Основная функция JIT-компиляции
// Основная функция JIT-компиляции
void* jit_compile_function(JIT* jit, void* code_ptr) {
    if (!jit || !code_ptr) {
        DPRINT("[JIT] Invalid parameters\n");
        return code_ptr;
    }
    
    CodeObj* original = (CodeObj*)code_ptr;
    
    DPRINT("[JIT] Compiling function: %s\n",
           original->name ? original->name : "anonymous");
    
    // 1. Проверяем кэш
    CodeObj* cached = jit_find_in_cache(jit, original);
    if (cached) {
        return cached;
    }
    
    CodeObj* optimized = original;
    bool was_optimized = false;
    
    // 2. Применяем свертку констант
    FoldStats cf_stats;
    CodeObj* cf_result = jit_optimize_constant_folding(original, &cf_stats);
    
    if (cf_result && cf_result != original) {
        DPRINT("[JIT] Constant folding: folded %zu constants, removed %zu instructions\n",
               cf_stats.folded_constants, cf_stats.removed_instructions);
        optimized = cf_result;
        was_optimized = true;
    }
    
    // 3. Применяем оптимизацию compare-and-swap
    CmpswapStats cas_stats;
    CodeObj* cas_result = jit_optimize_cmpswap(optimized, &cas_stats);
    
    if (cas_result && cas_result != optimized) {
        DPRINT("[JIT] Compare-and-swap: optimized %zu patterns\n",
               cas_stats.optimized_patterns);
        
        // Освобождаем предыдущую оптимизированную версию, если нужно
        if (was_optimized && optimized != original) {
            free_code_obj(optimized);
        }
        
        optimized = cas_result;
        was_optimized = true;
    }
    
    // 4. Добавляем в кэш, если функция была оптимизирована
    if (was_optimized && optimized) {
        jit_add_to_cache(jit, optimized);
        
        DPRINT("[JIT] Optimized bytecode for '%s':\n",
               optimized->name ? optimized->name : "anonymous");
        bytecode_array_print(&optimized->code);
        
        return optimized;
    }
    
    // 5. Если оптимизаций не было, но функция еще не в кэше
    // Создаем копию и добавляем в кэш для будущего использования
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

// Дополнительная функция для принудительной очистки кэша
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

// Функция для получения статистики кэша
JITStats jit_get_stats(JIT* jit) {
    JITStats stats = {0};
    
    if (jit) {
        stats.cache_size = jit->cache_size;
        stats.cache_capacity = jit->cache_capacity;
        stats.compiled_count = jit->compiled_count;
    }
    
    return stats;
}

// Функция для отладки - печатает содержимое кэша
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