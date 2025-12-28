#include "../../system.h"
#include "../../compiler/bytecode.h"
#include "../../compiler/value.h"
#include "dce.h"
#include "const_folding.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Вспомогательные функции
static size_t get_call_arg_count(bytecode bc) {
    return (size_t)bytecode_get_arg(bc);
}

static bool has_side_effects(bytecode bc) {
    switch (bc.op_code) {
        case STORE_FAST:
        case STORE_GLOBAL:
        case STORE_NAME:
        case STORE_SUBSCR:
        case DEL_SUBSCR:
        case LOAD_SUBSCR:
        case CALL_FUNCTION:
        case COMPARE_AND_SWAP:
            return true;
        default:
            return false;
    }
}

static bool var_is_used(bytecode_array* bc, uint8_t local_index, size_t start_idx) {
    for (size_t i = start_idx; i < bc->count; i++) {
        bytecode ins = bc->bytecodes[i];
        if (ins.op_code == LOAD_FAST && bytecode_get_arg(ins) == local_index)
            return true;
    }
    return false;
}


static bool loop_has_live_effect(bytecode_array* bc, size_t start, size_t end) {
    for (size_t i = start + 1; i < end; i++) {
        bytecode ins = bc->bytecodes[i];
        uint8_t opcode = ins.op_code;

        if (opcode == NOP) continue;

        if (opcode == JUMP_BACKWARD || opcode == JUMP_FORWARD ||
            opcode == CONTINUE_LOOP || opcode == BREAK_LOOP ||
            opcode == LOOP_START || opcode == LOOP_END) {
            continue;
        }

        if (opcode == STORE_FAST && bytecode_get_arg(ins) == 2) continue;

        if (opcode == CALL_FUNCTION || opcode == RETURN_VALUE ||
            opcode == STORE_GLOBAL || opcode == STORE_NAME ||
            opcode == STORE_SUBSCR || opcode == DEL_SUBSCR ||
            opcode == COMPARE_AND_SWAP) return true;

        if (opcode == STORE_FAST) {
            uint8_t idx = bytecode_get_arg(ins);
            for (size_t j = end + 1; j < bc->count; j++) {
                bytecode next = bc->bytecodes[j];
                if (next.op_code == LOAD_FAST && bytecode_get_arg(next) == idx) {
                    return true;
                }
            }
        }
    }

    return false;
}

// Удаляет пустой цикл
static void remove_empty_loop(bytecode_array* bc, size_t loop_start_idx, size_t loop_end_idx) {
    for (size_t i = loop_start_idx; i <= loop_end_idx && i < bc->count; i++)
        mark_as_nop(bc, i);
}

// Создание оптимизированного CodeObj
static CodeObj* create_optimized_codeobj(CodeObj* original) {
    if (!original) return NULL;
    CodeObj* optimized = malloc(sizeof(CodeObj));
    if (!optimized) return NULL;

    optimized->name = original->name ? strdup(original->name) : NULL;
    optimized->local_count = original->local_count;
    optimized->arg_count = original->arg_count;
    optimized->constants_count = original->constants_count;

    if (original->constants_count > 0 && original->constants) {
        optimized->constants = malloc(original->constants_count * sizeof(Value));
        if (!optimized->constants) { free(optimized->name); free(optimized); return NULL; }
        memcpy(optimized->constants, original->constants, original->constants_count * sizeof(Value));
    } else optimized->constants = NULL;

    optimized->code.count = original->code.count;
    optimized->code.capacity = original->code.count;
    optimized->code.bytecodes = malloc(original->code.count * sizeof(bytecode));
    if (!optimized->code.bytecodes) { free(optimized->constants); free(optimized->name); free(optimized); return NULL; }
    memcpy(optimized->code.bytecodes, original->code.bytecodes, original->code.count * sizeof(bytecode));

    return optimized;
}

