#include "heap.h"
#include "../../system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


static Object* find_free_object_in_pool(ObjectPool* pool) {
    if (!pool || !pool->first) return NULL;
    
    MemoryBlock* block = pool->first;
    while (block) {
        for (size_t i = 0; i < block->used; i++) {
            Object* obj = &block->memory[i];
            // Нашли свободный объект (ref_count == 0)
            if (obj && obj->ref_count == 0) {
                return obj;
            }
        }
        block = block->next;
    }
    return NULL;
}


static void recycle_object(Object* obj) {
    if (!obj) return;
    
    switch (obj->type) {
        case OBJ_ARRAY:
            if (obj->as.array.items) {
                for (size_t i = 0; i < obj->as.array.size; i++) {
                    if (obj->as.array.items[i]) {
                        object_decref(obj->as.array.items[i]);
                    }
                }
                free(obj->as.array.items);
                obj->as.array.items = NULL;
                obj->as.array.size = 0;
            }
            break;
            
        case OBJ_FLOAT:
            if (obj->as.float_value) {
                bigfloat_destroy(obj->as.float_value);
                obj->as.float_value = NULL;
            }
            break;
            
        case OBJ_NATIVE_FUNCTION:
            if (obj->as.native_function.name) {
                free((void*)obj->as.native_function.name);
                obj->as.native_function.name = NULL;
            }
            break;
            
        case OBJ_CODE:
        case OBJ_FUNCTION:
            break;
            
        default:
            break;
    }
    memset(obj, 0, sizeof(Object));
}


