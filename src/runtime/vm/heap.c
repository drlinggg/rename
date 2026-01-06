#include "heap.h"
#include "../../system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void init_int_cache(Heap* heap) {
    DPRINT("[heap] Initializing int cache for values %d..%d\n", 
           INT_CACHE_MIN, INT_CACHE_MAX);
    
    for (int i = INT_CACHE_MIN; i <= INT_CACHE_MAX; i++) {
        int idx = i - INT_CACHE_MIN;
        
        heap->int_cache[idx] = malloc(sizeof(Object));
        if (!heap->int_cache[idx]) {
            fprintf(stderr, "ERROR: Failed to allocate int cache for value %d\n", i);
            exit(1);
        }
        
        memset(heap->int_cache[idx], 0, sizeof(Object));
        heap->int_cache[idx]->type = OBJ_INT;
        heap->int_cache[idx]->ref_count = 0x7FFFFFFF;
        heap->int_cache[idx]->as.int_value = i;
    }
}

static void free_int_cache(Heap* heap) {
    for (int i = INT_CACHE_MIN; i <= INT_CACHE_MAX; i++) {
        int idx = i - INT_CACHE_MIN;
        if (heap->int_cache[idx]) {
            heap->int_cache[idx]->ref_count = 0;
            free(heap->int_cache[idx]);
            heap->int_cache[idx] = NULL;
        }
    }
}

static Object* get_int_from_cache(Heap* heap, int64_t v) {
    if (v >= INT_CACHE_MIN && v <= INT_CACHE_MAX) {
        return heap->int_cache[v - INT_CACHE_MIN];
    }
    return NULL;
}

static MemoryBlock* block_create(size_t capacity) {
    MemoryBlock* block = malloc(sizeof(MemoryBlock));
    if (!block) return NULL;
    
    block->memory = malloc(capacity * sizeof(Object));
    if (!block->memory) {
        free(block);
        return NULL;
    }
    
    block->capacity = capacity;
    block->used = 0;
    block->next = NULL;
    
    memset(block->memory, 0, capacity * sizeof(Object));
    
    return block;
}

static void block_destroy(MemoryBlock* block) {
    while (block) {
        MemoryBlock* next = block->next;
        if (block->memory) {
            free(block->memory);
        }
        free(block);
        block = next;
    }
}

static Object* block_alloc_bump(MemoryBlock* block) {
    if (block->used >= block->capacity) {
        return NULL;
    }
    
    Object* obj = &block->memory[block->used];
    block->used++;
    
    memset(obj, 0, sizeof(Object));
    return obj;
}


static void pool_init(ObjectPool* pool, size_t block_size) {
    pool->first = NULL;
    pool->current = NULL;
    pool->block_size = block_size;
    pool->total_allocations = 0;
}

static bool pool_add_block(ObjectPool* pool) {
    MemoryBlock* new_block = block_create(pool->block_size);
    if (!new_block) {
        return false;
    }
    
    if (!pool->first) {
        pool->first = new_block;
        pool->current = new_block;
    } else {
        pool->current->next = new_block;
        pool->current = new_block;
    }
    
    DPRINT("[pool] Added new block, total capacity: %zu objects\n",
           pool->block_size * (pool->total_allocations / pool->block_size + 1));
    
    return true;
}

static Object* pool_alloc(ObjectPool* pool) {
    if (!pool->current) {
        if (!pool_add_block(pool)) {
            return NULL;
        }
    }
    
    Object* obj = block_alloc_bump(pool->current);
    
    if (!obj) {
        if (!pool_add_block(pool)) {
            return NULL;
        }
        obj = block_alloc_bump(pool->current);
    }
    
    if (obj) {
        pool->total_allocations++;
    }
    
    return obj;
}

static size_t pool_used_objects(ObjectPool* pool) {
    size_t total = 0;
    MemoryBlock* block = pool->first;
    
    while (block) {
        total += block->used;
        block = block->next;
    }
    
    return total;
}

static void pool_destroy(ObjectPool* pool) {
    block_destroy(pool->first);
    pool->first = NULL;
    pool->current = NULL;
}


Heap* heap_create(void) {
    Heap* heap = malloc(sizeof(Heap));
    if (!heap) return NULL;

    init_int_cache(heap);
    
    pool_init(&heap->int_pool, 2000000);
    pool_init(&heap->array_pool, 100);
    pool_init(&heap->function_pool, 100);
    pool_init(&heap->code_pool, 100);
    pool_init(&heap->native_func_pool, 100);
    pool_init(&heap->float_pool, 10000);

    pool_init(&heap->bool_pool, 2);
    pool_init(&heap->none_pool, 1);

    heap->none_singleton = NULL;
    heap->true_singleton = NULL;
    heap->false_singleton = NULL;
    
    heap->total_allocations = 0;
    
    return heap;
}

