#include "cmpswap.h"
#include "../../compiler/bytecode.h"
#include "../../compiler/value.h"
#include "../../runtime/vm/vm.h"
#include "../../system.h"
#include <string.h>

static int is_load_fast_with_index(bytecode bc, uint32_t expected_index) {
    return (bc.op_code == LOAD_FAST && bytecode_get_arg(bc) == expected_index);
}

// arr[j] > arr[j+1]
static int find_bubble_sort_pattern(bytecode_array* code, size_t* start, size_t* end) {
    if (!code || !code->bytecodes || code->count < 25) {
        return 0;
    }
    
    for (size_t i = 0; i <= code->count - 25; i++) {
        bytecode* bc = &code->bytecodes[i];
        
        // 1. if (arr[j] > arr[j+1]) {
        // 2.   temp = arr[j];
        // 3.   arr[j] = arr[j+1];
        // 4.   arr[j+1] = temp;
        // }
        
        // Шаг 1: if condition
        if (bc[0].op_code != LOAD_FAST || bc[1].op_code != LOAD_FAST ||
            bc[2].op_code != LOAD_SUBSCR || bc[3].op_code != LOAD_FAST ||
            bc[4].op_code != LOAD_FAST || bc[5].op_code != LOAD_CONST ||
            bc[6].op_code != BINARY_OP || bc[7].op_code != LOAD_SUBSCR ||
            bc[8].op_code != BINARY_OP || bc[9].op_code != POP_JUMP_IF_FALSE) {
            continue;
        }
        
        if ((bytecode_get_arg(bc[6]) & 0xFF) != 0x00 ||  // ADD
            (bytecode_get_arg(bc[8]) & 0xFF) != 0x54) {  // GT
            continue;
        }
        
        // Шаг 2: temp = arr[j] (должен следовать сразу)
        if (i + 10 >= code->count) continue;
        if (bc[10].op_code != LOAD_FAST || bc[11].op_code != LOAD_FAST ||
            bc[12].op_code != LOAD_SUBSCR || bc[13].op_code != STORE_FAST) {
            continue;
        }
        
        // Проверяем что индексы совпадают
        uint32_t array_idx = bytecode_get_arg(bc[0]);
        uint32_t j_idx = bytecode_get_arg(bc[1]);
        
        if (bytecode_get_arg(bc[10]) != array_idx ||
            bytecode_get_arg(bc[11]) != j_idx) {
            continue;
        }
        
        // Шаг 3: arr[j] = arr[j+1]
        if (i + 14 >= code->count) continue;
        if (bc[14].op_code != LOAD_FAST || bc[15].op_code != LOAD_FAST ||
            bc[16].op_code != LOAD_CONST || bc[17].op_code != BINARY_OP ||
            bc[18].op_code != LOAD_SUBSCR || bc[19].op_code != STORE_SUBSCR) {
            continue;
        }
        
        // Шаг 4: arr[j+1] = temp
        if (i + 20 >= code->count) continue;
        if (bc[20].op_code != LOAD_FAST ||  // temp
            bc[21].op_code != LOAD_FAST ||  // array
            bc[22].op_code != LOAD_FAST ||  // j
            bc[23].op_code != LOAD_CONST || // 1
            bc[24].op_code != BINARY_OP) {  // ADD
            continue;
        }
        
        // Нашли полный паттерн!
        *start = i;
        *end = i + 24; // Последняя инструкция перед STORE_SUBSCR
        
        DPRINT("[JIT] Found complete bubble pattern at %zu-%zu\n", *start, *end);
        return 1;
    }
    
    return 0;
}

