#include "cmpswap.h"
#include "../../system.h"
#include <string.h>
#include <stdlib.h>

static int match_bubble_sort_pattern(bytecode_array* bc, size_t start, PatternMatch* match);
static int is_binary_add(bytecode bc);
static int is_binary_gt(bytecode bc);
static int is_load_local_with_idx(bytecode bc, size_t idx);
static int is_store_local_with_idx(bytecode bc, size_t idx);
static CodeObj* deep_copy_codeobj(CodeObj* original);
static void recalculate_jumps(bytecode_array* bc);

static int is_binary_add(bytecode bc) {
    if (bc.op_code != BINARY_OP) return 0;
    uint32_t arg = bytecode_get_arg(bc);
    return arg == 0x00;
}

static int is_binary_gt(bytecode bc) {
    if (bc.op_code != BINARY_OP) return 0;
    uint32_t arg = bytecode_get_arg(bc);
    return arg == 0x54;
}

static int is_load_local_with_idx(bytecode bc, size_t idx) {
    if (bc.op_code != LOAD_FAST) return 0;
    return bytecode_get_arg(bc) == idx;
}

static int is_store_local_with_idx(bytecode bc, size_t idx) {
    if (bc.op_code != STORE_FAST) return 0;
    return bytecode_get_arg(bc) == idx;
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

static void recalculate_jumps(bytecode_array* bc) {
    size_t* old_to_new = malloc(bc->count * sizeof(size_t));
    size_t new_count = 0;
    
    for (size_t i = 0; i < bc->count; i++) {
        if (bc->bytecodes[i].op_code != NOP) {
            old_to_new[i] = new_count++;
        } else {
            old_to_new[i] = (size_t)-1; // Помечаем как удаленную
        }
    }
    
    size_t new_idx = 0;
    for (size_t old_idx = 0; old_idx < bc->count; old_idx++) {
        if (bc->bytecodes[old_idx].op_code == NOP) {
            continue;
        }
        
        bytecode* ins = &bc->bytecodes[old_idx];
        uint8_t op = ins->op_code;
        
        int is_jump = (op == JUMP_FORWARD || op == JUMP_BACKWARD || 
                      op == POP_JUMP_IF_FALSE || op == POP_JUMP_IF_TRUE);
        
        if (is_jump) {
            uint32_t old_target;
            uint32_t old_arg = bytecode_get_arg(*ins);
            
            if (op == JUMP_BACKWARD) {
                old_target = old_idx - old_arg;
            } else {
                old_target = old_idx + old_arg + 1;
            }
            
            if (old_target < bc->count) {
                size_t new_target = old_to_new[old_target];
                
                if (new_target == (size_t)-1) {
                    if (op == JUMP_BACKWARD) {
                        for (size_t t = old_target; t > 0; t--) {
                            if (old_to_new[t] != (size_t)-1) {
                                new_target = old_to_new[t];
                                break;
                            }
                        }
                    } else {
                        for (size_t t = old_target; t < bc->count; t++) {
                            if (old_to_new[t] != (size_t)-1) {
                                new_target = old_to_new[t];
                                break;
                            }
                        }
                    }
                }
                
                uint32_t new_arg;
                if (op == JUMP_BACKWARD) {
                    if (new_target >= new_idx) {
                        ins->op_code = NOP;
                        continue;
                    }
                    new_arg = (uint32_t)(new_idx - new_target);
                } else {
                    if (new_target <= new_idx) {
                        ins->op_code = NOP;
                        continue;
                    }
                    new_arg = (uint32_t)(new_target - new_idx - 1);
                }
                
                *ins = bytecode_create_with_number(op, new_arg);
            } else {
                ins->op_code = NOP;
            }
        }
        
        new_idx++;
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
}

static int is_load_local(bytecode bc, size_t* idx) {
    if (bc.op_code != LOAD_FAST) return 0;
    *idx = bytecode_get_arg(bc);
    return 1;
}

static int is_store_local(bytecode bc, size_t* idx) {
    if (bc.op_code != STORE_FAST) return 0;
    *idx = bytecode_get_arg(bc);
    return 1;
}

static int match_bubble_sort_pattern(bytecode_array* bc, size_t start, PatternMatch* match) {
    if (start + 27 >= bc->count) return 0;
    
    bytecode* ins = &bc->bytecodes[start];
    
    // Проверяем общий паттерн сравнения и обмена
    // arr[j] > arr[j+1]
    size_t array_idx, idx_idx;
    if (!is_load_local(ins[0], &array_idx)) return 0;
    if (!is_load_local(ins[1], &idx_idx)) return 0;
    if (ins[2].op_code != LOAD_SUBSCR) return 0;
    
    if (!is_load_local(ins[3], &array_idx)) return 0;
    if (!is_load_local(ins[4], &idx_idx)) return 0;
    if (ins[5].op_code != LOAD_CONST) return 0;
    if (!is_binary_add(ins[6])) return 0;
    if (ins[7].op_code != LOAD_SUBSCR) return 0;
    
    if (!is_binary_gt(ins[8])) return 0;
    if (ins[9].op_code != POP_JUMP_IF_FALSE) return 0;
    
    match->array_local_idx = array_idx;
    match->index_local_idx = idx_idx;
    match->const_one_idx = bytecode_get_arg(ins[5]);
    
    // temp = arr[j]
    size_t temp_idx;
    if (!is_load_local(ins[10], &array_idx)) return 0;
    if (!is_load_local(ins[11], &idx_idx)) return 0;
    if (ins[12].op_code != LOAD_CONST) return 0;
    if (!is_binary_add(ins[13])) return 0;
    if (ins[14].op_code != LOAD_SUBSCR) return 0;
    if (!is_store_local(ins[15], &temp_idx)) return 0;
    
    match->temp_local_idx = temp_idx;
    
    // arr[j] = arr[j+1]
    if (!is_load_local(ins[16], &array_idx)) return 0;
    if (!is_load_local(ins[17], &idx_idx)) return 0;
    if (ins[18].op_code != LOAD_SUBSCR) return 0;
    if (!is_load_local(ins[19], &array_idx)) return 0;
    if (!is_load_local(ins[20], &idx_idx)) return 0;
    if (ins[21].op_code != LOAD_CONST) return 0;
    if (!is_binary_add(ins[22])) return 0;
    if (ins[23].op_code != STORE_SUBSCR) return 0;
    
    // arr[j+1] = temp
    if (!is_load_local(ins[24], &temp_idx)) return 0;
    if (!is_load_local(ins[25], &array_idx)) return 0;
    if (!is_load_local(ins[26], &idx_idx)) return 0;
    if (ins[27].op_code != LOAD_CONST) return 0;
    if (!is_binary_add(ins[28])) return 0;
    if (ins[29].op_code != STORE_SUBSCR) return 0;
    
    match->pattern_type = 0; // conditional swap
    return 1;
}

static int match_swap_pattern(bytecode_array* bc, size_t start, PatternMatch* match) {
    if (start + 13 >= bc->count) return 0;
    
    bytecode* ins = &bc->bytecodes[start];
    
    // Шаг 1: temp = numbers[i]
    if (ins[0].op_code != LOAD_FAST) return 0;
    size_t array_idx = bytecode_get_arg(ins[0]);
    
    if (ins[1].op_code != LOAD_FAST) return 0;
    size_t idx1 = bytecode_get_arg(ins[1]);
    
    if (ins[2].op_code != LOAD_SUBSCR) return 0;
    if (ins[3].op_code != STORE_FAST) return 0;
    size_t temp_idx = bytecode_get_arg(ins[3]);
    
    // Шаг 2: numbers[i] = numbers[j]
    if (!is_load_local_with_idx(ins[4], array_idx)) return 0;
    if (ins[5].op_code != LOAD_FAST) return 0;
    size_t idx2 = bytecode_get_arg(ins[5]);
    
    if (ins[6].op_code != LOAD_SUBSCR) return 0;
    if (!is_load_local_with_idx(ins[7], array_idx)) return 0;
    if (!is_load_local_with_idx(ins[8], idx1)) return 0;
    if (ins[9].op_code != STORE_SUBSCR) return 0;
    
    // Шаг 3: numbers[j] = temp
    if (!is_load_local_with_idx(ins[10], temp_idx)) return 0;
    if (!is_load_local_with_idx(ins[11], array_idx)) return 0;
    if (!is_load_local_with_idx(ins[12], idx2)) return 0;
    if (ins[13].op_code != STORE_SUBSCR) return 0;
    
    // Сохраняем найденные индексы
    match->array_local_idx = array_idx;
    match->index_local_idx = idx1;
    match->index2_local_idx = idx2;
    match->temp_local_idx = temp_idx;
    match->pattern_type = 1; // unconditional swap
    
    DPRINT("[JIT-CMPSWAP] Found swap pattern: array=%zu, i=%zu, j=%zu, temp=%zu\n",
           array_idx, idx1, idx2, temp_idx);
    
    return 1;
}

// Замена на COMPARE_AND_SWAP (условный обмен)
static void replace_with_cmpswap(bytecode_array* bc, size_t start, PatternMatch* match) {
    for (size_t i = start; i < start + 30 && i < bc->count; i++) {
        bc->bytecodes[i].op_code = NOP;
    }
    
    bc->bytecodes[start] = bytecode_create_with_number(LOAD_FAST, match->array_local_idx);
    bc->bytecodes[start + 1] = bytecode_create_with_number(LOAD_FAST, match->index_local_idx);
    bc->bytecodes[start + 2] = bytecode_create_with_number(LOAD_CONST, match->const_one_idx);
    bc->bytecodes[start + 3] = bytecode_create_with_number(COMPARE_AND_SWAP, 0x00);
}

// Замена на SWAP_ARRAY_ELEMENTS (безусловный обмен)
static void replace_with_swap(bytecode_array* bc, size_t start, PatternMatch* match) {
    for (size_t i = start; i < start + 14 && i < bc->count; i++) {
        bc->bytecodes[i].op_code = NOP;
    }
    
    bc->bytecodes[start] = bytecode_create_with_number(LOAD_FAST, match->array_local_idx);
    bc->bytecodes[start + 1] = bytecode_create_with_number(LOAD_FAST, match->index_local_idx);
    bc->bytecodes[start + 2] = bytecode_create_with_number(LOAD_FAST, match->index2_local_idx);
    bc->bytecodes[start + 3] = bytecode_create_with_number(SWAP_ARRAY_ELEMENTS, 0x00);
}

static int find_and_replace_patterns(CodeObj* code, CmpswapStats* stats) {
    bytecode_array* bc = &code->code;
    int changed = 0;
    
    for (size_t i = 0; i < bc->count; i++) {
        PatternMatch match;
        
        // Сначала проверяем безусловный обмен (он короче)
        if (match_swap_pattern(bc, i, &match)) {
            DPRINT("[JIT-CMPSWAP] Found unconditional swap pattern at position %zu\n", i);
            DPRINT("[JIT-CMPSWAP] array_idx=%zu, idx1=%zu, idx2=%zu, temp_idx=%zu\n",
                   match.array_local_idx, match.index_local_idx, 
                   match.index2_local_idx, match.temp_local_idx);
            
            replace_with_swap(bc, i, &match);
            stats->swap_patterns++;
            stats->optimized_patterns++;
            changed = 1;
            
            i += 13; // Пропускаем обработанные инструкции
            continue;
        }
        
        // Затем проверяем условный обмен (он длиннее)
        if (match_bubble_sort_pattern(bc, i, &match)) {
            DPRINT("[JIT-CMPSWAP] Found bubble sort pattern at position %zu\n", i);
            DPRINT("[JIT-CMPSWAP] array_idx=%zu, index_idx=%zu, const_idx=%zu, temp_idx=%zu\n",
                   match.array_local_idx, match.index_local_idx, 
                   match.const_one_idx, match.temp_local_idx);
            
            replace_with_cmpswap(bc, i, &match);
            stats->cmpswap_patterns++;
            stats->optimized_patterns++;
            changed = 1;
            
            i += 29; // Пропускаем обработанные инструкции
        }
    }
    
    if (changed) {
        DPRINT("[JIT-CMPSWAP] Recalculating jumps...\n");
        recalculate_jumps(bc);
    }
    
    return changed;
}

int has_sorting_pattern(CodeObj* code) {
    if (!code || !code->code.bytecodes) return 0;
    
    PatternMatch match;
    for (size_t i = 0; i < code->code.count; i++) {
        if (match_bubble_sort_pattern(&code->code, i, &match) ||
            match_swap_pattern(&code->code, i, &match)) {
            DPRINT("[JIT-CMPSWAP] Found pattern at position %zu\n", i);
            return 1;
        }
    }
    
    return 0;
}

CodeObj* jit_optimize_cmpswap(CodeObj* original, CmpswapStats* stats) {
    if (!original || !original->code.bytecodes || !original->constants) {
        DPRINT("[JIT-CMPSWAP] Invalid input\n");
        return NULL;
    }
    
    DPRINT("[JIT-CMPSWAP] Starting optimization for '%s'\n",
           original->name ? original->name : "anonymous");
    
    memset(stats, 0, sizeof(CmpswapStats));
    
    if (!has_sorting_pattern(original)) {
        DPRINT("[JIT-CMPSWAP] No sorting patterns found\n");
        return NULL;
    }
    
    CodeObj* optimized = deep_copy_codeobj(original);
    if (!optimized) {
        DPRINT("[JIT-CMPSWAP] Failed to copy CodeObj\n");
        return NULL;
    }
    
    DPRINT("[JIT-CMPSWAP] Original bytecode size: %zu\n", original->code.count);
    DPRINT("[JIT-CMPSWAP] Copied bytecode size: %zu\n", optimized->code.count);
    
    if (find_and_replace_patterns(optimized, stats)) {
        DPRINT("[JIT-CMPSWAP] Optimization successful. Patterns optimized: %zu\n",
               stats->optimized_patterns);
        DPRINT("[JIT-CMPSWAP] Optimized bytecode size: %zu\n", optimized->code.count);
        return optimized;
    }
    
    DPRINT("[JIT-CMPSWAP] Optimization did not apply\n");
    free(optimized->code.bytecodes);
    if (optimized->constants) free(optimized->constants);
    if (optimized->name) free(optimized->name);
    free(optimized);
    
    return NULL;
}
