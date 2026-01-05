#include "../system.h"
#include "bytecode.h"
#include "value.h"
#include <stdlib.h>

bool values_equal(Value a, Value b) {
    if (a.type != b.type) {
        return false;
    }
    
    switch (a.type) {
        case VAL_INT:
            return a.int_val == b.int_val;
        case VAL_BOOL:
            return a.bool_val == b.bool_val;
        case VAL_FLOAT:
            return strcmp(a.float_val, b.float_val) == 0;
        case VAL_NONE:
            return true;
        default:
            return false;
    }
}

Value value_create_int(int64_t value) {
    Value val;
    val.type = VAL_INT;
    val.int_val = value;
    return val;
}

Value value_create_bool(bool value) {
    Value val;
    val.type = VAL_BOOL;
    val.bool_val = value;
    return val;
}

Value value_create_float(const char* value) {
    Value val;
    val.type = VAL_FLOAT;
    val.float_val = strdup(value);
    return val;
}

Value value_create_none() {
    Value val;
    val.type = VAL_NONE;
    return val;
}

Value value_create_code(CodeObj* code_obj) {
    Value value;
    value.type = VAL_CODE;
    value.code_val = code_obj;
    return value;
}

void value_free(Value value) {
    switch (value.type) {
        case VAL_CODE:
            free_code_obj(value.code_val);
            break;
        case VAL_FLOAT:
            free(value.float_val);
            break;
        default:
            break;
    }
}

void free_code_obj(CodeObj* code) {
    if (!code) return;
    
    DPRINT("[MEM] Freeing CodeObj '%s'\n", code->name ? code->name : "anonymous");
    
    if (code->name) {
        free(code->name);
    }
    if (code->constants) {
        free(code->constants);
    }
    if (code->code.bytecodes) {
        free(code->code.bytecodes);
    }
    free(code);
}
