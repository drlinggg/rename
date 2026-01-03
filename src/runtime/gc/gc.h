#pragma once

#include "../vm/object.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct GC GC;

// Callback для обработки каждого объекта в sweep phase
typedef void (*GC_ObjectCallback)(void* user_data, Object* obj);

// Функция-итератор для обхода всех объектов в Heap
// Должна вызывать callback для каждого объекта
typedef void (*GC_ObjectIterator)(void* iterator_data, GC_ObjectCallback callback, void* callback_data);

GC* gc_create(void);
void gc_destroy(GC* gc);

// Hooked refcount helpers - currently simple wrappers/no-ops to be replaced by real GC
void gc_incref(GC* gc, Object* o);
void gc_decref(GC* gc, Object* o);

// Mark & Sweep сборка мусора
// roots - массив указателей на Object* (корни)
// roots_count - количество корней
// iterate_all_objects - функция для обхода всех объектов в Heap (может быть NULL)
// iterator_data - данные для функции итератора
void gc_collect(GC* gc, Object** roots, size_t roots_count, 
                GC_ObjectIterator iterate_all_objects, void* iterator_data);

// Упрощенная версия - только mark phase (для тестирования)
void gc_collect_simple(GC* gc, Object** roots, size_t roots_count);

// Регистрация корней (опционально, для удобства)
void gc_add_root(GC* gc, Object* root);
void gc_remove_root(GC* gc, Object* root);
void gc_clear_roots(GC* gc);

// Статистика
size_t gc_get_allocated_count(GC* gc);
size_t gc_get_marked_count(GC* gc);
size_t gc_get_collected_count(GC* gc);

