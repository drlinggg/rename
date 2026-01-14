#include "gc.h"
#include "../vm/object.h"
#include "../../system.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define GC_MARK_TABLE_SIZE 1024
#define GC_MARK_TABLE_LOAD_FACTOR 0.75

typedef struct {
    Object** marked_objects;
    size_t capacity;
    size_t size;
    size_t threshold;
} MarkTable;

struct GC {
    size_t allocated_count;
    size_t marked_count;
    size_t collected_count;

    MarkTable mark_table;

    Object** roots;
    size_t roots_capacity;
    size_t roots_count;
};

static size_t hash_pointer(Object* obj) {
    return ((size_t)obj) % GC_MARK_TABLE_SIZE;
}

static void mark_table_init(MarkTable* table) {
    table->capacity = GC_MARK_TABLE_SIZE;
    table->size = 0;
    table->threshold = (size_t)(GC_MARK_TABLE_SIZE * GC_MARK_TABLE_LOAD_FACTOR);
    table->marked_objects = calloc(table->capacity, sizeof(Object*));
}

static void mark_table_clear(MarkTable* table) {
    if (table->marked_objects) {
        memset(table->marked_objects, 0, table->capacity * sizeof(Object*));
    }
    table->size = 0;
}

static bool mark_table_add(MarkTable* table, Object* obj) {
    if (!obj || !table->marked_objects) return false;

    if (table->size >= table->threshold) {
        size_t new_capacity = table->capacity * 2;
        Object** new_table = calloc(new_capacity, sizeof(Object*));
        if (!new_table) return false;

        for (size_t i = 0; i < table->capacity; i++) {
            if (table->marked_objects[i]) {
                size_t hash = ((size_t)table->marked_objects[i]) % new_capacity;
                while (new_table[hash] != NULL) {
                    hash = (hash + 1) % new_capacity;
                }
                new_table[hash] = table->marked_objects[i];
            }
        }
        
        free(table->marked_objects);
        table->marked_objects = new_table;
        table->capacity = new_capacity;
        table->threshold = (size_t)(new_capacity * GC_MARK_TABLE_LOAD_FACTOR);
    }

    size_t hash = hash_pointer(obj) % table->capacity;
    while (table->marked_objects[hash] != NULL && table->marked_objects[hash] != obj) {
        hash = (hash + 1) % table->capacity;
    }
    
    if (table->marked_objects[hash] == NULL) {
        table->marked_objects[hash] = obj;
        table->size++;
        return true;
    }
    
    return false;
}

static bool mark_table_contains(MarkTable* table, Object* obj) {
    if (!obj || !table->marked_objects) return false;
    
    size_t hash = hash_pointer(obj) % table->capacity;
    size_t start_hash = hash;
    
    while (table->marked_objects[hash] != NULL) {
        if (table->marked_objects[hash] == obj) {
            return true;
        }
        hash = (hash + 1) % table->capacity;
        if (hash == start_hash) break;
    }
    
    return false;
}

static void mark_table_destroy(MarkTable* table) {
    if (table->marked_objects) {
        free(table->marked_objects);
        table->marked_objects = NULL;
    }
    table->capacity = 0;
    table->size = 0;
}

GC* gc_create(void) {
    GC* g = malloc(sizeof(GC));
    if (!g) return NULL;
    
    g->allocated_count = 0;
    g->marked_count = 0;
    g->collected_count = 0;
    
    mark_table_init(&g->mark_table);
    
    g->roots = NULL;
    g->roots_capacity = 0;
    g->roots_count = 0;
    
    return g;
}

void gc_destroy(GC* gc) {
    if (!gc) return;
    
    mark_table_destroy(&gc->mark_table);
    
    if (gc->roots) {
        free(gc->roots);
    }
    
    free(gc);
}

void gc_incref(GC* gc, Object* o) {
    (void)gc;
    if (!o) return;
    extern void object_incref(Object* o);
    object_incref(o);
}

void gc_decref(GC* gc, Object* o) {
    (void)gc;
    if (!o) return;
    extern void object_decref(Object* o);
    object_decref(o);
}

static void gc_mark_object(GC* gc, Object* obj) {
    if (!obj || !gc) return;

    if (mark_table_contains(&gc->mark_table, obj)) {
        return;
    }

    if (obj->ref_count == 0x7FFFFFFF) {
        return;
    }

    if (mark_table_add(&gc->mark_table, obj)) {
        gc->marked_count++;
    }

    switch (obj->type) {
        case OBJ_ARRAY:
            if (obj->as.array.items) {
                for (size_t i = 0; i < obj->as.array.size; i++) {
                    if (obj->as.array.items[i]) {
                        gc_mark_object(gc, obj->as.array.items[i]);
                    }
                }
            }
            break;
            
        case OBJ_FUNCTION:
            break;
            
        case OBJ_CODE:
            break;
            
        case OBJ_NATIVE_FUNCTION:
            break;
            
        case OBJ_INT:
        case OBJ_BOOL:
        case OBJ_NONE:
        case OBJ_FLOAT:
            break;
            
        default:
            break;
    }
}

