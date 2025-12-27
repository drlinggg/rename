#include "../../builtins/builtins.h"
#include "../../runtime/gc/gc.h"
#include "../../runtime/jit/jit.h"
#include "../../system.h"
#include "float_bigint.h"
#include "vm.h"
#include <stdio.h>
#include <string.h>

void gc_incref(GC* gc, Object* obj);
void gc_decref(GC* gc, Object* obj);

struct VM {
    Heap* heap;
    GC* gc;
    JIT* jit;

    Object** globals;
    size_t globals_count;

    Object* none_object;
    Object* true_object;
    Object* false_object;
};

struct Frame {
    VM* vm;
    CodeObj* code;

    Object** locals;
    size_t local_count;

    Object** stack;
    size_t stack_size;
    size_t stack_capacity;

    size_t ip;
};


typedef void (*OpHandler)(Frame*, uint32_t);

// Объявления всех функций-обработчиков
static void op_COMPARE_AND_SWAP(Frame* frame, uint32_t arg);
static void op_LOAD_CONST(Frame* frame, uint32_t arg);
static void op_LOAD_FAST(Frame* frame, uint32_t arg);
static void op_STORE_FAST(Frame* frame, uint32_t arg);
static void op_LOAD_GLOBAL(Frame* frame, uint32_t arg);
static void op_STORE_GLOBAL(Frame* frame, uint32_t arg);
static void op_BINARY_OP(Frame* frame, uint32_t arg);
static void op_UNARY_OP(Frame* frame, uint32_t arg);
static void op_PUSH_NULL(Frame* frame, uint32_t arg);
static void op_POP_TOP(Frame* frame, uint32_t arg);
static void op_MAKE_FUNCTION(Frame* frame, uint32_t arg);
static void op_CALL_FUNCTION(Frame* frame, uint32_t arg);
static void op_RETURN_VALUE(Frame* frame, uint32_t arg);
static void op_NOP(Frame* frame, uint32_t arg);
static void op_LOOP_START(Frame* frame, uint32_t arg);
static void op_LOOP_END(Frame* frame, uint32_t arg);
static void op_BREAK_LOOP(Frame* frame, uint32_t arg);
static void op_CONTINUE_LOOP(Frame* frame, uint32_t arg);
static void op_JUMP_BACKWARD(Frame* frame, uint32_t arg);
static void op_POP_JUMP_IF_FALSE(Frame* frame, uint32_t arg);
static void op_JUMP_FORWARD(Frame* frame, uint32_t arg);
static void op_POP_JUMP_IF_TRUE(Frame* frame, uint32_t arg);
static void op_POP_JUMP_IF_NONE(Frame* frame, uint32_t arg);
static void op_POP_JUMP_IF_NOT_NONE(Frame* frame, uint32_t arg);
static void op_JUMP_BACKWARD_NO_INTERRUPT(Frame* frame, uint32_t arg);
static void op_BUILD_ARRAY(Frame* frame, uint32_t arg);
static void op_STORE_SUBSCR(Frame* frame, uint32_t arg);
static void op_DEL_SUBSCR(Frame* frame, uint32_t arg);
static void op_LOAD_SUBSCR(Frame* frame, uint32_t arg);

// Таблица обработчиков
static OpHandler op_table[256] = {NULL};

// Инициализация таблицы
static void init_op_table(void) {
    if (op_table[0] != NULL) return; // Уже инициализирована
    
    op_table[LOAD_CONST] = op_LOAD_CONST;
    op_table[LOAD_FAST] = op_LOAD_FAST;
    op_table[STORE_FAST] = op_STORE_FAST;
    op_table[LOAD_GLOBAL] = op_LOAD_GLOBAL;
    op_table[STORE_GLOBAL] = op_STORE_GLOBAL;
    op_table[BINARY_OP] = op_BINARY_OP;
    op_table[UNARY_OP] = op_UNARY_OP;
    op_table[PUSH_NULL] = op_PUSH_NULL;
    op_table[POP_TOP] = op_POP_TOP;
    op_table[MAKE_FUNCTION] = op_MAKE_FUNCTION;
    op_table[CALL_FUNCTION] = op_CALL_FUNCTION;
    op_table[RETURN_VALUE] = op_RETURN_VALUE;
    op_table[NOP] = op_NOP;
    op_table[LOOP_START] = op_LOOP_START;
    op_table[LOOP_END] = op_LOOP_END;
    op_table[BREAK_LOOP] = op_BREAK_LOOP;
    op_table[CONTINUE_LOOP] = op_CONTINUE_LOOP;
    op_table[JUMP_BACKWARD] = op_JUMP_BACKWARD;
    op_table[POP_JUMP_IF_FALSE] = op_POP_JUMP_IF_FALSE;
    op_table[JUMP_FORWARD] = op_JUMP_FORWARD;
    op_table[POP_JUMP_IF_TRUE] = op_POP_JUMP_IF_TRUE;
    op_table[POP_JUMP_IF_NONE] = op_POP_JUMP_IF_NONE;
    op_table[POP_JUMP_IF_NOT_NONE] = op_POP_JUMP_IF_NOT_NONE;
    op_table[JUMP_BACKWARD_NO_INTERRUPT] = op_JUMP_BACKWARD_NO_INTERRUPT;
    op_table[BUILD_ARRAY] = op_BUILD_ARRAY;
    op_table[STORE_SUBSCR] = op_STORE_SUBSCR;
    op_table[DEL_SUBSCR] = op_DEL_SUBSCR;
    op_table[LOAD_SUBSCR] = op_LOAD_SUBSCR;
    op_table[COMPARE_AND_SWAP] = op_COMPARE_AND_SWAP;
}

static inline void frame_stack_ensure_capacity_fast(Frame* frame, size_t additional);
static void frame_stack_push(Frame* frame, Object* o) {
    frame_stack_ensure_capacity_fast(frame, 1);
    if (o && frame->vm && frame->vm->gc) {
        gc_incref(frame->vm->gc, o);
    }
    frame->stack[frame->stack_size++] = o;
}

static Object* frame_stack_pop(Frame* frame) {
    if (!frame || frame->stack_size == 0) return NULL;
    Object* o = frame->stack[--frame->stack_size];
    return o;
}


static Object* _vm_execute_with_args(VM* vm, CodeObj* code, Object** args, size_t argc);

void vm_register_builtins(VM* vm) {
    if (!vm || !vm->heap) return;
    
    // Создаем нативные функции с новым хипом
    Object* print_func = heap_alloc_native_function(vm->heap, 
        (NativeCFunc)builtin_print, "print");
    Object* input_func = heap_alloc_native_function(vm->heap, 
        (NativeCFunc)builtin_input, "input");
    Object* randint_func = heap_alloc_native_function(vm->heap,
        (NativeCFunc)builtin_randint, "randint");
    Object* sqrt_func = heap_alloc_native_function(vm->heap,
        (NativeCFunc)builtin_sqrt, "sqrt");
    
    print_func->ref_count = 0x7FFFFFFF;
    input_func->ref_count = 0x7FFFFFFF;
    randint_func->ref_count = 0x7FFFFFFF;
    sqrt_func->ref_count = 0x7FFFFFFF;
    
    size_t print_idx = 0;
    size_t input_idx = 1;
    size_t randint_idx = 2;
    size_t sqrt_idx = 3;
    
    vm_set_global(vm, print_idx, print_func);
    vm_set_global(vm, input_idx, input_func);
    vm_set_global(vm, randint_idx, randint_func);
    vm_set_global(vm, sqrt_idx, sqrt_func);
    
    DPRINT("[VM] Builtins registered: print at %p, input at %p, randint at %p, sqrt at %p\n", 
        (void*)print_func, (void*)input_func, (void*)randint_func, (void*)sqrt_func);
    DPRINT("[VM] Builtins ref counts: print=%u, input=%u, randint=%u, sqrt=%u\n",
        print_func->ref_count, input_func->ref_count, randint_func->ref_count, sqrt_func->ref_count);
}

