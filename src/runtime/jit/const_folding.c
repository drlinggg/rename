#include "const_folding.h"
#include "../../system.h"
#include <string.h>
#include <stdlib.h>

// Объявления вспомогательных функций
static int is_truthy(Value v);
static int is_falsy(Value v);
static size_t find_or_add_constant(CodeObj* code, Value v, FoldStats* stats);
static CodeObj* deep_copy_codeobj(CodeObj* original);
static int is_constant_foldable(Value a, Value b, uint8_t op);
static int is_unary_foldable(Value a, uint8_t op);
static Value fold_binary_constant(Value a, Value b, uint8_t op);
static Value fold_unary_constant(Value a, uint8_t op);

// Вспомогательная функция для пропуска NOP
static size_t skip_nops(bytecode_array* bc, size_t start_index) {
    while (start_index < bc->count && bc->bytecodes[start_index].op_code == NOP) {
        start_index++;
    }
    return start_index;
}

// Получение цели прыжка (абсолютный индекс)
static size_t get_jump_target(bytecode_array* bc, size_t ins_index, uint32_t arg, uint8_t op) {
    if (op == JUMP_BACKWARD) {
        return ins_index - arg;
    } else if (op == JUMP_FORWARD || op == POP_JUMP_IF_FALSE || op == POP_JUMP_IF_TRUE) {
        return ins_index + 1 + arg; // +1 потому что смещение от следующей инструкции
    }
    return (size_t)-1;
}

// Реализации функций проверки истинности
static int is_truthy(Value v) {
    if (v.type == VAL_BOOL) return v.bool_val;
    if (v.type == VAL_INT) return v.int_val != 0;
    return 0;
}

static int is_falsy(Value v) {
    return !is_truthy(v);
}

// Реализация поиска/добавления константы
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

// Реализация глубокого копирования CodeObj
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

