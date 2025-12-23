#ifndef CMPSWAP_H
#define CMPSWAP_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Структура для паттерна поиска
typedef struct {
    uint32_t array_index;      // Index of array in locals
    uint32_t j_index;          // Index of loop variable j
    uint32_t const_one_index;  // Index of constant 1
} PatternInfo;

int is_sorting_function(void* code);
void* jit_optimize_compare_and_swap(void* jit, void* original);

#endif // CMPSWAP_H
