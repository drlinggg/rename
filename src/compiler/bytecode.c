#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "../system.h"
#include "bytecode.h"

static inline void bytecode_set_arg(bytecode* bc, uint32_t arg) {
    bc->argument[0] = (arg >> 16) & 0xFF;
    bc->argument[1] = (arg >> 8) & 0xFF;
    bc->argument[2] = arg & 0xFF;
}

static const char* bytecode_opcode_to_string(uint8_t op_code) {
    switch (op_code) {
        case LOAD_FAST: return "LOAD_FAST";
        case LOAD_CONST: return "LOAD_CONST";
        case LOAD_GLOBAL: return "LOAD_GLOBAL";
        case LOAD_NAME: return "LOAD_NAME";
        case STORE_FAST: return "STORE_FAST";
        case STORE_GLOBAL: return "STORE_GLOBAL";
        case STORE_NAME: return "STORE_NAME";
        case BINARY_OP: return "BINARY_OP";
        case UNARY_OP: return "UNARY_OP";
        case CALL_FUNCTION: return "CALL_FUNCTION";
        case TO_BOOL: return "TO_BOOL";
        case TO_INT: return "TO_INT";
        case TO_LONG: return "TO_LONG";
        case STORE_SUBSCR: return "STORE_SUBSCR";
        case LOAD_SUBSCR: return "LOAD_SUBSCR";
        case DEL_SUBSCR: return "DEL_SUBSCR";
        case RETURN_VALUE: return "RETURN_VALUE";
        case NOP: return "NOP";
        case POP_TOP: return "POP_TOP";
        case COPY: return "COPY";
        case SWAP: return "SWAP";
        case JUMP_FORWARD: return "JUMP_FORWARD";
        case JUMP_BACKWARD: return "JUMP_BACKWARD";
        case JUMP_BACKWARD_NO_INTERRUPT: return "JUMP_BACKWARD_NO_INTERRUPT";
        case POP_JUMP_IF_TRUE: return "POP_JUMP_IF_TRUE";
        case POP_JUMP_IF_FALSE: return "POP_JUMP_IF_FALSE";
        case POP_JUMP_IF_NOT_NONE: return "POP_JUMP_IF_NOT_NONE";
        case POP_JUMP_IF_NONE: return "POP_JUMP_IF_NONE";
        case PUSH_NULL: return "PUSH_NULL";
        case MAKE_FUNCTION: return "MAKE_FUNCTION";
        case BREAK_LOOP: return "BREAK_LOOP";
        case CONTINUE_LOOP: return "CONTINUE_LOOP";
        case LOOP_START: return "LOOP_START";
        case LOOP_END: return "LOOP_END";
        case BUILD_ARRAY: return "BUILD_ARRAY";
        default: return "UNKNOWN";
    }
}

static const char* binary_op_to_string(uint8_t op_arg) {
    switch (op_arg) {
        case 0x00: return "ADD";
        case 0x01: return "AND";
        case 0x02: return "FLOOR_DIVIDE";
        case 0x03: return "LSHIFT";
        case 0x04: return "MATRIX_MULTIPLY";
        case 0x05: return "MULTIPLY";
        case 0x06: return "REMAINDER";
        case 0x07: return "OR";
        case 0x08: return "POWER";
        case 0x09: return "RSHIFT";
        case 0x0A: return "SUBTRACT";
        case 0x0B: return "TRUE_DIVIDE";
        case 0x0C: return "XOR";
        case 0x0D: return "INPLACE_ADD";
        case 0x0E: return "INPLACE_AND";
        case 0x0F: return "INPLACE_FLOOR_DIVIDE";
        case 0x10: return "INPLACE_LSHIFT";
        case 0x11: return "INPLACE_MATRIX_MULTIPLY";
        case 0x12: return "INPLACE_MULTIPLY";
        case 0x13: return "INPLACE_REMAINDER";
        case 0x14: return "INPLACE_OR";
        case 0x15: return "INPLACE_POWER";
        case 0x16: return "INPLACE_RSHIFT";
        case 0x17: return "INPLACE_SUBTRACT";
        case 0x18: return "INPLACE_TRUE_DIVIDE";
        case 0x19: return "INPLACE_XOR";
        default: return "UNKNOWN_BINARY_OP";
    }
}

static const char* unary_op_to_string(uint8_t op_arg) {
    switch (op_arg) {
        case 0x00: return "POSITIVE";
        case 0x0A: return "NEGATIVE";
        case 0x05: return "NOT";
        default: return "UNKNOWN_UNARY_OP";
    }
}

