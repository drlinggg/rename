#include "vm.h"
#include "../../runtime/gc/gc.h"
#include "../../runtime/jit/jit.h"
#include "../../debug.h"
#include "../../builtins/builtins.h"
#include <stdio.h>
#include <string.h>

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


void vm_register_builtins(VM* vm) {
    Object* print_func = heap_alloc_native_function(vm->heap, 
        (NativeCFunc)builtin_print, "print");
    Object* input_func = heap_alloc_native_function(vm->heap, 
        (NativeCFunc)builtin_input, "input");
    Object* randint_func = heap_alloc_native_function(vm->heap,
        (NativeCFunc)builtin_randint, "randint");
    
    print_func->ref_count = 0x7FFFFFFF;
    input_func->ref_count = 0x7FFFFFFF;
    randint_func->ref_count = 0x7FFFFFFF;
    
    size_t print_idx = 0;
    size_t input_idx = 1;
    size_t randint_idx = 2;
    
    vm_set_global(vm, print_idx, print_func);
    vm_set_global(vm, input_idx, input_func);
    vm_set_global(vm, randint_idx, randint_func);

    DPRINT("[VM] Builtins registered: print at %p, input at %p, randint at %p\n", 
        (void*)print_func, (void*)input_func, (void*)randint_func);
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

static void frame_stack_ensure_capacity(Frame* frame, size_t additional) {
    if (frame->stack_capacity == 0) {
        frame->stack_capacity = 16;
        frame->stack = malloc(frame->stack_capacity * sizeof(Object*));
        frame->stack_size = 0;
    }
    while (frame->stack_size + additional > frame->stack_capacity) {
        frame->stack_capacity *= 2;
        frame->stack = realloc(frame->stack, frame->stack_capacity * sizeof(Object*));
    }
}

static void frame_stack_push(Frame* frame, Object* o) {
    frame_stack_ensure_capacity(frame, 1);
    if (o && frame->vm && frame->vm->gc) {
        gc_incref(frame->vm->gc, o);
    }
    frame->stack[frame->stack_size++] = o;
}

static Object* frame_stack_pop(Frame* frame) {
    if (!frame || frame->stack_size == 0) return NULL;
    Object* o = frame->stack[--frame->stack_size];
    // manual decref must be done by caller via GC
    return o;
}

static Object* _vm_execute_with_args(VM* vm, CodeObj* code, Object** args, size_t argc);

VM* vm_create(Heap* heap, size_t global_count) {
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
    return vm;
}

void vm_destroy(VM* vm) {
    if (!vm) return;
    
    // no vm-level stack; frames own stacks
    if (vm->globals) {
        for (size_t i = 0; i < vm->globals_count; i++) {
            if (vm->gc) gc_decref(vm->gc, vm->globals[i]);
        }
        free(vm->globals);
    }
    
    if (vm->gc) {
        vm->none_object->ref_count = 0;
        vm->true_object->ref_count = 0;
        vm->false_object->ref_count = 0;
    }
    
    if (vm->gc) gc_destroy(vm->gc);
    if (vm->jit) jit_destroy(vm->jit);
    free(vm);
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
    if (vm->gc) gc_incref(vm->gc, value);
    if (vm->gc) gc_decref(vm->gc, vm->globals[idx]);
    vm->globals[idx] = value;
}

Object* vm_get_global(VM* vm, size_t idx) {
    if (!vm || idx >= vm->globals_count) return NULL;
    return vm->globals[idx];
}

GC* vm_get_gc(VM* vm) { return vm ? vm->gc : NULL; }
JIT* vm_get_jit(VM* vm) { return vm ? vm->jit : NULL; }

// Run a code object once. This is a simple recursive interpreter with no frame stack.
Object* vm_execute(VM* vm, CodeObj* code) {
    if (!vm || !code) return NULL;
    Frame* f = frame_create(vm, code);
    if (!f) return NULL;
    Object* r = frame_execute(f);
    frame_destroy(f);
    return r;
}

// Frame methods
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

Object* frame_execute(Frame* frame) {
    if (!frame || !frame->code) return NULL;
    CodeObj* code = frame->code;
    bytecode_array* code_arr = &code->code;
    Object* last_ret = NULL;
    while (frame->ip < code_arr->count) {
        bytecode bc = code_arr->bytecodes[frame->ip++];
        uint32_t arg = bytecode_get_arg(bc);
        switch (bc.op_code) {
            case LOAD_CONST: {
                if (arg >= code->constants_count) {
                    DPRINT("VM: LOAD_CONST index out of range %u\n", arg);
                    frame_stack_push(frame, vm_get_none(frame->vm));
                    break;
                }
    
                Value c = code->constants[arg];
                Object* o = NULL;
    
                if (c.type == VAL_NONE) {
                    o = vm_get_none(frame->vm);
                } else if (c.type == VAL_BOOL) {
                    o = c.bool_val ? vm_get_true(frame->vm) : vm_get_false(frame->vm);
                } else if (c.type == VAL_INT) {
                    o = heap_alloc_int(frame->vm->heap, c.int_val);
                } else {
                    o = heap_from_value(frame->vm->heap, c);
                }
    
                frame_stack_push(frame, o);
                break;
            }
            case LOAD_FAST: {
                if (arg >= code->local_count) {
                    DPRINT("VM: LOAD_FAST index out of range %u\n", arg);
                    frame_stack_push(frame, vm_get_none(frame->vm));
                    break;
                }
                Object* o = frame->locals[arg];
                frame_stack_push(frame, o);
                break;
            }
            case STORE_FAST: {
                if (arg >= code->local_count) {
                    DPRINT("VM: STORE_FAST index out of range %u\n", arg);
                    Object* v = frame_stack_pop(frame);
                    if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, v);
                    break;
                }
                Object* v = frame_stack_pop(frame);
                DPRINT("[VM] STORE_FAST index %u: storing %p", arg, (void*)v);
                if (v) {
                    DPRINT(" (type: %d", v->type);
                    if (v->type == OBJ_INT) {
                        DPRINT(", value: %lld", (long long)v->as.int_value);
                    }
                    DPRINT(")");
                }
                DPRINT("\n");
                
                if (frame->vm && frame->vm->gc) gc_incref(frame->vm->gc, v);
                if (frame->locals[arg] && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, frame->locals[arg]);
                frame->locals[arg] = v;
                break;
            }
            case LOAD_GLOBAL: {
                size_t gidx = arg >> 1;
                Object* val = vm_get_global(frame->vm, gidx);
                if (!val) val = vm_get_none(frame->vm);
                frame_stack_push(frame, val);
                break;
            }
            case STORE_GLOBAL: {
                size_t gidx = arg;
                Object* v = frame_stack_pop(frame);
                vm_set_global(frame->vm, gidx, v);
                if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, v);
                break;
            }
            case BINARY_OP: {
                uint8_t op = arg & 0xFF;
                Object* right = frame_stack_pop(frame);
                Object* left = frame_stack_pop(frame);
                if (!right) right = vm_get_none(frame->vm);
                if (!left) left = vm_get_none(frame->vm);
                Object* ret = NULL;

                // or and handeling
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

                else if (left->type == OBJ_INT && right->type == OBJ_INT) {
                    int64_t a = left->as.int_value;
                    int64_t b = right->as.int_value;
                    switch (op) {
                        case 0x00: // ADD
                            ret = heap_alloc_int(frame->vm->heap, a + b);
                            break;
                        case 0x0A: // SUB
                            ret = heap_alloc_int(frame->vm->heap, a - b);
                            break;
                        case 0x05: // MUL
                            ret = heap_alloc_int(frame->vm->heap, a * b);
                            break;
                        case 0x06: // REMAINDER
                            ret = heap_alloc_int(frame->vm->heap, a % b);
                            break;
                        case 0x0B: // DIV
                            if (b == 0) ret = heap_alloc_int(frame->vm->heap, 0);
                            else ret = heap_alloc_int(frame->vm->heap, a / b);
                            break;
                        case (0x50): {
                            ret = (left->as.int_value == right->as.int_value) ? 
                                  vm_get_true(frame->vm) : vm_get_false(frame->vm);
                            break;
                        }
                        case (0x51): {
                            ret = (left->as.int_value != right->as.int_value) ? 
                                  vm_get_true(frame->vm) : vm_get_false(frame->vm);
                            break;
                        }
                        case (0x52): {
                            ret = (left->as.int_value < right->as.int_value) ? 
                                  vm_get_true(frame->vm) : vm_get_false(frame->vm);
                            break;
                        }
                        case (0x53): {
                            ret = (left->as.int_value <= right->as.int_value) ? 
                                  vm_get_true(frame->vm) : vm_get_false(frame->vm);
                            break;
                        }
                        case (0x54): {
                            ret = (left->as.int_value > right->as.int_value) ? 
                                  vm_get_true(frame->vm) : vm_get_false(frame->vm);
                            break;
                        }
                        case (0x55): {
                            ret = (left->as.int_value >= right->as.int_value) ? 
                                  vm_get_true(frame->vm) : vm_get_false(frame->vm);
                            break;
                        }
                        case (0x56): {
                            ret = (left == right) ? 
                                  vm_get_true(frame->vm) : vm_get_false(frame->vm);
                            break;
                        }
                        default:
                            DPRINT("VM: Unsupported binary_op on ints: %u\n", op);
                            ret = vm_get_none(frame->vm);
                            break;
                    }
                }
                else if (left->type != right->type) {
                    switch (arg) {
                        case (0x50):
                        case (0x56): {
                            ret = vm_get_false(frame->vm);
                            break;
                        }
                        case (0x51): {
                            ret = vm_get_true(frame->vm);
                            break;
                        }
                        default:
                            DPRINT("VM: Unsupported binary_op %u on %d type and %d type\n", op, left->type, right->type);
                            ret = vm_get_none(frame->vm);
                            break;
                    }
                }
                else if (left->type == OBJ_BOOL && right->type == OBJ_BOOL) {
                    switch (arg) {
                        case (0x50): {
                            ret = (left->as.bool_value == right->as.bool_value) ? 
                                  vm_get_true(frame->vm) : vm_get_false(frame->vm);
                            break;
                        }
                        case (0x51): {
                            ret = (left->as.bool_value != right->as.bool_value) ? 
                                  vm_get_true(frame->vm) : vm_get_false(frame->vm);
                            break;
                        }
                        case (0x52):
                        case (0x53):
                        case (0x54):
                        case (0x55): {
                            ret = vm_get_false(frame->vm);
                            break;
                        }

                        case (0x56): {
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
                else if (left->type == right->type) {
                    switch (arg) {
                        case (0x56): {
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


                else {
                    DPRINT("VM: BINARY_OP for non-int-bool operands not implemented\n");
                    ret = vm_get_none(frame->vm);
                }
                // cleanup
                if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, left);
                if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, right);
                frame_stack_push(frame, ret);
                break;
            }
            case UNARY_OP: {
                uint8_t op = arg & 0xFF;
                Object* obj = frame_stack_pop(frame);
                if (!obj) obj = vm_get_none(frame->vm);
                Object* ret = NULL;
                if (obj->type == OBJ_INT) {
                    switch (op) {
                        case (0x00): {
                            ret = heap_alloc_int(frame->vm->heap, +obj->as.int_value); 
                            break;
                         }
                         case (0x01): {
                            ret = heap_alloc_int(frame->vm->heap, -obj->as.int_value); 
                            break;
                         }   
                         default: {
                             DPRINT("VM: Unsupported unary_op on ints: %u\n", op);
                             ret = vm_get_none(frame->vm);
                             break;
                         }
                    }
                }
                else if (obj->type == OBJ_BOOL) {
                    switch (op) {
                        case (0x03): {
                            if (obj->as.bool_value == true) {
                                ret = vm_get_false(frame->vm);
                            }
                            else {
                                ret = vm_get_true(frame->vm);
                            }
                            break;
                        }
                        default: {
                             DPRINT("VM: Unsupported unary_op on bools: %u\n", op);
                             ret = vm_get_none(frame->vm);
                             break;
                         }
                    }
                } else {
                    DPRINT("VM: Unsupported unary_op: %u\n", op);
                    ret = vm_get_none(frame->vm);
                    break;
                }

                if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, obj);
                frame_stack_push(frame, ret);
                break;
            }
            case PUSH_NULL: {
                frame_stack_push(frame, vm_get_none(frame->vm));
                break;
            }
            case POP_TOP: {
                Object* o = frame_stack_pop(frame);
                if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, o);
                break;
            }
            case MAKE_FUNCTION: {
                // top of stack should be code object
                Object* maybe = frame_stack_pop(frame);
                if (maybe && maybe->type == OBJ_CODE) {
                    CodeObj* codeptr = maybe->as.codeptr;
                    if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, maybe);
                    Object* func_obj = heap_alloc_function(frame->vm->heap, codeptr);
                    // push func
                    frame_stack_push(frame, func_obj);
                    if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, func_obj);
                } else {
                    if (maybe && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, maybe);
                    frame_stack_push(frame, vm_get_none(frame->vm));
                }
                break;
            }
            case CALL_FUNCTION: {
                uint32_t argc = arg;
                DPRINT("[VM] CALL_FUNCTION with %u arguments\n", argc);
                
                // 1. Собираем аргументы из стека
                Object** args = NULL;
                if (argc > 0) {
                    args = malloc(argc * sizeof(Object*));
                    if (!args) {
                        DPRINT("[VM] ERROR: Failed to allocate args array\n");
                        break;
                    }
                    for (int i = (int)argc - 1; i >= 0; i--) {
                        args[i] = frame_stack_pop(frame);
                        DPRINT("[VM] Popped arg[%d]: %p\n", i, (void*)args[i]);
                    }
                }
                
                // 2. Убираем NULL (если есть)
                Object* maybe_null = frame_stack_pop(frame);
                if (maybe_null && frame->vm && frame->vm->gc) {
                    gc_decref(frame->vm->gc, maybe_null);
                }
                
                // 3. Получаем функцию для вызова
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
                    // Пушим None в качестве результата
                    frame_stack_push(frame, vm_get_none(frame->vm));
                    break;
                }
                
                DPRINT("[VM] Callee: %p, type: %d", (void*)callee_obj, callee_obj->type);
                if (callee_obj->type == OBJ_NATIVE_FUNCTION && callee_obj->as.native_function.name) {
                    DPRINT(" (native '%s')", callee_obj->as.native_function.name);
                } else if (callee_obj->type == OBJ_FUNCTION && callee_obj->as.function.codeptr && 
                           callee_obj->as.function.codeptr->name) {
                    DPRINT(" (function '%s')", callee_obj->as.function.codeptr->name);
                }
                DPRINT("\n");
                
                // 4. Вызываем функцию
                Object* ret = NULL;
                
                if (callee_obj->type == OBJ_FUNCTION) {
                    CodeObj* callee_code = callee_obj->as.function.codeptr;
                    // Уменьшаем счетчик на callee_obj (он больше не нужен)
                    if (frame->vm && frame->vm->gc) {
                        gc_decref(frame->vm->gc, callee_obj);
                    }
                    
                    // Выполняем функцию с аргументами
                    ret = _vm_execute_with_args(frame->vm, callee_code, args, argc);
                    
                    // ret уже имеет правильный счетчик ссылок от _vm_execute_with_args
                    
                } else if (callee_obj->type == OBJ_NATIVE_FUNCTION) {
                    // Вызываем нативную функцию
                    NativeCFunc native_func = callee_obj->as.native_function.c_func;
                    ret = native_func(frame->vm, argc, args);
                    
                    // Увеличиваем счетчик для возвращаемого значения
                    // (нативная функция создает объект через heap_alloc_*, который имеет ref_count=1)
                    if (ret && frame->vm && frame->vm->gc) {
                        gc_incref(frame->vm->gc, ret);
                    }
                    
                    // Уменьшаем счетчик на callee_obj
                    if (frame->vm && frame->vm->gc) {
                        gc_decref(frame->vm->gc, callee_obj);
                    }
                    
                } else {
                    DPRINT("[VM] ERROR: Callee is not a function (type=%d)\n", callee_obj->type);
                    if (frame->vm && frame->vm->gc) {
                        gc_decref(frame->vm->gc, callee_obj);
                    }
                    // Создаем None как результат
                    ret = vm_get_none(frame->vm);
                }
                
                // 5. Очищаем аргументы
                if (args) {
                    for (uint32_t i = 0; i < argc; i++) {
                        if (args[i] && frame->vm && frame->vm->gc) {
                            gc_decref(frame->vm->gc, args[i]);
                        }
                    }
                    free(args);
                }
                
                // 6. Если функция вернула NULL, создаем None
                if (!ret) {
                    DPRINT("[VM] WARNING: Function returned NULL, using None\n");
                    ret = vm_get_none(frame->vm);
                }
                
                DPRINT("[VM] Function result: %p (type: %d)\n", (void*)ret, ret->type);
                
                // 7. Кладем результат в стек
                frame_stack_push(frame, ret);
                // Не уменьшаем счетчик ret здесь! frame_stack_push сам увеличит счетчик
                
                break;
            }
            case RETURN_VALUE: {
                Object* val = frame_stack_pop(frame);
                // cleanup locals is handled in frame_destroy
                if (val) {
                    // caller should receive an incremented reference
                    if (frame->vm && frame->vm->gc) gc_incref(frame->vm->gc, val);
                }
                return val;
            }
            case NOP:
                break;
            case LOOP_START:
            case LOOP_END: {
                DPRINT("[VM] %s at ip=%zu\n", 
                       bc.op_code == LOOP_START ? "LOOP_START" : "LOOP_END",
                       frame->ip - 1);
                break;
            }
            case BREAK_LOOP: {
                DPRINT("[VM] BREAK_LOOP instruction executed at ip=%zu\n", frame->ip - 1);
                
                size_t search_ip = frame->ip;
                int loop_depth = 1;
                
                while (search_ip < code_arr->count) {
                    bytecode current_bc = code_arr->bytecodes[search_ip];
                    
                    if (current_bc.op_code == LOOP_START) {
                        loop_depth++;
                        DPRINT("[VM] BREAK: Found nested LOOP_START, depth=%d at ip=%zu\n", 
                               loop_depth, search_ip);
                    } 
                    else if (current_bc.op_code == LOOP_END) {
                        loop_depth--;
                        DPRINT("[VM] BREAK: Found LOOP_END, depth=%d at ip=%zu\n", 
                               loop_depth, search_ip);
                        
                        if (loop_depth == 0) {
                            frame->ip = search_ip;
                            DPRINT("[VM] BREAK: Jumping to ip=%zu (after LOOP_END)\n", frame->ip);
                            break;
                        }
                    }
                    search_ip++;
                }
                
                if (loop_depth > 0) {
                    DPRINT("[VM] WARNING: BREAK_LOOP without matching LOOP_END\n");
                }
                break;
            }
            case CONTINUE_LOOP: {
                DPRINT("[VM] CONTINUE_LOOP instruction executed at ip=%zu\n", frame->ip - 1);
                
                size_t search_ip = frame->ip;
                int loop_depth = 1;
                
                while (search_ip < code_arr->count) {
                    bytecode current_bc = code_arr->bytecodes[search_ip];
                    
                    if (current_bc.op_code == LOOP_END) {
                        loop_depth++;
                        DPRINT("[VM] CONTINUE: Found LOOP_END, depth=%d at ip=%zu\n", 
                               loop_depth, search_ip);
                    } 
                    else if (current_bc.op_code == LOOP_START) {
                        loop_depth--;
                        DPRINT("[VM] CONTINUE: Found LOOP_START, depth=%d at ip=%zu\n", 
                               loop_depth, search_ip);
                        
                        if (loop_depth == 0) {
                            frame->ip = search_ip + 1;
                            DPRINT("[VM] CONTINUE: Jumping to ip=%zu (after LOOP_START)\n", frame->ip);
                            break;
                        }
                    }
                    
                    if (search_ip == 0) break;
                    search_ip--;
                }
                
                if (loop_depth > 0) {
                    DPRINT("[VM] WARNING: CONTINUE_LOOP without matching LOOP_START\n");
                }
                break;
            }

            case JUMP_BACKWARD: {
                // Безусловный переход назад
                frame->ip -= (int32_t)arg;
                DPRINT("[VM] JUMP_BACKWARD: jumping %d instructions to ip=%zu\n", 
                       (int32_t)arg, frame->ip);
                break;
            }
            case POP_JUMP_IF_FALSE: {
                Object* condition = frame_stack_pop(frame);
                bool should_jump = false;
                
                // Проверяем, является ли значение ложным
                if (condition) {
                    if (condition->type == OBJ_BOOL) {
                        should_jump = (condition->as.bool_value == false);
                    } else if (condition->type == OBJ_INT) {
                        should_jump = (condition->as.int_value == 0);
                    } else if (condition->type == OBJ_NONE) {
                        should_jump = true;
                    }
                }
                
                // Освобождаем значение условия
                if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, condition);
                
                if (should_jump) {
                    // arg содержит смещение в инструкциях (не байтах!)
                    frame->ip += (int32_t)arg;
                    DPRINT("[VM] POP_JUMP_IF_FALSE: jumping %d instructions to ip=%zu\n", 
                           (int32_t)arg, frame->ip);
                }
                break;
            }
            
            case JUMP_FORWARD: {
                // Безусловный переход вперед
                frame->ip += (int32_t)arg;
                DPRINT("[VM] JUMP_FORWARD: jumping %d instructions to ip=%zu\n", 
                       (int32_t)arg, frame->ip);
                break;
            }            
            case POP_JUMP_IF_TRUE: {
                Object* condition = frame_stack_pop(frame);
                bool should_jump = false;
                
                // Проверяем, является ли значение истинным
                if (condition) {
                    if (condition->type == OBJ_BOOL) {
                        should_jump = (condition->as.bool_value == true);
                    } else if (condition->type == OBJ_INT) {
                        should_jump = (condition->as.int_value != 0);
                    } else if (condition->type == OBJ_NONE) {
                        should_jump = false; // None считается ложным
                    } else {
                        // Для других типов считаем истинным
                        should_jump = true;
                    }
                }
                
                if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, condition);
                if (should_jump) {
                    frame->ip += (int32_t)arg;
                }
                break;
            }
            
            case POP_JUMP_IF_NONE: {
                Object* condition = frame_stack_pop(frame);
                bool should_jump = false;
                if (condition && condition->type == OBJ_NONE) {
                    should_jump = true;
                }
                if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, condition);
                if (should_jump) {
                    frame->ip += (int32_t)arg;
                }
                break;
            }
            
            case POP_JUMP_IF_NOT_NONE: {
                Object* condition = frame_stack_pop(frame);
                bool should_jump = false;
                if (condition && condition->type != OBJ_NONE) {
                    should_jump = true;
                }
                if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, condition);
                
                if (should_jump) {
                    frame->ip += (int32_t)arg;
                }
                break;
            }
            case JUMP_BACKWARD_NO_INTERRUPT: {
                // Безусловный переход назад без проверки прерываний
                frame->ip -= (int32_t)arg;
                break;
            }
            case BUILD_ARRAY: {
               DPRINT("[VM] BUILD_ARRAY with arg=%u\n", arg);
               
               uint32_t element_count = arg;
               
               if (element_count == 0) {
                   // Создание пустого массива заданного размера
                   // Размер массива должен быть в стеке
                   Object* size_obj = frame_stack_pop(frame);
                   if (!size_obj || size_obj->type != OBJ_INT) {
                       DPRINT("[VM] ERROR: BUILD_ARRAY(0) expected integer size on stack\n");
                       if (size_obj && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, size_obj);
                       frame_stack_push(frame, vm_get_none(frame->vm));
                       break;
                   }
                   
                   int64_t size = size_obj->as.int_value;
                   DPRINT("[VM] Creating empty array with size=%lld\n", (long long)size);
                   
                   // Создаем массив
                   Object* array = heap_alloc_array_with_size(frame->vm->heap, size);
                   
                   if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, size_obj);
                   
                   if (!array) {
                       DPRINT("[VM] ERROR: Failed to allocate array\n");
                       frame_stack_push(frame, vm_get_none(frame->vm));
                       break;
                   }
                   
                   frame_stack_push(frame, array);
               } else {
                   // Создание массива из элементов на стеке (например, [1, 2, 3])
                   // Проверяем, что в стеке достаточно элементов
                   if (frame->stack_size < element_count) {
                       DPRINT("[VM] ERROR: Not enough elements on stack for BUILD_ARRAY\n");
                       frame_stack_push(frame, vm_get_none(frame->vm));
                       break;
                   }
                   
                   // Создаем массив с заданным количеством элементов
                   Object* array = heap_alloc_array_with_size(frame->vm->heap, element_count);
                   
                   if (!array) {
                       DPRINT("[VM] ERROR: Failed to allocate array\n");
                       frame_stack_push(frame, vm_get_none(frame->vm));
                       break;
                   }
                   
                   DPRINT("[VM] Allocated array with size=%u at %p\n", element_count, (void*)array);
                   
                   // Заполняем массив элементами из стека (в обратном порядке)
                   for (int32_t i = (int32_t)element_count - 1; i >= 0; i--) {
                       Object* element = frame_stack_pop(frame);
                       
                       DPRINT("[VM] Popping element %d: %p\n", i, (void*)element);
                       
                       if (element) {
                           // Сохраняем элемент в массив
                           if (i < (int64_t)array->as.array.size) {
                               // Увеличиваем счетчик для элемента
                               if (frame->vm && frame->vm->gc) {
                                   gc_incref(frame->vm->gc, element);
                               }
                               
                               // Заменяем элемент (массив инициализирован None элементами)
                               Object* old_element = array->as.array.items[i];
                               if (old_element && frame->vm && frame->vm->gc) {
                                   gc_decref(frame->vm->gc, old_element);
                               }
                               
                               array->as.array.items[i] = element;
                           }
                           
                           // Уменьшаем счетчик после добавления в массив
                           if (frame->vm && frame->vm->gc) {
                               gc_decref(frame->vm->gc, element);
                           }
                       }
                       // Если element == NULL, оставляем None (уже установлено)
                   }
                   
                   // Кладем массив на стек
                   frame_stack_push(frame, array);
               }
               break;
           }
            case STORE_SUBSCR: {
                DPRINT("[VM] STORE_SUBSCR\n");
                
                // В стеке должны быть: значение, массив, индекс (в таком порядке)
                // STORE_SUBSCR arg=0 означает сохранение элемента в массив
                Object* index_obj = frame_stack_pop(frame);    // ПОП: индекс
                Object* array_obj = frame_stack_pop(frame);    // ПОП: массив
                Object* value_obj = frame_stack_pop(frame);    // ПОП: значение                
                if (!array_obj || !index_obj || !value_obj) {
                    DPRINT("[VM] ERROR: STORE_SUBSCR missing array, index or value\n");
                    if (value_obj && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, value_obj);
                    if (array_obj && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, array_obj);
                    if (index_obj && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, index_obj);
                    break;
                }
                
                // Проверяем типы
                if (array_obj->type != OBJ_ARRAY) {
                    DPRINT("[VM] ERROR: STORE_SUBSCR expected array, got type=%d\n", array_obj->type);
                    if (frame->vm && frame->vm->gc) {
                        gc_decref(frame->vm->gc, value_obj);
                        gc_decref(frame->vm->gc, array_obj);
                        gc_decref(frame->vm->gc, index_obj);
                    }
                    break;
                }
                
                int64_t index = 0;
                if (index_obj->type == OBJ_INT) {
                    index = index_obj->as.int_value;
                } else {
                    DPRINT("[VM] ERROR: STORE_SUBSCR index must be integer, got type=%d\n", index_obj->type);
                    if (frame->vm && frame->vm->gc) {
                        gc_decref(frame->vm->gc, value_obj);
                        gc_decref(frame->vm->gc, array_obj);
                        gc_decref(frame->vm->gc, index_obj);
                    }
                    break;
                }
                
                // Проверяем границы массива
                if (index < 0 || index >= (int64_t)array_obj->as.array.size) {
                    DPRINT("[VM] ERROR: STORE_SUBSCR index %lld out of bounds (size=%zu)\n", 
                           (long long)index, array_obj->as.array.size);
                    if (frame->vm && frame->vm->gc) {
                        gc_decref(frame->vm->gc, value_obj);
                        gc_decref(frame->vm->gc, array_obj);
                        gc_decref(frame->vm->gc, index_obj);
                    }
                    break;
                }
                
                // Заменяем элемент в массиве
                Object* old_element = array_obj->as.array.items[index];
                
                // Увеличиваем счетчик для нового значения
                if (frame->vm && frame->vm->gc) {
                    gc_incref(frame->vm->gc, value_obj);
                }
                
                // Устанавливаем новое значение
                array_obj->as.array.items[index] = value_obj;
                
                // Уменьшаем счетчик для старого значения
                if (old_element && frame->vm && frame->vm->gc) {
                    gc_decref(frame->vm->gc, old_element);
                }
                
                // Очищаем временные объекты
                if (frame->vm && frame->vm->gc) {
                    // array_obj остается владеть value_obj
                    gc_decref(frame->vm->gc, array_obj);
                    gc_decref(frame->vm->gc, index_obj);
                    // value_obj не удаляем здесь - он теперь принадлежит массиву
                }
                
                // STORE_SUBSCR не оставляет значения на стеке
                break;
            }
            case DEL_SUBSCR: {
                DPRINT("[VM] DEL_SUBSCR\n");
                
                // В стеке должны быть: массив, индекс
                Object* index_obj = frame_stack_pop(frame);
                Object* array_obj = frame_stack_pop(frame);
                
                if (!array_obj || !index_obj) {
                    DPRINT("[VM] ERROR: DEL_SUBSCR missing array or index\n");
                    if (index_obj && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, index_obj);
                    if (array_obj && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, array_obj);
                    break;
                }
                
                // Проверяем типы
                if (array_obj->type != OBJ_ARRAY) {
                    DPRINT("[VM] ERROR: DEL_SUBSCR expected array, got type=%d\n", array_obj->type);
                    if (frame->vm && frame->vm->gc) {
                        gc_decref(frame->vm->gc, index_obj);
                        gc_decref(frame->vm->gc, array_obj);
                    }
                    break;
                }
                
                // Преобразуем индекс в int
                int64_t index = 0;
                if (index_obj->type == OBJ_INT) {
                    index = index_obj->as.int_value;
                } else {
                    DPRINT("[VM] ERROR: DEL_SUBSCR index must be integer, got type=%d\n", index_obj->type);
                    if (frame->vm && frame->vm->gc) {
                        gc_decref(frame->vm->gc, index_obj);
                        gc_decref(frame->vm->gc, array_obj);
                    }
                    break;
                }
                
                // Проверяем границы массива
                if (index < 0 || index >= (int64_t)array_obj->as.array.size) {
                    DPRINT("[VM] ERROR: DEL_SUBSCR index %lld out of bounds (size=%zu)\n", 
                           (long long)index, array_obj->as.array.size);
                    if (frame->vm && frame->vm->gc) {
                        gc_decref(frame->vm->gc, index_obj);
                        gc_decref(frame->vm->gc, array_obj);
                    }
                    break;
                }
                
                // Удаляем элемент (устанавливаем в None)
                Object* old_element = array_obj->as.array.items[index];
                
                // Устанавливаем элемент в None
                array_obj->as.array.items[index] = vm_get_none(frame->vm);
                
                // Уменьшаем счетчик для старого значения
                if (old_element && frame->vm && frame->vm->gc) {
                    gc_decref(frame->vm->gc, old_element);
                }
                
                // Очищаем временные объекты
                if (frame->vm && frame->vm->gc) {
                    gc_decref(frame->vm->gc, index_obj);
                    gc_decref(frame->vm->gc, array_obj);
                }
                
                // DEL_SUBSCR не оставляет значения на стеке
                break;
            }
            case LOAD_SUBSCR: {
                DPRINT("[VM] LOAD_SUBSCR\n");
                
                // В стеке должны быть: индекс, массив
                Object* index_obj = frame_stack_pop(frame);
                Object* array_obj = frame_stack_pop(frame);
                
                if (!array_obj || !index_obj) {
                    DPRINT("[VM] ERROR: LOAD_SUBSCR missing array or index\n");
                    if (index_obj && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, index_obj);
                    if (array_obj && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, array_obj);
                    frame_stack_push(frame, vm_get_none(frame->vm));
                    break;
                }
                
                // Проверяем типы
                if (array_obj->type != OBJ_ARRAY) {
                    DPRINT("[VM] ERROR: LOAD_SUBSCR expected array, got type=%d\n", array_obj->type);
                    if (frame->vm && frame->vm->gc) {
                        gc_decref(frame->vm->gc, index_obj);
                        gc_decref(frame->vm->gc, array_obj);
                    }
                    frame_stack_push(frame, vm_get_none(frame->vm));
                    break;
                }
                
                // Преобразуем индекс в int
                int64_t index = 0;
                if (index_obj->type == OBJ_INT) {
                    index = index_obj->as.int_value;
                } else {
                    DPRINT("[VM] ERROR: LOAD_SUBSCR index must be integer, got type=%d\n", index_obj->type);
                    if (frame->vm && frame->vm->gc) {
                        gc_decref(frame->vm->gc, index_obj);
                        gc_decref(frame->vm->gc, array_obj);
                    }
                    frame_stack_push(frame, vm_get_none(frame->vm));
                    break;
                }
                
                // Проверяем границы массива
                if (index < 0 || index >= (int64_t)array_obj->as.array.size) {
                    DPRINT("[VM] ERROR: LOAD_SUBSCR index %lld out of bounds (size=%zu)\n", 
                           (long long)index, array_obj->as.array.size);
                    if (frame->vm && frame->vm->gc) {
                        gc_decref(frame->vm->gc, index_obj);
                        gc_decref(frame->vm->gc, array_obj);
                    }
                    frame_stack_push(frame, vm_get_none(frame->vm));
                    break;
                }
                
                // Получаем элемент из массива
                Object* element = array_obj->as.array.items[index];
                
                // Увеличиваем счетчик для возвращаемого элемента
                if (element && frame->vm && frame->vm->gc) {
                    gc_incref(frame->vm->gc, element);
                }
                
                // Очищаем временные объекты
                if (frame->vm && frame->vm->gc) {
                    gc_decref(frame->vm->gc, index_obj);
                    gc_decref(frame->vm->gc, array_obj);
                }
                
                // Кладем результат на стек
                frame_stack_push(frame, element ? element : vm_get_none(frame->vm));
                break;
            }
            default:
                // For unsupported operations, print and continue
                DPRINT("VM: Unsupported op code: 0x%02X\n", bc.op_code);
                break;
        }
    }

    // end of code without explicit return -> return None
    Object* nonev = vm_get_none(frame->vm);
    if (frame->vm && frame->vm->gc) gc_incref(frame->vm->gc, nonev);
    return nonev;
}

// Helper to run code with passed-in args assigned to initial locals
static Object* _vm_execute_with_args(VM* vm, CodeObj* code, Object** args, size_t argc) {
    if (!vm || !code) return NULL;
    Frame* frame = frame_create(vm, code);
    if (!frame) return NULL;
    // assign args to locals
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
