#ifndef HEAP_H
#define HEAP_H

#include "object.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define INT_CACHE_MIN -1000000
#define INT_CACHE_MAX 1000000
#define INT_CACHE_SIZE (INT_CACHE_MAX - INT_CACHE_MIN + 1)

typedef struct MemoryBlock {
    Object* memory;           // Память блока
    size_t capacity;          // Вместимость блока (в объектах)
    size_t used;              // Использовано в этом блоке
    struct MemoryBlock* next; // Следующий блок
} MemoryBlock;

typedef struct ObjectPool {
    MemoryBlock* first;       // Первый блок
    MemoryBlock* current;     // Текущий блок для аллокаций
    size_t block_size;        // Размер одного блока (в объектах)
    size_t total_allocations; // Всего аллокаций в этом пуле
} ObjectPool;

typedef struct Heap {
    // Пул для каждого типа объектов
    ObjectPool int_pool;
    ObjectPool array_pool;
    ObjectPool function_pool;
    ObjectPool code_pool;
    ObjectPool native_func_pool;
    ObjectPool float_pool;

    // Синглтоны для часто используемых значений
    ObjectPool bool_pool;
    ObjectPool none_pool;
    Object* none_singleton;
    Object* true_singleton;
    Object* false_singleton;

    Object* int_cache[INT_CACHE_SIZE];
    
    // Статистика
    size_t total_allocations;
} Heap;

// Создание и уничтожение
Heap* heap_create(void);
void heap_destroy(Heap* heap);

// Аллокация объектов
Object* heap_alloc_int(Heap* heap, int64_t v);
Object* heap_alloc_bool(Heap* heap, bool b);
Object* heap_alloc_none(Heap* heap);
Object* heap_alloc_float(Heap* heap, const char* v);
Object* heap_alloc_float_from_bf(Heap* heap, BigFloat* bf);  // Новая функция
Object* heap_alloc_code(Heap* heap, CodeObj* code);
Object* heap_alloc_function(Heap* heap, CodeObj* code);
Object* heap_alloc_array(Heap* heap);
Object* heap_alloc_array_with_size(Heap* heap, size_t size);
Object* heap_alloc_native_function(Heap* heap, NativeCFunc c_func, const char* name);

// Утилиты
Object* heap_from_value(Heap* heap, Value val);
size_t heap_live_objects(Heap* heap);
void heap_print_stats(Heap* heap);

// GC интеграция: итерация по всем объектам
typedef void (*HeapObjectCallback)(void* user_data, Object* obj);
void heap_iterate_all_objects(Heap* heap, HeapObjectCallback callback, void* user_data);

#endif // HEAP_H