// arr[i] > pivot
static int find_quicksort_pattern(bytecode_array* code, size_t* start, size_t* end) {
    if (!code || !code->bytecodes || code->count < 8) {
        return 0;
    }
    
    for (size_t i = 0; i <= code->count - 8; i++) {
        bytecode* bc = &code->bytecodes[i];
        
        // Ищем любой паттерн сравнения элементов массива
        // LOAD_FAST arr, LOAD_FAST index, LOAD_SUBSCR, 
        // LOAD_FAST something, BINARY_OP COMPARE, POP_JUMP_IF_FALSE
        if (bc[0].op_code != LOAD_FAST || 
            bc[1].op_code != LOAD_FAST || 
            bc[2].op_code != LOAD_SUBSCR ||
            bc[3].op_code != LOAD_FAST ||
            bc[4].op_code != BINARY_OP ||
            bc[5].op_code != POP_JUMP_IF_FALSE) {
            continue;
        }
        
        uint32_t compare_op = bytecode_get_arg(bc[4]) & 0xFF;
        if (compare_op != 0x53 && // LESS_THAN
            compare_op != 0x54 && // GREATER_THAN
            compare_op != 0x55) { // LESS_EQUAL
            continue;
        }
        
        *start = i;
        
        // Ищем ближайший конец блока
        size_t potential_end = i + 6;
        size_t max_search = i + 30;
        if (max_search > code->count) max_search = code->count;
        
        for (size_t j = i + 6; j < max_search; j++) {
            if (code->bytecodes[j].op_code == JUMP_BACKWARD || 
                code->bytecodes[j].op_code == JUMP_FORWARD) {
                potential_end = j;
                break;
            }
        }
        
        *end = potential_end;
        return 1;
    }
    
    return 0;
}

// General
static int find_sorting_pattern_in_bytecode(bytecode_array* code, size_t* start, size_t* end) {
    // Сначала ищем пузырек
    if (find_bubble_sort_pattern(code, start, end)) {
        DPRINT("[JIT] Found bubble sort pattern\n");
        return 1;
    }
    
    // Потом быструю сортировку
    if (find_quicksort_pattern(code, start, end)) {
        DPRINT("[JIT] Found quicksort pattern\n");
        return 1;
    }
    
    return 0;
}

static int extract_pattern_info(bytecode_array* code, size_t pattern_start, PatternInfo* info) {
    if (!code || pattern_start + 9 >= code->count) {
        return 0;
    }
    
    bytecode* bc = &code->bytecodes[pattern_start];
    
    if (bc[0].op_code == LOAD_FAST && bc[1].op_code == LOAD_FAST &&
        bc[2].op_code == LOAD_SUBSCR && bc[3].op_code == LOAD_FAST) {
        
        info->array_index = bytecode_get_arg(bc[0]);
        info->j_index = bytecode_get_arg(bc[1]);
        info->const_one_index = bytecode_get_arg(bc[5]);
        
        if (bytecode_get_arg(bc[3]) != info->array_index) {
            DPRINT("[JIT] Warning: Inconsistent array indices\n");
            return 0;
        }
        
        if (bytecode_get_arg(bc[4]) != info->j_index) {
            DPRINT("[JIT] Warning: Inconsistent j indices\n");
            return 0;
        }
        
        DPRINT("[JIT] Extracted pattern: array@%u, j@%u, const_1@%u\n",
               info->array_index, info->j_index, info->const_one_index);
        
        return 1;
    }
    
    return 0;
}

static bytecode_array generate_compare_and_swap_code(PatternInfo* info) {
    bytecode new_instructions[6];
    
    new_instructions[0] = bytecode_create_with_number(LOAD_FAST, info->array_index);
    new_instructions[1] = bytecode_create_with_number(LOAD_FAST, info->j_index);
    new_instructions[2] = bytecode_create_with_number(LOAD_FAST, info->j_index);
    new_instructions[3] = bytecode_create_with_number(LOAD_CONST, info->const_one_index);
    new_instructions[4] = bytecode_create_with_number(BINARY_OP, 0x000000);
    new_instructions[5] = bytecode_create_with_number(COMPARE_AND_SWAP, 0x000000);
    
    bytecode* bc_copy = malloc(6 * sizeof(bytecode));
    if (!bc_copy) {
        bytecode_array empty = {0};
        return empty;
    }
    
    memcpy(bc_copy, new_instructions, 6 * sizeof(bytecode));
    return create_bytecode_array(bc_copy, 6);
}