static void gc_mark_phase(GC* gc, Object** roots, size_t roots_count) {
    if (!gc) return;

    mark_table_clear(&gc->mark_table);
    gc->marked_count = 0;
    
    DPRINT("[GC] Mark phase: starting with %zu roots\n", roots_count);

    for (size_t i = 0; i < roots_count; i++) {
        if (roots[i]) {
            gc_mark_object(gc, roots[i]);
        }
    }
    
    DPRINT("[GC] Mark phase: marked %zu objects\n", gc->marked_count);
}

typedef struct {
    GC* gc;
    Object** unmarked;
    size_t count;
    size_t capacity;
} SweepContext;

static void sweep_collect_callback(void* user_data, Object* obj) {
    SweepContext* ctx = (SweepContext*)user_data;
    if (!ctx || !obj) return;

    if (obj->ref_count == 0x7FFFFFFF) {
        return;
    }

    if (!mark_table_contains(&ctx->gc->mark_table, obj)) {
        if (ctx->count >= ctx->capacity) {
            ctx->capacity = ctx->capacity == 0 ? 64 : ctx->capacity * 2;
            ctx->unmarked = realloc(ctx->unmarked, ctx->capacity * sizeof(Object*));
            if (!ctx->unmarked) return;
        }
        ctx->unmarked[ctx->count++] = obj;
    }
}

static void gc_sweep_phase(GC* gc, GC_ObjectIterator iterate_all_objects, void* iterator_data) {
    if (!gc) return;
    
    size_t collected = 0;
    
    DPRINT("[GC] Sweep phase: starting\n");

    if (!iterate_all_objects) {
        DPRINT("[GC] Sweep phase: no iterator provided, skipping\n");
        return;
    }

    SweepContext ctx = {0};
    ctx.gc = gc;
    ctx.capacity = 64;
    ctx.unmarked = malloc(ctx.capacity * sizeof(Object*));
    
    if (!ctx.unmarked) {
        DPRINT("[GC] Sweep phase: failed to allocate memory\n");
        return;
    }

    iterate_all_objects(iterator_data, sweep_collect_callback, &ctx);

    for (size_t i = 0; i < ctx.count; i++) {
        Object* obj = ctx.unmarked[i];
        if (obj) {
            DPRINT("[GC] Sweep: collecting object type=%d at %p\n", obj->type, (void*)obj);

            switch (obj->type) {
                case OBJ_ARRAY:
                    if (obj->as.array.items) {
                        free(obj->as.array.items);
                        obj->as.array.items = NULL;
                        obj->as.array.size = 0;
                    }
                    break;
                    
                case OBJ_NATIVE_FUNCTION:
                    if (obj->as.native_function.name) {
                        free((void*)obj->as.native_function.name);
                        obj->as.native_function.name = NULL;
                    }
                    break;
                    
                case OBJ_FLOAT:
                    if (obj->as.float_value) {
                        extern void bigfloat_destroy(BigFloat*);
                        bigfloat_destroy(obj->as.float_value);
                        obj->as.float_value = NULL;
                    }
                    break;
                    
                default:
                    break;
            }
            obj->ref_count = 0;
            collected++;
        }
    }
    
    free(ctx.unmarked);
    
    gc->collected_count += collected;
    DPRINT("[GC] Sweep phase: collected %zu objects\n", collected);
}

void gc_collect(GC* gc, Object** roots, size_t roots_count,
                GC_ObjectIterator iterate_all_objects, void* iterator_data) {
    if (!gc) return;
    
    DPRINT("[GC] Starting garbage collection...\n");

    gc_mark_phase(gc, roots, roots_count);

    if (iterate_all_objects) {
        gc_sweep_phase(gc, iterate_all_objects, iterator_data);
    }
    
    DPRINT("[GC] Garbage collection completed. Marked: %zu, Collected: %zu\n",
           gc->marked_count, gc->collected_count);
}

void gc_collect_simple(GC* gc, Object** roots, size_t roots_count) {
    if (!gc) return;
    gc_mark_phase(gc, roots, roots_count);
}

void gc_add_root(GC* gc, Object* root) {
    if (!gc || !root) return;
    
    if (gc->roots_count >= gc->roots_capacity) {
        size_t new_capacity = gc->roots_capacity == 0 ? 16 : gc->roots_capacity * 2;
        Object** new_roots = realloc(gc->roots, new_capacity * sizeof(Object*));
        if (!new_roots) return;
        gc->roots = new_roots;
        gc->roots_capacity = new_capacity;
    }
    
    gc->roots[gc->roots_count++] = root;
}

void gc_remove_root(GC* gc, Object* root) {
    if (!gc || !root) return;
    
    for (size_t i = 0; i < gc->roots_count; i++) {
        if (gc->roots[i] == root) {
            for (size_t j = i; j < gc->roots_count - 1; j++) {
                gc->roots[j] = gc->roots[j + 1];
            }
            gc->roots_count--;
            return;
        }
    }
}

void gc_clear_roots(GC* gc) {
    if (!gc) return;
    gc->roots_count = 0;
}

size_t gc_get_allocated_count(GC* gc) {
    return gc ? gc->allocated_count : 0;
}

size_t gc_get_marked_count(GC* gc) {
    return gc ? gc->marked_count : 0;
}

size_t gc_get_collected_count(GC* gc) {
    return gc ? gc->collected_count : 0;
}