Heap* vm_get_heap(VM* vm) {
    return vm ? vm->heap : NULL;
}

Object* vm_get_none(VM* vm) {
    if (!vm->none_object) {
        vm->none_object = heap_alloc_none(vm->heap);
        vm->none_object->ref_count = 0x7FFFFFFF;
    }
    return vm->none_object;
}

Object* vm_get_true(VM* vm) {
    if (!vm->true_object) {
        vm->true_object = heap_alloc_bool(vm->heap, true);
        vm->true_object->ref_count = 0x7FFFFFFF;
    }
    return vm->true_object;
}

Object* vm_get_false(VM* vm) {
    if (!vm->false_object) {
        vm->false_object = heap_alloc_bool(vm->heap, false);
        vm->false_object->ref_count = 0x7FFFFFFF;
    }
    return vm->false_object;
}

static void vm_print_object(VM* vm, Object* obj) {
    if (!vm || !obj) return;
    
    char* str = object_to_string(obj);
    if (str) {
        printf("%s", str);
        free(str);
    }
}

void vm_set_global(VM* vm, size_t idx, Object* value) {
    if (!vm || idx >= vm->globals_count) return;
    if (value && vm->gc) {
        gc_incref(vm->gc, value);
    }
    Object* old_value = vm->globals[idx];
    if (old_value && vm->gc) {
        gc_decref(vm->gc, old_value);
    }
    vm->globals[idx] = value;
}

Object* vm_get_global(VM* vm, size_t idx) {
    if (!vm || idx >= vm->globals_count) return NULL;
    return vm->globals[idx];
}

GC* vm_get_gc(VM* vm) { return vm ? vm->gc : NULL; }
JIT* vm_get_jit(VM* vm) { return vm ? vm->jit : NULL; }

VM* vm_create(Heap* heap, size_t global_count) {
    static bool table_inited = false;
    if (!table_inited) {
        init_op_table();
        table_inited = true;
    }
    
    VM* vm = malloc(sizeof(VM));
    vm->heap = heap;
    vm->gc = gc_create();
    vm->jit = jit_create();
    vm->globals_count = global_count;
    
    if (global_count > 0) {
        vm->globals = calloc(global_count, sizeof(Object*));
        for (size_t i = 0; i < global_count; i++) {
            vm->globals[i] = heap_alloc_none(heap);
        }
    } else {
        vm->globals = NULL;
    }
    
    vm->none_object = heap_alloc_none(heap);
    vm->true_object = heap_alloc_bool(heap, true);
    vm->false_object = heap_alloc_bool(heap, false);
    
    if (vm->gc) {
        vm->none_object->ref_count = 0x7FFFFFFF;
        vm->true_object->ref_count = 0x7FFFFFFF;
        vm->false_object->ref_count = 0x7FFFFFFF;
    }
    
    vm_register_builtins(vm);
    return vm;
}

void vm_destroy(VM* vm) {
    if (!vm) return;
    
    if (vm->globals) {
        for (size_t i = 0; i < vm->globals_count; i++) {
            if (vm->gc) {
                gc_decref(vm->gc, vm->globals[i]);
            }
        }
        free(vm->globals);
    }
    
    if (vm->gc) gc_destroy(vm->gc);
    if (vm->jit) jit_destroy(vm->jit);
    
    free(vm);
}

Frame* frame_create(VM* vm, CodeObj* code) {
    if (!vm || !code) return NULL;
    Frame* f = malloc(sizeof(Frame));
    f->vm = vm;
    f->code = code;
    f->local_count = code->local_count;
    f->locals = calloc(f->local_count, sizeof(Object*));
    for (size_t i = 0; i < f->local_count; i++) {
        f->locals[i] = vm->heap ? vm_get_none(vm) : NULL;
        if (f->locals[i] && vm->gc) gc_incref(vm->gc, f->locals[i]);
    }
    f->stack = NULL;
    f->stack_capacity = 0;
    f->stack_size = 0;
    f->ip = 0;
    return f;
}

void frame_destroy(Frame* frame) {
    if (!frame) return;
    if (frame->stack) {
        for (size_t i = 0; i < frame->stack_size; i++) {
            if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, frame->stack[i]);
        }
        free(frame->stack);
    }
    if (frame->locals) {
        for (size_t i = 0; i < frame->local_count; i++) {
            if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, frame->locals[i]);
        }
        free(frame->locals);
    }
    free(frame);
}

Object* vm_execute(VM* vm, CodeObj* code) {
    if (!vm || !code) return NULL;
    Frame* f = frame_create(vm, code);
    if (!f) return NULL;
    Object* r = frame_execute(f);
    frame_destroy(f);
    return r;
}

Object* frame_execute(Frame* frame) {
    if (!frame || !frame->code) return NULL;
    
    CodeObj* code = frame->code;
    bytecode_array* code_arr = &code->code;
    
    // Основной цикл выполнения
    while (frame->ip < code_arr->count) {
        bytecode bc = code_arr->bytecodes[frame->ip++];
        uint32_t arg = bytecode_get_arg(bc);
        
        OpHandler handler = op_table[bc.op_code];
        if (handler) {
            handler(frame, arg);
            
            if (bc.op_code == RETURN_VALUE) {
                Object* val = frame_stack_pop(frame);
                if (val && frame->vm && frame->vm->gc) {
                    gc_incref(frame->vm->gc, val);
                }
                return val ? val : vm_get_none(frame->vm);
            }
        } else {
            DPRINT("VM: Unsupported op code: 0x%02X\n", bc.op_code);
        }
    }
    
    // Возврат None если нет явного return
    Object* nonev = vm_get_none(frame->vm);
    if (frame->vm && frame->vm->gc) gc_incref(frame->vm->gc, nonev);
    return nonev;
}

static inline void frame_stack_ensure_capacity_fast(Frame* frame, size_t additional) {
    if (frame->stack_capacity == 0) {
        frame->stack_capacity = 32;
        frame->stack = malloc(frame->stack_capacity * sizeof(Object*));
        frame->stack_size = 0;
        return;
    }
    
    if (frame->stack_size + additional > frame->stack_capacity) {
        size_t new_capacity = frame->stack_capacity * 2;
        while (new_capacity < frame->stack_size + additional) {
            new_capacity *= 2;
        }
        frame->stack_capacity = new_capacity;
        frame->stack = realloc(frame->stack, new_capacity * sizeof(Object*));
    }
}

