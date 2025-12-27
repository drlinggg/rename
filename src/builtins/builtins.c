#include "builtins.h"
#include "../runtime/vm/vm.h"
#include "../runtime/vm/object.h"
#include "../runtime/vm/heap.h"
#include "../system.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


Object* builtin_print(VM* vm, int arg_count, Object** args) {
    Heap* heap = vm_get_heap(vm);
    if (!heap) {
        DPRINT("[BUILTIN_PRINT] ERROR: Cannot get heap from VM\n");
        return NULL;
    }
    
    DPRINT("[BUILTIN_PRINT] Called with %d arguments\n", arg_count);
    
    for (int i = 0; i < arg_count; i++) {
        if (args[i]) {
            DPRINT("[BUILTIN_PRINT] Arg %d: type=%d at %p\n", 
                   i, args[i]->type, (void*)args[i]);
            
            char* str = object_to_string(args[i]);
            if (str) {
                printf("%s", str);
                free(str);
            } else {
                printf("<error converting to string>");
            }
            if (i < arg_count - 1) {
                printf(" ");
            }
        } else {
            DPRINT("[BUILTIN_PRINT] Arg %d is NULL\n", i);
            printf("None");
            if (i < arg_count - 1) {
                printf(" ");
            }
        }
    }
    printf("\n");
    fflush(stdout);
    
    Object* result = heap_alloc_none(heap);
    DPRINT("[BUILTIN_PRINT] Returning none at %p\n", (void*)result);
    return result;
}

Object* builtin_input(VM* vm, int arg_count, Object** args) {
    Heap* heap = vm_get_heap(vm);
    if (!heap) {
        DPRINT("[BUILTIN_INPUT] ERROR: Cannot get heap from VM\n");
        return NULL;
    }
    
    DPRINT("[BUILTIN_INPUT] Called with %d arguments\n", arg_count);
    
    // Если есть аргумент - печатаем приглашение
    if (arg_count > 0 && args[0]) {
        char* str = object_to_string(args[0]);
        if (str) {
            printf("%s", str);
            free(str);
        }
        printf(" ");
        fflush(stdout);
    }
    
    // Читаем строку
    char buffer[4096];
    int64_t result = 0;
    
    DPRINT("[BUILTIN_INPUT] Waiting for input...\n");
    
    if (fgets(buffer, sizeof(buffer), stdin)) {
        // Убираем перевод строки
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
            len--;
        }
        
        DPRINT("[BUILTIN_INPUT] Got input: '%s'\n", buffer);
        
        // Преобразуем в число
        char* endptr;
        int64_t value = strtoll(buffer, &endptr, 10);
        
        // Если преобразование успешно, возвращаем число
        if (endptr != buffer && *endptr == '\0') {
            result = value;
            DPRINT("[BUILTIN_INPUT] Converted to int: %lld\n", (long long)result);
        } else {
            DPRINT("[BUILTIN_INPUT] Failed to convert to int, using 0\n");
            result = 0;
        }
    } else {
        DPRINT("[BUILTIN_INPUT] Failed to read input\n");
        result = 0;
    }
    
    Object* obj = heap_alloc_int(heap, result);
    DPRINT("[BUILTIN_INPUT] Returning object at %p (type: %d, value: %lld)\n", 
           (void*)obj, obj ? obj->type : -1, (long long)result);
    
    return obj;
}

Object* builtin_randint(VM* vm, int arg_count, Object** args) {
    Heap* heap = vm_get_heap(vm);
    if (!heap) {
        DPRINT("[BUILTIN_RANDINT] ERROR: Cannot get heap from VM\n");
        return NULL;
    }
    
    DPRINT("[BUILTIN_RANDINT] Called with %d arguments\n", arg_count);
    
    if (arg_count != 2) {
        DPRINT("[BUILTIN_RANDINT] ERROR: Expected 2 arguments, got %d\n", arg_count);
        return heap_alloc_none(heap);
    }
    
    if (!args[0] || args[0]->type != OBJ_INT || 
        !args[1] || args[1]->type != OBJ_INT) {
        DPRINT("[BUILTIN_RANDINT] ERROR: Arguments must be integers\n");
        return heap_alloc_none(heap);
    }
    
    int64_t low = args[0]->as.int_value;
    int64_t high = args[1]->as.int_value;
    
    DPRINT("[BUILTIN_RANDINT] Range: [%lld, %lld]\n", (long long)low, (long long)high);
    
    if (low > high) {
        DPRINT("[BUILTIN_RANDINT] ERROR: Invalid range, low > high\n");
        return heap_alloc_none(heap);
    }
    
    if (low == high) {
        DPRINT("[BUILTIN_RANDINT] low == high, returning same value\n");
        return heap_alloc_int(heap, low);
    }
    
    static int initialized = 0;
    if (!initialized) {
        srand(time(NULL));
        initialized = 1;
        DPRINT("[BUILTIN_RANDINT] Random generator initialized\n");
    }
    
    uint64_t range = (uint64_t)(high - low + 1);
    
    int64_t result;
    
    if (range <= RAND_MAX) {
        result = low + (rand() % (int)range);
    } else {
        uint64_t rand_max_plus_one = (uint64_t)RAND_MAX + 1;
        uint64_t random_value = 0;
        uint64_t max_random = RAND_MAX;
        
        while (max_random < range - 1) {
            random_value = random_value * rand_max_plus_one + rand();
            max_random = max_random * rand_max_plus_one + RAND_MAX;
        }
        
        result = low + (int64_t)(random_value % range);
    }
    
    DPRINT("[BUILTIN_RANDINT] Generated random number: %lld\n", (long long)result);
    
    Object* obj = heap_alloc_int(heap, result);
    return obj;
}

Object* builtin_sqrt(VM* vm, int arg_count, Object** args) {
    Heap* heap = vm_get_heap(vm);
    if (!heap) return NULL;

    if (arg_count != 1 || !args[0]) return heap_alloc_none(heap);

    double val = 0.0;
    if (args[0]->type == OBJ_FLOAT && args[0]->as.float_value) {
        char* s = bigfloat_to_string(args[0]->as.float_value);
        if (s) {
            val = strtod(s, NULL);
            free(s);
        }
    } else if (args[0]->type == OBJ_INT) {
        val = (double) args[0]->as.int_value;
    } else if (args[0]->type == OBJ_BOOL) {
        val = args[0]->as.bool_value ? 1.0 : 0.0;
    } else {
        return heap_alloc_none(heap);
    }

    double res = sqrt(val);
    char buf[64];
    snprintf(buf, sizeof(buf), "%.15g", res);
    return heap_alloc_float(heap, strdup(buf));
}
