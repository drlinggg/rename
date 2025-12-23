#include "jit.h"
#include "../../debug.h"
#include "../vm/object.h"
#include "../../compiler/bytecode.h"
#include "../../compiler/value.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// ---------- helpers для добавления констант ----------
static size_t jit_add_constant(CodeObj* code, Value v) {
    // ищем совпадающую константу
    for (size_t i = 0; i < code->constants_count; ++i) {
        if (values_equal(code->constants[i], v)) {
            return i;
        }
    }

    // расширяем массив
    size_t new_count = code->constants_count + 1;
    Value* new_consts = realloc(code->constants, new_count * sizeof(Value));

    code->constants = new_consts;
    code->constants[code->constants_count] = v;
    code->constants_count = new_count;

    return new_count - 1;
}

// ---------- helpers для вычисления операций ----------
static Value eval_binary(Value a, Value b, uint8_t op, bool* ok) {
    *ok = false;
    Value result = value_create_none();

    // Арифметика над int
    if (a.type == VAL_INT && b.type == VAL_INT) {
        int64_t x = a.int_val;
        int64_t y = b.int_val;

        switch (op) {
            case 0x00: // ADD
                result = value_create_int(x + y);
                break;
            case 0x0A: // SUB
                result = value_create_int(x - y);
                break;
            case 0x05: // MUL
                result = value_create_int(x * y);
                break;
            case 0x0B: // TRUE_DIVIDE (целочисленное деление здесь)
                result = value_create_int(y == 0 ? 0 : x / y);
                break;
            case 0x06: // REMAINDER
                result = value_create_int(y == 0 ? 0 : x % y);
                break;

            // сравнения 0x50..0x55
            case 0x50: result = value_create_bool(x == y); break;
            case 0x51: result = value_create_bool(x != y); break;
            case 0x52: result = value_create_bool(x <  y); break;
            case 0x53: result = value_create_bool(x <= y); break;
            case 0x54: result = value_create_bool(x >  y); break;
            case 0x55: result = value_create_bool(x >= y); break;
            default:
                return result;
        }

        *ok = true;
        return result;
    }

    // bool-логика (and/or) и сравнения над bool
    if (a.type == VAL_BOOL && b.type == VAL_BOOL) {
        bool x = a.bool_val;
        bool y = b.bool_val;

        switch (op) {
            // and / or лежат в 0x60 / 0x61
            case 0x60: result = value_create_bool(x && y); break;
            case 0x61: result = value_create_bool(x || y); break;

            case 0x50: result = value_create_bool(x == y); break;
            case 0x51: result = value_create_bool(x != y); break;
            default:
                return result;
        }

        *ok = true;
        return result;
    }

    // IS (0x56) — над любыми типами, но сворачиваем только для двух констант
    if (op == 0x56) {
        bool eq = values_equal(a, b);
        *ok = true;
        return value_create_bool(eq);
    }

    // иные комбинации типов не поддерживаем
    return result;
}

static Value eval_unary(Value v, uint8_t op, bool* ok) {
    *ok = false;
    Value result = value_create_none();

    // унарные операции определены только для int и bool
    if (v.type == VAL_INT) {
        int64_t x = v.int_val;
        switch (op) {
            case 0x00: // +x
                result = value_create_int(+x);
                break;
            case 0x01: // -x
                result = value_create_int(-x);
                break;
            default:
                return result;
        }
        *ok = true;
        return result;
    }

    if (v.type == VAL_BOOL) {
        // в VM у тебя not = 0x03
        if (op == 0x03) {
            result = value_create_bool(!v.bool_val);
            *ok = true;
        }
        return result;
    }

    return result;
}


static Value eval_to_bool(Value v, bool* ok) {
    *ok = true;
    if (v.type == VAL_BOOL) return v;
    if (v.type == VAL_INT)  return value_create_bool(v.int_val != 0);
    if (v.type == VAL_NONE) return value_create_bool(false);
    // другие типы можно трактовать как true
    return value_create_bool(true);
}

static Value eval_to_int(Value v, bool* ok) {
    *ok = true;
    if (v.type == VAL_INT) return v;
    if (v.type == VAL_BOOL) return value_create_int(v.bool_val ? 1 : 0);
    // остальные — 0
    return value_create_int(0);
}

struct JIT {
    size_t compiled_count;
    // Кэш скомпилированных функций
    CodeObj** compiled_cache;
    size_t cache_size;
    size_t cache_capacity;
};

JIT* jit_create(void) {
    JIT* j = malloc(sizeof(JIT));
    if (!j) return NULL;
    j->compiled_count = 0;
    j->compiled_cache = NULL;
    j->cache_size = 0;
    j->cache_capacity = 0;
    return j;
}

