
#include "const_folding.h"
#include "../../system.h"
#include <string.h>
#include <stdlib.h>

static int is_truthy(Value v) {
    if (v.type == VAL_BOOL) return v.bool_val;
    if (v.type == VAL_INT)  return v.int_val != 0;
    return 0;
}

static int is_falsy(Value v) {
    return !is_truthy(v);
}

int is_constant_foldable(Value a, Value b, uint8_t op) {
    if (op >= 0x00 && op <= 0x0B) { // ADD, SUB, MUL, DIV, MOD
        return a.type == VAL_INT && b.type == VAL_INT;
    }
    
    if (op >= 0x50 && op <= 0x55) { // EQ, NE, LT, LE, GT, GE
        return (a.type == VAL_INT && b.type == VAL_INT) ||
               (a.type == VAL_BOOL && b.type == VAL_BOOL);
    }
    
    if (op == 0x60 || op == 0x61) { // AND, OR
        return a.type == VAL_BOOL && b.type == VAL_BOOL;
    }
    
    return 0;
}

int is_unary_foldable(Value a, uint8_t op) {
    if (op == 0x00 || op == 0x01) { // POSITIVE, NEGATIVE
        return a.type == VAL_INT;
    }
    
    if (op == 0x03) { // NOT
        return a.type == VAL_BOOL;
    }
    
    return 0;
}

Value fold_binary_constant(Value a, Value b, uint8_t op) {
    if (a.type == VAL_INT && b.type == VAL_INT) {
        int64_t x = a.int_val;
        int64_t y = b.int_val;
        
        switch (op) {
            case 0x00: return value_create_int(x + y);      // ADD
            case 0x0A: return value_create_int(x - y);      // SUB
            case 0x05: return value_create_int(x * y);      // MUL
            case 0x0B: return value_create_int(y != 0 ? x / y : 0); // DIV
            case 0x06: return value_create_int(y != 0 ? x % y : 0); // MOD
            
            case 0x50: return value_create_bool(x == y);    // EQ
            case 0x51: return value_create_bool(x != y);    // NE
            case 0x52: return value_create_bool(x < y);     // LT
            case 0x53: return value_create_bool(x <= y);    // LE
            case 0x54: return value_create_bool(x > y);     // GT
            case 0x55: return value_create_bool(x >= y);    // GE
        }
    }
    
    if (a.type == VAL_BOOL && b.type == VAL_BOOL) {
        bool x = a.bool_val;
        bool y = b.bool_val;
        
        switch (op) {
            case 0x60: return value_create_bool(x && y);    // AND
            case 0x61: return value_create_bool(x || y);    // OR
            case 0x50: return value_create_bool(x == y);    // EQ
            case 0x51: return value_create_bool(x != y);    // NE
        }
    }
    
    return value_create_none();
}

Value fold_unary_constant(Value a, uint8_t op) {
    if (a.type == VAL_INT) {
        int64_t x = a.int_val;
        switch (op) {
            case 0x00: return value_create_int(+x);        // POSITIVE
            case 0x01: return value_create_int(-x);        // NEGATIVE
        }
    }
    
    if (a.type == VAL_BOOL && op == 0x03) {
        return value_create_bool(!a.bool_val);            // NOT
    }
    
    return value_create_none();
}

// Оптимизация константных условий с учётом ip++
static void optimize_constant_conditions(CodeObj* code, bytecode_array* bc, FoldStats* stats) {
    for (size_t i = 0; i + 1 < bc->count; i++) {
        if (bc->bytecodes[i].op_code != LOAD_CONST) continue;
        if (bc->bytecodes[i + 1].op_code != POP_JUMP_IF_FALSE) continue;
        
        uint32_t const_idx = bytecode_get_arg(bc->bytecodes[i]);
        if (const_idx >= code->constants_count) continue;
        
        Value v = code->constants[const_idx];
        
        if (is_falsy(v)) {
            uint32_t jump_offset = bytecode_get_arg(bc->bytecodes[i + 1]);
            bc->bytecodes[i].op_code = NOP;
            bc->bytecodes[i + 1] = bytecode_create_with_number(JUMP_FORWARD, jump_offset);
            stats->peephole_optimizations++;
        } else if (is_truthy(v)) {
            bc->bytecodes[i].op_code = NOP;
            bc->bytecodes[i + 1].op_code = NOP;
            stats->peephole_optimizations++;
        }
    }
}