void heap_destroy(Heap* heap) {
    if (!heap) return;

    free_int_cache(heap);
    
    pool_destroy(&heap->int_pool);
    pool_destroy(&heap->bool_pool);
    pool_destroy(&heap->none_pool);
    pool_destroy(&heap->array_pool);
    pool_destroy(&heap->function_pool);
    pool_destroy(&heap->code_pool);
    pool_destroy(&heap->native_func_pool);
    pool_destroy(&heap->float_pool);
    
    free(heap);
}

Object* heap_alloc_int(Heap* heap, int64_t v) {
    Object* cached = get_int_from_cache(heap, v);
    if (cached) {
        return cached;
    }
    
    heap->total_allocations++;
    
    Object* o = pool_alloc(&heap->int_pool);
    if (!o) {
        DPRINT("ERROR: Failed to allocate int object\n");
    }

    o->type = OBJ_INT;
    o->ref_count = 1;
    o->as.int_value = v;
    
    return o;
}

Object* heap_alloc_float(Heap* heap, const char* v) {
    heap->total_allocations++;
    
    Object* o = pool_alloc(&heap->float_pool);
    if (!o) {
        DPRINT("ERROR: Failed to allocate float object\n");
    }

    o->type = OBJ_FLOAT;
    o->ref_count = 1;
    o->as.float_value = bigfloat_create(v);
    
    return o;
}

Object* heap_alloc_float_from_bf(Heap* heap, BigFloat* bf) {
    heap->total_allocations++;
    
    Object* o = pool_alloc(&heap->float_pool);
    if (!o) {
        DPRINT("ERROR: Failed to allocate float object from BigFloat\n");
        bigfloat_destroy(bf);
    }

    o->type = OBJ_FLOAT;
    o->ref_count = 1;
    o->as.float_value = bf;
    
    return o;
}

Object* heap_alloc_bool(Heap* heap, bool b) {
    heap->total_allocations++;
    
    if (b) {
        if (!heap->true_singleton) {
            Object* o = pool_alloc(&heap->bool_pool);
            o->type = OBJ_BOOL;
            o->ref_count = 0x7FFFFFFF;
            o->as.bool_value = true;
            heap->true_singleton = o;
        }
        return heap->true_singleton;
    } else {
        if (!heap->false_singleton) {
            Object* o = pool_alloc(&heap->bool_pool);
            o->type = OBJ_BOOL;
            o->ref_count = 0x7FFFFFFF;
            o->as.bool_value = false;
            heap->false_singleton = o;
        }
        return heap->false_singleton;
    }
}

Object* heap_alloc_none(Heap* heap) {
    if (!heap->none_singleton) {
        Object* o = pool_alloc(&heap->none_pool);
        o->type = OBJ_NONE;
        o->ref_count = 0x7FFFFFFF;
        heap->none_singleton = o;
    }
    return heap->none_singleton;
}

Object* heap_alloc_code(Heap* heap, CodeObj* code) {
    heap->total_allocations++;
    
    Object* o = pool_alloc(&heap->code_pool);
    o->type = OBJ_CODE;
    o->ref_count = 1;
    o->as.codeptr = code;
    
    return o;
}

Object* heap_alloc_function(Heap* heap, CodeObj* code) {
    heap->total_allocations++;
    
    Object* o = pool_alloc(&heap->function_pool);
    o->type = OBJ_FUNCTION;
    o->ref_count = 1;
    o->as.function.codeptr = code;
    
    return o;
}

Object* heap_alloc_array(Heap* heap) {
    heap->total_allocations++;
    
    Object* o = pool_alloc(&heap->array_pool);
    o->type = OBJ_ARRAY;
    o->ref_count = 1;
    o->as.array.items = NULL;
    o->as.array.size = 0;
    
    return o;
}

Object* heap_alloc_array_with_size(Heap* heap, size_t size) {
    Object* array = heap_alloc_array(heap);
    
    array->as.array.items = malloc(size * sizeof(Object*));
    array->as.array.size = size;
    
    Object* none = heap_alloc_none(heap);
    for (size_t i = 0; i < size; i++) {
        array->as.array.items[i] = none;
    }
    
    return array;
}

