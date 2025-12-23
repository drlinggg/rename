#include "jit.h"
#include "../../debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
    
    return code; // Возвращаем оригинал, если не оптимизировали
}
