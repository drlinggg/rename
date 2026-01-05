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
    size_t target;
} JumpFixup;

CodeObj* jit_optimize_constant_folding(CodeObj* original, FoldStats* stats);
int jit_can_constant_fold(CodeObj* code);

size_t skip_nops(bytecode_array* bc, size_t start_index);
size_t get_jump_target(bytecode_array* bc, size_t ins_index, uint32_t arg, uint8_t op);
void recalculate_jumps(bytecode_array* bc);
void mark_as_nop(bytecode_array* bc, size_t index);
void mark_as_nops(bytecode_array* bc, size_t start, size_t end);
int is_truthy(Value v);
int is_falsy(Value v);
size_t find_or_add_constant(CodeObj* code, Value v, FoldStats* stats);
int is_constant_foldable(Value a, Value b, uint8_t op);
Value fold_binary_constant(Value a, Value b, uint8_t op);
int is_unary_foldable(Value a, uint8_t op);
Value fold_unary_constant(Value a, uint8_t op);
CodeObj* deep_copy_codeobj(CodeObj* original);

#endif

