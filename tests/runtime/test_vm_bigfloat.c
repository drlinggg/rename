#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../../src/system.h"
#include "../../src/runtime/vm/vm.h"
#include "../../src/runtime/vm/heap.h"
#include "../../src/compiler/bytecode.h"
#include "../../src/compiler/value.h"
#include "../../src/builtins/builtins.h"

static void test_float_operations() {
    printf("=== Testing Float Operations ===\n");
    
    // Test 1: 1.5 + 2.5 = 4.0
    {
        Value* consts = malloc(2 * sizeof(Value));
        consts[0] = value_create_float("1.5");
        consts[1] = value_create_float("2.5");
        
        bytecode* bcs = malloc(4 * sizeof(bytecode));
        bcs[0] = bytecode_create_with_number(LOAD_CONST, 0);
        bcs[1] = bytecode_create_with_number(LOAD_CONST, 1);
        bcs[2] = bytecode_create_with_number(BINARY_OP, 0x00); // ADD
        bcs[3] = bytecode_create_with_number(RETURN_VALUE, 0);
        
        bytecode_array arr = create_bytecode_array(bcs, 4);
        
        CodeObj* code_obj = malloc(sizeof(CodeObj));
        code_obj->code = arr;
        code_obj->name = strdup("test_float_add");
        code_obj->arg_count = 0;
        code_obj->local_count = 0;
        code_obj->constants = consts;
        code_obj->constants_count = 2;
        
        Heap* heap = heap_create();
        VM* vm = vm_create(heap, 0);
        
        Object* ret = vm_execute(vm, code_obj);
        assert(ret != NULL);
        assert(ret->type == OBJ_FLOAT);
        
        char* result_str = object_to_string(ret);
        printf("1.5 + 2.5 = %s ✓\n", result_str);
        free(result_str);
        
        object_decref(ret);
        vm_destroy(vm);
        //heap_destroy((heap);
        
        //free(code_obj->name);
        //free(code_obj->constants);
        //free(code_obj->code.bytecodes);
        //free(code_obj);
        //free(bcs);

    }
    
    // Test 2: 5.0 - 2.5 = 2.5
    {
        Value* consts = malloc(2 * sizeof(Value));
        consts[0] = value_create_float("5.0");
        consts[1] = value_create_float("2.5");
        
        bytecode* bcs = malloc(4 * sizeof(bytecode));
        bcs[0] = bytecode_create_with_number(LOAD_CONST, 0);
        bcs[1] = bytecode_create_with_number(LOAD_CONST, 1);
        bcs[2] = bytecode_create_with_number(BINARY_OP, 0x0A); // SUB
        bcs[3] = bytecode_create_with_number(RETURN_VALUE, 0);
        
        bytecode_array arr = create_bytecode_array(bcs, 4);
        
        CodeObj* code_obj = malloc(sizeof(CodeObj));
        code_obj->code = arr;
        code_obj->name = strdup("test_float_sub");
        code_obj->arg_count = 0;
        code_obj->local_count = 0;
        code_obj->constants = consts;
        code_obj->constants_count = 2;
        
        Heap* heap = heap_create();
        VM* vm = vm_create(heap, 0);
        
        Object* ret = vm_execute(vm, code_obj);
        assert(ret != NULL);
        assert(ret->type == OBJ_FLOAT);
        
        char* result_str = object_to_string(ret);
        printf("5.0 - 2.5 = %s ✓\n", result_str);
        free(result_str);
        
        object_decref(ret);
        vm_destroy(vm);
        //heap_destroy((heap);
        
        //free(code_obj->name);
        //free(code_obj->constants);
        //free(code_obj->code.bytecodes);
        //free(code_obj);
        //free(bcs);
    }
    
    // Test 3: 2.5 * 3.0 = 7.5
    {
        Value* consts = malloc(2 * sizeof(Value));
        consts[0] = value_create_float("2.5");
        consts[1] = value_create_float("3.0");
        
        bytecode* bcs = malloc(4 * sizeof(bytecode));
        bcs[0] = bytecode_create_with_number(LOAD_CONST, 0);
        bcs[1] = bytecode_create_with_number(LOAD_CONST, 1);
        bcs[2] = bytecode_create_with_number(BINARY_OP, 0x05); // MUL
        bcs[3] = bytecode_create_with_number(RETURN_VALUE, 0);
        
        bytecode_array arr = create_bytecode_array(bcs, 4);
        
        CodeObj* code_obj = malloc(sizeof(CodeObj));
        code_obj->code = arr;
        code_obj->name = strdup("test_float_mul");
        code_obj->arg_count = 0;
        code_obj->local_count = 0;
        code_obj->constants = consts;
        code_obj->constants_count = 2;
        
        Heap* heap = heap_create();
        VM* vm = vm_create(heap, 0);
        
        Object* ret = vm_execute(vm, code_obj);
        assert(ret != NULL);
        assert(ret->type == OBJ_FLOAT);
        
        char* result_str = object_to_string(ret);
        printf("2.5 * 3.0 = %s ✓\n", result_str);
        free(result_str);
        
        object_decref(ret);
        vm_destroy(vm);
        //heap_destroy((heap);
        
        //free(code_obj->name);
        //free(code_obj->constants);
        //free(code_obj->code.bytecodes);
        //free(code_obj);
        //free(bcs);
    }
    
    // Test 4: 10.0 / 4.0 = 2.5
    {
        Value* consts = malloc(2 * sizeof(Value));
        consts[0] = value_create_float("10.0");
        consts[1] = value_create_float("4.0");
        
        bytecode* bcs = malloc(4 * sizeof(bytecode));
        bcs[0] = bytecode_create_with_number(LOAD_CONST, 0);
        bcs[1] = bytecode_create_with_number(LOAD_CONST, 1);
        bcs[2] = bytecode_create_with_number(BINARY_OP, 0x0B); // DIV
        bcs[3] = bytecode_create_with_number(RETURN_VALUE, 0);
        
        bytecode_array arr = create_bytecode_array(bcs, 4);
        
        CodeObj* code_obj = malloc(sizeof(CodeObj));
        code_obj->code = arr;
        code_obj->name = strdup("test_float_div");
        code_obj->arg_count = 0;
        code_obj->local_count = 0;
        code_obj->constants = consts;
        code_obj->constants_count = 2;
        
        Heap* heap = heap_create();
        VM* vm = vm_create(heap, 0);
        
        Object* ret = vm_execute(vm, code_obj);
        assert(ret != NULL);
        assert(ret->type == OBJ_FLOAT);
        
        char* result_str = object_to_string(ret);
        printf("10.0 / 4.0 = %s ✓\n", result_str);
        free(result_str);
        
        object_decref(ret);
        vm_destroy(vm);
        //heap_destroy((heap);
        
        //free(code_obj->name);
        //free(code_obj->constants);
        //free(code_obj->code.bytecodes);
        //free(code_obj);
        //free(bcs);
    }
    
    // Test 5: -3.14
    {
        Value* consts = malloc(1 * sizeof(Value));
        consts[0] = value_create_float("3.14");
        
        bytecode* bcs = malloc(3 * sizeof(bytecode));
        bcs[0] = bytecode_create_with_number(LOAD_CONST, 0);
        bcs[1] = bytecode_create_with_number(UNARY_OP, 0x01); // UNARY_MINUS
        bcs[2] = bytecode_create_with_number(RETURN_VALUE, 0);
        
        bytecode_array arr = create_bytecode_array(bcs, 3);
        
        CodeObj* code_obj = malloc(sizeof(CodeObj));
        code_obj->code = arr;
        code_obj->name = strdup("test_float_neg");
        code_obj->arg_count = 0;
        code_obj->local_count = 0;
        code_obj->constants = consts;
        code_obj->constants_count = 1;
        
        Heap* heap = heap_create();
        VM* vm = vm_create(heap, 0);
        
        Object* ret = vm_execute(vm, code_obj);
        assert(ret != NULL);
        assert(ret->type == OBJ_FLOAT);
        
        char* result_str = object_to_string(ret);
        printf("-3.14 = %s ✓\n", result_str);
        free(result_str);
        
        object_decref(ret);
        vm_destroy(vm);
        //heap_destroy((heap);
        
        free(code_obj->name);
        free(code_obj->constants);
        free(code_obj->code.bytecodes);
        free(code_obj);
        // bytecodes are owned by code_obj->code.bytecodes and already freed
        //free(bcs);
    }
    
    printf("Float operations: ALL TESTS PASSED ✓\n\n");
}

