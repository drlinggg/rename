#include "jit_types.h"
#include "cmpswap.h"
#include "../../compiler/bytecode.h"
#include "../../compiler/value.h"
#include "../../runtime/vm/vm.h"
#include "../../debug.h"
#include <string.h>

static int is_load_fast_with_index(bytecode bc, uint32_t expected_index) {
    return (bc.op_code == LOAD_FAST && bytecode_get_arg(bc) == expected_index);
}

static int find_sorting_pattern_in_bytecode(bytecode_array* code, size_t* start, size_t* end) {
    if (!code || !code->bytecodes || code->count < 10) {
        return 0;
    }
    
    // Look for the pattern: if (arr[j] > arr[j+1]) { swap }
    for (size_t i = 0; i <= code->count - 10; i++) {
        bytecode* bc = &code->bytecodes[i];
        
        // Pattern: LOAD_FAST arr, LOAD_FAST j, LOAD_SUBSCR,
        //          LOAD_FAST arr, LOAD_FAST j, LOAD_CONST 1,
        //          BINARY_OP ADD, LOAD_SUBSCR, BINARY_OP GT,
        //          POP_JUMP_IF_FALSE
        
        // Check opcodes
        if (bc[0].op_code != LOAD_FAST || bc[1].op_code != LOAD_FAST ||
            bc[2].op_code != LOAD_SUBSCR || bc[3].op_code != LOAD_FAST ||
            bc[4].op_code != LOAD_FAST || bc[5].op_code != LOAD_CONST ||
            bc[6].op_code != BINARY_OP || bc[7].op_code != LOAD_SUBSCR ||
            bc[8].op_code != BINARY_OP || bc[9].op_code != POP_JUMP_IF_FALSE) {
            continue;
        }
        
        // Check that array indices are the same
        uint32_t arr_idx1 = bytecode_get_arg(bc[0]);
        uint32_t arr_idx2 = bytecode_get_arg(bc[3]);
        if (arr_idx1 != arr_idx2) {
            continue;
        }
        
        // Check that j indices are the same
        uint32_t j_idx1 = bytecode_get_arg(bc[1]);
        uint32_t j_idx2 = bytecode_get_arg(bc[4]);
        if (j_idx1 != j_idx2) {
            continue;
        }
        
        // Check that BINARY_OP at position 6 is ADD
        if ((bytecode_get_arg(bc[6]) & 0xFF) != 0x00) {
            continue;
        }
        
        // Check that BINARY_OP at position 8 is GT
        if ((bytecode_get_arg(bc[8]) & 0xFF) != 0x54) {
            continue;
        }
        
        // Found the comparison pattern
        *start = i;
        
        // Try to find the end of the swap block (approximately)
        size_t potential_end = i + 10;
        size_t max_search = i + 30;
        if (max_search > code->count) max_search = code->count;
        
        // find a JUMP_BACKWARD after the comparison
        for (size_t j = i + 10; j < max_search; j++) {
            if (code->bytecodes[j].op_code == JUMP_BACKWARD) {
                potential_end = j;
                break;
            }
        }
        
        *end = potential_end;
        return 1;
    }
    
    return 0;
}