// Строим карту живых инструкций
static void build_instruction_map(bytecode_array* bc, size_t* map) {
    size_t new_index = 0;
    for (size_t i = 0; i < bc->count; i++) {
        map[i] = (bc->bytecodes[i].op_code == NOP) ? (size_t)-1 : new_index++;
    }
}

// Пересчитываем переходы
static void rebuild_jumps(bytecode_array* bc, size_t* map) {
    for (size_t i = 0; i < bc->count; i++) {
        bytecode* ins = &bc->bytecodes[i];
        uint8_t op = ins->op_code;
        if (op != JUMP_FORWARD && op != JUMP_BACKWARD && op != POP_JUMP_IF_FALSE) continue;
        
        uint32_t old_offset = bytecode_get_arg(*ins);
        size_t old_target = (op == JUMP_BACKWARD) ? i - old_offset : i + old_offset;
        
        size_t new_i = map[i];
        size_t new_target = map[old_target];
        
        if (new_target == (size_t)-1) {
            for (size_t j = old_target + 1; j < bc->count; j++) {
                if (map[j] != (size_t)-1) { new_target = map[j]; break; }
            }
        }
        if (new_target == (size_t)-1) new_target = new_i + 1;
        
        uint32_t new_offset = (op == JUMP_BACKWARD) ? (uint32_t)(new_i - new_target)
                                                     : (uint32_t)(new_target - new_i);
        *ins = bytecode_create_with_number(op, new_offset);
    }
}

// Компактируем байткод
static void compact_bytecode_array(bytecode_array* bc) {
    bytecode* new_array = malloc(sizeof(bytecode) * bc->count);
    size_t new_count = 0;
    for (size_t i = 0; i < bc->count; i++) {
        if (bc->bytecodes[i].op_code != NOP) new_array[new_count++] = bc->bytecodes[i];
    }
    free(bc->bytecodes);
    bc->bytecodes = new_array;
    bc->count = new_count;
    bc->capacity = new_count;
}

// Добавление константы, если ещё нет
static size_t find_or_add_constant(CodeObj* code, Value v, FoldStats* stats) {
    for (size_t i = 0; i < code->constants_count; i++) {
        if (values_equal(code->constants[i], v)) return i;
    }
    
    size_t new_count = code->constants_count + 1;
    Value* new_consts = realloc(code->constants, new_count * sizeof(Value));
    if (!new_consts) return (size_t)-1;
    
    code->constants = new_consts;
    code->constants[code->constants_count] = v;
    code->constants_count = new_count;
    
    return code->constants_count - 1;
}

// Копируем CodeObj
static CodeObj* deep_copy_codeobj(CodeObj* original) {
    if (!original) return NULL;
    CodeObj* copy = malloc(sizeof(CodeObj));
    if (!copy) return NULL;
    
    copy->name = original->name ? strdup(original->name) : NULL;
    copy->local_count = original->local_count;
    copy->arg_count = original->arg_count;
    copy->constants_count = original->constants_count;
    
    if (original->constants_count > 0 && original->constants) {
        copy->constants = malloc(original->constants_count * sizeof(Value));
        memcpy(copy->constants, original->constants, original->constants_count * sizeof(Value));
    } else { copy->constants = NULL; }
    
    copy->code.count = original->code.count;
    copy->code.capacity = original->code.count;
    copy->code.bytecodes = malloc(original->code.count * sizeof(bytecode));
    memcpy(copy->code.bytecodes, original->code.bytecodes, original->code.count * sizeof(bytecode));
    
    return copy;
}

// Удаляем инструкции
static void remove_instructions(bytecode_array* bc, size_t i, size_t count) {
    if (i >= bc->count || count == 0) return;
    size_t to_remove = (i + count > bc->count) ? (bc->count - i) : count;
    for (size_t j = i; j < bc->count - to_remove; j++) bc->bytecodes[j] = bc->bytecodes[j + to_remove];
    bc->count -= to_remove;
}

// Пеепхол оптимизации
static void apply_peephole_optimizations(CodeObj* code, bytecode_array* bc, FoldStats* stats) {
    size_t i = 0;
    while (i < bc->count) {
        if (i + 1 < bc->count &&
            bc->bytecodes[i].op_code == LOAD_CONST &&
            bc->bytecodes[i+1].op_code == POP_TOP) {
            
            remove_instructions(bc, i, 2);
            stats->peephole_optimizations++;
            stats->removed_instructions += 2;
            continue;
        }
        
        if (i + 1 < bc->count &&
            bc->bytecodes[i].op_code == LOAD_FAST &&
            bc->bytecodes[i+1].op_code == STORE_FAST &&
            bytecode_get_arg(bc->bytecodes[i]) == bytecode_get_arg(bc->bytecodes[i+1])) {
            
            remove_instructions(bc, i, 2);
            stats->peephole_optimizations++;
            stats->removed_instructions += 2;
            continue;
        }
        
        i++;
    }
}