void bytecode_print(bytecode* bc) {
    if (!bc) {
        DPRINT("[NULL bytecode]\n");
        return;
    }
    
    const char* op_name = bytecode_opcode_to_string(bc->op_code);
    uint32_t arg = bytecode_get_arg(*bc);
    
    DPRINT("[ %-25s 0x%02X | arg: 0x%06X (%u) ", 
           op_name, bc->op_code, arg, arg);
    
    switch (bc->op_code) {
        case BINARY_OP:
            DPRINT("| %s ", binary_op_to_string(arg & 0xFF));
            break;
        case UNARY_OP:
            DPRINT("| %s ", unary_op_to_string(arg & 0xFF));
            break;
        case LOAD_CONST:
            DPRINT("| const_index: %u ", arg);
            break;
        case LOAD_FAST:
            DPRINT("| local_index: %u ", arg);
            break;
        case STORE_FAST:
            DPRINT("| local_index: %u ", arg);
            break;
        case LOAD_GLOBAL:
            DPRINT("| global_index: %u ", arg >> 1);
            if (arg % 2 != 0) {
                DPRINT("| push_null ");
            }
            break;
        case STORE_GLOBAL:
            DPRINT("| global_index: %u ", arg);
            break;
        case CALL_FUNCTION:
            DPRINT("| argc: %u ", arg);
            break;
        case JUMP_FORWARD:
        case JUMP_BACKWARD:
        case JUMP_BACKWARD_NO_INTERRUPT:
        case POP_JUMP_IF_TRUE:
        case POP_JUMP_IF_FALSE:
        case POP_JUMP_IF_NOT_NONE:
        case POP_JUMP_IF_NONE:
            DPRINT("| offset: %u ", arg);
            break;
        case COPY:
        case SWAP:
            DPRINT("| position: %u ", arg);
            break;
        case BREAK_LOOP:
            DPRINT("| break out of loop ");
            break;
        case CONTINUE_LOOP:
            DPRINT("| continue to next iteration ");
            break;
        case LOOP_START:
            DPRINT("| start of loop block ");
            break;
        case LOOP_END:
            DPRINT("| end of loop block ");
        case BUILD_ARRAY:
            DPRINT("| element_count: %u ", arg);
            break;
        case LOAD_SUBSCR:
        case STORE_SUBSCR:
        case DEL_SUBSCR:
            DPRINT("| no additional info", arg);
            break;
    }
    
    DPRINT("]\n");
}

void bytecode_array_print(bytecode_array* bc_array) {
    if (!bc_array) {
        DPRINT("NULL bytecode array\n");
        return;
    }
    
    if (!bc_array->bytecodes || bc_array->count == 0) {
        DPRINT("Empty bytecode array\n");
        return;
    }
    
    DPRINT("Bytecode array (%u instructions):\n", bc_array->count);
    for (uint32_t i = 0; i < bc_array->count; i++) {
        DPRINT("%4u: ", i);
        bytecode_print(&bc_array->bytecodes[i]);
    }
}

uint32_t bytecode_get_arg(const bytecode bc) {
    return (bc.argument[0] << 16) | (bc.argument[1] << 8) | bc.argument[2];
}

bytecode bytecode_create(uint8_t op_code, uint8_t argument1, uint8_t argument2, uint8_t argument3) {
    if (op_code == 0){
        op_code = NOP;
    }
    if (argument1 == 0) {
        argument1 = 0x00;
    }
    if (argument2 == 0) {
        argument2 = 0x00;
    }    
    if (argument3 == 0) {
        argument3 = 0x00;
    }    

    bytecode a = {
        .op_code = op_code,
        .argument = {argument1, argument2, argument3}
    };
    return a;
}

bytecode bytecode_create_from_array(uint8_t op_code, const uint8_t argument[3]) {
    if (argument != NULL) {
        return bytecode_create(op_code, argument[0], argument[1], argument[2]);
    }
    return bytecode_create(op_code, 0x00, 0x00, 0x00);
}

bytecode bytecode_create_with_number(uint8_t op_code, uint32_t number) {
    bytecode bc;
    bc.op_code = op_code;
    bytecode_set_arg(&bc, number);
    return bc;
}

bytecode_array create_bytecode_array(bytecode* bytecode, uint32_t count) {
    return (bytecode_array) {bytecode, count};
}

void free_bytecode_array(bytecode_array array) {
    if (array.bytecodes) {
        free(array.bytecodes);
    }
}

// Эта функция может быть полезной для отладки
static uint8_t* bytecode_to_byte_array(const bytecode_array* bc_array, size_t* byte_count) {
    if (!bc_array || !bc_array->bytecodes || bc_array->count == 0) {
        *byte_count = 0;
        return NULL;
    }
    
    *byte_count = bc_array->count * sizeof(bytecode);
    uint8_t* byte_array = malloc(*byte_count);
    if (!byte_array) return NULL;
    
    memcpy(byte_array, bc_array->bytecodes, *byte_count);
    return byte_array;
}

// И обратная функция
static bytecode_array byte_array_to_bytecode(const uint8_t* byte_array, size_t byte_count) {
    if (!byte_array || byte_count % sizeof(bytecode) != 0) {
        return create_bytecode_array(NULL, 0);
    }
    
    size_t bc_count = byte_count / sizeof(bytecode);
    bytecode* bc_array = malloc(bc_count * sizeof(bytecode));
    if (!bc_array) return create_bytecode_array(NULL, 0);
    
    memcpy(bc_array, byte_array, byte_count);
    return create_bytecode_array(bc_array, bc_count);
}
