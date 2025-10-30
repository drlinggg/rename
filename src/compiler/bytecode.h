#pragma once

typedef enum op_code {
    // 1. loads
    LOAD_FAST = 0x01,
    LOAD_CONST = 0x02,
    LOAD_GLOBAL = 0x03,
    LOAD_NAME = 0x04,
    
    // 2. stores
    STORE_FAST = 0x05,
    STORE_GLOBAL = 0x06,
    STORE_NAME = 0x07,
    
    // 3. binarys & compares
    BINARY_OP = 0x08,
    COMPARE_OP = 0x18,
    // 3. binarys & compares
    //==========================================================
    //// binary arg map:
    //   op_map = {
    //        0x00: '+',    # BINARY_OP_ADD
    //        0x01: '&',    # BINARY_OP_AND
    //        0x02: '//',   # BINARY_OP_FLOOR_DIVIDE
    //        0x03: '<<',   # BINARY_OP_LSHIFT
    //        0x04: '@',    # BINARY_OP_MATRIX_MULTIPLY
    //        0x05: '*',    # BINARY_OP_MULTIPLY
    //        0x06: '%',    # BINARY_OP_REMAINDER
    //        0x07: '|',    # BINARY_OP_OR
    //        0x08: '**',   # BINARY_OP_POWER
    //        0x09: '>>',   # BINARY_OP_RSHIFT
    //        0x0A: '-',   # BINARY_OP_SUBTRACT
    //        0x0B: '/',   # BINARY_OP_TRUE_DIVIDE
    //        0x0C: '^',   # BINARY_OP_XOR
    //        # Inplace operations
    //        0x0D: '+',   # BINARY_OP_INPLACE_ADD
    //        0x0E: '&',   # BINARY_OP_INPLACE_AND
    //        0x0F: '//',  # BINARY_OP_INPLACE_FLOOR_DIVIDE
    //        0x10: '<<',  # BINARY_OP_INPLACE_LSHIFT
    //        0x11: '@',   # BINARY_OP_INPLACE_MATRIX_MULTIPLY
    //        0x12: '*',   # BINARY_OP_INPLACE_MULTIPLY
    //        0x13: '%',   # BINARY_OP_INPLACE_REMAINDER
    //        0x14: '|',   # BINARY_OP_INPLACE_OR
    //        0x15: '**',  # BINARY_OP_INPLACE_POWER
    //        0x16: '>>',  # BINARY_OP_INPLACE_RSHIFT
    //        0x17: '-',   # BINARY_OP_INPLACE_SUBTRACT
    //        0x18: '/',   # BINARY_OP_INPLACE_TRUE_DIVIDE
    //        0x19: '^',   # BINARY_OP_INPLACE_XOR
    //    }
    //==========================================================
    //// compare arg map:
    //   cmp_map = {
    //        0x00: '<',    # COMPARE_LT
    //        0x01: '<=',   # COMPARE_LE
    //        0x02: '==',   # COMPARE_EQ
    //        0x03: '!=',   # COMPARE_NE
    //        0x04: '>',    # COMPARE_GT
    //        0x05: '>=',   # COMPARE_GE
    //        0x06: 'in',   # COMPARE_IN
    //        0x07: 'not in', # COMPARE_NOT_IN
    //    }
    //==========================================================
    
    // to_smth
    to_bool = 0x0A,
    to_int = 0x0B,
    to_long = 0x0C,
    
    // array, indexing
    build_array = 0x17,
    store_subscr = 0x0D,
    del_subscr = 0x0E,
    
    // functions
    call_function = 0x09,
    return_value = 0x0F,
    
    // condition flow
    END_FOR = 0x12,
    JUMP_FORWARD = 0x19,
    JUMP_BACKWARD = 0x1A,
    JUMP_BACKWARD_NO_INTERRUPT = 0x1B,
    POP_JUMP_IF_TRUE = 0x1C,
    POP_JUMP_IF_NONE = 0x1F,
    POP_JUMP_IF_FALSE = 0x1D,
    POP_JUMP_IF_NOT_NONE = 0x1E,
    
    // special
    NOP = 0x10,
    POP_TOP = 0x11,
    COPY = 0x13,
    SWAP = 0x14,
    PUSH_NULL = 0x20,
    
    // unary
    UNARY_NEGATIVE = 0x15,
    UNARY_NOT = 0x16

} op_code;

typedef struct bytecode {
    op_code code;
    uint8_t argument[3];
} bytecode __attribute__((packed));
