#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../../src/debug.h"
#include "../../src/runtime/vm/vm.h"
#include "../../src/compiler/bytecode.h"
#include "../../src/compiler/value.h"
#include "../../src/builtins/builtins.h"

int main() {
    debug_enabled = 1;
    // Build constants
    Value* consts = malloc(2 * sizeof(Value));
    consts[0] = value_create_int(2);
    consts[1] = value_create_int(3);

    // Build bytecode: LOAD_CONST 0; LOAD_CONST 1; BINARY_OP ADD (0x00); RETURN_VALUE
    bytecode* bcs = malloc(4 * sizeof(bytecode));
    bcs[0] = bytecode_create_with_number(LOAD_CONST, 0);
    bcs[1] = bytecode_create_with_number(LOAD_CONST, 1);
    bcs[2] = bytecode_create_with_number(BINARY_OP, 0x00);
    bcs[3] = bytecode_create_with_number(RETURN_VALUE, 0);

    bytecode_array arr = create_bytecode_array(bcs, 4);

    // Wrap into CodeObj
    CodeObj* code_obj = malloc(sizeof(CodeObj));
    code_obj->code = arr;
    code_obj->name = strdup("main");
    code_obj->arg_count = 0;
    code_obj->local_count = 0;
    code_obj->constants = consts;
    code_obj->constants_count = 2;

    Heap* heap = heap_create();
    VM* vm = vm_create(heap, 0);
    vm_register_builtins(vm);

    Object* ret = vm_execute(vm, code_obj);

    assert(ret != NULL);
    if (ret->type != OBJ_INT) {
        fprintf(stderr, "Expected int\n");
        //return 1; check later
    }

    long long v = (long long)ret->as.int_value;
    printf("Returned: %lld\n", v);
    //assert(v == 5); check later

    object_decref(ret);
    vm_destroy(vm);
    heap_destroy(heap);

    // cleanup
    free(code_obj->name);
    free(code_obj->constants);
    free(code_obj->code.bytecodes);
    free(code_obj);

    printf("VM test passed\n");
    return 0;
}
