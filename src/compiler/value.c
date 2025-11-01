#include "value.h"
#include "bytecode.h"
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
            free_bytecode_array(value.code_val->code);
            free(value.code_val->name);
            free(value.code_val->constants);
            free(value.code_val);
            break;
        default:
            break;
    }
}


/*Value value_create_function(CodeObj* code_obj, Value* closures, size_t closures_count) {
    Value value;
    value.type = VAL_FUNCTION;
    value.func_val = malloc(sizeof(FunctionObj));
    
    value.func_val->code = code_obj;
    value.func_val->name = code_obj ? strdup(code_obj->name) : strdup("anonymous");
    
    if (closures && closures_count > 0) {
        value.func_val->closures = malloc(closures_count * sizeof(Value));
        value.func_val->closures_count = closures_count;
        
        for (size_t i = 0; i < closures_count; i++) {
            value.func_val->closures[i] = closures[i];
        }
    } else {
        value.func_val->closures = NULL;
        value.func_val->closures_count = 0;
    }
    
    return value;
}*/