// Быстрый PUSH с GC
#define FAST_PUSH_GC(frame, obj) \
    do { \
        if ((frame)->stack_size >= (frame)->stack_capacity) { \
            frame_stack_ensure_capacity_fast((frame), 1); \
        } \
        if ((obj) && (frame)->vm && (frame)->vm->gc) { \
            gc_incref((frame)->vm->gc, (obj)); \
        } \
        (frame)->stack[(frame)->stack_size++] = (obj); \
    } while(0)

// Быстрый PUSH без GC (для кэшированных объектов)
#define FAST_PUSH_NO_GC(frame, obj) \
    do { \
        if ((frame)->stack_size >= (frame)->stack_capacity) { \
            frame_stack_ensure_capacity_fast((frame), 1); \
        } \
        (frame)->stack[(frame)->stack_size++] = (obj); \
    } while(0)

// Быстрый POP без GC (мы сами управляем decref)
#define FAST_POP_NO_GC(frame) \
    ((frame)->stack[--(frame)->stack_size])

// Быстрый POP с GC
#define FAST_POP_GC(frame) \
    ({ \
        Object* _obj = (frame)->stack[--(frame)->stack_size]; \
        if (_obj && (frame)->vm && (frame)->vm->gc) { \
            gc_decref((frame)->vm->gc, _obj); \
        } \
        _obj; \
    })

// Получить элемент без удаления
#define FAST_PEEK(frame, offset) \
    ((frame)->stack[(frame)->stack_size - 1 - (offset)])

// Уменьшить ссылку на объект если GC активен
#define GC_DECREF_IF_NEEDED(frame, obj) \
    do { \
        if ((obj) && (frame)->vm && (frame)->vm->gc) { \
            gc_decref((frame)->vm->gc, (obj)); \
        } \
    } while(0)

// Увеличить ссылку на объект если GC активен
#define GC_INCREF_IF_NEEDED(frame, obj) \
    do { \
        if ((obj) && (frame)->vm && (frame)->vm->gc) { \
            gc_incref((frame)->vm->gc, (obj)); \
        } \
    } while(0)


