#include "const_folding.h"
#include "../../system.h"
#include <string.h>
#include <stdlib.h>

int is_truthy(Value v);
int is_falsy(Value v);
size_t find_or_add_constant(CodeObj* code, Value v, FoldStats* stats);
CodeObj* deep_copy_codeobj(CodeObj* original);
int is_constant_foldable(Value a, Value b, uint8_t op);
int is_unary_foldable(Value a, uint8_t op);
Value fold_binary_constant(Value a, Value b, uint8_t op);
Value fold_unary_constant(Value a, uint8_t op);

size_t skip_nops(bytecode_array* bc, size_t start_index) {
    while (start_index < bc->count && bc->bytecodes[start_index].op_code == NOP) {
        start_index++;
    }
    return start_index;
}

size_t get_jump_target(bytecode_array* bc, size_t ins_index, uint32_t arg, uint8_t op) {
    if (op == JUMP_BACKWARD) {
        return ins_index - arg;
    } else if (op == JUMP_FORWARD || op == POP_JUMP_IF_FALSE || op == POP_JUMP_IF_TRUE) {
        return ins_index + 1 + arg;
    }
    return (size_t)-1;
}

int is_truthy(Value v) {
    if (v.type == VAL_BOOL) return v.bool_val;
    if (v.type == VAL_INT) return v.int_val != 0;
    return 0;
}

int is_falsy(Value v) {
    return !is_truthy(v);
}

size_t find_or_add_constant(CodeObj* code, Value v, FoldStats* stats) {
    for (size_t i = 0; i < code->constants_count; i++) {
        if (values_equal(code->constants[i], v)) {
            return i;
        }
    }
    
    size_t new_count = code->constants_count + 1;
    Value* new_consts = realloc(code->constants, new_count * sizeof(Value));
    if (!new_consts) return (size_t)-1;
    
    code->constants = new_consts;
    code->constants[code->constants_count] = v;
    code->constants_count = new_count;
    
    return code->constants_count - 1;
}

CodeObj* deep_copy_codeobj(CodeObj* original) {
    if (!original) return NULL;
    
    CodeObj* copy = malloc(sizeof(CodeObj));
    if (!copy) return NULL;
    
    copy->name = original->name ? strdup(original->name) : NULL;
    copy->local_count = original->local_count;
    copy->arg_count = original->arg_count;
    copy->constants_count = original->constants_count;
    
    if (original->constants_count > 0 && original->constants) {
        copy->constants = malloc(original->constants_count * sizeof(Value));
        if (!copy->constants) {
            free(copy->name);
            free(copy);
            return NULL;
        }
        memcpy(copy->constants, original->constants,
               original->constants_count * sizeof(Value));
    } else {
        copy->constants = NULL;
    }
    
    copy->code.count = original->code.count;
    copy->code.capacity = original->code.count;
    copy->code.bytecodes = malloc(original->code.count * sizeof(bytecode));
    if (!copy->code.bytecodes) {
        free(copy->constants);
        free(copy->name);
        free(copy);
        return NULL;
    }
    memcpy(copy->code.bytecodes, original->code.bytecodes,
           original->code.count * sizeof(bytecode));
    
    return copy;
}

