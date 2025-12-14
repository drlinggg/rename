#include "object.h"
#include <stdio.h>
#include <string.h>

Object* object_new_int(int64_t v) {
    Object* o = malloc(sizeof(Object));
    o->type = OBJ_INT;
    o->ref_count = 1;
    o->as.int_value = v;
    return o;
}

Object* object_new_bool(bool v) {
    Object* o = malloc(sizeof(Object));
    o->type = OBJ_BOOL;
    o->ref_count = 1;
    o->as.bool_value = v;
    return o;
}

Object* object_new_none(void) {
    Object* o = malloc(sizeof(Object));
    o->type = OBJ_NONE;
    o->ref_count = 1;
    return o;
}

Object* object_new_code(CodeObj* code) {
    Object* o = malloc(sizeof(Object));
    o->type = OBJ_CODE;
    o->ref_count = 1;
    o->as.codeptr = code;
    return o;
}

Object* object_new_function(CodeObj* code) {
    Object* o = malloc(sizeof(Object));
    o->type = OBJ_FUNCTION;
    o->ref_count = 1;
    o->as.function.executed_times = 0;
    o->as.function.codeptr = code;
    o->as.function.jit_codeptr = NULL;
    return o;
}

Object* object_new_array(void) {
    Object* o = malloc(sizeof(Object));
    o->type = OBJ_ARRAY;
    o->ref_count = 1;
    o->as.array.items = NULL;
    o->as.array.count = 0;
    o->as.array.capacity = 0;
    return o;
}

void object_incref(Object* o) {
    if (!o) return;
    // avoid overflow
    if (o->ref_count < UINT32_MAX) {
        o->ref_count++;
    }
}

void object_decref(Object* o) {
    if (!o) return;
    if (o->ref_count == 0) return; // already freed
    o->ref_count--;
    if (o->ref_count == 0) {
        switch (o->type) {
            case OBJ_STRING:
                if (o->as.string.data) free(o->as.string.data);
                break;
            case OBJ_ARRAY:
                if (o->as.array.items) {
                    for (size_t i = 0; i < o->as.array.count; i++) {
                        object_decref(o->as.array.items[i]);
                    }
                    free(o->as.array.items);
                }
                break;
            case OBJ_CODE:
            case OBJ_FUNCTION:
            case OBJ_INT:
            case OBJ_BOOL:
            case OBJ_NONE:
            default:
                break;
        }
        free(o);
    }
}

bool object_is_truthy(Object* o) {
    if (!o) return false;
    switch (o->type) {
        case OBJ_INT:
            return o->as.int_value != 0;
        case OBJ_BOOL:
            return o->as.bool_value;
        case OBJ_NONE:
            return false;
        case OBJ_ARRAY:
            return o->as.array.count != 0;
        case OBJ_STRING:
            return o->as.string.len != 0;
        case OBJ_FUNCTION:
        case OBJ_CODE:
            return true;
        default:
            return false;
    }
}

char* object_to_string(Object* o) {
    if (!o) return strdup("<null>");
    char buf[128];
    switch (o->type) {
        case OBJ_INT:
            snprintf(buf, sizeof(buf), "%lld", (long long)o->as.int_value);
            return strdup(buf);
        case OBJ_BOOL:
            return strdup(o->as.bool_value ? "true" : "false");
        case OBJ_NONE:
            return strdup("None");
        case OBJ_STRING:
            return strdup(o->as.string.data);
        case OBJ_ARRAY: {
            char* s = strdup("[");
            for (size_t i = 0; i < o->as.array.count; i++) {
                char* item_s = object_to_string(o->as.array.items[i]);
                size_t new_len = strlen(s) + strlen(item_s) + 3;
                s = realloc(s, new_len);
                strcat(s, item_s);
                if (i + 1 < o->as.array.count) strcat(s, ", ");
                free(item_s);
            }
            s = realloc(s, strlen(s) + 3);
            strcat(s, "]");
            return s;
        }
        case OBJ_FUNCTION:
            return strdup("<function>");
        case OBJ_CODE:
            return strdup("<code>");
        default:
            return strdup("<object>");
    }
}