// Улучшенный пересчет прыжков с учетом особенностей VM
static void recalculate_jumps(bytecode_array* bc) {
    // Сначала строим карту: старый индекс -> новый индекс
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
    
    // Пересчитываем все прыжки
    for (size_t new_i = 0; new_i < new_count; new_i++) {
        size_t old_i = new_to_old[new_i];
        bytecode* ins = &bc->bytecodes[old_i];
        uint8_t op = ins->op_code;
        
        if (op != JUMP_FORWARD && op != JUMP_BACKWARD && 
            op != POP_JUMP_IF_FALSE && op != POP_JUMP_IF_TRUE) {
            continue;
        }
        
        uint32_t old_arg = bytecode_get_arg(*ins);
        
        // Получаем старую цель прыжка
        size_t old_target = get_jump_target(bc, old_i, old_arg, op);
        if (old_target >= bc->count) {
            continue;
        }
        
        // Находим новый индекс цели
        size_t new_target = old_to_new[old_target];
        if (new_target == (size_t)-1) {
            // Цель была удалена - ищем ближайшую живую инструкцию
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
        
        // Вычисляем новое смещение
        uint32_t new_arg;
        if (op == JUMP_BACKWARD) {
            if (new_target > new_i) continue;
            new_arg = (uint32_t)(new_i - new_target);
        } else {
            // JUMP_FORWARD, POP_JUMP_IF_FALSE, POP_JUMP_IF_TRUE
            if (new_target < new_i + 1) continue;
            new_arg = (uint32_t)(new_target - new_i - 1); // -1 для VM
        }
        
        // Обновляем инструкцию
        *ins = bytecode_create_with_number(op, new_arg);
    }
    
    // Удаляем NOP и сжимаем массив
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

static void mark_as_nop(bytecode_array* bc, size_t index) {
    if (index < bc->count) {
        bc->bytecodes[index].op_code = NOP;
    }
}

// Жадное сворачивание цепочек операций
static int fold_operation_chain(CodeObj* code, bytecode_array* bc, size_t start, FoldStats* stats) {
    // Мы будем складывать константы в стек и сворачивать операции по мере их появления
    size_t pos = start;
    Value stack[64];  // Максимальная глубина стека
    size_t stack_size = 0;
    int changed = 0;
    
    // Массивы для отметки инструкций, которые нужно удалить
    int* to_remove = calloc(bc->count, sizeof(int));
    
    while (pos < bc->count) {
        bytecode ins = bc->bytecodes[pos];
        uint8_t op = ins.op_code;
        
        if (op == LOAD_CONST) {
            uint32_t const_idx = bytecode_get_arg(ins);
            if (const_idx >= code->constants_count) break;
            stack[stack_size++] = code->constants[const_idx];
            to_remove[pos] = 1;  // Пометить для удаления
            pos = skip_nops(bc, pos + 1);
        }
        else if (op == BINARY_OP && stack_size >= 2) {
            uint8_t binop = bytecode_get_arg(ins) & 0xFF;
            Value b = stack[--stack_size];
            Value a = stack[--stack_size];
            
            if (is_constant_foldable(a, b, binop)) {
                Value result = fold_binary_constant(a, b, binop);
                if (result.type != VAL_NONE) {
                    stack[stack_size++] = result;
                    to_remove[pos] = 1;  // Пометить для удаления
                    changed = 1;
                } else {
                    // Нельзя свернуть, возвращаем значения обратно
                    stack[stack_size++] = a;
                    stack[stack_size++] = b;
                    break;  // Заканчиваем цепочку
                }
            } else {
                // Нельзя свернуть
                break;
            }
            pos = skip_nops(bc, pos + 1);
        }
        else if (op == UNARY_OP && stack_size >= 1) {
            uint8_t unop = bytecode_get_arg(ins) & 0xFF;
            Value a = stack[--stack_size];
            
            if (is_unary_foldable(a, unop)) {
                Value result = fold_unary_constant(a, unop);
                if (result.type != VAL_NONE) {
                    stack[stack_size++] = result;
                    to_remove[pos] = 1;  // Пометить для удаления
                    changed = 1;
                } else {
                    stack[stack_size++] = a;
                    break;
                }
            } else {
                break;
            }
            pos = skip_nops(bc, pos + 1);
        }
        else {
            break;  // Не операция для свертки
        }
    }
    
    // Если что-то свернулось и остался результат
    if (changed && stack_size == 1) {
        // Добавляем новую константу
        size_t new_const_idx = find_or_add_constant(code, stack[0], stats);
        if (new_const_idx != (size_t)-1) {
            // Заменяем первую инструкцию в цепочке на результат
            bc->bytecodes[start] = bytecode_create_with_number(LOAD_CONST, new_const_idx);
            // Удаляем остальные инструкции в цепочке
            for (size_t i = start + 1; i < pos; i++) {
                if (to_remove[i]) {
                    mark_as_nop(bc, i);
                    stats->removed_instructions++;
                }
            }
            stats->folded_constants++;
        }
    }
    
    free(to_remove);
    return changed;
}

// Поиск цепочек операций для свертки
static int find_and_fold_chains(CodeObj* code, bytecode_array* bc, FoldStats* stats) {
    int changed = 0;
    
    for (size_t i = 0; i < bc->count; i = skip_nops(bc, i + 1)) {
        // Ищем начало цепочки: LOAD_CONST
        if (bc->bytecodes[i].op_code == LOAD_CONST) {
            // Пробуем свернуть цепочку, начиная с этой позиции
            if (fold_operation_chain(code, bc, i, stats)) {
                changed = 1;
                break;  // Начинаем заново после изменений
            }
        }
    }
    
    return changed;
}

// Упрощенная оптимизация условий if
static void optimize_constant_ifs(CodeObj* code, bytecode_array* bc, FoldStats* stats) {
    for (size_t i = 0; i < bc->count; i = skip_nops(bc, i + 1)) {
        // Ищем LOAD_CONST -> POP_JUMP_IF_FALSE
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
                // if (false) - заменяем на безусловный прыжок
                uint32_t jump_offset = bytecode_get_arg(bc->bytecodes[jump_idx]);
                
                // Удаляем LOAD_CONST
                mark_as_nop(bc, const_idx);
                
                // Заменяем POP_JUMP_IF_FALSE на JUMP_FORWARD
                bc->bytecodes[jump_idx] = bytecode_create_with_number(JUMP_FORWARD, jump_offset);
                
                stats->peephole_optimizations++;
                
            } else if (is_truthy(v)) {
                // if (true) - удаляем условный прыжок
                mark_as_nop(bc, const_idx);
                mark_as_nop(bc, jump_idx);
                stats->peephole_optimizations++;
            }
        }
    }
}

// Реализации функций для свертки констант
static int is_constant_foldable(Value a, Value b, uint8_t op) {
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

static int is_unary_foldable(Value a, uint8_t op) {
    if (op == 0x00 || op == 0x01) { // POSITIVE, NEGATIVE
        return a.type == VAL_INT;
    }
    
    if (op == 0x03) { // NOT
        return a.type == VAL_BOOL;
    }
    
    return 0;
}

static Value fold_binary_constant(Value a, Value b, uint8_t op) {
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

static Value fold_unary_constant(Value a, uint8_t op) {
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
    
    do {
        changed = 0;
        
        // Шаг 1: Сворачиваем цепочки операций
        if (find_and_fold_chains(optimized, bc, stats)) {
            changed = 1;
            recalculate_jumps(bc);
            continue;
        }
        
        // Шаг 2: Оптимизация условий if
        optimize_constant_ifs(optimized, bc, stats);
        if (stats->peephole_optimizations > 0) {
            changed = 1;
            recalculate_jumps(bc);
            stats->peephole_optimizations = 0; // Сброс для следующей итерации
        }
        
    } while (changed && bc->count > 0);
    
    // Финальный пересчет прыжков
    recalculate_jumps(bc);
    
    DPRINT("[JIT-CF] Optimization complete:\n");
    DPRINT("  Folded constants: %zu\n", stats->folded_constants);
    DPRINT("  Removed instructions: %zu\n", stats->removed_instructions);
    DPRINT("  Peephole optimizations: %zu\n", stats->peephole_optimizations);
    DPRINT("  Original size: %zu, Optimized size: %zu\n", 
           original->code.count, bc->count);
    
    return optimized;
}
