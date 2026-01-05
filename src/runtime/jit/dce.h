#ifndef DCE_H
#define DCE_H

#include "../../system.h"
#include "../../compiler/bytecode.h"
#include "../../compiler/value.h"

typedef struct {
    size_t removed_calls;
    size_t removed_instructions;
    size_t optimized_functions;
    size_t side_effect_calls;
} DCEStats;

CodeObj* jit_optimize_dce(CodeObj* code, DCEStats* stats);

#endif // DCE_H