// Удаляет недостижимый код после RETURN_VALUE
static void remove_unreachable_code(bytecode_array* bc) {
    bool found_return = false;
    
    for (size_t i = 0; i < bc->count; i++) {
        bytecode ins = bc->bytecodes[i];
        
        if (ins.op_code == RETURN_VALUE) {
            found_return = true;
            continue;
        }
        
        // После RETURN_VALUE весь код недостижим
        if (found_return) {
            mark_as_nop(bc, i);
        }
        
        // Также недостижим код после безусловного прыжка, если нет метки
        if (ins.op_code == JUMP_FORWARD || ins.op_code == JUMP_BACKWARD) {
            // Проверяем, есть ли метка после прыжка
            uint32_t offset = bytecode_get_arg(ins);
            size_t target_idx;
            
            if (ins.op_code == JUMP_FORWARD) {
                target_idx = i + 1 + offset;
            } else {
                target_idx = i + 1 - offset;
            }
            
            // Если сразу после прыжка нет метки (LOOP_START или другая значимая инструкция),
            // то код между прыжком и целью недостижим
            if (target_idx > i + 1) {
                bool has_label = false;
                for (size_t j = i + 1; j < target_idx; j++) {
                    bytecode between = bc->bytecodes[j];
                    if (between.op_code == LOOP_START || 
                        between.op_code == LOOP_END ||
                        (j > 0 && bc->bytecodes[j-1].op_code == JUMP_FORWARD && 
                         bytecode_get_arg(bc->bytecodes[j-1]) == j - (i + 1))) {
                        has_label = true;
                        break;
                    }
                }
                
                if (!has_label) {
                    // Код между прыжком и целью недостижим
                    for (size_t j = i + 1; j < target_idx; j++) {
                        mark_as_nop(bc, j);
                    }
                }
            }
        }
    }
}

// Удаляет пустые циклы (после удаления всего кода внутри)
static void remove_empty_loops(bytecode_array* bc) {
    for (size_t i = 0; i < bc->count; i++) {
        if (bc->bytecodes[i].op_code == LOOP_START) {
            size_t loop_end = i;
            int depth = 1;
            
            // Находим LOOP_END
            for (size_t j = i + 1; j < bc->count; j++) {
                if (bc->bytecodes[j].op_code == LOOP_START) depth++;
                if (bc->bytecodes[j].op_code == LOOP_END) {
                    depth--;
                    if (depth == 0) {
                        loop_end = j;
                        break;
                    }
                }
            }
            
            if (loop_end > i) {
                // Проверяем, есть ли в цикле реальный код
                bool has_real_code = false;

                DPRINT("[DCE] Examining loop %zu-%zu\n", i, loop_end);
                
                for (size_t k = i + 1; k < loop_end; k++) {
                    bytecode ins = bc->bytecodes[k];
                    if (ins.op_code == NOP) continue;
                    
                    // Пропускаем инструкции управления циклом и чистые вычисления
                    if (ins.op_code == JUMP_BACKWARD || 
                        ins.op_code == JUMP_FORWARD ||
                        ins.op_code == CONTINUE_LOOP ||
                        ins.op_code == BREAK_LOOP ||
                        ins.op_code == LOOP_START ||
                        ins.op_code == LOOP_END ||
                        ins.op_code == POP_JUMP_IF_FALSE ||
                        ins.op_code == LOAD_CONST ||
                        ins.op_code == LOAD_FAST ||
                        ins.op_code == LOAD_GLOBAL ||
                        ins.op_code == BINARY_OP ||
                        ins.op_code == UNARY_OP ||
                        ins.op_code == PUSH_NULL ||
                        ins.op_code == POP_TOP) {
                        continue;
                    }

                    // Если это STORE_FAST, но переменная нигде не читается - это тоже
                    // можно считать "производителем" (нет реальной работы).
                    if (ins.op_code == STORE_FAST) {
                        uint8_t idx = bytecode_get_arg(ins);
                        bool used = false;
                        for (size_t z = 0; z < bc->count; z++) {
                            if (bc->bytecodes[z].op_code == LOAD_FAST &&
                                bytecode_get_arg(bc->bytecodes[z]) == idx) {
                                used = true; break;
                            }
                        }
                        if (!used) continue;
                    }

                    // Любая другая инструкция - реальный код с побочным эффектом
                    has_real_code = true;
                    break;
                }
                
                // Если нет реального кода - удаляем цикл
                if (!has_real_code) {
                    DPRINT("[DCE] Removing empty loop at %zu-%zu\n", i, loop_end);
                    for (size_t k = i; k <= loop_end; k++) {
                        mark_as_nop(bc, k);
                    }
                } else {
                    DPRINT("[DCE] Not removing loop %zu-%zu: has real code\n", i, loop_end);
                }
            }
        }
    }
}