// JIT constant folding
CodeObj* jit_optimize_constant_folding(CodeObj* original, FoldStats* stats) {
    if (!original || !original->code.bytecodes || !original->constants) return NULL;
    memset(stats, 0, sizeof(FoldStats));
    
    CodeObj* optimized = deep_copy_codeobj(original);
    bytecode_array* bc = &optimized->code;
    int changed;
    
    do {
        changed = 0;
        for (size_t i = 0; i < bc->count; i++) {
            if (i + 2 < bc->count &&
                bc->bytecodes[i].op_code == LOAD_CONST &&
                bc->bytecodes[i+1].op_code == LOAD_CONST &&
                bc->bytecodes[i+2].op_code == BINARY_OP) {
                
                uint32_t idx_a = bytecode_get_arg(bc->bytecodes[i]);
                uint32_t idx_b = bytecode_get_arg(bc->bytecodes[i+1]);
                uint8_t binop = bytecode_get_arg(bc->bytecodes[i+2]) & 0xFF;
                
                if (idx_a < optimized->constants_count &&
                    idx_b < optimized->constants_count) {
                    
                    Value val_a = optimized->constants[idx_a];
                    Value val_b = optimized->constants[idx_b];
                    
                    if (is_constant_foldable(val_a, val_b, binop)) {
                        Value result = fold_binary_constant(val_a, val_b, binop);
                        if (result.type != VAL_NONE) {
                            size_t new_idx = find_or_add_constant(optimized, result, stats);
                            if (new_idx != (size_t)-1) {
                                bc->bytecodes[i] = bytecode_create_with_number(LOAD_CONST, new_idx);
                                remove_instructions(bc, i + 1, 2);
                                stats->folded_constants++;
                                stats->removed_instructions += 2;
                                changed = 1;
                                break;
                            }
                        }
                    }
                }
            }
            
            if (i + 1 < bc->count &&
                bc->bytecodes[i].op_code == LOAD_CONST &&
                bc->bytecodes[i+1].op_code == UNARY_OP) {
                
                uint32_t idx_a = bytecode_get_arg(bc->bytecodes[i]);
                uint8_t unop = bytecode_get_arg(bc->bytecodes[i+1]) & 0xFF;
                
                if (idx_a < optimized->constants_count) {
                    Value val_a = optimized->constants[idx_a];
                    if (is_unary_foldable(val_a, unop)) {
                        Value result = fold_unary_constant(val_a, unop);
                        if (result.type != VAL_NONE) {
                            size_t new_idx = find_or_add_constant(optimized, result, stats);
                            if (new_idx != (size_t)-1) {
                                bc->bytecodes[i] = bytecode_create_with_number(LOAD_CONST, new_idx);
                                remove_instructions(bc, i + 1, 1);
                                stats->folded_constants++;
                                stats->removed_instructions += 1;
                                changed = 1;
                                break;
                            }
                        }
                    }
                }
            }
        }
    } while (changed && bc->count > 0);
    
    apply_peephole_optimizations(optimized, bc, stats);
    optimize_constant_conditions(optimized, bc, stats);
    
    size_t* map = malloc(sizeof(size_t) * bc->count);
    build_instruction_map(bc, map);
    rebuild_jumps(bc, map);
    compact_bytecode_array(bc);
    free(map);
    
    return optimized;
}

int jit_can_constant_fold(CodeObj* code) {
    if (!code || !code->code.bytecodes || !code->constants || code->code.count < 2) return 0;
    bytecode_array* bc = &code->code;
    int patterns = 0;
    
    for (size_t i = 0; i < bc->count - 1; i++) {
        if (bc->bytecodes[i].op_code == LOAD_CONST &&
            bc->bytecodes[i+1].op_code == UNARY_OP) patterns++;
        if (i + 2 < bc->count &&
            bc->bytecodes[i].op_code == LOAD_CONST &&
            bc->bytecodes[i+1].op_code == LOAD_CONST &&
            bc->bytecodes[i+2].op_code == BINARY_OP) patterns++;
    }
    
    return patterns > 0;
}