Object* heap_alloc_native_function(Heap* heap, NativeCFunc c_func, const char* name) {
    heap->total_allocations++;
    
    Object* o = pool_alloc(&heap->native_func_pool);
    o->type = OBJ_NATIVE_FUNCTION;
    o->ref_count = 1;
    o->as.native_function.c_func = c_func;
    o->as.native_function.name = strdup(name);
    
    return o;
}

Object* heap_from_value(Heap* heap, Value val) {
    switch (val.type) {
        case VAL_INT:
            return heap_alloc_int(heap, val.int_val);
        case VAL_BOOL:
            return heap_alloc_bool(heap, val.bool_val);
        case VAL_CODE:
            return heap_alloc_code(heap, val.code_val);
        case VAL_FUNCTION:
            return heap_alloc_function(heap, val.code_val);
        case VAL_NONE:
            return heap_alloc_none(heap);
        default:
            return heap_alloc_none(heap);
    }
}

size_t heap_live_objects(Heap* heap) {
    if (!heap) return 0;
    
    return pool_used_objects(&heap->int_pool) +
           pool_used_objects(&heap->bool_pool) +
           pool_used_objects(&heap->none_pool) +
           pool_used_objects(&heap->array_pool) +
           pool_used_objects(&heap->function_pool) +
           pool_used_objects(&heap->code_pool) +
           pool_used_objects(&heap->native_func_pool);
}

static size_t pool_block_count(ObjectPool* pool) {
    size_t count = 0;
    MemoryBlock* block = pool->first;
    
    while (block) {
        count++;
        block = block->next;
    }
    
    return count;
}

void heap_print_stats(Heap* heap) {
    if (!heap) return;
    
    DPRINT("\n=== Heap Statistics ===\n");
    DPRINT("Total allocations: %zu\n", heap->total_allocations);
    DPRINT("Estimated live objects: %zu\n", heap_live_objects(heap));
    DPRINT("\n");
    
    DPRINT("Int pool: %zu objects, %zu blocks\n", 
           pool_used_objects(&heap->int_pool), 
           pool_block_count(&heap->int_pool));
    
    DPRINT("Bool pool: %zu objects, %zu blocks\n",
           pool_used_objects(&heap->bool_pool),
           pool_block_count(&heap->bool_pool));
    
    DPRINT("Array pool: %zu objects, %zu blocks\n",
           pool_used_objects(&heap->array_pool),
           pool_block_count(&heap->array_pool));
    
    DPRINT("Function pool: %zu objects, %zu blocks\n",
           pool_used_objects(&heap->function_pool),
           pool_block_count(&heap->function_pool));
    
    DPRINT("Code pool: %zu objects, %zu blocks\n",
           pool_used_objects(&heap->code_pool),
           pool_block_count(&heap->code_pool));
    
    DPRINT("Native function pool: %zu objects, %zu blocks\n",
           pool_used_objects(&heap->native_func_pool),
           pool_block_count(&heap->native_func_pool));
    
    DPRINT("Total memory used: ~%.2f MB\n",
           (heap->total_allocations * sizeof(Object)) / (1024.0 * 1024.0));
}

// Итерация по всем объектам в пуле
static void pool_iterate_objects(ObjectPool* pool, HeapObjectCallback callback, void* user_data) {
    if (!pool || !callback) return;
    
    MemoryBlock* block = pool->first;
    while (block) {
        for (size_t i = 0; i < block->used; i++) {
            Object* obj = &block->memory[i];
            if (obj) {
                callback(user_data, obj);
            }
        }
        block = block->next;
    }
}

// Итерация по всем объектам в Heap (для GC sweep phase)
void heap_iterate_all_objects(Heap* heap, HeapObjectCallback callback, void* user_data) {
    if (!heap || !callback) return;
    
    // Итерируемся по всем пулам
    pool_iterate_objects(&heap->int_pool, callback, user_data);
    pool_iterate_objects(&heap->array_pool, callback, user_data);
    pool_iterate_objects(&heap->function_pool, callback, user_data);
    pool_iterate_objects(&heap->code_pool, callback, user_data);
    pool_iterate_objects(&heap->native_func_pool, callback, user_data);
    pool_iterate_objects(&heap->float_pool, callback, user_data);
    pool_iterate_objects(&heap->bool_pool, callback, user_data);
    pool_iterate_objects(&heap->none_pool, callback, user_data);
    
    // Также обрабатываем int_cache (но это immortal объекты, их можно пропустить)
}