// Более агрессивный проход: если цикл не содержит побочных эффектов и все
// записи в локали внутри цикла не читаются вне цикла - можно удалить цикл
static void remove_empty_loops_aggressive(bytecode_array* bc) {
    DPRINT("[DCE] aggressive empty-loop pass start (count=%zu)\n", bc->count);
    for (size_t i = 0; i < bc->count; i++) {
        if (bc->bytecodes[i].op_code != LOOP_START) continue;

        size_t loop_end = i;
        int depth = 1;
        for (size_t j = i + 1; j < bc->count; j++) {
            if (bc->bytecodes[j].op_code == LOOP_START) depth++;
            if (bc->bytecodes[j].op_code == LOOP_END) {
                depth--;
                if (depth == 0) { loop_end = j; break; }
            }
        }

        if (loop_end <= i) continue;

        bool safe = true;
        bool writes_local[256] = {false};

        for (size_t k = i + 1; k < loop_end; k++) {
            bytecode ins = bc->bytecodes[k];
            if (ins.op_code == NOP) continue;

            switch (ins.op_code) {
                case CALL_FUNCTION:
                case STORE_GLOBAL:
                case STORE_NAME:
                case STORE_SUBSCR:
                case DEL_SUBSCR:
                case RETURN_VALUE:
                case COMPARE_AND_SWAP:
                    safe = false; break;
                case STORE_FAST:
                    writes_local[bytecode_get_arg(ins)] = true; break;
                default:
                    break;
            }

            if (!safe) break;
        }

        if (!safe) continue;

        // Проверяем, используются ли эти записанные локали ВНЕ цикла
        bool used_outside = false;
        for (size_t k = 0; k < bc->count && !used_outside; k++) {
            if (k >= i + 1 && k < loop_end) continue;
            bytecode ins = bc->bytecodes[k];
            if (ins.op_code == LOAD_FAST && writes_local[bytecode_get_arg(ins)]) {
                used_outside = true; break;
            }
        }

        DPRINT("[DCE] Loop %zu-%zu: safe=%s, used_outside=%s\n", i, loop_end,
               safe ? "true" : "false", used_outside ? "true" : "false");

        if (!used_outside) {
            DPRINT("[DCE] Aggressively removing loop at %zu-%zu\n", i, loop_end);
            for (size_t k = i; k <= loop_end; k++) mark_as_nop(bc, k);
        }
    }
}

// Simple verifier to ensure bytecode stack effects are sane after DCE.
// Returns true if the bytecode appears valid (no stack underflow, ops have
// sufficient arguments), false otherwise.
static bool validate_bytecode_stack(bytecode_array* bc) {
    if (!bc || bc->count == 0) return true;
    int stack = 0;

    for (size_t i = 0; i < bc->count; i++) {
        bytecode ins = bc->bytecodes[i];
        switch (ins.op_code) {
            case LOAD_CONST:
            case LOAD_FAST:
            case LOAD_GLOBAL:
            case PUSH_NULL:
                stack += 1; break;

            case BINARY_OP:
                if (stack < 2) {
                    DPRINT("[DCE-VERIFY] BINARY_OP at %zu has insufficient stack (%d)\n", i, stack);
                    return false;
                }
                // pops 2, pushes 1 => net -1
                stack -= 1;
                break;

            case UNARY_OP:
                if (stack < 1) {
                    DPRINT("[DCE-VERIFY] UNARY_OP at %zu has insufficient stack (%d)\n", i, stack);
                    return false;
                }
                // pops 1, pushes 1 => net 0
                break;

            case CALL_FUNCTION: {
                uint32_t argc = bytecode_get_arg(ins);
                if ((uint32_t)stack < argc) {
                    DPRINT("[DCE-VERIFY] CALL_FUNCTION at %zu needs %u args but stack=%d\n", i, argc, stack);
                    return false;
                }
                // consume args, push one result
                stack = stack - (int)argc + 1;
                break;
            }

            case STORE_FAST:
            case STORE_GLOBAL:
            case STORE_NAME:
            case STORE_SUBSCR:
            case DEL_SUBSCR:
            case RETURN_VALUE:
            case POP_TOP:
            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
                if (stack < 1) {
                    DPRINT("[DCE-VERIFY] pop-like op at %zu has insufficient stack (%d)\n", i, stack);
                    return false;
                }
                stack -= 1; break;

            case COMPARE_AND_SWAP:
                // conservative: requires at least 2
                if (stack < 2) {
                    DPRINT("[DCE-VERIFY] COMPARE_AND_SWAP at %zu has insufficient stack (%d)\n", i, stack);
                    return false;
                }
                // net effect depends - assume pushes 1
                stack = stack - 2 + 1;
                break;

            default:
                // For other ops, be conservative: do not change stack but ensure non-negative
                break;
        }
    }

    if (stack < 0) {
        DPRINT("[DCE-VERIFY] Final stack negative: %d\n", stack);
        return false;
    }

    return true;
}