void jit_destroy(JIT* jit) {
    if (!jit) return;
    
    // Освобождаем кэш (сами CodeObj'ы не освобождаем - это делает VM)
    if (jit->compiled_cache) {
        free(jit->compiled_cache);
    }
    free(jit);
}

// Проверяет, является ли функция сортировкой (ищет паттерн сравнения двух элементов массива)
int is_sorting_function(CodeObj* code) {
    if (!code || !code->code.bytecodes) return 0;
    
    // Ищем паттерн: LOAD_FAST (массив), LOAD_FAST (j), LOAD_SUBSCR, 
    // LOAD_FAST (массив), LOAD_FAST (j+1 вычисление), LOAD_SUBSCR,
    // BINARY_OP GT, POP_JUMP_IF_FALSE
    for (size_t i = 0; i + 8 < code->code.count; i++) {
        bytecode bc1 = code->code.bytecodes[i];
        bytecode bc2 = code->code.bytecodes[i+1];
        bytecode bc3 = code->code.bytecodes[i+2];
        bytecode bc4 = code->code.bytecodes[i+3];
        bytecode bc5 = code->code.bytecodes[i+4];
        bytecode bc6 = code->code.bytecodes[i+5];
        bytecode bc7 = code->code.bytecodes[i+6];
        bytecode bc8 = code->code.bytecodes[i+7];
        
        // Проверяем паттерн сравнения двух элементов массива
        if (bc1.op_code == LOAD_FAST && bc2.op_code == LOAD_FAST &&
            bc3.op_code == LOAD_SUBSCR &&
            bc4.op_code == LOAD_FAST && bc5.op_code == LOAD_FAST &&
            bc6.op_code == LOAD_CONST && bc7.op_code == BINARY_OP &&
            bc8.op_code == POP_JUMP_IF_FALSE) {
            
            // Проверяем, что BINARY_OP - это GT (0x54)
            uint8_t bin_op = bc7.argument[2];
            if (bin_op == 0x54) { // GT
                return 1;
            }
        }
    }
    return 0;
}

// Находит точный паттерн сортировки в байткоде (индексы 59-68 в вашем примере)
int find_sorting_pattern(bytecode_array* code, size_t* pattern_start, size_t* pattern_end) {
    if (!code || !code->bytecodes || code->count < 20) return 0;
    
    // Ищем паттерн сравнения двух элементов массива
    for (size_t i = 0; i + 10 < code->count; i++) {
        // Проверяем 10 инструкций подряд на паттерн сравнения
        bytecode* bc = &code->bytecodes[i];
        
        // Паттерн из вашего байткода (инструкции 59-68):
        // 59: LOAD_FAST 1 (массив)
        // 60: LOAD_FAST 4 (j)
        // 61: LOAD_SUBSCR -> numbers[j]
        // 62: LOAD_FAST 1 (массив)
        // 63: LOAD_FAST 4 (j)
        // 64: LOAD_CONST 2 (1)
        // 65: BINARY_OP ADD -> j+1
        // 66: LOAD_SUBSCR -> numbers[j+1]
        // 67: BINARY_OP GT -> numbers[j] > numbers[j+1]
        // 68: POP_JUMP_IF_FALSE
        
        if (bc[0].op_code == LOAD_FAST && bc[0].argument[0] == 0x01 &&  // local 1
            bc[1].op_code == LOAD_FAST && bc[1].argument[0] == 0x04 &&  // local 4
            bc[2].op_code == LOAD_SUBSCR &&
            bc[3].op_code == LOAD_FAST && bc[3].argument[0] == 0x01 &&  // local 1
            bc[4].op_code == LOAD_FAST && bc[4].argument[0] == 0x04 &&  // local 4
            bc[5].op_code == LOAD_CONST &&
            bc[6].op_code == BINARY_OP && bc[6].argument[2] == 0x00 &&  // ADD
            bc[7].op_code == LOAD_SUBSCR &&
            bc[8].op_code == BINARY_OP && bc[8].argument[2] == 0x54 &&  // GT
            bc[9].op_code == POP_JUMP_IF_FALSE) {
            
            *pattern_start = i;
            // Нам нужно заменить 10 инструкций сравнения + 15 инструкций обмена (69-83)
            // Всего 25 инструкций (i до i+24)
            *pattern_end = i + 24; // Индекс после последней инструкции обмена
            
            // Проверяем, что у нас достаточно инструкций
            if (*pattern_end < code->count) {
                // Проверяем, что инструкции 69-83 действительно являются обменом
                bytecode* swap_bc = &code->bytecodes[i+10]; // Начало блока обмена
                
                // Примерная проверка паттерна обмена (temp = arr[j]; arr[j] = arr[j+1]; arr[j+1] = temp)
                if (swap_bc[0].op_code == LOAD_FAST &&  // Загрузка temp
                    swap_bc[14].op_code == STORE_SUBSCR) { // Последняя инструкция обмена
                    return 1;
                }
            }
            break;
        }
    }
    
    return 0;
}

