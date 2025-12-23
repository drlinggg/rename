#ifndef JIT_TYPES_H
#define JIT_TYPES_H

#include <stddef.h>
#include "../../runtime/vm/vm.h"

typedef struct JIT {
    CodeObj** compiled_cache;
    size_t cache_size;
    size_t cache_capacity;
    size_t compiled_count;
} JIT;

#endif // JIT_TYPES_H