static int replace_pattern(bytecode_array* code, size_t pattern_start, size_t pattern_end, 
                          PatternInfo* info) {
    if (!code || pattern_end <= pattern_start || pattern_end >= code->count) {
        return 0;
    }
    
    bytecode_array optimized = generate_compare_and_swap_code(info);
    if (optimized.count == 0) {
        return 0;
    }
    
    size_t old_size = pattern_end - pattern_start + 1;
    size_t new_size = optimized.count;
    
    DPRINT("[JIT] Replacing %zu instructions with %zu optimized instructions\n", 
           old_size, new_size);
    
    size_t new_total = code->count - old_size + new_size;
    bytecode* new_bytecodes = malloc(new_total * sizeof(bytecode));
    if (!new_bytecodes) {
        free_bytecode_array(optimized);
        return 0;
    }
    
    memcpy(new_bytecodes, code->bytecodes, pattern_start * sizeof(bytecode));
    memcpy(&new_bytecodes[pattern_start], optimized.bytecodes, new_size * sizeof(bytecode));
    
    size_t after_pattern = pattern_end + 1;
    size_t remaining = code->count - after_pattern;
    memcpy(&new_bytecodes[pattern_start + new_size], 
           &code->bytecodes[after_pattern], 
           remaining * sizeof(bytecode));
    
    free(code->bytecodes);
    code->bytecodes = new_bytecodes;
    code->count = new_total;
    code->capacity = new_total;
    
    free_bytecode_array(optimized);
    return 1;
}

int is_sorting_function(void* code_ptr) {
    CodeObj* code = (CodeObj*)code_ptr;
    if (!code || !code->code.bytecodes) {
        return 0;
    }
    
    size_t start, end;
    return find_sorting_pattern_in_bytecode(&code->code, &start, &end);
}

void* jit_optimize_compare_and_swap(void* jit_ptr, void* original_ptr) {
    CodeObj* original = (CodeObj*)original_ptr;
    
    DPRINT("[JIT] Checking function: %s\n", 
           original->name ? original->name : "anonymous");
    DPRINT("[JIT] Bytecode count: %zu\n", original->code.count);
    
    for (int i = 0; i < 20 && i < original->code.count; i++) {
        bytecode bc = original->code.bytecodes[i];
        bytecode_print(&bc);
    }
    size_t pattern_start, pattern_end;
    if (!find_sorting_pattern_in_bytecode(&original->code, &pattern_start, &pattern_end)) {
        DPRINT("[JIT] No sorting pattern found\n");
        return NULL;
    }
    
    // Extract pattern information
    PatternInfo info;
    if (!extract_pattern_info(&original->code, pattern_start, &info)) {
        DPRINT("[JIT] Failed to extract pattern information\n");
        return NULL;
    }
    
    // Create optimized copy
    CodeObj* optimized = malloc(sizeof(CodeObj));
    if (!optimized) {
        return NULL;
    }
    
    // Copy basic information
    optimized->name = original->name ? strdup(original->name) : NULL;
    optimized->local_count = original->local_count;
    optimized->constants_count = original->constants_count;
    optimized->arg_count = original->arg_count;
    
    // Copy constants
    if (original->constants_count > 0 && original->constants) {
        optimized->constants = malloc(original->constants_count * sizeof(Value));
        memcpy(optimized->constants, original->constants, 
               original->constants_count * sizeof(Value));
    } else {
        optimized->constants = NULL;
    }
    
    // Copy bytecode
    optimized->code.count = original->code.count;
    optimized->code.capacity = original->code.count;
    optimized->code.bytecodes = malloc(original->code.count * sizeof(bytecode));
    memcpy(optimized->code.bytecodes, original->code.bytecodes, 
           original->code.count * sizeof(bytecode));
    
    // Apply optimization
    if (!replace_pattern(&optimized->code, pattern_start, pattern_end, &info)) {
        DPRINT("[JIT] Failed to apply optimization\n");
        free(optimized->code.bytecodes);
        if (optimized->constants) free(optimized->constants);
        if (optimized->name) free(optimized->name);
        free(optimized);
        return NULL;
    }
    
    DPRINT("[JIT] Successfully optimized function\n");
    return optimized;
}
