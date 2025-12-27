#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "bytecode.h"

typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_BOOL,
    VAL_NONE,
    VAL_CODE,
    VAL_FUNCTION,
} ValueType;

// Forward declaration
typedef struct CodeObj CodeObj;

typedef struct {
    ValueType type;
    union {
        int64_t int_val;
        bool bool_val;
        CodeObj* code_val;
        char* float_val;
    };
} Value;

typedef struct CodeObj {
    bytecode_array code;

    char* name;
    uint8_t arg_count;
    uint8_t local_count;

    Value* constants;
    size_t constants_count;
} CodeObj;

bool values_equal(Value a, Value b);
Value value_create_int(int64_t value);
Value value_create_bool(bool value);
Value value_create_float(const char* value);
Value value_create_none();
Value value_create_code(CodeObj* code_obj);
void value_free(Value value);
void free_code_obj(CodeObj* code);

#endif // VALUE_H
