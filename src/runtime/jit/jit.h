#pragma once

#include "../vm/object.h"

typedef struct JIT JIT;

JIT* jit_create(void);
void jit_destroy(JIT* jit);

// placeholder - compile a CodeObj into optimized representation
CodeObj* jit_compile_function(JIT* jit, CodeObj* code);

CodeObj* jit_optimize_sorting_function(JIT* jit, CodeObj* code);

// Проверяет, является ли функция сортировкой
int is_sorting_function(CodeObj* code);

// Находит паттерн сравнения в байткоде
int find_sorting_pattern(bytecode_array* code, size_t* pattern_start, size_t* pattern_end);
// Заменяет паттерн на COMPARE_AND_SWAP
int replace_with_compare_and_swap(bytecode_array* code, size_t pattern_start, size_t pattern_end);