void recalculate_jumps(bytecode_array* bc) {
    size_t* old_to_new = malloc(bc->count * sizeof(size_t));
    size_t* new_to_old = malloc(bc->count * sizeof(size_t));
    size_t new_count = 0;
    
    for (size_t i = 0; i < bc->count; i++) {
        if (bc->bytecodes[i].op_code != NOP) {
            old_to_new[i] = new_count;
            new_to_old[new_count] = i;
            new_count++;
        } else {
            old_to_new[i] = (size_t)-1;
        }
    }
    
    for (size_t new_i = 0; new_i < new_count; new_i++) {
        size_t old_i = new_to_old[new_i];
        bytecode* ins = &bc->bytecodes[old_i];
        uint8_t op = ins->op_code;
        
        if (op != JUMP_FORWARD && op != JUMP_BACKWARD && 
            op != POP_JUMP_IF_FALSE && op != POP_JUMP_IF_TRUE) {
            continue;
        }
        
        uint32_t old_arg = bytecode_get_arg(*ins);
        
        size_t old_target = get_jump_target(bc, old_i, old_arg, op);
        if (old_target >= bc->count) {
            continue;
        }
        
        size_t new_target = old_to_new[old_target];
        if (new_target == (size_t)-1) {
            if (op == JUMP_BACKWARD) {
                for (size_t j = old_target; j > 0; j--) {
                    if (old_to_new[j] != (size_t)-1) {
                        new_target = old_to_new[j];
                        break;
                    }
                }
            } else {
                for (size_t j = old_target; j < bc->count; j++) {
                    if (old_to_new[j] != (size_t)-1) {
                        new_target = old_to_new[j];
                        break;
                    }
                }
            }
            if (new_target == (size_t)-1) continue;
        }
        
        uint32_t new_arg;
        if (op == JUMP_BACKWARD) {
            if (new_target > new_i) continue;
            new_arg = (uint32_t)(new_i - new_target);
        } else {
            if (new_target < new_i + 1) continue;
            new_arg = (uint32_t)(new_target - new_i - 1);
        }
        
        *ins = bytecode_create_with_number(op, new_arg);
    }
    
    bytecode* new_array = malloc(new_count * sizeof(bytecode));
    size_t idx = 0;
    for (size_t i = 0; i < bc->count; i++) {
        if (bc->bytecodes[i].op_code != NOP) {
            new_array[idx++] = bc->bytecodes[i];
        }
    }
    
    free(bc->bytecodes);
    bc->bytecodes = new_array;
    bc->count = new_count;
    bc->capacity = new_count;
    
    free(old_to_new);
    free(new_to_old);
}

void mark_as_nop(bytecode_array* bc, size_t index) {
    if (index < bc->count) {
        bc->bytecodes[index].op_code = NOP;
    }
}

void mark_as_nops(bytecode_array* bc, size_t start, size_t end) {
    for (size_t i = start; i <= end && i < bc->count; i++) {
        bc->bytecodes[i].op_code = NOP;
    }
}

int fold_operation_chain(CodeObj* code, bytecode_array* bc, size_t start, FoldStats* stats) {
    size_t pos = start;
    int changed = 0;
    
    size_t chain_start = pos;
    size_t chain_end = pos;
    
    while (pos < bc->count) {
        bytecode ins = bc->bytecodes[pos];
        uint8_t op = ins.op_code;
        
        if (op == LOAD_CONST) {
            chain_end = pos;
            pos = skip_nops(bc, pos + 1);
        } 
        else if (op == BINARY_OP) {
            uint8_t binop = bytecode_get_arg(ins) & 0xFF;
            if (binop < 0x00 || binop > 0x0B) {
                break;
            }
            chain_end = pos;
            pos = skip_nops(bc, pos + 1);
        }
        else {
            break;
        }
    }
    
    if (chain_end - chain_start < 2) {
        return 0;
    }
    
    Value current_value = value_create_none();
    uint8_t last_op = 0xFF;
    int has_value = 0;
    
    for (size_t i = chain_start; i <= chain_end; i = skip_nops(bc, i + 1)) {
        bytecode ins = bc->bytecodes[i];
        uint8_t op = ins.op_code;
        
        if (op == LOAD_CONST) {
            uint32_t const_idx = bytecode_get_arg(ins);
            if (const_idx >= code->constants_count) break;
            
            Value v = code->constants[const_idx];
            
            if (!has_value) {
                current_value = v;
                has_value = 1;
            } else if (last_op != 0xFF) {
                if (is_constant_foldable(current_value, v, last_op)) {
                    current_value = fold_binary_constant(current_value, v, last_op);
                    changed = 1;
                } else {
                    break;
                }
            }
        }
        else if (op == BINARY_OP) {
            last_op = bytecode_get_arg(ins) & 0xFF;
        }
    }
    
    if (changed && has_value) {
        size_t new_const_idx = find_or_add_constant(code, current_value, stats);
        if (new_const_idx != (size_t)-1) {
            bc->bytecodes[chain_start] = bytecode_create_with_number(LOAD_CONST, new_const_idx);
            
            for (size_t i = chain_start + 1; i <= chain_end; i++) {
                mark_as_nop(bc, i);
                stats->removed_instructions++;
            }
            
            stats->folded_constants++;
            return 1;
        }
    }
    
    return 0;
}