static void op_BINARY_OP(Frame* frame, uint32_t arg) {
    uint8_t op = arg & 0xFF;
    
    // Извлекаем операнды без автоматического decref (сами будем управлять)
    Object* right = FAST_POP_NO_GC(frame);
    Object* left = FAST_POP_NO_GC(frame);
    
    if (!right) right = vm_get_none(frame->vm);
    if (!left) left = vm_get_none(frame->vm);
    
    Object* ret = NULL;

    // Логические операции OR/AND (0x60, 0x61)
    if (op == 0x60 || op == 0x61) {
        bool left_bool = false;
        bool right_bool = false;
        
        switch (left->type) {
            case OBJ_INT:
                left_bool = (bool) left->as.int_value;
                break;
            case OBJ_BOOL:
                left_bool = left->as.bool_value;
                break;
        }
        switch (right->type) {
            case OBJ_INT:
                right_bool = (bool) right->as.int_value;
                break;
            case OBJ_BOOL:
                right_bool = right->as.bool_value;
                break;
        }

        if (op == 0x60 && (left_bool && right_bool)) {
            ret = vm_get_true(frame->vm);
        }
        else if (op == 0x61 && (left_bool || right_bool)) {
            ret = vm_get_true(frame->vm);
        }
        else {
            ret = vm_get_false(frame->vm);
        }
    }
    
    // БЫСТРАЯ ВЕТКА: оба операнда int (самый частый случай)
    else if (left->type == OBJ_INT && right->type == OBJ_INT) {
        int64_t a = left->as.int_value;
        int64_t b = right->as.int_value;
        
        switch (op) {
            case 0x00: { // ADD
                int64_t result = a + b;
                // Используем кэш если возможно
                if (result >= INT_CACHE_MIN && result <= INT_CACHE_MAX) {
                    ret = frame->vm->heap->int_cache[result - INT_CACHE_MIN];
                } else {
                    ret = heap_alloc_int(frame->vm->heap, result);
                }
                break;
            }
            case 0x0A: { // SUB
                int64_t result = a - b;
                if (result >= INT_CACHE_MIN && result <= INT_CACHE_MAX) {
                    ret = frame->vm->heap->int_cache[result - INT_CACHE_MIN];
                } else {
                    ret = heap_alloc_int(frame->vm->heap, result);
                }
                break;
            }
            case 0x05: { // MUL
                int64_t result = a * b;
                if (result >= INT_CACHE_MIN && result <= INT_CACHE_MAX) {
                    ret = frame->vm->heap->int_cache[result - INT_CACHE_MIN];
                } else {
                    ret = heap_alloc_int(frame->vm->heap, result);
                }
                break;
            }
            case 0x06: { // REMAINDER
                int64_t result = a % b;
                if (result >= INT_CACHE_MIN && result <= INT_CACHE_MAX) {
                    ret = frame->vm->heap->int_cache[result - INT_CACHE_MIN];
                } else {
                    ret = heap_alloc_int(frame->vm->heap, result);
                }
                break;
            }
            case 0x0B: { // DIV
                int64_t result = (b == 0) ? 0 : a / b;
                if (result >= INT_CACHE_MIN && result <= INT_CACHE_MAX) {
                    ret = frame->vm->heap->int_cache[result - INT_CACHE_MIN];
                } else {
                    ret = heap_alloc_int(frame->vm->heap, result);
                }
                break;
            }
            case 0x50: { // EQ
                ret = (a == b) ? vm_get_true(frame->vm) : vm_get_false(frame->vm);
                break;
            }
            case 0x51: { // NE
                ret = (a != b) ? vm_get_true(frame->vm) : vm_get_false(frame->vm);
                break;
            }
            case 0x52: { // LT
                ret = (a < b) ? vm_get_true(frame->vm) : vm_get_false(frame->vm);
                break;
            }
            case 0x53: { // LE
                ret = (a <= b) ? vm_get_true(frame->vm) : vm_get_false(frame->vm);
                break;
            }
            case 0x54: { // GT
                ret = (a > b) ? vm_get_true(frame->vm) : vm_get_false(frame->vm);
                break;
            }
            case 0x55: { // GE
                ret = (a >= b) ? vm_get_true(frame->vm) : vm_get_false(frame->vm);
                break;
            }
            case 0x56: { // IS
                ret = (left == right) ? vm_get_true(frame->vm) : vm_get_false(frame->vm);
                break;
            }
            default:
                DPRINT("VM: Unsupported binary_op on ints: %u\n", op);
                ret = vm_get_none(frame->vm);
                break;
        }
    }
    
    // МЕДЛЕННЫЕ ВЕТКИ
    // Сначала обработаем ветку для операций с float (включая смешанные float/int/bool)
    else if (left->type == OBJ_FLOAT || right->type == OBJ_FLOAT) {
        BigFloat* bf_left = NULL;
        BigFloat* bf_right = NULL;
        
        if (left->type == OBJ_FLOAT) {
            bf_left = left->as.float_value;
        } else if (left->type == OBJ_INT) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%lld", (long long)left->as.int_value);
            bf_left = bigfloat_create(buf);
        } else if (left->type == OBJ_BOOL) {
            bf_left = bigfloat_create(left->as.bool_value ? "1" : "0");
        }
        
        if (right->type == OBJ_FLOAT) {
            bf_right = right->as.float_value;
        } else if (right->type == OBJ_INT) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%lld", (long long)right->as.int_value);
            bf_right = bigfloat_create(buf);
        } else if (right->type == OBJ_BOOL) {
            bf_right = bigfloat_create(right->as.bool_value ? "1" : "0");
        }
        
        if (!bf_left || !bf_right) {
            ret = vm_get_none(frame->vm);
        } else {
            BigFloat* bf_result = NULL;
            
            switch (op) {
                case 0x00: // ADD
                    bf_result = bigfloat_add(bf_left, bf_right);
                    break;
                case 0x0A: // SUB
                    bf_result = bigfloat_sub(bf_left, bf_right);
                    break;
                case 0x05: // MUL
                    bf_result = bigfloat_mul(bf_left, bf_right);
                    break;
                case 0x0B: // DIV
                    bf_result = bigfloat_div(bf_left, bf_right);
                    break;
                case 0x06: // REMAINDER
                    bf_result = bigfloat_mod(bf_left, bf_right);
                    break;
                case 0x50: // EQ
                    ret = bigfloat_eq(bf_left, bf_right) ? 
                          vm_get_true(frame->vm) : vm_get_false(frame->vm);
                    break;
                case 0x51: // NE
                    ret = !bigfloat_eq(bf_left, bf_right) ? 
                          vm_get_true(frame->vm) : vm_get_false(frame->vm);
                    break;
                case 0x52: // LT
                    ret = bigfloat_lt(bf_left, bf_right) ? 
                          vm_get_true(frame->vm) : vm_get_false(frame->vm);
                    break;
                case 0x53: // LE
                    ret = bigfloat_le(bf_left, bf_right) ? 
                          vm_get_true(frame->vm) : vm_get_false(frame->vm);
                    break;
                case 0x54: // GT
                    ret = bigfloat_gt(bf_left, bf_right) ? 
                          vm_get_true(frame->vm) : vm_get_false(frame->vm);
                    break;
                case 0x55: // GE
                    ret = bigfloat_ge(bf_left, bf_right) ? 
                          vm_get_true(frame->vm) : vm_get_false(frame->vm);
                    break;
                default:
                    ret = vm_get_none(frame->vm);
                    break;
            }
            
            if (bf_result) {
                ret = heap_alloc_float_from_bf(frame->vm->heap, bf_result);
            }
            
            // Освобождаем временные BigFloat если они были созданы
            if (left->type != OBJ_FLOAT && bf_left) bigfloat_destroy(bf_left);
            if (right->type != OBJ_FLOAT && bf_right) bigfloat_destroy(bf_right);
        }
    }
    else if (left->type == OBJ_BOOL && right->type == OBJ_BOOL) {
        switch (arg) {
            case 0x50: {
                ret = (left->as.bool_value == right->as.bool_value) ? 
                      vm_get_true(frame->vm) : vm_get_false(frame->vm);
                break;
            }
            case 0x51: {
                ret = (left->as.bool_value != right->as.bool_value) ? 
                      vm_get_true(frame->vm) : vm_get_false(frame->vm);
                break;
            }
            case 0x52:
            case 0x53:
            case 0x54:
            case 0x55: {
                ret = vm_get_false(frame->vm);
                break;
            }
            case 0x56: {
                ret = (left == right) ? 
                      vm_get_true(frame->vm) : vm_get_false(frame->vm);
                break;
            }
            default:
                DPRINT("VM: Unsupported binary_op %u on %d type and %d type\n", op, left->type, right->type);
                ret = vm_get_none(frame->vm);
                break;
        }
    }
    else if (left->type == OBJ_FLOAT || right->type == OBJ_FLOAT) {
        BigFloat* bf_left = NULL;
        BigFloat* bf_right = NULL;
        
        if (left->type == OBJ_FLOAT) {
            bf_left = left->as.float_value;
        } else if (left->type == OBJ_INT) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%lld", (long long)left->as.int_value);
            bf_left = bigfloat_create(buf);
        } else if (left->type == OBJ_BOOL) {
            bf_left = bigfloat_create(left->as.bool_value ? "1" : "0");
        }
        
        if (right->type == OBJ_FLOAT) {
            bf_right = right->as.float_value;
        } else if (right->type == OBJ_INT) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%lld", (long long)right->as.int_value);
            bf_right = bigfloat_create(buf);
        } else if (right->type == OBJ_BOOL) {
            bf_right = bigfloat_create(right->as.bool_value ? "1" : "0");
        }
        
        if (!bf_left || !bf_right) {
            ret = vm_get_none(frame->vm);
        } else {
            BigFloat* bf_result = NULL;
            
            switch (op) {
                case 0x00: // ADD
                    bf_result = bigfloat_add(bf_left, bf_right);
                    break;
                case 0x0A: // SUB
                    bf_result = bigfloat_sub(bf_left, bf_right);
                    break;
                case 0x05: // MUL
                    bf_result = bigfloat_mul(bf_left, bf_right);
                    break;
                case 0x0B: // DIV
                    bf_result = bigfloat_div(bf_left, bf_right);
                    break;
                case 0x06: // REMAINDER
                    bf_result = bigfloat_mod(bf_left, bf_right);
                    break;
                case 0x50: // EQ
                    ret = bigfloat_eq(bf_left, bf_right) ? 
                          vm_get_true(frame->vm) : vm_get_false(frame->vm);
                    break;
                case 0x51: // NE
                    ret = !bigfloat_eq(bf_left, bf_right) ? 
                          vm_get_true(frame->vm) : vm_get_false(frame->vm);
                    break;
                case 0x52: // LT
                    ret = bigfloat_lt(bf_left, bf_right) ? 
                          vm_get_true(frame->vm) : vm_get_false(frame->vm);
                    break;
                case 0x53: // LE
                    ret = bigfloat_le(bf_left, bf_right) ? 
                          vm_get_true(frame->vm) : vm_get_false(frame->vm);
                    break;
                case 0x54: // GT
                    ret = bigfloat_gt(bf_left, bf_right) ? 
                          vm_get_true(frame->vm) : vm_get_false(frame->vm);
                    break;
                case 0x55: // GE
                    ret = bigfloat_ge(bf_left, bf_right) ? 
                          vm_get_true(frame->vm) : vm_get_false(frame->vm);
                    break;
                default:
                    ret = vm_get_none(frame->vm);
                    break;
            }
            
            if (bf_result) {
                ret = heap_alloc_float_from_bf(frame->vm->heap, bf_result);
            }
            
            // Освобождаем временные BigFloat если они были созданы
            if (left->type != OBJ_FLOAT && bf_left) bigfloat_destroy(bf_left);
            if (right->type != OBJ_FLOAT && bf_right) bigfloat_destroy(bf_right);
        }
    }

    else if (left->type == right->type) {
        switch (arg) {
            case 0x56: {
                ret = (left == right) ? 
                      vm_get_true(frame->vm) : vm_get_false(frame->vm);
                break;
            }
            default:
                DPRINT("VM: Unsupported binary_op on current type: %u\n", op);
                ret = vm_get_none(frame->vm);
                break;
        }
    }
    else if (left->type != right->type) {
        switch (arg) {
            case 0x50:
            case 0x56: {
                ret = vm_get_false(frame->vm);
                break;
            }
            case 0x51: {
                ret = vm_get_true(frame->vm);
                break;
            }
            default:
                DPRINT("VM: Unsupported binary_op %u on %d type and %d type\n", op, left->type, right->type);
                ret = vm_get_none(frame->vm);
                break;
        }
    } else {
        DPRINT("VM: BINARY_OP for non-int-bool operands not implemented\n");
        ret = vm_get_none(frame->vm);
    }
    
    // ОСВОБОЖДАЕМ ВРЕМЕННЫЕ ОБЪЕКТЫ
    GC_DECREF_IF_NEEDED(frame, left);
    GC_DECREF_IF_NEEDED(frame, right);
    
    // КЛАДЕМ РЕЗУЛЬТАТ НА СТЕК
    // Для кэшированных объектов и синглтонов не нужно incref
    if (ret->ref_count == 0x7FFFFFFF) {
        // Бессмертные объекты (кэш, синглтоны)
        FAST_PUSH_NO_GC(frame, ret);
    } else {
        // Обычные объекты - нужен incref
        FAST_PUSH_GC(frame, ret);
    }
}


