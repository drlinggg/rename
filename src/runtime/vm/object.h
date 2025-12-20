#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../../compiler/value.h"

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
    OBJ_NATIVE_FUNCTION
} ObjectType;

struct Object {
    ObjectType type;
    uint32_t ref_count;
    union {
        int64_t int_value;

        bool bool_value;

        CodeObj* codeptr; // for OBJ_CODE
        
        struct {
            int64_t executed_times;
            CodeObj* codeptr;
            CodeObj* jit_codeptr;
        } function;

        struct {
            Object** items;
            size_t count;
            size_t capacity;
        } array;

        struct {
            NativeCFunc c_func;
            const char* name;
        } native_function;
    } as;
};

Object* object_new_int(int64_t v);
Object* object_new_bool(bool v);
Object* object_new_none(void);
Object* object_new_code(CodeObj* code);
Object* object_new_function(CodeObj* code);
Object* object_new_array(void);

void object_incref(Object* o);
void object_decref(Object* o);

// Helpers
bool object_is_truthy(Object* o);
char* object_to_string(Object* o);

