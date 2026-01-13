#ifndef CMPSWAP_H
#define CMPSWAP_H

#include "../../compiler/bytecode.h"
#include "../../compiler/value.h"

typedef struct {
    size_t optimized_patterns;
    size_t swap_patterns;
    size_t cmpswap_patterns;
} CmpswapStats;

typedef struct {
    size_t array_local_idx;   // индекс массива в локальных переменных
    size_t index_local_idx;   // индекс переменной цикла (j/i)
    size_t index2_local_idx;  // второй индекс (для swap)
    size_t temp_local_idx;    // индекс временной переменной (если есть)
    size_t const_one_idx;     // индекс константы 1
    int pattern_type;         // 0 = conditional (cmp), 1 = unconditional (swap)
} PatternMatch;

int has_sorting_pattern(CodeObj* code);
CodeObj* jit_optimize_cmpswap(CodeObj* original, CmpswapStats* stats);

#endif // CMPSWAP_H