static void test_mixed_operations() {
    printf("=== Testing Mixed Int/Float Operations ===\n");
    
    // Test 1: int + float
    {
        Value* consts = malloc(2 * sizeof(Value));
        consts[0] = value_create_int(5);
        consts[1] = value_create_float("2.5");
        
        bytecode* bcs = malloc(4 * sizeof(bytecode));
        bcs[0] = bytecode_create_with_number(LOAD_CONST, 0);
        bcs[1] = bytecode_create_with_number(LOAD_CONST, 1);
        bcs[2] = bytecode_create_with_number(BINARY_OP, 0x00); // ADD
        bcs[3] = bytecode_create_with_number(RETURN_VALUE, 0);
        
        bytecode_array arr = create_bytecode_array(bcs, 4);
        
        CodeObj* code_obj = malloc(sizeof(CodeObj));
        code_obj->code = arr;
        code_obj->name = strdup("test_mixed_add");
        code_obj->arg_count = 0;
        code_obj->local_count = 0;
        code_obj->constants = consts;
        code_obj->constants_count = 2;
        
        Heap* heap = heap_create();
        VM* vm = vm_create(heap, 0);
        
        Object* ret = vm_execute(vm, code_obj);
        assert(ret != NULL);
        assert(ret->type == OBJ_FLOAT); // Result should be float
        
        char* result_str = object_to_string(ret);
        printf("5 + 2.5 = %s ✓\n", result_str);
        free(result_str);
        
        object_decref(ret);
        vm_destroy(vm);
        //heap_destroy((heap);
        
        free(code_obj->name);
        free(code_obj->constants);
        free(code_obj->code.bytecodes);
        free(code_obj);
        // bytecodes are owned by code_obj->code.bytecodes and already freed
        //free(bcs);
    }
    
    // Test 2: float == float
    {
        Value* consts = malloc(2 * sizeof(Value));
        consts[0] = value_create_float("1.5");
        consts[1] = value_create_float("1.5");
        
        bytecode* bcs = malloc(4 * sizeof(bytecode));
        bcs[0] = bytecode_create_with_number(LOAD_CONST, 0);
        bcs[1] = bytecode_create_with_number(LOAD_CONST, 1);
        bcs[2] = bytecode_create_with_number(BINARY_OP, 0x50); // EQ
        bcs[3] = bytecode_create_with_number(RETURN_VALUE, 0);
        
        bytecode_array arr = create_bytecode_array(bcs, 4);
        
        CodeObj* code_obj = malloc(sizeof(CodeObj));
        code_obj->code = arr;
        code_obj->name = strdup("test_float_eq");
        code_obj->arg_count = 0;
        code_obj->local_count = 0;
        code_obj->constants = consts;
        code_obj->constants_count = 2;
        
        Heap* heap = heap_create();
        VM* vm = vm_create(heap, 0);
        
        Object* ret = vm_execute(vm, code_obj);
        assert(ret != NULL);
        assert(ret->type == OBJ_BOOL);
        assert(ret->as.bool_value == true);
        printf("1.5 == 1.5 = true ✓\n");
        
        object_decref(ret);
        vm_destroy(vm);
        //heap_destroy((heap);
        
        free(code_obj->name);
        free(code_obj->constants);
        free(code_obj->code.bytecodes);
        free(code_obj);
        // bytecodes are owned by code_obj->code.bytecodes and already freed
        //free(bcs);
    }
    
    // Test 3: 2.0 < 3.0
    {
        Value* consts = malloc(2 * sizeof(Value));
        consts[0] = value_create_float("2.0");
        consts[1] = value_create_float("3.0");
        
        bytecode* bcs = malloc(4 * sizeof(bytecode));
        bcs[0] = bytecode_create_with_number(LOAD_CONST, 0);
        bcs[1] = bytecode_create_with_number(LOAD_CONST, 1);
        bcs[2] = bytecode_create_with_number(BINARY_OP, 0x52); // LT
        bcs[3] = bytecode_create_with_number(RETURN_VALUE, 0);
        
        bytecode_array arr = create_bytecode_array(bcs, 4);
        
        CodeObj* code_obj = malloc(sizeof(CodeObj));
        code_obj->code = arr;
        code_obj->name = strdup("test_float_lt");
        code_obj->arg_count = 0;
        code_obj->local_count = 0;
        code_obj->constants = consts;
        code_obj->constants_count = 2;
        
        Heap* heap = heap_create();
        VM* vm = vm_create(heap, 0);
        
        Object* ret = vm_execute(vm, code_obj);
        assert(ret != NULL);
        assert(ret->type == OBJ_BOOL);
        assert(ret->as.bool_value == true);
        printf("2.0 < 3.0 = true ✓\n");
        
        object_decref(ret);
        vm_destroy(vm);
        //heap_destroy((heap);
        
        //free(code_obj->name);
        //free(code_obj->constants);
        //free(code_obj->code.bytecodes);
        //free(code_obj);
        //free(bcs);
    }
    
    printf("Mixed operations: ALL TESTS PASSED ✓\n\n");
}

int main() {
    debug_enabled = 0; // Disable debug for cleaner output
    
    printf("=== Starting VM Float Tests ===\n\n");
    
    test_float_operations();
    test_mixed_operations();
    
    printf("=== ALL VM FLOAT TESTS PASSED ✓ ===\n");
    return 0;
}
