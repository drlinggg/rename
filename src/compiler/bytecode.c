#include "bytecode.h"
#include <stddef.h>

static inline void bytecode_set_arg(bytecode* bc, uint32_t arg) {
    bc->argument[0] = (arg >> 16) & 0xFF;
    bc->argument[1] = (arg >> 8) & 0xFF;
    bc->argument[2] = arg & 0xFF;
}

uint32_t bytecode_get_arg(const bytecode bc) {
    return (bc.argument[0] << 16) | (bc.argument[1] << 8) | bc.argument[2];
}

bytecode bytecode_create(uint8_t op_code, uint8_t argument1, uint8_t argument2, uint8_t argument3) {
    if (op_code == NULL){
        op_code = NOP;
    }
    if (argument1 == NULL) {
        argument1 = 0x00;
    }
    if (argument2 == NULL) {
        argument2 = 0x00;
    }    
    if (argument3 == NULL) {
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