static void init_int_cache(Heap* heap) {
    DPRINT("[heap] Initializing int cache for values %d..%d\n", 
           INT_CACHE_MIN, INT_CACHE_MAX);
    
    for (int i = INT_CACHE_MIN; i <= INT_CACHE_MAX; i++) {
        int idx = i - INT_CACHE_MIN;
        
        heap->int_cache[idx] = malloc(sizeof(Object));
        if (!heap->int_cache[idx]) {
            DPRINT("ERROR: Failed to allocate int cache for value %d\n", i);
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
    // 1. Проверяем кэш
    if (v >= INT_CACHE_MIN && v <= INT_CACHE_MAX) {
        return heap->int_cache[v - INT_CACHE_MIN];
    }
    
    heap->total_allocations++;
    
    // 2. Ищем свободный int в пуле
    MemoryBlock* block = heap->int_pool.first;
    while (block) {
        for (size_t i = 0; i < block->used; i++) {
            Object* obj = &block->memory[i];
            if (obj && obj->ref_count == 0) {
                // Переиспользуем!
                obj->type = OBJ_INT;
                obj->ref_count = 1;
                obj->as.int_value = v;
                DPRINT("[HEAP] Reused int %lld at %p\n", 
                        (long long)v, (void*)obj);
                return obj;
            }
        }
        block = block->next;
    }
    
    // 3. Если не нашли - создаем новый
    Object* o = pool_alloc(&heap->int_pool);
    if (!o) {
        DPRINT("ERROR: Failed to allocate int object\n");
        return NULL;
    }

    o->type = OBJ_INT;
    o->ref_count = 1;
    o->as.int_value = v;
    
    DPRINT("[HEAP] New int %lld at %p\n", 
            (long long)v, (void*)o);
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

Object* heap_alloc_array(Heap* heap) {
    heap->total_allocations++;
    
    Object* o = find_free_object_in_pool(&heap->array_pool);
    if (o) {
        DPRINT("[HEAP] Reusing array object at %p\n", (void*)o);
        recycle_object(o);
    } else {
        o = pool_alloc(&heap->array_pool);
        if (!o) {
            DPRINT("ERROR: Failed to allocate array object\n");
            return NULL;
        }
    }
    
    o->type = OBJ_ARRAY;
    o->ref_count = 1;
    o->as.array.items = NULL;
    o->as.array.size = 0;
    
    return o;
}

Object* heap_alloc_array_with_size(Heap* heap, size_t size) {
    Object* array = heap_alloc_array(heap);
    if (!array) {
        return NULL;
    }
    
    array->as.array.items = malloc(size * sizeof(Object*));
    if (!array->as.array.items) {
        DPRINT("ERROR: Failed to allocate array items\n");
        return array;
    }
    
    array->as.array.size = size;
    
    Object* none = heap_alloc_none(heap);
    for (size_t i = 0; i < size; i++) {
        array->as.array.items[i] = none;
    }
    
    return array;
}

Object* heap_alloc_float(Heap* heap, const char* v) {
    heap->total_allocations++;
    
    Object* o = find_free_object_in_pool(&heap->float_pool);
    if (o) {
        DPRINT("[HEAP] Reusing float object at %p\n", (void*)o);
        recycle_object(o);
    } else {
        o = pool_alloc(&heap->float_pool);
        if (!o) {
            DPRINT("ERROR: Failed to allocate float object\n");
            return NULL;
        }
    }
    
    o->type = OBJ_FLOAT;
    o->ref_count = 1;
    o->as.float_value = bigfloat_create(v);
    
    return o;
}

Object* heap_alloc_function(Heap* heap, CodeObj* code) {
    heap->total_allocations++;
    
    Object* o = find_free_object_in_pool(&heap->function_pool);
    if (o) {
        DPRINT("[HEAP] Reusing function object at %p\n", (void*)o);
        recycle_object(o);
    } else {
        o = pool_alloc(&heap->function_pool);
        if (!o) {
            DPRINT("ERROR: Failed to allocate function object\n");
            return NULL;
        }
    }
    
    o->type = OBJ_FUNCTION;
    o->ref_count = 1;
    o->as.function.codeptr = code;
    
    return o;
}

Object* heap_alloc_code(Heap* heap, CodeObj* code) {
    heap->total_allocations++;
    
    Object* o = find_free_object_in_pool(&heap->code_pool);
    if (o) {
        DPRINT("[HEAP] Reusing code object at %p\n", (void*)o);
        recycle_object(o);
    } else {
        o = pool_alloc(&heap->code_pool);
        if (!o) {
            DPRINT("ERROR: Failed to allocate code object\n");
            return NULL;
        }
    }
    
    o->type = OBJ_CODE;
    o->ref_count = 1;
    o->as.codeptr = code;
    
    return o;
}

Object* heap_alloc_native_function(Heap* heap, NativeCFunc c_func, const char* name) {
    heap->total_allocations++;
    
    Object* o = find_free_object_in_pool(&heap->native_func_pool);
    if (o) {
        DPRINT("[HEAP] Reusing native function object at %p\n", (void*)o);
        recycle_object(o);
    } else {
        o = pool_alloc(&heap->native_func_pool);
        if (!o) {
            DPRINT("ERROR: Failed to allocate native function object\n");
            return NULL;
        }
    }
    
    o->type = OBJ_NATIVE_FUNCTION;
    o->ref_count = 1;
    o->as.native_function.c_func = c_func;
    o->as.native_function.name = strdup(name);
    
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
    if (!heap || !debug_enabled) return;
    
    DPRINT("\n=== Heap Statistics ===\n");
    DPRINT("Total allocations: %zu\n", heap->total_allocations);
    
    typedef struct {
        const char* name;
        size_t live;
        size_t free;
        size_t immortal;
        size_t blocks;
    } PoolStats;
    
    PoolStats stats[] = {
        {"Int", 0, 0, 0, 0},
        {"Array", 0, 0, 0, 0},
        {"Function", 0, 0, 0, 0},
        {"Code", 0, 0, 0, 0},
        {"NativeFunc", 0, 0, 0, 0},
        {"Float", 0, 0, 0, 0},
        {"Bool", 0, 0, 0, 0},
        {"None", 0, 0, 0, 0}
    };
    
    ObjectPool* pools[] = {
        &heap->int_pool,
        &heap->array_pool,
        &heap->function_pool,
        &heap->code_pool,
        &heap->native_func_pool,
        &heap->float_pool,
        &heap->bool_pool,
        &heap->none_pool
    };
    
    const char* pool_names[] = {
        "Int", "Array", "Function", "Code", 
        "NativeFunc", "Float", "Bool", "None"
    };
    
    for (int pool_idx = 0; pool_idx < 8; pool_idx++) {
        ObjectPool* pool = pools[pool_idx];
        PoolStats* s = &stats[pool_idx];
        
        s->blocks = pool_block_count(pool);
        
        if (!pool || !pool->first) continue;
        
        MemoryBlock* block = pool->first;
        while (block) {
            for (size_t i = 0; i < block->used; i++) {
                Object* obj = &block->memory[i];
                if (obj) {
                    if (obj->ref_count == 0) {
                        s->free++;           
                    } else if (obj->ref_count == 0x7FFFFFFF) {
                        s->immortal++;       
                    } else {
                        s->live++;           
                    }
                }
            }
            block = block->next;
        }
    }
    
    size_t total_live = 0;
    size_t total_free = 0;
    size_t total_immortal = 0;
    size_t total_objects = 0;
    size_t total_blocks = 0;
    
    for (int i = 0; i < 8; i++) {
        total_live += stats[i].live;
        total_free += stats[i].free;
        total_immortal += stats[i].immortal;
        total_objects += stats[i].live + stats[i].free + stats[i].immortal;
        total_blocks += stats[i].blocks;
    }
    
    DPRINT("Total live objects: %zu\n", total_live);
    DPRINT("Total free objects: %zu\n", total_free);
    DPRINT("Total immortal objects: %zu\n", total_immortal);
    DPRINT("Total objects in pools: %zu\n", total_objects);
    DPRINT("Total memory blocks: %zu\n", total_blocks);
    
    DPRINT("%-12s | %6s | %6s | %6s | %6s | %7s\n", 
            "Pool", "Live", "Free", "Immort", "Blocks", "Eff %");
    DPRINT("------------|--------|--------|--------|--------|--------\n");
    
    for (int i = 0; i < 8; i++) {
        size_t total_in_pool = stats[i].live + stats[i].free + stats[i].immortal;
        double efficiency = (total_in_pool > 0) ? 
            (double)stats[i].live * 100.0 / (total_in_pool) : 0.0;
        
        DPRINT("%-12s | %6zu | %6zu | %6zu | %6zu | %6.1f%%\n",
                pool_names[i],
                stats[i].live,
                stats[i].free,
                stats[i].immortal,
                stats[i].blocks,
                efficiency);
    }
    
    size_t estimated_memory = 0;
    for (int i = 0; i < 8; i++) {
        if (pools[i]->block_size > 0) {
            estimated_memory += stats[i].blocks * pools[i]->block_size * sizeof(Object);
        }
    }
    DPRINT("\nEstimated memory usage: ~%.2f MB\n",
           estimated_memory / (1024.0 * 1024.0));
    DPRINT("=======================\n");
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