static void op_LOAD_CONST(Frame* frame, uint32_t arg) {
    CodeObj* code = frame->code;
    if (arg >= code->constants_count) {
        DPRINT("VM: LOAD_CONST index out of range %u\n", arg);
        FAST_PUSH_NO_GC(frame, vm_get_none(frame->vm));
        return;
    }

    Value c = code->constants[arg];
    Object* o = NULL;

    if (c.type == VAL_NONE) {
        o = vm_get_none(frame->vm);
        FAST_PUSH_NO_GC(frame, o);
    } else if (c.type == VAL_BOOL) {
        o = c.bool_val ? vm_get_true(frame->vm) : vm_get_false(frame->vm);
        FAST_PUSH_NO_GC(frame, o);
    } else if (c.type == VAL_INT) {
        int64_t val = c.int_val;
        if (val >= INT_CACHE_MIN && val <= INT_CACHE_MAX) {
            Object* o = frame->vm->heap->int_cache[val - INT_CACHE_MIN];
            DPRINT("[VM] LOAD_CONST %lld: CACHE HIT at %p (ref_count=%u)\n", 
                (long long)val, (void*)o, o->ref_count);
            FAST_PUSH_NO_GC(frame, o);
        } else {
            DPRINT("[VM] LOAD_CONST %lld: CACHE MISS\n", (long long)val);            
            o = heap_alloc_int(frame->vm->heap, val);
            FAST_PUSH_GC(frame, o);
        }
    } else if (c.type == VAL_FLOAT) {
        o = heap_alloc_float(frame->vm->heap, strdup(c.float_val));
        FAST_PUSH_GC(frame, o);
    } else {
        o = heap_from_value(frame->vm->heap, c);
        FAST_PUSH_GC(frame, o);
    }
}

static void op_LOAD_SUBSCR(Frame* frame, uint32_t arg) {
    Object* index_obj = FAST_POP_NO_GC(frame);
    Object* array_obj = FAST_POP_NO_GC(frame);
    
    if (!array_obj || !index_obj) {
        GC_DECREF_IF_NEEDED(frame, index_obj);
        GC_DECREF_IF_NEEDED(frame, array_obj);
        FAST_PUSH_NO_GC(frame, vm_get_none(frame->vm));
        return;
    }
    
    // Получаем элемент
    int64_t index = index_obj->as.int_value;
    Object* element = array_obj->as.array.items[index];
    
    // Кладем элемент на стек
    if (element && element->ref_count == 0x7FFFFFFF) {
        FAST_PUSH_NO_GC(frame, element);
    } else {
        FAST_PUSH_GC(frame, element ? element : vm_get_none(frame->vm));
    }
    
    // Освобождаем временные объекты
    GC_DECREF_IF_NEEDED(frame, index_obj);
    GC_DECREF_IF_NEEDED(frame, array_obj);
}

static void op_STORE_SUBSCR(Frame* frame, uint32_t arg) {
    Object* index_obj = FAST_POP_NO_GC(frame);
    Object* array_obj = FAST_POP_NO_GC(frame);
    Object* value_obj = FAST_POP_NO_GC(frame);
    
    if (!array_obj || !index_obj || !value_obj) {
        GC_DECREF_IF_NEEDED(frame, value_obj);
        GC_DECREF_IF_NEEDED(frame, array_obj);
        GC_DECREF_IF_NEEDED(frame, index_obj);
        return;
    }
    
    // Быстрая замена элемента
    int64_t index = index_obj->as.int_value;
    Object* old_element = array_obj->as.array.items[index];
    array_obj->as.array.items[index] = value_obj;
    
    // Обновляем счетчики ссылок
    GC_INCREF_IF_NEEDED(frame, value_obj);
    GC_DECREF_IF_NEEDED(frame, old_element);
    
    // Освобождаем временные объекты
    GC_DECREF_IF_NEEDED(frame, index_obj);
    GC_DECREF_IF_NEEDED(frame, array_obj);

    /* Debug: log stores to the main benchmark array (size 1000) for low indices */
    if (debug_enabled && array_obj && array_obj->type == OBJ_ARRAY && array_obj->as.array.size == 1000 && index >= 0 && index < 30) {
        int64_t val = 0;
        if (value_obj && value_obj->type == OBJ_INT) val = value_obj->as.int_value;
        DPRINT("[VM] STORE_SUBSCR: array=%p idx=%lld <- %lld (old=%lld)\n", (void*)array_obj, (long long)index, (long long)val, (long long)(old_element ? old_element->as.int_value : 0));
    }
}

static void op_LOAD_FAST(Frame* frame, uint32_t arg) {
    if (arg >= frame->code->local_count) {
        DPRINT("VM: LOAD_FAST index out of range %u\n", arg);
        frame_stack_push(frame, vm_get_none(frame->vm));
        return;
    }
    Object* o = frame->locals[arg];
    frame_stack_push(frame, o);
}

static void op_STORE_FAST(Frame* frame, uint32_t arg) {
    if (arg >= frame->code->local_count) {
        DPRINT("VM: STORE_FAST index out of range %u\n", arg);
        Object* v = frame_stack_pop(frame);
        if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, v);
        return;
    }
    Object* v = frame_stack_pop(frame);
    
    if (frame->vm && frame->vm->gc) gc_incref(frame->vm->gc, v);
    if (frame->locals[arg] && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, frame->locals[arg]);
    frame->locals[arg] = v;
}

static void op_LOAD_GLOBAL(Frame* frame, uint32_t arg) {
    size_t gidx = arg >> 1;
    Object* val = vm_get_global(frame->vm, gidx);
    if (!val) val = vm_get_none(frame->vm);
    frame_stack_push(frame, val);
}

static void op_STORE_GLOBAL(Frame* frame, uint32_t arg) {
    size_t gidx = arg;
    Object* v = frame_stack_pop(frame);
    vm_set_global(frame->vm, gidx, v);
    if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, v);
}


