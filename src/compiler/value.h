#include <stdint.h>
#include <stdbool.h>
#pragma once

typedef enum {
    VAL_INT,
    VAL_BOOL,
    VAL_NONE,
} ValueType;

typedef struct {
    ValueType type;
    union {
        int64_t int_val;
        bool bool_val;
    };
} Value;

bool values_equal(Value a, Value b);
Value value_create_int(int64_t value);
Value value_create_bool(bool value);
Value value_create_null();