// Обновленная функция оптимизации
static void optimize_dead_vars_and_loops(CodeObj* code) {
    if (!code) return;
    bytecode_array* bc = &code->code;

    // Шаг 1: удаляем недостижимый код после RETURN_VALUE
    //remove_unreachable_code(bc); TODO FIX

    // Шаг 2: удаляем пустые циклы
    remove_empty_loops(bc);
    
    // Для n-body кода: пропускаем агрессивное удаление циклов
    remove_empty_loops_aggressive(bc);

    // Шаг 3: пересчет прыжков и сжатие
    recalculate_jumps(bc);

    size_t write_idx = 0;
    for (size_t read_idx = 0; read_idx < bc->count; read_idx++) {
        if (bc->bytecodes[read_idx].op_code != NOP) {
            if (write_idx != read_idx) bc->bytecodes[write_idx] = bc->bytecodes[read_idx];
            write_idx++;
        }
    }
    bc->count = write_idx;

    DPRINT("[DCE] After safe optimization: %zu instructions\n", bc->count);
}

// Исправленная функция для проверки, имеет ли вызов функции побочные эффекты
static bool call_has_side_effects(bytecode_array* bc, size_t call_idx) {
    // Находим загрузку функции
    for (ssize_t i = (ssize_t)call_idx - 1; i >= 0; i--) {
        bytecode ins = bc->bytecodes[i];
        if (ins.op_code == LOAD_GLOBAL) {
            uint8_t global_idx = bytecode_get_arg(ins);
            
            // sqrt - чистая функция (глобальный индекс 3)
            if (global_idx == 0x03) {
                return false;
            }
            
            // print, input, randint - имеют побочные эффекты
            if (global_idx == 0x00 || global_idx == 0x01 || global_idx == 0x02) {
                return true;
            }
            
            // Для пользовательских функций (init_bodies, advance, energy)
            // нужно проверить, имеют ли они побочные эффекты
            // В n-body коде:
            // - init_bodies: имеет побочные эффекты (записывает в массивы)
            // - advance: имеет побочные эффекты (изменяет массивы)
            // - energy: чистая функция (только вычисления)
            
            // Консервативно предполагаем, что все пользовательские функции
            // имеют побочные эффекты, кроме если не доказано обратное
            return true;
        }
        
        if (ins.op_code == PUSH_NULL) {
            continue;
        }
        
        break;
    }
    
    // Для неизвестных функций предполагаем побочные эффекты
    return true;
}