int find_and_fold_chains(CodeObj* code, bytecode_array* bc, FoldStats* stats) {
    int changed = 0;
    
    for (size_t i = 0; i < bc->count; i = skip_nops(bc, i + 1)) {
        if (bc->bytecodes[i].op_code == LOAD_CONST) {
            if (fold_operation_chain(code, bc, i, stats)) {
                changed = 1;
                break;
            }
        }
    }
    
    return changed;
}

void optimize_constant_ifs(CodeObj* code, bytecode_array* bc, FoldStats* stats) {
    for (size_t i = 0; i < bc->count; i = skip_nops(bc, i + 1)) {
        size_t const_idx = skip_nops(bc, i);
        if (const_idx >= bc->count) break;
        
        size_t jump_idx = skip_nops(bc, const_idx + 1);
        if (jump_idx >= bc->count) break;
        
        if (bc->bytecodes[const_idx].op_code == LOAD_CONST &&
            bc->bytecodes[jump_idx].op_code == POP_JUMP_IF_FALSE) {
            
            uint32_t cidx = bytecode_get_arg(bc->bytecodes[const_idx]);
            if (cidx >= code->constants_count) continue;
            
            Value v = code->constants[cidx];
            
            if (is_falsy(v)) {
                uint32_t jump_offset = bytecode_get_arg(bc->bytecodes[jump_idx]);
                
                mark_as_nop(bc, const_idx);
                
                bc->bytecodes[jump_idx] = bytecode_create_with_number(JUMP_FORWARD, jump_offset);
                
                stats->peephole_optimizations++;
                
            } else if (is_truthy(v)) {
                mark_as_nop(bc, const_idx);
                mark_as_nop(bc, jump_idx);
                stats->peephole_optimizations++;
            }
        }
    }
}

int is_constant_foldable(Value a, Value b, uint8_t op) {
    if (op >= 0x00 && op <= 0x0B) {
        return a.type == VAL_INT && b.type == VAL_INT;
    }
    
    if (op >= 0x50 && op <= 0x55) {
        return (a.type == VAL_INT && b.type == VAL_INT) ||
               (a.type == VAL_BOOL && b.type == VAL_BOOL);
    }
    
    if (op == 0x60 || op == 0x61) {
        return a.type == VAL_BOOL && b.type == VAL_BOOL;
    }
    
    return 0;
}

int is_unary_foldable(Value a, uint8_t op) {
    if (op == 0x00 || op == 0x01) {
        return a.type == VAL_INT;
    }
    
    if (op == 0x03) {
        return a.type == VAL_BOOL;
    }
    
    return 0;
}

Value fold_binary_constant(Value a, Value b, uint8_t op) {
    if (a.type == VAL_INT && b.type == VAL_INT) {
        int64_t x = a.int_val;
        int64_t y = b.int_val;
        
        switch (op) {
            case 0x00: return value_create_int(x + y);
            case 0x0A: return value_create_int(x - y);
            case 0x05: return value_create_int(x * y);
            case 0x0B: return value_create_int(y != 0 ? x / y : 0);
            case 0x06: return value_create_int(y != 0 ? x % y : 0);
            
            case 0x50: return value_create_bool(x == y);
            case 0x51: return value_create_bool(x != y);
            case 0x52: return value_create_bool(x < y);
            case 0x53: return value_create_bool(x <= y);
            case 0x54: return value_create_bool(x > y);
            case 0x55: return value_create_bool(x >= y);
        }
    }
    
    if (a.type == VAL_BOOL && b.type == VAL_BOOL) {
        bool x = a.bool_val;
        bool y = b.bool_val;
        
        switch (op) {
            case 0x60: return value_create_bool(x && y);
            case 0x61: return value_create_bool(x || y);
            case 0x50: return value_create_bool(x == y);
            case 0x51: return value_create_bool(x != y);
        }
    }
    
    return value_create_none();
}

