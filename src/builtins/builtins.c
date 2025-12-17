#include "builtins.h"
#include "../runtime/vm/vm.h"
#include "../runtime/vm/object.h"
#include "../runtime/vm/heap.h"
#include "../debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