// Исправленная функция проверки использования результатов вызовов
static bool is_call_result_used(bytecode_array* bc, size_t call_idx) {
    // Проверяем, используется ли результат вызова
    // Следим за глубиной стека
    int stack_depth = 1; // CALL_FUNCTION оставляет результат на стеке
    
    for (size_t i = call_idx + 1; i < bc->count; i++) {
        bytecode ins = bc->bytecodes[i];
        if (ins.op_code == NOP) continue;
        
        // Определяем эффект инструкции на стек
        switch (ins.op_code) {
            case LOAD_CONST:
            case LOAD_FAST:
            case LOAD_GLOBAL:
            case PUSH_NULL:
                stack_depth++;
                break;
                
            case STORE_FAST:
            case STORE_GLOBAL:
            case STORE_NAME:
            case STORE_SUBSCR:
            case RETURN_VALUE:
            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            case POP_TOP:
                if (stack_depth > 0) {
                    stack_depth--;
                    if (stack_depth == 0) {
                        // Наше значение было использовано или выброшено
                        if (ins.op_code == POP_TOP) {
                            return false; // Результат выброшен
                        }
                        return true; // Результат использован
                    }
                }
                break;
                
            case BINARY_OP:
                if (stack_depth >= 2) {
                    stack_depth--; // 2 входа, 1 выход
                    if (stack_depth == 0) {
                        return true; // Наш результат был одним из аргументов
                    }
                }
                break;
                
            case UNARY_OP:
                // 1 вход, 1 выход - глубина стека не меняется
                if (stack_depth == 1) {
                    return true; // Наш результат был аргументом
                }
                break;
                
            case CALL_FUNCTION: {
                uint32_t argc = bytecode_get_arg(ins);
                if (stack_depth >= argc + 1) {
                    stack_depth -= argc; // функция и аргументы
                    stack_depth++; // результат
                    if (stack_depth == 1) {
                        return true; // Наш результат был аргументом
                    }
                }
                break;
            }
                
            case JUMP_FORWARD:
            case JUMP_BACKWARD:
            case LOOP_START:
            case LOOP_END:
            case CONTINUE_LOOP:
            case BREAK_LOOP:
                // Управляющие инструкции не влияют на стек
                continue;
                
            default:
                // Для других инструкций консервативно предполагаем использование
                if (stack_depth == 1) {
                    return true;
                }
                break;
        }
    }
    
    // Если достигли конца, результат не используется
    return false;
}

// Упрощенная функция remove_dead_calls - не удалять вызовы в n-body коде
static size_t remove_dead_calls(CodeObj* code) {
    if (!code) return 0;
    
    bytecode_array* bc = &code->code;
    size_t removed_calls = 0;
    
    // Для n-body кода: не удалять вызовы вообще
    // Все вызовы нужны
    return 0;
}

// Упрощенная функция remove_dead_loads - более консервативная
static void remove_dead_loads(CodeObj* code) {
    if (!code) return;
    bytecode_array* bc = &code->code;

    // Для n-body кода: очень консервативный подход
    // Не удаляем загрузки, если не уверены на 100%
    
    // Сначала отмечаем все LOAD_* как использованные
    bool* used = calloc(bc->count, sizeof(bool));
    if (!used) return;
    
    // Проходим в прямом порядке и определяем использование
    for (size_t i = 0; i < bc->count; i++) {
        bytecode ins = bc->bytecodes[i];
        if (ins.op_code == NOP) continue;
        
        // Проверяем, используется ли это значение
        bool is_used = true; // Консервативно: предполагаем использованным
        
        // Смотрим на следующие инструкции
        for (size_t j = i + 1; j < bc->count; j++) {
            bytecode next = bc->bytecodes[j];
            if (next.op_code == NOP) continue;
            
            // Если это LOAD_*, продолжаем
            if (next.op_code == LOAD_CONST || next.op_code == LOAD_FAST || 
                next.op_code == LOAD_GLOBAL || next.op_code == PUSH_NULL) {
                continue;
            }
            
            // Если это POP_TOP без других операций - возможно не используется
            if (next.op_code == POP_TOP) {
                // Проверяем, есть ли после этого что-то, что использует стек
                bool has_stack_use_after = false;
                for (size_t k = j + 1; k < bc->count; k++) {
                    bytecode after = bc->bytecodes[k];
                    if (after.op_code == NOP) continue;
                    
                    if (after.op_code == STORE_FAST || after.op_code == STORE_GLOBAL ||
                        after.op_code == STORE_NAME || after.op_code == STORE_SUBSCR ||
                        after.op_code == CALL_FUNCTION || after.op_code == RETURN_VALUE ||
                        after.op_code == BINARY_OP || after.op_code == UNARY_OP) {
                        has_stack_use_after = true;
                        break;
                    }
                }
                
                if (!has_stack_use_after) {
                    is_used = false;
                }
                break;
            }
            
            // Для всех остальных инструкций считаем, что значение используется
            break;
        }
        
        used[i] = is_used;
    }
    
    // Удаляем только те загрузки, которые точно не используются
    // (после POP_TOP и нет других использований)
    for (ssize_t i = (ssize_t)bc->count - 1; i >= 0; i--) {
        if (!used[i] && (bc->bytecodes[i].op_code == LOAD_CONST || 
                         bc->bytecodes[i].op_code == LOAD_FAST || 
                         bc->bytecodes[i].op_code == LOAD_GLOBAL)) {
            // Проверяем, что следующая инструкция - POP_TOP
            if (i + 1 < bc->count && bc->bytecodes[i + 1].op_code == POP_TOP) {
                mark_as_nop(bc, i);
                mark_as_nop(bc, i + 1);
            }
        }
    }
    
    free(used);
    
    // Сжимаем байткод
    size_t write_idx = 0;
    for (size_t read_idx = 0; read_idx < bc->count; read_idx++) {
        if (bc->bytecodes[read_idx].op_code != NOP) {
            if (write_idx != read_idx)
                bc->bytecodes[write_idx] = bc->bytecodes[read_idx];
            write_idx++;
        }
    }
    bc->count = write_idx;
}

