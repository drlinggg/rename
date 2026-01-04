#ifndef RENAME_DEBUG_H
#define RENAME_DEBUG_H

#include <stdio.h>

extern int debug_enabled;
extern int jit_enabled;
extern int gc_enabled;

#define DPRINT(fmt, ...) do { if (debug_enabled) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

#define JIT_COMPILE_IF_ENABLED(vm_ptr, code_ptr) \
    do { \
        if (jit_enabled && (vm_ptr) && (vm_ptr)->jit && (code_ptr)) { \
            DPRINT("[VM] JIT compiling function: %s\n", \
                   (code_ptr)->name ? (code_ptr)->name : "anonymous"); \
            \
            CodeObj* jit_code = jit_compile_function((vm_ptr)->jit, (code_ptr)); \
            \
            bytecode_array_print(&jit_code->code); \
            \
            if (jit_code && jit_code != (code_ptr)) { \
                DPRINT("[VM] JIT compilation successful, using optimized version\n"); \
                (code_ptr) = jit_code; \
            } \
        } \
    } while(0)

#endif
