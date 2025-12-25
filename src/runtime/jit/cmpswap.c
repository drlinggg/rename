#include "cmpswap.h"
#include "../../system.h"
#include <string.h>
#include <stdlib.h>

// Вспомогательные функции
static int match_bubble_sort_pattern(bytecode_array* bc, size_t start, PatternMatch* match);
static int is_binary_add(bytecode bc);
static int is_binary_gt(bytecode bc);
static int is_load_local_with_idx(bytecode bc, size_t idx);
static int is_store_local_with_idx(bytecode bc, size_t idx);
static CodeObj* deep_copy_codeobj(CodeObj* original);
static void recalculate_jumps(bytecode_array* bc);

// Проверка, является ли инструкция BINARY_OP ADD
static int is_binary_add(bytecode bc) {
    if (bc.op_code != BINARY_OP) return 0;
    uint32_t arg = bytecode_get_arg(bc);
    // ADD обычно имеет код 0x00
    return arg == 0x00;
}

// Проверка, является ли инструкция BINARY_OP GT
static int is_binary_gt(bytecode bc) {
    if (bc.op_code != BINARY_OP) return 0;
    uint32_t arg = bytecode_get_arg(bc);
    // GT (greater than) обычно имеет код 0x54
    return arg == 0x54;
}

// Проверка LOAD_FAST с определенным индексом
static int is_load_local_with_idx(bytecode bc, size_t idx) {
    if (bc.op_code != LOAD_FAST) return 0;
    return bytecode_get_arg(bc) == idx;
}

// Проверка STORE_FAST с определенным индексом
static int is_store_local_with_idx(bytecode bc, size_t idx) {
    if (bc.op_code != STORE_FAST) return 0;
    return bytecode_get_arg(bc) == idx;
}

// Поиск паттерна пузырьковой сортировки
static int match_bubble_sort_pattern(bytecode_array* bc, size_t start, PatternMatch* match) {
    if (start + 25 >= bc->count) return 0;
    
    bytecode* ins = &bc->bytecodes[start];
    
    // Шаг 1: if (arr[j] > arr[j+1])
    // arr[j]
    if (!is_load_local_with_idx(ins[0], 0)) return 0;  // array (локальная 0)
    if (!is_load_local_with_idx(ins[1], 4)) return 0;  // j (локальная 4, как видно из байткода)
    if (ins[2].op_code != LOAD_SUBSCR) return 0;
    
    // arr[j+1]
    if (!is_load_local_with_idx(ins[3], 0)) return 0;  // array
    if (!is_load_local_with_idx(ins[4], 4)) return 0;  // j
    if (ins[5].op_code != LOAD_CONST) return 0;
    if (!is_binary_add(ins[6])) return 0;
    if (ins[7].op_code != LOAD_SUBSCR) return 0;
    
    // Сравнение >
    if (!is_binary_gt(ins[8])) return 0;
    if (ins[9].op_code != POP_JUMP_IF_FALSE) return 0;
    
    // Запоминаем индексы
    match->array_local_idx = bytecode_get_arg(ins[0]);
    match->index_local_idx = bytecode_get_arg(ins[1]);
    match->const_one_idx = bytecode_get_arg(ins[5]);
    
    // Шаг 2: temp = arr[j+1] (в коде: int temp = array[j + 1])
    if (!is_load_local_with_idx(ins[10], 0)) return 0;  // array
    if (!is_load_local_with_idx(ins[11], 4)) return 0;  // j
    if (ins[12].op_code != LOAD_CONST) return 0;
    if (!is_binary_add(ins[13])) return 0;
    if (ins[14].op_code != LOAD_SUBSCR) return 0;
    if (ins[15].op_code != STORE_FAST) return 0;
    
    match->temp_local_idx = bytecode_get_arg(ins[15]);
    
    // Шаг 3: array[j + 1] = array[j]
    if (!is_load_local_with_idx(ins[16], 0)) return 0;  // array
    if (!is_load_local_with_idx(ins[17], 4)) return 0;  // j
    if (ins[18].op_code != LOAD_SUBSCR) return 0;
    if (!is_load_local_with_idx(ins[19], 0)) return 0;  // array
    if (!is_load_local_with_idx(ins[20], 4)) return 0;  // j
    if (ins[21].op_code != LOAD_CONST) return 0;
    if (!is_binary_add(ins[22])) return 0;
    if (ins[23].op_code != STORE_SUBSCR) return 0;
    
    // Шаг 4: array[j] = temp
    if (!is_load_local_with_idx(ins[24], match->temp_local_idx)) return 0;
    if (!is_load_local_with_idx(ins[25], 0)) return 0;  // array
    if (!is_load_local_with_idx(ins[26], 4)) return 0;  // j
    if (ins[27].op_code != STORE_SUBSCR) return 0;
    
    return 1;
}