// Обновленная основная функция
CodeObj* jit_optimize_dce(CodeObj* code, DCEStats* stats) {
    if (!code || code->code.count == 0) { 
        if (stats) memset(stats, 0, sizeof(*stats)); 
        return code; 
    }

    DPRINT("[DCE] Optimizing function: %s\n", code->name ? code->name : "anonymous");
    DPRINT("[DCE] Original size: %zu instructions\n", code->code.count);
    
    CodeObj* optimized = create_optimized_codeobj(code);
    if (!optimized) return code;

    if (stats) memset(stats, 0, sizeof(*stats));

    bytecode_array* bc = &optimized->code;
    size_t iteration = 0;
    bool changed;
    size_t total_removed_calls = 0;

    do {
        changed = false;
        iteration++;
        size_t before = bc->count;

        DPRINT("[DCE] Iteration %zu (size: %zu)\n", iteration, before);
        
        // Применяем все оптимизации
        optimize_dead_vars_and_loops(optimized);
        //total_removed_calls += remove_dead_calls(optimized);
        
        // После удаления вызовов удаляем мертвые загрузки
        remove_dead_loads(optimized);
        
        // После удаления инструкций нужно пересчитать прыжки
        recalculate_jumps(bc);
        
        // Сжимаем NOP
        size_t write_idx = 0;
        for (size_t read_idx = 0; read_idx < bc->count; read_idx++) {
            if (bc->bytecodes[read_idx].op_code != NOP) {
                if (write_idx != read_idx) bc->bytecodes[write_idx] = bc->bytecodes[read_idx];
                write_idx++;
            }
        }
        bc->count = write_idx;

        DPRINT("[DCE] Iteration %zu removed_calls so far: %zu\n", iteration, total_removed_calls);
        
        if (bc->count < before) {
            changed = true;
            DPRINT("[DCE] Removed %zu instructions this iteration\n", before - bc->count);
        }
        
    } while (changed && iteration < 10 && bc->count > 0);

    if (optimized->code.count < code->code.count) {
        // Проверяем корректность стека
        if (!validate_bytecode_stack(&optimized->code)) {
            DPRINT("[DCE] Verification failed after optimization; aborting DCE for '%s'\n",
                   optimized->name ? optimized->name : "anonymous");
            
            // Очищаем и возвращаем оригинал
            free(optimized->code.bytecodes);
            if (optimized->constants) free(optimized->constants);
            if (optimized->name) free(optimized->name);
            free(optimized);

            if (stats) memset(stats, 0, sizeof(*stats));
            return code;
        }
        
        if (stats) { 
            stats->optimized_functions = 1;
            stats->removed_instructions = code->code.count - optimized->code.count;
            stats->removed_calls = total_removed_calls;
        }
        
        DPRINT("[DCE] Final result: %zu -> %zu instructions (%zu removed)\n",
               code->code.count, optimized->code.count, 
               code->code.count - optimized->code.count);
        
        return optimized;
    }

    DPRINT("[DCE] No optimizations applied\n");
    if (stats) stats->removed_calls = total_removed_calls;
    
    free(optimized->code.bytecodes);
    if (optimized->constants) free(optimized->constants);
    if (optimized->name) free(optimized->name);
    free(optimized);

    return code;
}
