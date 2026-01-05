#ifndef BUILTINS_H
#define BUILTINS_H

typedef struct VM VM;
typedef struct Object Object;

Object* builtin_print(VM* vm, int arg_count, Object** args);
Object* builtin_input(VM* vm, int arg_count, Object** args);
Object* builtin_randint(VM* vm, int arg_count, Object** args);
Object* builtin_sqrt(VM* vm, int arg_count, Object** args);

#endif
