#include "vm.h"
#include "../../runtime/gc/gc.h"
#include "../../runtime/jit/jit.h"
#include <stdio.h>
#include "../../debug.h"
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
    if (o && frame->vm && frame->vm->gc) gc_incref(frame->vm->gc, o);
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
        f->locals[i] = vm->heap ? heap_alloc_none(vm->heap) : NULL;
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
                } else {
                    o = heap_from_value(frame->vm->heap, c);
                }
    
                frame_stack_push(frame, o);
                break;
            }
            case LOAD_FAST: {
                if (arg >= code->local_count) {
                    DPRINT("VM: LOAD_FAST index out of range %u\n", arg);
                    frame_stack_push(frame, heap_alloc_none(frame->vm->heap));
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
                if (frame->vm && frame->vm->gc) gc_incref(frame->vm->gc, v);
                if (frame->locals[arg] && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, frame->locals[arg]);
                frame->locals[arg] = v;
                break;
            }
            case LOAD_GLOBAL: {
                size_t gidx = arg >> 1;
                Object* val = vm_get_global(frame->vm, gidx);
                if (!val) val = heap_alloc_none(frame->vm->heap);
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
                if (!right) right = heap_alloc_none(frame->vm->heap);
                if (!left) left = heap_alloc_none(frame->vm->heap);
                Object* ret = NULL;
                if (left->type == OBJ_INT && right->type == OBJ_INT) {
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
                        case 0x0B: // DIV
                            if (b == 0) ret = heap_alloc_int(frame->vm->heap, 0);
                            else ret = heap_alloc_int(frame->vm->heap, a / b);
                            break;
                        default:
                            DPRINT("VM: Unsupported binary_op on ints: %u\n", op);
                            ret = heap_alloc_none(frame->vm->heap);
                            break;
                    }
                } else {
                    // fallback: string concat if strings? not implemented.
                    DPRINT("VM: BINARY_OP for non-int operands not implemented\n");
                    ret = heap_alloc_none(frame->vm->heap);
                }
                // cleanup
                if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, left);
                if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, right);
                frame_stack_push(frame, ret);
                break;
            }
            case PUSH_NULL: {
                frame_stack_push(frame, heap_alloc_none(frame->vm->heap));
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
                    frame_stack_push(frame, heap_alloc_none(frame->vm->heap));
                }
                break;
            }
            case CALL_FUNCTION: {
                uint32_t argc = arg;
                // pop args into array; last arg popped goes into args[argc-1]
                Object** args = NULL;
                if (argc > 0) args = malloc(argc * sizeof(Object*));
                for (int i = (int)argc - 1; i >= 0; i--) {
                    args[i] = frame_stack_pop(frame);
                }
                // pop null
                Object* maybe_null = frame_stack_pop(frame);
                if (maybe_null && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, maybe_null);
                // pop callee
                Object* callee_obj = frame_stack_pop(frame);
                Object* ret = heap_alloc_none(frame->vm->heap);
                if (callee_obj && callee_obj->type == OBJ_FUNCTION) {

                    CodeObj* callee_code = callee_obj->as.function.codeptr;
                    // prepare local values
                    // create a new frame by recursively calling vm_execute
                    // but we need to set its locals to args
                    // We'll simulate by creating a copy of callee_code where locals[0..argc-1] are set to args

                    // Create a temporary frame locals allocation
                    // no-op

                    // when executing nested function, vm_execute will create its own locals
                    // but we want to pass arguments as initial locals -> approach: push args on vm->stack, but vm_execute reads only constants and locals; we need to set the locals array
                    // For simplicity, we will create a small helper that can execute CodeObj with args assigned into locals

                    // We implement a local helper: run_function
                    // Release this call's reference to callee_obj
                    if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, callee_obj);

                    // Run nested function
                    // push args into new locals by creating new local array inside vm_execute

                    // We'll call a new helper _vm_execute_with_args
                    ret = _vm_execute_with_args(frame->vm, callee_code, args, argc);
                } else {
                    if (callee_obj && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, callee_obj);
                    DPRINT("VM: CALL_FUNCTION callee not a function\n");
                }
                // cleanup args array
                for (uint32_t i = 0; i < argc; i++) if (args && args[i] && frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, args[i]);
                free(args);
                frame_stack_push(frame, ret);
                if (frame->vm && frame->vm->gc) gc_decref(frame->vm->gc, ret);
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
            case NOP: {
                break;
            }
            default:
                // For unsupported operations, print and continue
                DPRINT("VM: Unsupported op code: 0x%02X\n", bc.op_code);
                break;
        }
    }

    // end of code without explicit return -> return None
    Object* nonev = heap_alloc_none(frame->vm->heap);
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
