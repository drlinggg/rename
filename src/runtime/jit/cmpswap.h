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
    size_t array_local_idx;
    size_t index_local_idx;
    size_t index2_local_idx;
    size_t temp_local_idx;
    size_t const_one_idx;
    int pattern_type;
} PatternMatch;

int has_sorting_pattern(CodeObj* code);
CodeObj* jit_optimize_cmpswap(CodeObj* original, CmpswapStats* stats);

#endif
