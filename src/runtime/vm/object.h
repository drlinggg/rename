#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../../compiler/value.h"
#include "float_bigint.h"

typedef struct VM VM;
typedef struct Object Object;
typedef struct CodeObj CodeObj;

typedef Object* (*NativeCFunc)(VM* heap, int arg_count, Object** args);

typedef enum {
    OBJ_INT,
    OBJ_BOOL,
    OBJ_NONE,
    OBJ_CODE,
    OBJ_FUNCTION,
    OBJ_ARRAY,
    OBJ_NATIVE_FUNCTION,
    OBJ_FLOAT,
} ObjectType;

struct Object {
    ObjectType type;
    uint32_t ref_count;
    union {
        int64_t int_value;

        bool bool_value;

        BigFloat* float_value;

        CodeObj* codeptr;
        
        struct {
            CodeObj* codeptr;
        } function;

        struct {
            Object** items;
            size_t size;
        } array;

        struct {
            NativeCFunc c_func;
            const char* name;
        } native_function;
    } as;
};

Object* object_new_int(int64_t v);
Object* object_new_float(const char* v);
Object* object_new_float_from_bf(BigFloat* bf);
Object* object_new_bool(bool v);
Object* object_new_none(void);
Object* object_new_code(CodeObj* code);
Object* object_new_function(CodeObj* code);

Object* object_new_array(void);
Object* object_new_array_with_size(size_t initial_size);
void object_array_append(Object* array, Object* element);
Object* object_array_get(Object* array, size_t index);
void object_array_set(Object* array, size_t index, Object* element);
void object_array_free(Object* array);

void object_incref(Object* o);
void object_decref(Object* o);

bool object_is_truthy(Object* o);
char* object_to_string(Object* o);