static void op_UNARY_OP(Frame* frame, uint32_t arg) {
    uint8_t op = arg & 0xFF;
    Object* obj = frame_stack_pop(frame);
    if (!obj) obj = vm_get_none(frame->vm);
    Object* ret = NULL;
    
    if (obj->type == OBJ_INT) {
        switch (op) {
            case 0x00: ret = heap_alloc_int(frame->vm->heap, +obj->as.int_value); break;
            case 0x01: ret = heap_alloc_int(frame->vm->heap, -obj->as.int_value); break;
            default:
                DPRINT("VM: Unsupported unary_op on ints: %u\n", op);
                ret = vm_get_none(frame->vm);
                break;
        }
    }
    else if (obj->type == OBJ_BOOL) {
        switch (op) {
            case 0x03: // NOT
                ret = obj->as.bool_value ? vm_get_false(frame->vm) : vm_get_true(frame->vm);
                break;
            default:
                DPRINT("VM: Unsupported unary_op on bools: %u\n", op);
                ret = vm_get_none(frame->vm);
                break;
        }
    }
    else if (obj->type == OBJ_FLOAT) {
        switch (op) {
            case 0x00: // UNARY_PLUS
                // +float возвращает тот же float
                ret = heap_alloc_float_from_bf(frame->vm->heap, 
                                               bigfloat_create(bigfloat_to_string(obj->as.float_value)));
                break;
            case 0x01: // UNARY_MINUS (отрицание)
                ret = heap_alloc_float_from_bf(frame->vm->heap, 
                                               bigfloat_neg(obj->as.float_value));
                break;
            case 0x03: // NOT для float (преобразует в bool)
                if (bigfloat_eq(obj->as.float_value, bigfloat_zero())) {
                    ret = vm_get_false(frame->vm);
                } else {
                    ret = vm_get_true(frame->vm);
                }
                break;
            default:
                DPRINT("VM: Unsupported unary_op on floats: %u\n", op);
                ret = vm_get_none(frame->vm);
                break;
        }
    }
    else if (obj->type == OBJ_NONE) {
        switch (op) {
            case 0x03: // NOT для None
                ret = vm_get_true(frame->vm); // not None == True
                break;
            default:
                DPRINT("VM: Unsupported unary_op on None: %u\n", op);
                ret = vm_get_none(frame->vm);
                break;
        }
    }
    else {
        DPRINT("VM: Unsupported unary_op: %u for type %d\n", op, obj->type);
        ret = vm_get_none(frame->vm);
    }

    if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, obj);
    
    // Кладем результат на стек
    if (ret->ref_count == 0x7FFFFFFF) {
        FAST_PUSH_NO_GC(frame, ret);
    } else {
        FAST_PUSH_GC(frame, ret);
    }
}

static void op_PUSH_NULL(Frame* frame, uint32_t arg) {
    frame_stack_push(frame, vm_get_none(frame->vm));
}

static void op_POP_TOP(Frame* frame, uint32_t arg) {
    Object* o = frame_stack_pop(frame);
    if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, o);
}

static void op_MAKE_FUNCTION(Frame* frame, uint32_t arg) {
    Object* maybe = frame_stack_pop(frame);
    
    if (maybe && maybe->type == OBJ_CODE) {
        CodeObj* codeptr = maybe->as.codeptr;
        
        JIT_COMPILE_IF_ENABLED(frame->vm, codeptr);

        if (frame->vm && frame->vm->gc) {
            gc_decref(frame->vm->gc, maybe);
        }
        
        Object* func_obj = heap_alloc_function(frame->vm->heap, codeptr);
        frame_stack_push(frame, func_obj);
        
        if (frame->vm && frame->vm->gc) {
            gc_decref(frame->vm->gc, func_obj);
        }
    } else {
        if (maybe && frame->vm && frame->vm->gc) {
            gc_decref(frame->vm->gc, maybe);
        }
        frame_stack_push(frame, vm_get_none(frame->vm));
    }
}

static void op_CALL_FUNCTION(Frame* frame, uint32_t arg) {
    uint32_t argc = arg;
    DPRINT("[VM] CALL_FUNCTION with %u arguments\n", argc);
    
    // Собираем аргументы
    Object** args = NULL;
    if (argc > 0) {
        args = malloc(argc * sizeof(Object*));
        if (!args) {
            DPRINT("[VM] ERROR: Failed to allocate args array\n");
            return;
        }
        for (int i = (int)argc - 1; i >= 0; i--) {
            args[i] = frame_stack_pop(frame);
        }
    }
    
    // Убираем NULL
    Object* maybe_null = frame_stack_pop(frame);
    if (maybe_null && frame->vm && frame->vm->gc) {
        gc_decref(frame->vm->gc, maybe_null);
    }
    
    // Получаем функцию
    Object* callee_obj = frame_stack_pop(frame);
    if (!callee_obj) {
        DPRINT("[VM] ERROR: Callee is NULL\n");
        if (args) {
            for (uint32_t i = 0; i < argc; i++) {
                if (args[i] && frame->vm && frame->vm->gc) {
                    gc_decref(frame->vm->gc, args[i]);
                }
            }
            free(args);
        }
        frame_stack_push(frame, vm_get_none(frame->vm));
        return;
    }
    
    // Вызываем функцию
    Object* ret = NULL;
    if (callee_obj->type == OBJ_FUNCTION) {
        CodeObj* callee_code = callee_obj->as.function.codeptr;
        if (frame->vm && frame->vm->gc) {
            gc_decref(frame->vm->gc, callee_obj);
        }
        ret = _vm_execute_with_args(frame->vm, callee_code, args, argc);
    } 
    else if (callee_obj->type == OBJ_NATIVE_FUNCTION) {
        NativeCFunc native_func = callee_obj->as.native_function.c_func;
        ret = native_func(frame->vm, argc, args);
        if (ret && frame->vm && frame->vm->gc) {
            gc_incref(frame->vm->gc, ret);
        }
        if (frame->vm && frame->vm->gc) {
            gc_decref(frame->vm->gc, callee_obj);
        }
    } 
    else {
        DPRINT("[VM] ERROR: Callee is not a function (type=%d)\n", callee_obj->type);
        if (frame->vm && frame->vm->gc) {
            gc_decref(frame->vm->gc, callee_obj);
        }
        ret = vm_get_none(frame->vm);
    }
    
    // Очищаем аргументы
    if (args) {
        for (uint32_t i = 0; i < argc; i++) {
            if (args[i] && frame->vm && frame->vm->gc) {
                gc_decref(frame->vm->gc, args[i]);
            }
        }
        free(args);
    }
    
    if (!ret) {
        DPRINT("[VM] WARNING: Function returned NULL, using None\n");
        ret = vm_get_none(frame->vm);
    }
    
    DPRINT("[VM] Function result: %p (type: %d)\n", (void*)ret, ret->type);
    frame_stack_push(frame, ret);
}

static void op_RETURN_VALUE(Frame* frame, uint32_t arg) {
    Object* val = frame_stack_pop(frame);
    if (val && frame->vm && frame->vm->gc) {
        gc_incref(frame->vm->gc, val);
    }
    // Возврат обрабатывается в основном цикле frame_execute
    frame_stack_push(frame, val);
}

static void op_NOP(Frame* frame, uint32_t arg) {
    // Ничего не делаем
}