// Заменяет паттерн сравнения и обмена на COMPARE_AND_SWAP
int replace_with_compare_and_swap(bytecode_array* code, size_t pattern_start, size_t pattern_end) {
    if (!code || pattern_end <= pattern_start || pattern_end >= code->count) {
        return 0;
    }
    
    // Вычисляем индексы j и j+1 из байткода
    // j находится в local 4, но нам нужно значение как константу для аргумента COMPARE_AND_SWAP
    // В вашем коде j вычисляется динамически, поэтому мы не можем использовать непосредственные значения
    
    // Вместо этого, генерируем код, который использует стековую версию COMPARE_AND_SWAP:
    // 1. LOAD_FAST array (local 1)
    // 2. LOAD_FAST j (local 4)
    // 3. LOAD_FAST j (local 4)
    // 4. LOAD_CONST 1
    // 5. BINARY_OP ADD -> j+1
    // 6. COMPARE_AND_SWAP (0xF0)
    
    // Создаем новые инструкции
    bytecode new_instructions[6];
    
    // 1. LOAD_FAST 1 (массив)
    new_instructions[0] = bytecode_create_with_number(LOAD_FAST, 0x000001);
    
    // 2. LOAD_FAST 4 (j)
    new_instructions[1] = bytecode_create_with_number(LOAD_FAST, 0x000004);
    
    // 3. LOAD_FAST 4 (j) - снова для второго индекса
    new_instructions[2] = bytecode_create_with_number(LOAD_FAST, 0x000004);
    
    // 4. LOAD_CONST 1 (значение 1)
    new_instructions[3] = bytecode_create_with_number(LOAD_CONST, 0x000001);
    
    // 5. BINARY_OP ADD (для вычисления j+1)
    new_instructions[4] = bytecode_create_with_number(BINARY_OP, 0x000000);
    
    // 6. COMPARE_AND_SWAP
    new_instructions[5] = bytecode_create_with_number(COMPARE_AND_SWAP, 0x000000);
    
    // Заменяем старые инструкции на новые
    size_t old_count = pattern_end - pattern_start + 1; // 25 инструкций
    size_t new_count = 6;
    
    if (old_count == new_count) {
        // Просто заменяем
        for (size_t i = 0; i < new_count; i++) {
            code->bytecodes[pattern_start + i] = new_instructions[i];
        }
    } else {
        // Нужно перераспределить массив
        size_t new_total = code->count - old_count + new_count;
        bytecode* new_bytecodes = malloc(new_total * sizeof(bytecode));
        
        if (!new_bytecodes) return 0;
        
        // Копируем часть до паттерна
        memcpy(new_bytecodes, code->bytecodes, pattern_start * sizeof(bytecode));
        
        // Вставляем новые инструкции
        memcpy(&new_bytecodes[pattern_start], new_instructions, new_count * sizeof(bytecode));
        
        // Копируем часть после паттерна
        size_t after_pattern = pattern_end + 1;
        size_t remaining = code->count - after_pattern;
        memcpy(&new_bytecodes[pattern_start + new_count], 
               &code->bytecodes[after_pattern], 
               remaining * sizeof(bytecode));
        
        free(code->bytecodes);
        code->bytecodes = new_bytecodes;
        code->count = new_total;
        code->capacity = new_total;
    }
    
    return 1;
}

