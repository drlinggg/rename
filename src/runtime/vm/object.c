#include "object.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_ARRAY;
    obj->ref_count = 1;
    obj->as.array.items = NULL;
    obj->as.array.size = 0;
    return obj;
}

// Функция для создания массива с заданной начальной емкостью
Object* object_new_array_with_size(size_t initial_size) {
    Object* obj = malloc(sizeof(Object));
    obj->type = OBJ_ARRAY;
    obj->ref_count = 1;
    obj->as.array.size = initial_size;
    
    if (initial_size > 0) {
        obj->as.array.items = calloc(initial_size, sizeof(Object*));
        for (size_t i = 0; i < initial_size; i++) {
            obj->as.array.items[i] = NULL;
        }
    } else {
        obj->as.array.items = NULL;
    }
    
    return obj;
}

// Функция для получения элемента массива по индексу
Object* object_array_get(Object* array, size_t index) {
    if (!array || array->type != OBJ_ARRAY) return NULL;
    if (index >= array->as.array.size) return NULL;
    return array->as.array.items[index];
}

// Функция для установки элемента массива по индексу
void object_array_set(Object* array, size_t index, Object* element) {
    if (!array || array->type != OBJ_ARRAY || index >= array->as.array.size) return;
    
    // Уменьшаем счетчик для старого элемента
    if (array->as.array.items[index]) {
        object_decref(array->as.array.items[index]);
    }
    
    object_incref(element);
    
    array->as.array.items[index] = element;
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
            case OBJ_ARRAY:
                if (o->as.array.items) {
                    for (size_t i = 0; i < o->as.array.size; i++) {
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
            return true;
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
        case OBJ_ARRAY: {
            char* s = strdup("[");
            for (size_t i = 0; i < o->as.array.size; i++) {
                char* item_s = object_to_string(o->as.array.items[i]);
                size_t new_len = strlen(s) + strlen(item_s) + 3;
                s = realloc(s, new_len);
                strcat(s, item_s);
                if (i + 1 < o->as.array.size) strcat(s, ", ");
                free(item_s);
            }
            s = realloc(s, strlen(s) + 3);
            strcat(s, "]");
            return s;
        }
        case OBJ_FUNCTION:
            if (o->as.function.codeptr && o->as.function.codeptr->name) {
                snprintf(buf, sizeof(buf), "<function '%s'>", o->as.function.codeptr->name);
                return strdup(buf);
            }
            return strdup("<function>");
        case OBJ_CODE:
            return strdup("<code>");
        case OBJ_NATIVE_FUNCTION:
            if (o->as.native_function.name) {
                snprintf(buf, sizeof(buf), "<native function '%s'>", o->as.native_function.name);
                return strdup(buf);
            }
            return strdup("<native function>");
        default:
            snprintf(buf, sizeof(buf), "<object type=%d>", o->type);
            return strdup(buf);
    }
}
