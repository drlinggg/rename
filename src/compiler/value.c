#include "value.h"

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

Value value_create_null() {
    Value val;
    val.type = VAL_NONE;
    return val;
}