// Extract indices from the found pattern
static int extract_pattern_info(bytecode_array* code, size_t pattern_start, PatternInfo* info) {
    if (!code || pattern_start + 9 >= code->count) {
        return 0;
    }
    
    bytecode* bc = &code->bytecodes[pattern_start];
    
    // Extract array index (from first LOAD_FAST)
    info->array_index = bytecode_get_arg(bc[0]);
    
    // Extract j index (from second LOAD_FAST)
    info->j_index = bytecode_get_arg(bc[1]);
    
    // Extract constant 1 index (from LOAD_CONST)
    info->const_one_index = bytecode_get_arg(bc[5]);
    
    // Verify the pattern
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

// Generate optimized bytecode for COMPARE_AND_SWAP
static bytecode_array generate_compare_and_swap_code(PatternInfo* info) {
    bytecode new_instructions[6];
    
    // 1. LOAD_FAST array
    new_instructions[0] = bytecode_create_with_number(LOAD_FAST, info->array_index);
    
    // 2. LOAD_FAST j
    new_instructions[1] = bytecode_create_with_number(LOAD_FAST, info->j_index);
    
    // 3. LOAD_FAST j (for j+1 calculation)
    new_instructions[2] = bytecode_create_with_number(LOAD_FAST, info->j_index);
    
    // 4. LOAD_CONST 1
    new_instructions[3] = bytecode_create_with_number(LOAD_CONST, info->const_one_index);
    
    // 5. BINARY_OP ADD (calculate j+1)
    new_instructions[4] = bytecode_create_with_number(BINARY_OP, 0x000000); // ADD
    
    // 6. COMPARE_AND_SWAP
    new_instructions[5] = bytecode_create_with_number(COMPARE_AND_SWAP, 0x000000);
    
    // Create bytecode array
    bytecode* bc_copy = malloc(6 * sizeof(bytecode));
    if (!bc_copy) {
        bytecode_array empty = {0};
        return empty;
    }
    
    memcpy(bc_copy, new_instructions, 6 * sizeof(bytecode));
    return create_bytecode_array(bc_copy, 6);
}

// Replace pattern with optimized code
static int replace_pattern(bytecode_array* code, size_t pattern_start, size_t pattern_end, 
                          PatternInfo* info) {
    if (!code || pattern_end <= pattern_start || pattern_end >= code->count) {
        return 0;
    }
    
    // Generate optimized code
    bytecode_array optimized = generate_compare_and_swap_code(info);
    if (optimized.count == 0) {
        return 0;
    }
    
    size_t old_size = pattern_end - pattern_start + 1;
    size_t new_size = optimized.count;
    
    DPRINT("[JIT] Replacing %zu instructions with %zu optimized instructions\n", 
           old_size, new_size);
    
    // Create new bytecode array
    size_t new_total = code->count - old_size + new_size;
    bytecode* new_bytecodes = malloc(new_total * sizeof(bytecode));
    if (!new_bytecodes) {
        free_bytecode_array(optimized);
        return 0;
    }
    
    // Copy part before pattern
    memcpy(new_bytecodes, code->bytecodes, pattern_start * sizeof(bytecode));
    
    // Insert optimized code
    memcpy(&new_bytecodes[pattern_start], optimized.bytecodes, new_size * sizeof(bytecode));
    
    // Copy part after pattern
    size_t after_pattern = pattern_end + 1;
    size_t remaining = code->count - after_pattern;
    memcpy(&new_bytecodes[pattern_start + new_size], 
           &code->bytecodes[after_pattern], 
           remaining * sizeof(bytecode));
    
    // Replace old array
    free(code->bytecodes);
    code->bytecodes = new_bytecodes;
    code->count = new_total;
    code->capacity = new_total;
    
    free_bytecode_array(optimized);
    return 1;
}

// Public functions
int is_sorting_function(void* code_ptr) {
    CodeObj* code = (CodeObj*)code_ptr;
    if (!code || !code->code.bytecodes) {
        return 0;
    }
    
    size_t start, end;
    return find_sorting_pattern_in_bytecode(&code->code, &start, &end);
}

void* jit_optimize_compare_and_swap(void* jit_ptr, void* original_ptr) {
    JIT* jit = (JIT*)jit_ptr;
    CodeObj* original = (CodeObj*)original_ptr;
    
    if (!jit || !original) {
        return NULL;
    }
    
    DPRINT("[JIT] Analyzing function: %s\n", 
           original->name ? original->name : "anonymous");
    
    // Find sorting pattern
    size_t pattern_start, pattern_end;
    if (!find_sorting_pattern_in_bytecode(&original->code, &pattern_start, &pattern_end)) {
        DPRINT("[JIT] No sorting pattern found\n");
        return NULL;
    }
    
    DPRINT("[JIT] Found sorting pattern at [%zu-%zu]\n", pattern_start, pattern_end);
    
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
