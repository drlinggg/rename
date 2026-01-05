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

Object* object_new_float(const char* v) {
    Object* o = malloc(sizeof(Object));
    o->type = OBJ_FLOAT;
    o->ref_count = 1;
    o->as.float_value = bigfloat_create(v);
    return o;
}

Object* object_new_float_from_bf(BigFloat* bf) {
    Object* o = malloc(sizeof(Object));
    o->type = OBJ_FLOAT;
    o->ref_count = 1;
    o->as.float_value = bf;
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
    o->as.function.codeptr = code;
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

void object_array_append(Object* array, Object* element) {
    if (array->type != OBJ_ARRAY) return;
    
    array->as.array.items = realloc(array->as.array.items, 
                                   (array->as.array.size + 1) * sizeof(Object*));
    array->as.array.items[array->as.array.size] = element;
    array->as.array.size++;
}

Object* object_array_get(Object* array, size_t index) {
    if (array->type != OBJ_ARRAY || index >= array->as.array.size) {
        return NULL;
    }
    return array->as.array.items[index];
}

void object_array_set(Object* array, size_t index, Object* element) {
    if (array->type != OBJ_ARRAY || index >= array->as.array.size) {
        return;
    }
    
    if (array->as.array.items[index]) {
        object_decref(array->as.array.items[index]);
    }
    
    if (element) {
        object_incref(element);
    }
    array->as.array.items[index] = element;
}

void object_array_free(Object* array) {
    if (array->type != OBJ_ARRAY) return;
    
    for (size_t i = 0; i < array->as.array.size; i++) {
        if (array->as.array.items[i]) {
            object_decref(array->as.array.items[i]);
        }
    }
    free(array->as.array.items);
    array->as.array.items = NULL;
    array->as.array.size = 0;
}

void object_incref(Object* obj) {
    if (!obj) return;
    
    if (obj->ref_count != 0x7FFFFFFF) {
        obj->ref_count++;
    }
}

void object_decref(Object* obj) {
    if (!obj) return;
    
    if (obj->ref_count > 0 && obj->ref_count != 0x7FFFFFFF) {
        obj->ref_count--;
        
        if (obj->ref_count == 0) {
            switch (obj->type) {
                case OBJ_ARRAY:
                    if (obj->as.array.items) {
                        for (size_t i = 0; i < obj->as.array.size; i++) {
                            if (obj->as.array.items[i]) {
                                object_decref(obj->as.array.items[i]);
                            }
                        }
                        free(obj->as.array.items);
                    }
                    break;
                
                case OBJ_NATIVE_FUNCTION:
                    if (obj->as.native_function.name) {
                        free((void*)obj->as.native_function.name);
                    }
                    break;
                
                case OBJ_FLOAT:
                    if (obj->as.float_value) {
                        bigfloat_destroy(obj->as.float_value);
                    }
                    break;
                
                case OBJ_CODE:
                case OBJ_FUNCTION:
                    break;
                
                default:
                    break;
            }
            obj->ref_count = 0;
        }
    }
}

bool object_is_truthy(Object* o) {
    if (!o) return false;
    switch (o->type) {
        case OBJ_INT:
            return o->as.int_value != 0;
        case OBJ_BOOL:
            return o->as.bool_value;
        case OBJ_FLOAT:
            if (!o->as.float_value) return false;
            return bigfloat_eq(o->as.float_value, bigfloat_zero());
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
        case OBJ_FLOAT: {
            if (o->as.float_value) {
                char* bf_str = bigfloat_to_string(o->as.float_value);
                char* result = strdup(bf_str);
                free(bf_str);
                return result;
            }
            return strdup("0.0");
        }
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