static void op_LOOP_START(Frame* frame, uint32_t arg) {
    DPRINT("[VM] LOOP_START at ip=%zu\n", frame->ip - 1);
}

static void op_LOOP_END(Frame* frame, uint32_t arg) {
    DPRINT("[VM] LOOP_END at ip=%zu\n", frame->ip - 1);
}

static void op_BREAK_LOOP(Frame* frame, uint32_t arg) {
    DPRINT("[VM] BREAK_LOOP instruction executed at ip=%zu\n", frame->ip - 1);
    
    CodeObj* code = frame->code;
    bytecode_array* code_arr = &code->code;
    size_t search_ip = frame->ip;
    int loop_depth = 1;
    
    while (search_ip < code_arr->count) {
        bytecode current_bc = code_arr->bytecodes[search_ip];
        
        if (current_bc.op_code == LOOP_START) {
            loop_depth++;
        } 
        else if (current_bc.op_code == LOOP_END) {
            loop_depth--;
            if (loop_depth == 0) {
                frame->ip = search_ip;
                break;
            }
        }
        search_ip++;
    }
}

static void op_CONTINUE_LOOP(Frame* frame, uint32_t arg) {
    DPRINT("[VM] CONTINUE_LOOP instruction executed at ip=%zu\n", frame->ip - 1);
    
    CodeObj* code = frame->code;
    bytecode_array* code_arr = &code->code;
    size_t search_ip = frame->ip;
    int loop_depth = 1;
    
    while (search_ip < code_arr->count) {
        bytecode current_bc = code_arr->bytecodes[search_ip];
        
        if (current_bc.op_code == LOOP_END) {
            loop_depth++;
        } 
        else if (current_bc.op_code == LOOP_START) {
            loop_depth--;
            if (loop_depth == 0) {
                frame->ip = search_ip + 1;
                break;
            }
        }
        
        if (search_ip == 0) break;
        search_ip--;
    }
}

static void op_JUMP_BACKWARD(Frame* frame, uint32_t arg) {
    frame->ip -= (int32_t)arg;
    DPRINT("[VM] JUMP_BACKWARD: jumping %d instructions to ip=%zu\n", 
           (int32_t)arg, frame->ip);
}

static void op_POP_JUMP_IF_FALSE(Frame* frame, uint32_t arg) {
    Object* condition = frame_stack_pop(frame);
    bool should_jump = false;
    
    if (condition) {
        if (condition->type == OBJ_BOOL) {
            should_jump = (condition->as.bool_value == false);
        } else if (condition->type == OBJ_INT) {
            should_jump = (condition->as.int_value == 0);
        } else if (condition->type == OBJ_NONE) {
            should_jump = true;
        }
    }
    
    if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, condition);
    
    if (should_jump) {
        frame->ip += (int32_t)arg;
        DPRINT("[VM] POP_JUMP_IF_FALSE: jumping %d instructions to ip=%zu\n", 
               (int32_t)arg, frame->ip);
    }
}

static void op_JUMP_FORWARD(Frame* frame, uint32_t arg) {
    frame->ip += (int32_t)arg;
    DPRINT("[VM] JUMP_FORWARD: jumping %d instructions to ip=%zu\n", 
           (int32_t)arg, frame->ip);
}

static void op_POP_JUMP_IF_TRUE(Frame* frame, uint32_t arg) {
    Object* condition = frame_stack_pop(frame);
    bool should_jump = false;
    
    if (condition) {
        if (condition->type == OBJ_BOOL) {
            should_jump = (condition->as.bool_value == true);
        } else if (condition->type == OBJ_INT) {
            should_jump = (condition->as.int_value != 0);
        } else if (condition->type == OBJ_NONE) {
            should_jump = false;
        } else {
            should_jump = true;
        }
    }
    
    if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, condition);
    if (should_jump) {
        frame->ip += (int32_t)arg;
    }
}

static void op_POP_JUMP_IF_NONE(Frame* frame, uint32_t arg) {
    Object* condition = frame_stack_pop(frame);
    bool should_jump = false;
    if (condition && condition->type == OBJ_NONE) {
        should_jump = true;
    }
    if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, condition);
    if (should_jump) {
        frame->ip += (int32_t)arg;
    }
}

static void op_POP_JUMP_IF_NOT_NONE(Frame* frame, uint32_t arg) {
    Object* condition = frame_stack_pop(frame);
    bool should_jump = false;
    if (condition && condition->type != OBJ_NONE) {
        should_jump = true;
    }
    if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, condition);
    
    if (should_jump) {
        frame->ip += (int32_t)arg;
    }
}

static void op_JUMP_BACKWARD_NO_INTERRUPT(Frame* frame, uint32_t arg) {
    frame->ip -= (int32_t)arg;
}

static void op_BUILD_ARRAY(Frame* frame, uint32_t arg) {
    DPRINT("[VM] BUILD_ARRAY with arg=%u\n", arg);
    
    uint32_t element_count = arg;
    
    if (element_count == 0) {
        Object* size_obj = frame_stack_pop(frame);
        if (!size_obj || size_obj->type != OBJ_INT) {
            DPRINT("[VM] ERROR: BUILD_ARRAY(0) expected integer size on stack\n");
            if (size_obj && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, size_obj);
            frame_stack_push(frame, vm_get_none(frame->vm));
            return;
        }
        
        int64_t size = size_obj->as.int_value;
        DPRINT("[VM] Creating empty array with size=%lld\n", (long long)size);
        
        Object* array = heap_alloc_array_with_size(frame->vm->heap, size);
        
        if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, size_obj);
        
        if (!array) {
            DPRINT("[VM] ERROR: Failed to allocate array\n");
            frame_stack_push(frame, vm_get_none(frame->vm));
            return;
        }
        
        frame_stack_push(frame, array);
    } else {
        if (frame->stack_size < element_count) {
            DPRINT("[VM] ERROR: Not enough elements on stack for BUILD_ARRAY\n");
            frame_stack_push(frame, vm_get_none(frame->vm));
            return;
        }
        
        Object* array = heap_alloc_array_with_size(frame->vm->heap, element_count);
        
        if (!array) {
            DPRINT("[VM] ERROR: Failed to allocate array\n");
            frame_stack_push(frame, vm_get_none(frame->vm));
            return;
        }
        
        DPRINT("[VM] Allocated array with size=%u at %p\n", element_count, (void*)array);
        
        for (int32_t i = (int32_t)element_count - 1; i >= 0; i--) {
            Object* element = frame_stack_pop(frame);
            
            DPRINT("[VM] Popping element %d: %p\n", i, (void*)element);
            
            if (element) {
                if (i < (int64_t)array->as.array.size) {
                    if (frame->vm && frame->vm->gc) {
                        gc_incref(frame->vm->gc, element);
                    }
                    
                    Object* old_element = array->as.array.items[i];
                    if (old_element && frame->vm && frame->vm->gc) {
                        gc_decref(frame->vm->gc, old_element);
                    }
                    
                    array->as.array.items[i] = element;
                }
                
                if (frame->vm && frame->vm->gc) {
                    gc_decref(frame->vm->gc, element);
                }
            }
        }
        
        frame_stack_push(frame, array);
    }
}

