#ifndef CONST_FOLDING_H
#define CONST_FOLDING_H

#include "../../compiler/bytecode.h"
#include "../../compiler/value.h"
#include "../../runtime/vm/vm.h"

typedef struct {
    size_t folded_constants;
    size_t removed_instructions;
    size_t peephole_optimizations;
} FoldStats;

typedef struct {
    size_t insn;      // индекс jump-инструкции
    size_t target;    // индекс целевой инструкции (до компакции)
} JumpFixup;


CodeObj* jit_optimize_constant_folding(CodeObj* original, FoldStats* stats);
int jit_can_constant_fold(CodeObj* code);

#endif // CONST_FOLDING_H
