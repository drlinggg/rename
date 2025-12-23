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

// Находит точный паттерн сортировки в байткоде
int find_sorting_pattern(bytecode_array* code, size_t* pattern_start, size_t* pattern_end) {
    if (!code || !code->bytecodes || code->count < 20) return 0;
    
    // Ищем паттерн сравнения двух элементов массива
    for (size_t i = 0; i + 10 < code->count; i++) {
        // Проверяем 10 инструкций подряд на паттерн сравнения
        bytecode* bc = &code->bytecodes[i];
        
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

int replace_with_compare_and_swap(bytecode_array* code, size_t pattern_start, size_t pattern_end) {
    if (!code || pattern_end <= pattern_start || pattern_end >= code->count) {
        return 0;
    }
    
    // АНАЛИЗИРУЕМ СУЩЕСТВУЮЩИЙ БАЙТКОД, чтобы получить фактические индексы
    bytecode* orig = &code->bytecodes[pattern_start];
    
    // Извлекаем индексы из оригинального байткода:
    // Паттерн: LOAD_FAST array_idx, LOAD_FAST j_idx, LOAD_SUBSCR,
    //          LOAD_FAST array_idx, LOAD_FAST j_idx, LOAD_CONST 1,
    //          BINARY_OP ADD, LOAD_SUBSCR, BINARY_OP GT, ...
    
    if (pattern_start + 9 >= code->count) return 0;
    
    // 1. Получаем индекс массива (должен быть одинаковый в двух местах)
    uint32_t array_idx1 = bytecode_get_arg(orig[0]);  // Первый LOAD_FAST
    uint32_t array_idx2 = bytecode_get_arg(orig[3]);  // Четвертый LOAD_FAST
    
    if (array_idx1 != array_idx2) {
        DPRINT("[JIT] Warning: array indices differ: %u vs %u\n", array_idx1, array_idx2);
        // Используем первый
    }
    uint32_t array_index = array_idx1;
    
    // 2. Получаем индекс j (переменной цикла)
    uint32_t j_idx1 = bytecode_get_arg(orig[1]);  // Второй LOAD_FAST
    uint32_t j_idx2 = bytecode_get_arg(orig[4]);  // Пятый LOAD_FAST
    
    if (j_idx1 != j_idx2) {
        DPRINT("[JIT] Warning: j indices differ: %u vs %u\n", j_idx1, j_idx2);
    }
    uint32_t j_index = j_idx1;
    
    // 3. Получаем константу 1 (для j+1 вычисления)
    uint32_t const_one_idx = bytecode_get_arg(orig[5]);  // LOAD_CONST
    
    // 4. Проверяем, что это действительно ADD операция
    bytecode add_op = orig[6];
    if (add_op.op_code != BINARY_OP || (bytecode_get_arg(add_op) & 0xFF) != 0x00) {
        DPRINT("[JIT] Warning: Expected BINARY_OP ADD, got something else\n");
        return 0;
    }
    
    DPRINT("[JIT] Found pattern: array_idx=%u, j_idx=%u, const_1_idx=%u\n",
           array_index, j_index, const_one_idx);
    
    // Создаем новые инструкции с ИЗВЛЕЧЕННЫМИ индексами
    bytecode new_instructions[6];
    
    // 1. LOAD_FAST array_index (массив)
    new_instructions[0] = bytecode_create_with_number(LOAD_FAST, array_index);
    
    // 2. LOAD_FAST j_index (j)
    new_instructions[1] = bytecode_create_with_number(LOAD_FAST, j_index);
    
    // 3. LOAD_FAST j_index (j) - снова для второго индекса
    new_instructions[2] = bytecode_create_with_number(LOAD_FAST, j_index);
    
    // 4. LOAD_CONST const_one_idx (значение 1)
    new_instructions[3] = bytecode_create_with_number(LOAD_CONST, const_one_idx);
    
    // 5. BINARY_OP ADD (для вычисления j+1)
    new_instructions[4] = bytecode_create_with_number(BINARY_OP, 0x000000); // 0x00 = ADD
    
    // 6. COMPARE_AND_SWAP
    new_instructions[5] = bytecode_create_with_number(COMPARE_AND_SWAP, 0x000000);
    
    // Заменяем старые инструкции на новые
    size_t old_count = pattern_end - pattern_start + 1;
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
