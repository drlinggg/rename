### THIS IS ALPHA VERSION AND IT CAN AND SHOULD BE CHANGED LATER

### instructions format:
```
opcode (1 byte) | arg (3 bytes) |
```

## Примеры байткода

### Пример 1: Простое выражение
```python
# Исходный код: x = 10 + 20
LOAD_CONST     0   # 10
LOAD_CONST     1   # 20  
BINARY_OP      0   # Сложение
STORE_FAST     0   # Сохраняем в x
```

### Пример 2: Условное выражение
```python
# if (a > b): 
LOAD_FAST      0   # a
LOAD_FAST      1   # b
COMPARE_OP     4   # > (greater than)
POP_JUMP_IF_FALSE 10  # Переход если false
# then блок
LOAD_CONST     2   # then value
JUMP_FORWARD    5   # Пропускаем else
# else блок  
LOAD_CONST     3   # else value
```

### Пример 3: Вызов функции
```python
# result = add(5, 3)
LOAD_GLOBAL    0   # Функция add
LOAD_CONST     0   # Аргумент 5
LOAD_CONST     1   # Аргумент 3
CALL_FUNCTION  2   # Вызов с 2 аргументами
STORE_FAST     0   # Сохраняем результат
```

## bytecode operations documentation:

### - LOAD_FAST 0x01:
push(locals[arg])

### - LOAD_CONST 0x02:
push(arg)

### - LOAD_GLOBAL 0x03
push_null = arg % 2 != 0 # if function => pushing null
push(globals[arg>>1] or builtins[arg>>1])
lowest bit says if you need to push null

### - LOAD_NAME 0x04
push(locals[arg] or globals[arg] or builtins[arg])

### - STORE_FAST 0x05
locals[arg] = pop().

### - STORE_GLOBAL 0x06
globals[arg] = pop().

### - STORE_NAME 0x07
???

### - BINARY_OP 0x08
right = pop()
left = pop()
push(left arg right)
```
   op_map = {
        0x00: '+',    # BINARY_OP_ADD
        0x01: '&',    # BINARY_OP_AND
        0x02: '//',   # BINARY_OP_FLOOR_DIVIDE
        0x03: '<<',   # BINARY_OP_LSHIFT
        0x04: '@',    # BINARY_OP_MATRIX_MULTIPLY
        0x05: '*',    # BINARY_OP_MULTIPLY
        0x06: '%',    # BINARY_OP_REMAINDER
        0x07: '|',    # BINARY_OP_OR
        0x08: '**',   # BINARY_OP_POWER
        0x09: '>>',   # BINARY_OP_RSHIFT
        0x0A: '-',   # BINARY_OP_SUBTRACT
        0x0B: '/',   # BINARY_OP_TRUE_DIVIDE
        0x0C: '^',   # BINARY_OP_XOR
        # Inplace operations
        0x0D: '+',   # BINARY_OP_INPLACE_ADD
        0x0E: '&',   # BINARY_OP_INPLACE_AND
        0x0F: '//',  # BINARY_OP_INPLACE_FLOOR_DIVIDE
        0x10: '<<',  # BINARY_OP_INPLACE_LSHIFT
        0x11: '@',   # BINARY_OP_INPLACE_MATRIX_MULTIPLY
        0x12: '*',   # BINARY_OP_INPLACE_MULTIPLY
        0x13: '%',   # BINARY_OP_INPLACE_REMAINDER
        0x14: '|',   # BINARY_OP_INPLACE_OR
        0x15: '**',  # BINARY_OP_INPLACE_POWER
        0x16: '>>',  # BINARY_OP_INPLACE_RSHIFT
        0x17: '-',   # BINARY_OP_INPLACE_SUBTRACT
        0x18: '/',   # BINARY_OP_INPLACE_TRUE_DIVIDE
        0x19: '^',   # BINARY_OP_INPLACE_XOR
    }
```

### - CALL_FUNCTION 0x09
callee = pop()
null = pop()
args = n times pop()
push(callee(args))

### - TO_BOOL 0x0A
push(bool(pop()))

### - TO_INT 0x0B
push(int(pop()))

### - TO_LONG 0x0C
push(long(pop()))

### - STORE_SUBSCR 0x0D
key = pop()
container = pop()
value = pop()
container[key] = value

### - DEL_SUBSCR 0x0E
key = STACK.pop()
container = STACK.pop()
del container[key]

### - RETURN_VALUE 0x0F
return_value = pop()

### - NOP 0x10
nothing

### - POP_TOP 0x11
stack.pop()

### - END_FOR 0x12
similar to pop_top

### - COPY 0x13
assert arg > 0
STACK.push(STACK[-i])

### - SWAP 0x14
STACK[-i], STACK[-1] = STACK[-1], STACK[-i]

### - UNARY_OP 0x15
```
   unary_op_map = {
        0x00: '+',    # UNARY_POSITIVE
        0x01: '-',    # UNARY_NEGATIVE
        0x02: '~',    # UNARY_INVERT # not implemented yet
        0x03: 'not',  # UNARY_NOT
    }
```
stack[-1] = unary_op(stack[-1])

### - FREE_TO_SET 0x16
free pos for bytecode

### - BUILD_ARRAY 0x17
if arg == 0:
    value = ()
else:
    value = array(elements=STACK[-arg:])
    STACK = STACK.pop arg times

STACK.append(value)

### - COMPARE_OP 0x18

### - JUMP_FORWARD 0x19
Increments instruction pointer by `arg` (offset).

### - JUMP_BACKWARD 0x1A  
Decrements instruction pointer by `arg` (offset). Checks for interrupts.

### - JUMP_BACKWARD_NO_INTERRUPT 0x1B
Decrements instruction pointer by `arg` (offset). Does not check for interrupts.

### - POP_JUMP_IF_TRUE 0x1C
If `STACK[-1]` is true, increments instruction pointer by `arg`. Pops `STACK[-1]`.

### - POP_JUMP_IF_FALSE 0x1D
If `STACK[-1]` is false, increments instruction pointer by `arg`. Pops `STACK[-1]`.

### - POP_JUMP_IF_NOT_NONE 0x1E
If `STACK[-1]` is not None, increments instruction pointer by `arg`. Pops `STACK[-1]`.

### - POP_JUMP_IF_NONE 0x1F
If `STACK[-1]` is None, increments instruction pointer by `arg`. Pops `STACK[-1]`.

### - PUSH_NULL 0x20
Pushes a NULL to the stack. Used for function call preparation.

### - MAKE_FUNCTION 0x21
Pushes a new function object on the stack built from the code object at `STACK[-1].`
