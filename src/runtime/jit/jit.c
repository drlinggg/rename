#include "jit.h"
#include <stdlib.h>
#include <stdio.h>

struct JIT {
    // placeholder for JIT internal state
    size_t compiled_count;
};

JIT* jit_create(void) {
    JIT* j = malloc(sizeof(JIT));
    if (!j) return NULL;
    j->compiled_count = 0;
    return j;
}

void jit_destroy(JIT* jit) {
    if (!jit) return;
    free(jit);
}

CodeObj* jit_compile_function(JIT* jit, CodeObj* code) {
    // int a = 1 + 2 + 3;
    // Bytecode array (6 instructions):
    //   0: [ LOAD_CONST                0x02 | arg: 0x000000 (0) | const_index: 0 ]
    //  1: [ LOAD_CONST                0x02 | arg: 0x000001 (1) | const_index: 1 ]
    //   2: [ BINARY_OP                 0x08 | arg: 0x000000 (0) | ADD ]
    //   3: [ LOAD_CONST                0x02 | arg: 0x000002 (2) | const_index: 2 ]
    //   4: [ BINARY_OP                 0x08 | arg: 0x000000 (0) | ADD ]    
    // Необходимо находить подобную ситуацию и сжимать байткод в [ LOAD_CONST 0x02 | arg: 0x000003 (3) | const_index 3 ] т.е. высчитать константу и добавить ее в пул констант (см. add_constant метод)
    //
    // Аналогично для COMPARE_OP UNARY_OP TO_BOOL TO_INT

    (void)jit;
    (void)code;
    // placeholder - returns NULL meaning no compiled representation available
    // fprintf(stderr, "[JIT] compile called but not implemented\n");
    return NULL;
}
