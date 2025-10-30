### THIS IS ALPHA VERSION AND IT CAN AND SHOULD BE CHANGED LATER

### instructions format:
```
opcode (1 байт) | arg (3 байта) |
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

### - load_fast 0x01:
push(locals[arg])

### - load_const 0x02:
push(arg)

### - load_global 0x03
push_null = arg % 2 != 0 # if function => pushing null
push(globals[arg>>1] or builtins[arg>>1])
lowest bit says if you need to push null

### - load name 0x04
push(locals[arg] or globals[arg] or builtins[arg])

### - store_fast 0x05
locals[arg] = pop().

### - store_global 0x06
globals[arg] = pop().

### - store_name 0x07
???

### - BINARY_OP 0x08
right = pop()
left = pop()
push(left arg right)
```
       op_map = {
            0: '+',    # BINARY_OP_ADD
            1: '&',    # BINARY_OP_AND
            2: '//',   # BINARY_OP_FLOOR_DIVIDE
            3: '<<',   # BINARY_OP_LSHIFT
            4: '@',    # BINARY_OP_MATRIX_MULTIPLY
            5: '*',    # BINARY_OP_MULTIPLY
            6: '%',    # BINARY_OP_REMAINDER
            7: '|',    # BINARY_OP_OR
            8: '**',   # BINARY_OP_POWER
            9: '>>',   # BINARY_OP_RSHIFT
            10: '-',   # BINARY_OP_SUBTRACT
            11: '/',   # BINARY_OP_TRUE_DIVIDE
            12: '^',   # BINARY_OP_XOR
            # Inplace operations
            13: '+',   # BINARY_OP_INPLACE_ADD
            14: '&',   # BINARY_OP_INPLACE_AND
            15: '//',  # BINARY_OP_INPLACE_FLOOR_DIVIDE
            16: '<<',  # BINARY_OP_INPLACE_LSHIFT
            17: '@',   # BINARY_OP_INPLACE_MATRIX_MULTIPLY
            18: '*',   # BINARY_OP_INPLACE_MULTIPLY
            19: '%',   # BINARY_OP_INPLACE_REMAINDER
            20: '|',   # BINARY_OP_INPLACE_OR
            21: '**',  # BINARY_OP_INPLACE_POWER
            22: '>>',  # BINARY_OP_INPLACE_RSHIFT
            23: '-',   # BINARY_OP_INPLACE_SUBTRACT
            24: '/',   # BINARY_OP_INPLACE_TRUE_DIVIDE
            25: '^',   # BINARY_OP_INPLACE_XOR
        }
```

### - CALL_FUNCTION_OP 0x09
callee = pop()
null = pop()
args = n times pop()
push(callee(args))

### - to_bool_op 0x0A
push(bool(pop()))

### - to_int_op 0x0B
push(int(pop()))

### - to_long_op 0x0C
push(long(pop()))

### - STORE_SUBSCR 0x0D
key = pop()
container = pop()
value = pop()
container[key] = value

### - del_subscr 0x0E
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

### - unary_negative 0x15
STACK[-1] = -STACK[-1]

### - UNARY_NOT 0x16
STACK[-1] = not STACK[-1]

### - BUILD_LIST 0x17
if arg == 0:
    value = ()
else:
    value = array(elements=STACK[-arg:])
    STACK = STACK.pop arg times

STACK.append(value)

### - COMPARE_OP 0x18
```
        match arg:
            case '<':
                result = self.compare_lt_op(None)
            case '<=':
                result = self.compare_le_op(None)
            case '==':
                result = self.compare_eq_op(None)
            case '!=':
                result = self.compare_ne_op(None)
            case '>':
                result = self.compare_gt_op(None)
            case '>=':
                result = self.compare_ge_op(None)
            case 'in':
                result = self.compare_in_op(None)
            case 'not in':
                result = self.compare_not_in_op(None)
            case _:
                raise NotImplementedError(f"Compare operation {op} not implemented")
```

### - JUMP_FORWARD 0x19
Increments bytecode counter by `arg` (offset).

### - JUMP_BACKWARD 0x1A  
Decrements bytecode counter by `arg` (offset). Checks for interrupts.

### - JUMP_BACKWARD_NO_INTERRUPT 0x1B
Decrements bytecode counter by `arg` (offset). Does not check for interrupts.

### - POP_JUMP_IF_TRUE 0x1C
If `STACK[-1]` is true, increments bytecode counter by `arg`. Pops `STACK[-1]`.

### - POP_JUMP_IF_FALSE 0x1D
If `STACK[-1]` is false, increments bytecode counter by `arg`. Pops `STACK[-1]`.

### - POP_JUMP_IF_NOT_NONE 0x1E
If `STACK[-1]` is not None, increments bytecode counter by `arg`. Pops `STACK[-1]`.

### - POP_JUMP_IF_NONE 0x1F
If `STACK[-1]` is None, increments bytecode counter by `arg`. Pops `STACK[-1]`.

### - PUSH_NULL 0x20
Pushes a NULL to the stack. Used for function call preparation.