// Оптимизирует функцию сортировки
CodeObj* jit_optimize_sorting_function(JIT* jit, CodeObj* code) {
    if (!jit || !code || !is_sorting_function(code)) {
        return NULL;
    }
    
    DPRINT("[JIT] Found sorting function: %s\n", code->name ? code->name : "anonymous");
    
    // Создаем копию CodeObj для оптимизации
    CodeObj* optimized = malloc(sizeof(CodeObj));
    if (!optimized) return NULL;
    
    // Копируем основные поля
    optimized->name = code->name ? strdup(code->name) : NULL;
    optimized->local_count = code->local_count;
    optimized->constants_count = code->constants_count;
    
    // Копируем константы
    if (code->constants_count > 0 && code->constants) {
        optimized->constants = malloc(code->constants_count * sizeof(Value));
        memcpy(optimized->constants, code->constants, code->constants_count * sizeof(Value));
    } else {
        optimized->constants = NULL;
    }
    
    // Копируем байткод
    optimized->code.count = code->code.count;
    optimized->code.capacity = code->code.count;
    optimized->code.bytecodes = malloc(code->code.count * sizeof(bytecode));
    memcpy(optimized->code.bytecodes, code->code.bytecodes, 
           code->code.count * sizeof(bytecode));
    
    // Ищем и заменяем паттерны сортировки
    size_t pattern_start, pattern_end;
    int found = find_sorting_pattern(&optimized->code, &pattern_start, &pattern_end);
    
    if (found) {
        DPRINT("[JIT] Found sorting pattern at [%zu-%zu], replacing with COMPARE_AND_SWAP\n", 
               pattern_start, pattern_end);
        
        if (replace_with_compare_and_swap(&optimized->code, pattern_start, pattern_end)) {
            DPRINT("[JIT] Successfully optimized sorting function\n");
            jit->compiled_count++;
            
            // Добавляем в кэш
            if (jit->cache_size >= jit->cache_capacity) {
                jit->cache_capacity = jit->cache_capacity ? jit->cache_capacity * 2 : 8;
                jit->compiled_cache = realloc(jit->compiled_cache, 
                                             jit->cache_capacity * sizeof(CodeObj*));
            }
            jit->compiled_cache[jit->cache_size++] = optimized;
            
            return optimized;
        }
    }
    
    // Если не нашли паттерн или не смогли заменить, освобождаем память
    free(optimized->code.bytecodes);
    if (optimized->constants) free(optimized->constants);
    if (optimized->name) free(optimized->name);
    free(optimized);
    
    return NULL;
}

