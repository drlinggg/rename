#pragma once

#include <stdint.h>

#define LOAD_FAST 0x01
#define LOAD_CONST 0x02
#define LOAD_GLOBAL 0x03
#define LOAD_NAME 0x04
#define STORE_FAST 0x05
#define STORE_GLOBAL 0x06
#define STORE_NAME 0x07
#define BINARY_OP 0x08
#define COMPARE_OP 0x18
#define TO_BOOL 0x0A
#define TO_INT 0x0B
#define TO_LONG 0x0C
#define BUILD_ARRAY 0x17
#define STORE_SUBSCR 0x0D
#define DEL_SUBSCR 0x0E
#define CALL_FUNCTION 0x09
#define RETURN_VALUE 0x0F
#define END_FOR 0x12
#define JUMP_FORWARD 0x19
#define JUMP_BACKWARD 0x1A
#define JUMP_BACKWARD_NO_INTERRUPT 0x1B
#define POP_JUMP_IF_TRUE 0x1C
#define POP_JUMP_IF_NONE 0x1F
#define POP_JUMP_IF_FALSE 0x1D
#define POP_JUMP_IF_NOT_NONE 0x1E
#define NOP 0x10
#define POP_TOP 0x11
#define COPY 0x13
#define SWAP 0x14
#define PUSH_NULL 0x20
#define UNARY_NEGATIVE 0x15
#define UNARY_NOT 0x16

typedef struct __attribute__((packed, aligned(1))) {
    uint8_t op_code;
    uint8_t argument[3]; // 0x00 0x2D 0xFA values => 002DFA hex => 11770 dec
} bytecode;

bytecode bytecode_create(uint8_t op_code, uint8_t argument1, uint8_t argument2, uint8_t argument3);
bytecode bytecode_create_from_array(uint8_t op_code, const uint8_t argument[3]);
bytecode bytecode_create_with_number(uint8_t op_code, uint32_t number);

uint32_t bytecode_get_arg(const bytecode bc);