Value fold_unary_constant(Value a, uint8_t op) {
    if (a.type == VAL_INT) {
        int64_t x = a.int_val;
        switch (op) {
            case 0x00: return value_create_int(+x);
            case 0x01: return value_create_int(-x);
        }
    }
    
    if (a.type == VAL_BOOL && op == 0x03) {
        return value_create_bool(!a.bool_val);
    }
    
    return value_create_none();
}

int aggressive_constant_folding(CodeObj* code, bytecode_array* bc, FoldStats* stats) {
    int changed = 0;
    
    int local_changed;
    do {
        local_changed = 0;
        
        for (size_t i = 0; i < bc->count; i = skip_nops(bc, i + 1)) {
            if (i + 2 < bc->count) {
                bytecode ins1 = bc->bytecodes[i];
                bytecode ins2 = bc->bytecodes[i + 1];
                bytecode ins3 = bc->bytecodes[skip_nops(bc, i + 2)];
                
                if (ins1.op_code == LOAD_CONST && 
                    ins2.op_code == LOAD_CONST && 
                    ins3.op_code == BINARY_OP) {
                    
                    uint32_t const_idx1 = bytecode_get_arg(ins1);
                    uint32_t const_idx2 = bytecode_get_arg(ins2);
                    uint8_t binop = bytecode_get_arg(ins3) & 0xFF;
                    
                    if (const_idx1 < code->constants_count && 
                        const_idx2 < code->constants_count) {
                        
                        Value a = code->constants[const_idx1];
                        Value b = code->constants[const_idx2];
                        
                        if (is_constant_foldable(a, b, binop)) {
                            Value result = fold_binary_constant(a, b, binop);
                            size_t new_idx = find_or_add_constant(code, result, stats);
                            
                            if (new_idx != (size_t)-1) {
                                bc->bytecodes[i] = bytecode_create_with_number(LOAD_CONST, new_idx);
                                mark_as_nops(bc, i + 1, i + 2);
                                stats->folded_constants++;
                                stats->removed_instructions += 2;
                                local_changed = 1;
                                changed = 1;
                            }
                        }
                    }
                }
            }
        }
        
        if (local_changed) {
            recalculate_jumps(bc);
        }
        
    } while (local_changed);
    
    return changed;
}

CodeObj* jit_optimize_constant_folding(CodeObj* original, FoldStats* stats) {
    if (!original || !original->code.bytecodes || !original->constants) {
        return NULL;
    }
    
    DPRINT("[JIT-CF] Starting optimization for '%s'\n",
           original->name ? original->name : "anonymous");
    
    memset(stats, 0, sizeof(FoldStats));
    
    CodeObj* optimized = deep_copy_codeobj(original);
    if (!optimized) {
        return NULL;
    }
    
    bytecode_array* bc = &optimized->code;
    int changed;
    int iteration = 0;
    
    do {
        changed = 0;
        iteration++;
        
        DPRINT("[JIT-CF] Iteration %d\n", iteration);
        
        if (aggressive_constant_folding(optimized, bc, stats)) {
            changed = 1;
        }
        
        if (find_and_fold_chains(optimized, bc, stats)) {
            changed = 1;
            recalculate_jumps(bc);
        }
        
        optimize_constant_ifs(optimized, bc, stats);
        if (stats->peephole_optimizations > 0) {
            changed = 1;
            recalculate_jumps(bc);
            stats->peephole_optimizations = 0;
        }
        
        if (iteration > 100) {
            DPRINT("[JIT-CF] Warning: reached iteration limit\n");
            break;
        }
        
    } while (changed && bc->count > 0);
    
    recalculate_jumps(bc);
    
    DPRINT("[JIT-CF] Optimization complete:\n");
    DPRINT("  Folded constants: %zu\n", stats->folded_constants);
    DPRINT("  Removed instructions: %zu\n", stats->removed_instructions);
    DPRINT("  Original size: %zu, Optimized size: %zu\n", 
           original->code.count, bc->count);
    
    return optimized;
}