static void op_DEL_SUBSCR(Frame* frame, uint32_t arg) {
    Object* index_obj = frame_stack_pop(frame);
    Object* array_obj = frame_stack_pop(frame);
    
    if (!array_obj || !index_obj) {
        DPRINT("[VM] ERROR: DEL_SUBSCR missing array or index\n");
        if (index_obj && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, index_obj);
        if (array_obj && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, array_obj);
        return;
    }
    
    if (array_obj->type != OBJ_ARRAY) {
        DPRINT("[VM] ERROR: DEL_SUBSCR expected array, got type=%d\n", array_obj->type);
        if (frame->vm && frame->vm->gc) {
            gc_decref(frame->vm->gc, index_obj);
            gc_decref(frame->vm->gc, array_obj);
        }
        return;
    }
    
    int64_t index = 0;
    if (index_obj->type == OBJ_INT) {
        index = index_obj->as.int_value;
    } else {
        DPRINT("[VM] ERROR: DEL_SUBSCR index must be integer, got type=%d\n", index_obj->type);
        if (frame->vm && frame->vm->gc) {
            gc_decref(frame->vm->gc, index_obj);
            gc_decref(frame->vm->gc, array_obj);
        }
        return;
    }
    
    if (index < 0 || index >= (int64_t)array_obj->as.array.size) {
        DPRINT("[VM] ERROR: DEL_SUBSCR index %lld out of bounds (size=%zu)\n", 
               (long long)index, array_obj->as.array.size);
        if (frame->vm && frame->vm->gc) {
            gc_decref(frame->vm->gc, index_obj);
            gc_decref(frame->vm->gc, array_obj);
        }
        return;
    }
    
    Object* old_element = array_obj->as.array.items[index];
    array_obj->as.array.items[index] = vm_get_none(frame->vm);
    
    if (old_element && frame->vm && frame->vm->gc) {
        gc_decref(frame->vm->gc, old_element);
    }
    
    if (frame->vm && frame->vm->gc) {
        gc_decref(frame->vm->gc, index_obj);
        gc_decref(frame->vm->gc, array_obj);
    }
}

static void op_COMPARE_AND_SWAP(Frame* frame, uint32_t arg) {
    
    if (frame->stack_size < 3) {
        DPRINT("[VM] COMPARE_AND_SWAP: stack underflow\n");
        return;
    }
    
    // Снимаем со стека в обратном порядке (последний загруженный - первый снимается)
    Object* j_plus_1_obj = FAST_POP_NO_GC(frame); // j+1
    Object* j_obj = FAST_POP_NO_GC(frame);        // j
    Object* array_obj = FAST_POP_NO_GC(frame);    // массив
    
    if (!array_obj || array_obj->type != OBJ_ARRAY) {
        DPRINT("[VM] COMPARE_AND_SWAP: expected array\n");
        // Возвращаем обратно
        FAST_PUSH_NO_GC(frame, array_obj);
        FAST_PUSH_NO_GC(frame, j_obj);
        FAST_PUSH_NO_GC(frame, j_plus_1_obj);
        return;
    }
    
    if (!j_obj || j_obj->type != OBJ_INT || !j_plus_1_obj || j_plus_1_obj->type != OBJ_INT) {
        DPRINT("[VM] COMPARE_AND_SWAP: indices must be integers\n");
        // Возвращаем обратно
        FAST_PUSH_NO_GC(frame, array_obj);
        FAST_PUSH_NO_GC(frame, j_obj);
        FAST_PUSH_NO_GC(frame, j_plus_1_obj);
        return;
    }
    
    int64_t j = j_obj->as.int_value;
    int64_t j_plus_1 = j_plus_1_obj->as.int_value;
    
    // Проверка границ (только в debug)
    #ifdef DEBUG
    if (j < 0 || j >= array_obj->as.array.size || 
        j_plus_1 < 0 || j_plus_1 >= array_obj->as.array.size) {
        DPRINT("[VM] COMPARE_AND_SWAP: indices out of bounds: %lld, %lld (size=%zu)\n",
               (long long)j, (long long)j_plus_1, array_obj->as.array.size);
        // Возвращаем обратно
        FAST_PUSH_NO_GC(frame, array_obj);
        FAST_PUSH_NO_GC(frame, j_obj);
        FAST_PUSH_NO_GC(frame, j_plus_1_obj);
        return;
    }
    #endif
    
    // Получаем элементы
    Object* a = array_obj->as.array.items[j];
    Object* b = array_obj->as.array.items[j_plus_1];
    
    // Сравниваем и меняем если нужно
    if (a->as.int_value > b->as.int_value) {
        DPRINT("[VM] COMPARE_AND_SWAP: swapping indices %lld and %lld (vals %lld > %lld) frame=%p\n",
            (long long)j, (long long)j_plus_1,
            (long long)(a ? a->as.int_value : 0), (long long)(b ? b->as.int_value : 0), (void*)frame);
        /* Preserve old pointers and update refcounts similarly to STORE_SUBSCR
         * to avoid premature frees or leaks when swapping elements. */
        Object* old_a = array_obj->as.array.items[j];
        Object* old_b = array_obj->as.array.items[j_plus_1];

        /* Place b into position j */
        array_obj->as.array.items[j] = old_b;
        GC_INCREF_IF_NEEDED(frame, old_b);
        GC_DECREF_IF_NEEDED(frame, old_a);

        /* Place a into position j+1 */
        array_obj->as.array.items[j_plus_1] = old_a;
        GC_INCREF_IF_NEEDED(frame, old_a);
        GC_DECREF_IF_NEEDED(frame, old_b);

        /* Debug: print post-swap values and a quick sanity check */
        Object* new_a = array_obj->as.array.items[j];
        Object* new_b = array_obj->as.array.items[j_plus_1];
        DPRINT("[VM] COMPARE_AND_SWAP: after swap indices %lld=%lld, %lld=%lld\n",
            (long long)j, (long long)(new_a ? new_a->as.int_value : 0),
            (long long)j_plus_1, (long long)(new_b ? new_b->as.int_value : 0));
    #ifdef DEBUG
        if (new_a && new_b && new_a->as.int_value > new_b->as.int_value) {
            DPRINT("[VM] COMPARE_AND_SWAP: sanity check FAILED at frame=%p for indices %lld,%lld\n",
            (void*)frame, (long long)j, (long long)j_plus_1);
        }
    #endif
    }
    
    // Освобождаем временные объекты (индексы)
    GC_DECREF_IF_NEEDED(frame, j_obj);
    GC_DECREF_IF_NEEDED(frame, j_plus_1_obj);
    // Массив не освобождаем - он остается с теми же ссылками
}

static Object* _vm_execute_with_args(VM* vm, CodeObj* code, Object** args, size_t argc) {
    if (!vm || !code) return NULL;
    Frame* frame = frame_create(vm, code);
    if (!frame) return NULL;
    
    for (size_t i = 0; i < frame->local_count && i < argc; i++) {
        if (args && args[i]) {
            if (frame->locals[i] && vm->gc) gc_decref(vm->gc, frame->locals[i]);
            frame->locals[i] = args[i];
            if (vm->gc) gc_incref(vm->gc, frame->locals[i]);
        }
    }
    
    Object* res = frame_execute(frame);
    frame_destroy(frame);
    return res;
}
