#include "const_folding.h"
#include "../../system.h"
#include <string.h>
#include <stdlib.h>

static int is_truthy(Value v) {
    if (v.type == VAL_BOOL) return v.bool_val;
    if (v.type == VAL_INT) return v.int_val != 0;
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

// Упрощенная оптимизация условий
static void optimize_conditions_simple(CodeObj* code, bytecode_array* bc, FoldStats* stats) {
    // Проходим по байткоду и оптимизируем условия
    for (size_t i = 0; i + 1 < bc->count; i++) {
        // Паттерн: LOAD_CONST -> POP_JUMP_IF_FALSE
        if (bc->bytecodes[i].op_code == LOAD_CONST &&
            bc->bytecodes[i + 1].op_code == POP_JUMP_IF_FALSE) {
            
            uint32_t const_idx = bytecode_get_arg(bc->bytecodes[i]);
            if (const_idx >= code->constants_count) continue;
            
            Value v = code->constants[const_idx];
            
            if (is_falsy(v)) {
                // if (false) - заменяем на безусловный прыжок
                uint32_t jump_offset = bytecode_get_arg(bc->bytecodes[i + 1]);
                bc->bytecodes[i].op_code = NOP;
                bc->bytecodes[i + 1] = bytecode_create_with_number(JUMP_FORWARD, jump_offset);
                stats->peephole_optimizations++;
            } else if (is_truthy(v)) {
                // if (true) - удаляем обе инструкции
                bc->bytecodes[i].op_code = NOP;
                bc->bytecodes[i + 1].op_code = NOP;
                stats->peephole_optimizations++;
            }
        }
    }
}

// Простая функция для удаления NOP и фиксации прыжков
static void remove_nops_and_fix_jumps(bytecode_array* bc) {
    // Первый проход: построение карты перехода
    size_t* new_indices = malloc(bc->count * sizeof(size_t));
    size_t new_count = 0;
    
    for (size_t i = 0; i < bc->count; i++) {
        if (bc->bytecodes[i].op_code == NOP) {
            new_indices[i] = (size_t)-1;
        } else {
            new_indices[i] = new_count++;
        }
    }
    
    // Второй проход: пересчет смещений для прыжков
    for (size_t i = 0; i < bc->count; i++) {
        bytecode* ins = &bc->bytecodes[i];
        uint8_t op = ins->op_code;
        
        if (new_indices[i] == (size_t)-1) continue; // Пропускаем NOP
        
        // Обрабатываем только инструкции с прыжками
        if (op != JUMP_FORWARD && op != JUMP_BACKWARD && 
            op != POP_JUMP_IF_FALSE && op != POP_JUMP_IF_TRUE) {
            continue;
        }
        
        uint32_t old_offset = bytecode_get_arg(*ins);
        size_t old_target;
        
        // Вычисляем старую цель
        if (op == JUMP_BACKWARD) {
            old_target = i - old_offset;
        } else {
            old_target = i + old_offset;
        }
        
        // Проверяем, что цель в пределах
        if (old_target >= bc->count) {
            continue;
        }
        
        // Находим новый индекс цели
        size_t new_target = new_indices[old_target];
        
        // Если цель была удалена, пропускаем
        if (new_target == (size_t)-1) {
            // Ищем ближайшую живую инструкцию
            if (op == JUMP_BACKWARD) {
                for (size_t j = old_target; j > 0; j--) {
                    if (new_indices[j] != (size_t)-1) {
                        new_target = new_indices[j];
                        break;
                    }
                }
            } else {
                for (size_t j = old_target; j < bc->count; j++) {
                    if (new_indices[j] != (size_t)-1) {
                        new_target = new_indices[j];
                        break;
                    }
                }
            }
            
            if (new_target == (size_t)-1) {
                continue;
            }
        }
        
        // Вычисляем новое смещение
        size_t new_i = new_indices[i];
        uint32_t new_offset;
        
        if (op == JUMP_BACKWARD) {
            if (new_target > new_i) {
                // Это не должно происходить, но на всякий случай
                continue;
            }
            new_offset = (uint32_t)(new_i - new_target);
        } else {
            if (new_target < new_i) {
                // Это не должно происходить, но на всякий случай
                continue;
            }
            new_offset = (uint32_t)(new_target - new_i);
        }
        
        // Обновляем инструкцию
        *ins = bytecode_create_with_number(op, new_offset);
    }
    
    // Третий проход: создаем новый массив без NOP
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
    
    free(new_indices);
}

static size_t find_or_add_constant(CodeObj* code, Value v, FoldStats* stats) {
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

static void nop_instructions(bytecode_array* bc, size_t start, size_t count) {
    if (start >= bc->count || count == 0) return;
    
    size_t to_nop = count;
    if (start + to_nop > bc->count) {
        to_nop = bc->count - start;
    }
    
    for (size_t i = start; i < start + to_nop; i++) {
        bc->bytecodes[i].op_code = NOP;
    }
}

static void apply_simple_peephole(CodeObj* code, bytecode_array* bc, FoldStats* stats) {
    size_t i = 0;
    
    while (i < bc->count) {
        // Упрощенный паттерн: LOAD_CONST -> LOAD_CONST -> BINARY_OP
        if (i + 2 < bc->count &&
            bc->bytecodes[i].op_code == LOAD_CONST &&
            bc->bytecodes[i+1].op_code == LOAD_CONST &&
            bc->bytecodes[i+2].op_code == BINARY_OP) {
            
            uint32_t idx_a = bytecode_get_arg(bc->bytecodes[i]);
            uint32_t idx_b = bytecode_get_arg(bc->bytecodes[i+1]);
            uint8_t binop = bytecode_get_arg(bc->bytecodes[i+2]) & 0xFF;
            
            if (idx_a < code->constants_count && idx_b < code->constants_count) {
                Value val_a = code->constants[idx_a];
                Value val_b = code->constants[idx_b];
                
                // Только для целочисленных операций
                if (val_a.type == VAL_INT && val_b.type == VAL_INT) {
                    int64_t x = val_a.int_val;
                    int64_t y = val_b.int_val;
                    
                    // Простые оптимизации
                    if (binop == 0x00) { // ADD
                        if (x == 0) {
                            // 0 + y -> y
                            bc->bytecodes[i] = bc->bytecodes[i+1];
                            nop_instructions(bc, i+1, 2);
                            stats->peephole_optimizations++;
                            continue;
                        } else if (y == 0) {
                            // x + 0 -> x
                            nop_instructions(bc, i+1, 2);
                            stats->peephole_optimizations++;
                            continue;
                        }
                    } else if (binop == 0x05) { // MUL
                        if (x == 1) {
                            // 1 * y -> y
                            bc->bytecodes[i] = bc->bytecodes[i+1];
                            nop_instructions(bc, i+1, 2);
                            stats->peephole_optimizations++;
                            continue;
                        } else if (y == 1) {
                            // x * 1 -> x
                            nop_instructions(bc, i+1, 2);
                            stats->peephole_optimizations++;
                            continue;
                        } else if (x == 0 || y == 0) {
                            // x * 0 -> 0
                            bc->bytecodes[i] = bytecode_create_with_number(LOAD_CONST, 0);
                            nop_instructions(bc, i+1, 2);
                            stats->peephole_optimizations++;
                            continue;
                        }
                    }
                }
            }
        }
        
        i++;
    }
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
    
    // Простая константная свертка
    do {
        changed = 0;
        
        for (size_t i = 0; i < bc->count; i++) {
            // Паттерн: LOAD_CONST a, LOAD_CONST b, BINARY_OP op
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
                                nop_instructions(bc, i + 1, 2);
                                
                                stats->folded_constants++;
                                stats->removed_instructions += 2;
                                changed = 1;
                                break;
                            }
                        }
                    }
                }
            }
            
            // Паттерн: LOAD_CONST a, UNARY_OP op
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
                                nop_instructions(bc, i + 1, 1);
                                
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
    
    // Применяем простые peephole оптимизации
    apply_simple_peephole(optimized, bc, stats);
    
    // Оптимизируем условия
    optimize_conditions_simple(optimized, bc, stats);
    
    // Удаляем NOP и фиксируем прыжки
    remove_nops_and_fix_jumps(bc);
    
    DPRINT("[JIT-CF] Optimization complete:\n");
    DPRINT("  Folded constants: %zu\n", stats->folded_constants);
    DPRINT("  Removed instructions: %zu\n", stats->removed_instructions);
    DPRINT("  Peephole optimizations: %zu\n", stats->peephole_optimizations);
    DPRINT("  Original size: %zu, Optimized size: %zu\n", 
           original->code.count, bc->count);
    
    return optimized;
}

int jit_can_constant_fold(CodeObj* code) {
    if (!code || !code->code.bytecodes || !code->constants || code->code.count < 2) {
        return 0;
    }
    
    bytecode_array* bc = &code->code;
    int patterns = 0;
    
    for (size_t i = 0; i < bc->count - 1; i++) {
        // LOAD_CONST -> UNARY_OP
        if (bc->bytecodes[i].op_code == LOAD_CONST &&
            bc->bytecodes[i+1].op_code == UNARY_OP) {
            patterns++;
        }
        
        // LOAD_CONST -> LOAD_CONST -> BINARY_OP
        if (i + 2 < bc->count &&
            bc->bytecodes[i].op_code == LOAD_CONST &&
            bc->bytecodes[i+1].op_code == LOAD_CONST &&
            bc->bytecodes[i+2].op_code == BINARY_OP) {
            patterns++;
        }
    }
    
    return patterns > 0;
}