// Глубокая копия CodeObj
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

// Пересчет прыжков (упрощенная версия)
// Улучшенный пересчет прыжков
static void recalculate_jumps(bytecode_array* bc) {
    // Шаг 1: Строим карту старых позиций -> новых позиций
    size_t* old_to_new = malloc(bc->count * sizeof(size_t));
    size_t new_count = 0;
    
    for (size_t i = 0; i < bc->count; i++) {
        if (bc->bytecodes[i].op_code != NOP) {
            old_to_new[i] = new_count++;
        } else {
            old_to_new[i] = (size_t)-1; // Помечаем как удаленную
        }
    }
    
    // Шаг 2: Пересчитываем аргументы инструкций прыжков
    size_t new_idx = 0;
    for (size_t old_idx = 0; old_idx < bc->count; old_idx++) {
        if (bc->bytecodes[old_idx].op_code == NOP) {
            continue;
        }
        
        bytecode* ins = &bc->bytecodes[old_idx];
        uint8_t op = ins->op_code;
        
        // Проверяем, является ли инструкция прыжком
        int is_jump = (op == JUMP_FORWARD || op == JUMP_BACKWARD || 
                      op == POP_JUMP_IF_FALSE || op == POP_JUMP_IF_TRUE);
        
        if (is_jump) {
            uint32_t old_target;
            uint32_t old_arg = bytecode_get_arg(*ins);
            
            // Вычисляем старую цель прыжка
            if (op == JUMP_BACKWARD) {
                old_target = old_idx - old_arg;
            } else {
                // JUMP_FORWARD, POP_JUMP_IF_FALSE, POP_JUMP_IF_TRUE
                old_target = old_idx + old_arg + 1; // +1 потому что прыжок вперед считается от следующей инструкции
            }
            
            // Проверяем, что цель в пределах массива
            if (old_target < bc->count) {
                // Находим новую цель
                size_t new_target = old_to_new[old_target];
                
                // Если цель была удалена, находим ближайшую живую инструкцию
                if (new_target == (size_t)-1) {
                    // Ищем вперед или назад в зависимости от типа прыжка
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
                
                // Вычисляем новый аргумент
                uint32_t new_arg;
                if (op == JUMP_BACKWARD) {
                    if (new_target >= new_idx) {
                        // Некорректный прыжок назад - делаем NOP
                        ins->op_code = NOP;
                        continue;
                    }
                    new_arg = (uint32_t)(new_idx - new_target);
                } else {
                    if (new_target <= new_idx) {
                        // Некорректный прыжок вперед - делаем NOP
                        ins->op_code = NOP;
                        continue;
                    }
                    new_arg = (uint32_t)(new_target - new_idx - 1);
                }
                
                // Обновляем инструкцию
                *ins = bytecode_create_with_number(op, new_arg);
            } else {
                // Цель вне границ - делаем NOP
                ins->op_code = NOP;
            }
        }
        
        new_idx++;
    }
    
    // Шаг 3: Удаляем NOP инструкции
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

// Замена паттерна на COMPARE_AND_SWAP
static void replace_with_cmpswap(bytecode_array* bc, size_t start, PatternMatch* match) {
    // Помечаем старые инструкции как NOP (28 инструкций в паттерне)
    for (size_t i = start; i < start + 28 && i < bc->count; i++) {
        bc->bytecodes[i].op_code = NOP;
    }
    
    // Вставляем новые инструкции на место первой инструкции паттерна
    // 1. LOAD_FAST array
    bc->bytecodes[start] = bytecode_create_with_number(LOAD_FAST, match->array_local_idx);
    
    // 2. LOAD_FAST j
    bc->bytecodes[start + 1] = bytecode_create_with_number(LOAD_FAST, match->index_local_idx);
    
    // 3. LOAD_FAST j (снова, для j+1)
    bc->bytecodes[start + 2] = bytecode_create_with_number(LOAD_FAST, match->index_local_idx);
    
    // 4. LOAD_CONST 1
    bc->bytecodes[start + 3] = bytecode_create_with_number(LOAD_CONST, match->const_one_idx);
    
    // 5. BINARY_OP ADD (вычисляем j+1)
    bc->bytecodes[start + 4] = bytecode_create_with_number(BINARY_OP, 0x00); // ADD
    
    // 6. COMPARE_AND_SWAP
    bc->bytecodes[start + 5] = bytecode_create_with_number(COMPARE_AND_SWAP, 0x00);
    
    // Оставшиеся позиции (start + 6 до start + 27) остаются как NOP
}

// Поиск всех паттернов в функции
static int find_and_replace_patterns(CodeObj* code, CmpswapStats* stats) {
    bytecode_array* bc = &code->code;
    int changed = 0;
    
    for (size_t i = 0; i < bc->count; i++) {
        PatternMatch match;
        if (match_bubble_sort_pattern(bc, i, &match)) {
            DPRINT("[JIT-CMPSWAP] Found pattern at position %zu\n", i);
            DPRINT("[JIT-CMPSWAP] array_idx=%zu, index_idx=%zu, const_idx=%zu, temp_idx=%zu\n",
                   match.array_local_idx, match.index_local_idx, 
                   match.const_one_idx, match.temp_local_idx);
            
            replace_with_cmpswap(bc, i, &match);
            stats->optimized_patterns++;
            changed = 1;
            
            // Пропускаем замененный паттерн
            i += 27; // 28 инструкций - 1 (уже инкрементируется в цикле)
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
        if (match_bubble_sort_pattern(&code->code, i, &match)) {
            DPRINT("[JIT-CMPSWAP] Found sorting pattern at position %zu\n", i);
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
    
    // Проверяем, есть ли паттерн
    if (!has_sorting_pattern(original)) {
        DPRINT("[JIT-CMPSWAP] No sorting patterns found\n");
        return NULL;
    }
    
    // Создаем копию для оптимизации
    CodeObj* optimized = deep_copy_codeobj(original);
    if (!optimized) {
        DPRINT("[JIT-CMPSWAP] Failed to copy CodeObj\n");
        return NULL;
    }
    
    DPRINT("[JIT-CMPSWAP] Original bytecode size: %zu\n", original->code.count);
    DPRINT("[JIT-CMPSWAP] Copied bytecode size: %zu\n", optimized->code.count);
    
    // Применяем оптимизацию
    if (find_and_replace_patterns(optimized, stats)) {
        DPRINT("[JIT-CMPSWAP] Optimization successful. Patterns optimized: %zu\n",
               stats->optimized_patterns);
        DPRINT("[JIT-CMPSWAP] Optimized bytecode size: %zu\n", optimized->code.count);
        return optimized;
    }
    
    // Если оптимизация не применилась, освобождаем память
    DPRINT("[JIT-CMPSWAP] Optimization did not apply\n");
    free(optimized->code.bytecodes);
    if (optimized->constants) free(optimized->constants);
    if (optimized->name) free(optimized->name);
    free(optimized);
    
    return NULL;
}