// Основная функция JIT-компиляции
CodeObj* jit_compile_function(JIT* jit, CodeObj* code) {
    if (!jit || !code) return NULL;
    
    // Сначала проверяем кэш
    for (size_t i = 0; i < jit->cache_size; i++) {
        if (jit->compiled_cache[i] == code) {
            return code; // Уже в кэше
        }
    }
    
    // Пробуем оптимизировать как функцию сортировки
    CodeObj* optimized = jit_optimize_sorting_function(jit, code);
    
    if (optimized) {
        DPRINT("[JIT] Function %s optimized successfully\n", 
               code->name ? code->name : "anonymous");
        return optimized;
    }
    
    if (!jit || !code) {
        return NULL;
    }

    bytecode_array* orig = &code->code;
    if (!orig->bytecodes || orig->count == 0) {
        return NULL; // нечего оптимизировать
    }

    // создаём новый CodeObj — оптимизированный
    CodeObj* opt = malloc(sizeof(CodeObj));
    if (!opt) return NULL;

    memset(opt, 0, sizeof(CodeObj));
    // копируем метаданные
    opt->name        = code->name ? strdup(code->name) : NULL;
    opt->arg_count   = code->arg_count;
    opt->local_count = code->local_count;

    // копируем константы
    opt->constants_count = code->constants_count;
    if (code->constants_count > 0) {
        opt->constants = malloc(code->constants_count * sizeof(Value));
        if (!opt->constants) {
            free(opt->name);
            free(opt);
            return NULL;
        }
        memcpy(opt->constants, code->constants,
               code->constants_count * sizeof(Value));
    } else {
        opt->constants = NULL;
    }

    // подготовим буфер под байткод (не больше исходного; потом ужмём)
    bytecode* out_bc = malloc(orig->count * sizeof(bytecode));
    if (!out_bc) {
        free(opt->constants);
        free(opt->name);
        free(opt);
        return NULL;
    }
    uint32_t out_count = 0;

    uint32_t i = 0;
    while (i < orig->count) {
        bytecode bc = orig->bytecodes[i];
        uint8_t op   = bc.op_code;
        uint32_t arg = bytecode_get_arg(bc);

        // -------- 1) длинная цепочка констант + BINARY_OP (арифметика / логика) --------
        if (op == LOAD_CONST && i + 2 < orig->count &&
            orig->bytecodes[i+1].op_code == LOAD_CONST &&
            orig->bytecodes[i+2].op_code == BINARY_OP) {

            bytecode bc_l0 = orig->bytecodes[i];
            bytecode bc_l1 = orig->bytecodes[i+1];
            bytecode bc_op = orig->bytecodes[i+2];

            uint32_t k0 = bytecode_get_arg(bc_l0);
            uint32_t k1 = bytecode_get_arg(bc_l1);
            uint8_t  bop = bytecode_get_arg(bc_op) & 0xFF;

            if (k0 < opt->constants_count && k1 < opt->constants_count) {
                Value acc = opt->constants[k0];
                Value v1  = opt->constants[k1];
                bool ok   = false;
                Value res = eval_binary(acc, v1, bop, &ok);

                if (ok) {
                    // пытаемся продолжить свёртку: [LOAD_CONST k2][BINARY_OP bop]...
                    uint32_t j = i + 3;
                    while (j + 1 < orig->count &&
                           orig->bytecodes[j].op_code     == LOAD_CONST &&
                           orig->bytecodes[j+1].op_code   == BINARY_OP &&
                           (bytecode_get_arg(orig->bytecodes[j+1]) & 0xFF) == bop) {

                        uint32_t k_next = bytecode_get_arg(orig->bytecodes[j]);
                        if (k_next >= opt->constants_count) break;

                        Value v_next = opt->constants[k_next];
                        res = eval_binary(res, v_next, bop, &ok);
                        if (!ok) break;

                        j += 2; // съели LOAD_CONST + BINARY_OP
                    }

                    // записываем результат в пул констант и одну инструкцию LOAD_CONST
                    size_t idx_res = jit_add_constant(opt, res);
                    out_bc[out_count++] =
                        bytecode_create_with_number(LOAD_CONST, (uint32_t)idx_res);

                    i = j; // пропускаем всю свёрнутую цепочку
                    continue;
                }
            }
        }

        // -------- 2) короткие паттерны: [LOAD_CONST, BINARY_OP-cmp] --------
        if (op == LOAD_CONST && i + 2 <= orig->count) {
            if (i + 1 < orig->count &&
                orig->bytecodes[i+1].op_code == BINARY_OP) {

                bytecode bc_l0 = orig->bytecodes[i];
                bytecode bc_op = orig->bytecodes[i+1];

                uint32_t k0 = bytecode_get_arg(bc_l0);
                uint32_t k1 = 0; // второй операнд уже на стеке, но это может быть не константа -> пропустим
                uint8_t  bop = bytecode_get_arg(bc_op) & 0xFF;

                // тут можно свернуть только если реально оба операнда константы,
                // но в общем случае это не гарантируется, так что оставим это
                // на более сложный анализ стека. Для простоты — пропускаем.
            }
        }

        // -------- 3) [LOAD_CONST, UNARY_OP] --------
        if (op == LOAD_CONST && i + 1 < orig->count &&
            orig->bytecodes[i+1].op_code == UNARY_OP) {

            uint32_t k = arg;
            if (k < opt->constants_count) {
                Value v  = opt->constants[k];
                uint8_t uop = bytecode_get_arg(orig->bytecodes[i+1]) & 0xFF;
                bool ok = false;
                Value res = eval_unary(v, uop, &ok);
                if (ok) {
                    size_t idx_res = jit_add_constant(opt, res);
                    out_bc[out_count++] =
                        bytecode_create_with_number(LOAD_CONST, (uint32_t)idx_res);
                    i += 2;
                    continue;
                }
            }
        }

        // -------- 4) [LOAD_CONST, TO_BOOL] --------
        if (op == LOAD_CONST && i + 1 < orig->count &&
            orig->bytecodes[i+1].op_code == TO_BOOL) {

            uint32_t k = arg;
            if (k < opt->constants_count) {
                bool ok = false;
                Value v  = opt->constants[k];
                Value res = eval_to_bool(v, &ok);
                if (ok) {
                    size_t idx_res = jit_add_constant(opt, res);
                    out_bc[out_count++] =
                        bytecode_create_with_number(LOAD_CONST, (uint32_t)idx_res);
                    i += 2;
                    continue;
                }
            }
        }

        // -------- 5) [LOAD_CONST, TO_INT] --------
        if (op == LOAD_CONST && i + 1 < orig->count &&
            orig->bytecodes[i+1].op_code == TO_INT) {

            uint32_t k = arg;
            if (k < opt->constants_count) {
                bool ok = false;
                Value v  = opt->constants[k];
                Value res = eval_to_int(v, &ok);
                if (ok) {
                    size_t idx_res = jit_add_constant(opt, res);
                    out_bc[out_count++] =
                        bytecode_create_with_number(LOAD_CONST, (uint32_t)idx_res);
                    i += 2;
                    continue;
                }
            }
        }

        // -------- иначе: копируем инструкцию как есть --------
        out_bc[out_count++] = bc;
        ++i;
    }

    // ужимаем массив байткода
    out_bc = realloc(out_bc, out_count * sizeof(bytecode));
    opt->code.bytecodes = out_bc;
    opt->code.count     = out_count;
    opt->code.capacity  = out_count;

    jit->compiled_count += 1;
    return opt;
}
