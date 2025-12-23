#include "../../src/compiler/bytecode.h"
#include "../../src/system.h"
#include <stdio.h>
#include <assert.h>

void test_bytecode_size() {
    bytecode bc = {0};
    size_t size = sizeof(bc);
    printf("Size: %zu\n", size);
    assert(size == 4);
}

void test_bytecode_create() {
    bytecode bc = bytecode_create(LOAD_CONST, 0x12, 0x34, 0x56);
    assert(bc.op_code == LOAD_CONST);
    assert(bc.argument[0] == 0x12);
    assert(bc.argument[1] == 0x34);
    assert(bc.argument[2] == 0x56);
}

void test_bytecode_create_from_array() {
    uint8_t args[] = {0xAB, 0xCD, 0xEF};
    bytecode bc = bytecode_create_from_array(STORE_FAST, args);
    assert(bc.op_code == STORE_FAST);
    assert(bc.argument[0] == 0xAB);
    assert(bc.argument[1] == 0xCD);
    assert(bc.argument[2] == 0xEF);
}

void test_bytecode_create_with_number() {
    bytecode bc = bytecode_create_with_number(JUMP_FORWARD, 0x123456);
    assert(bc.op_code == JUMP_FORWARD);
    assert(bc.argument[0] == 0x12);
    assert(bc.argument[1] == 0x34);
    assert(bc.argument[2] == 0x56);
}

void test_bytecode_get_arg() {
    bytecode bc = bytecode_create(BINARY_OP, 0x00, 0x2D, 0xFA);
    uint32_t arg = bytecode_get_arg(bc);
    assert(arg == 11770);
}

void test_all_opcodes() {
    bytecode test_cases[] = {
        bytecode_create(LOAD_FAST, 0x01, 0x02, 0x03),
        bytecode_create(STORE_GLOBAL, 0xFF, 0xFE, 0xFD),
        bytecode_create(CALL_FUNCTION, 0x00, 0x00, 0x01),
        bytecode_create(RETURN_VALUE, 0x00, 0x00, 0x00),
        bytecode_create(NOP, 0x11, 0x22, 0x33)
    };
    
    uint8_t expected_opcodes[] = {LOAD_FAST, STORE_GLOBAL, CALL_FUNCTION, RETURN_VALUE, NOP};
    
    for (int i = 0; i < 5; i++) {
        assert(test_cases[i].op_code == expected_opcodes[i]);
    }
}

// gcc tests/bytecode/test_bytecode.c src/compiler/bytecode.c
int main() {
    debug_enabled = 1;
    test_bytecode_size();
    test_bytecode_create();
    test_bytecode_create_from_array();
    test_bytecode_create_with_number();
    test_bytecode_get_arg();
    test_all_opcodes();
    
    printf("All tests passed\n");
    return 0;
}
