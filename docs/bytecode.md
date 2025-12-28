# Bytecode Documentation

## Instruction Format
Each instruction is 4 bytes:
```
opcode (1 byte) | argument (3 bytes)
```

The 3-byte argument can represent:
- **Local variable index** (for LOAD_FAST, STORE_FAST)
- **Constant index** (for LOAD_CONST)
- **Global name index** (for LOAD_GLOBAL, STORE_GLOBAL)
- **Jump offset** (for JUMP_*, POP_JUMP_*)
- **Operator code** (for BINARY_OP, UNARY_OP)
- **Argument count** (for CALL_FUNCTION, BUILD_ARRAY)

## Instruction Set

### 1. Load and Store Operations

#### **LOAD_FAST** (0x01)
Load a local variable onto the stack.
- **Operation**: `push(locals[arg])`
- **Argument**: Local variable index

#### **LOAD_CONST** (0x02)
Load a constant onto the stack.
- **Operation**: `push(constants[arg])`
- **Argument**: Constant pool index

#### **LOAD_GLOBAL** (0x03)
Load a global variable onto the stack.
- **Operation**: 
  ```python
  push_null = (arg % 2 != 0)  # Lowest bit indicates null push
  push(globals[arg >> 1] or builtins[arg >> 1])
  ```
- **Argument**: Global name index with null flag in LSB

#### **LOAD_NAME** (0x04)
Load a name from locals, globals, or builtins.
- **Operation**: `push(locals[arg] or globals[arg] or builtins[arg])`
- **Argument**: Name index

#### **STORE_FAST** (0x05)
Store a value into a local variable.
- **Operation**: `locals[arg] = pop()`
- **Argument**: Local variable index

#### **STORE_GLOBAL** (0x06)
Store a value into a global variable.
- **Operation**: `globals[arg] = pop()`
- **Argument**: Global name index

#### **STORE_NAME** (0x07)
Store a name in the appropriate namespace.
- **Operation**: `names[arg] = pop()`
- **Argument**: Name index

### 2. Array Operations

#### **BUILD_ARRAY** (0x17)
Build an array from stack elements.
- **Operation**:
  ```python
  if arg == 0:
      value = []
  else:
      value = [STACK[-arg:]]  # Create array from top 'arg' elements
      STACK = STACK[:-arg]    # Remove elements from stack
  STACK.append(value)
  ```
- **Argument**: Number of elements to pop from stack

#### **STORE_SUBSCR** (0x0D)
Store into a subscripted element.
- **Operation**:
  ```python
  key = pop()        # Index
  container = pop()  # Array
  value = pop()      # Value to store
  container[key] = value
  ```
- **Argument**: Unused (0)

#### **LOAD_SUBSCR** (0x18)
Load a subscripted element.
- **Operation**:
  ```python
  key = pop()        # Index
  container = pop()  # Array
  value = container[key]
  push(value)
  ```
- **Argument**: Unused (0)

#### **DEL_SUBSCR** (0x0E)
Delete a subscripted element.
- **Operation**:
  ```python
  key = pop()
  container = pop()
  del container[key]
  ```
- **Argument**: Unused (0)

### 3. Function Operations

#### **CALL_FUNCTION** (0x09)
Call a function.
- **Operation**:
  ```python
  callee = pop()      # Function to call
  null = pop()        # Null sentinel
  args = [pop() for _ in range(arg)]  # Arguments
  result = callee(*args)
  push(result)
  ```
- **Argument**: Number of arguments

#### **MAKE_FUNCTION** (0x21)
Create a function object from a code object.
- **Operation**: `push(FunctionObject(STACK[-1]))`
- **Argument**: Unused (0)

#### **PUSH_NULL** (0x20)
Push a null sentinel onto the stack.
- **Operation**: `push(None)`
- **Argument**: Unused (0)

#### **RETURN_VALUE** (0x0F)
Return from a function.
- **Operation**: `return pop()`
- **Argument**: Unused (0)

### 4. Binary Operations

#### **BINARY_OP** (0x08)
Perform a binary operation.
- **Operation**: `right = pop(); left = pop(); push(left op right)`
- **Argument**: Operator code

**Binary Operator Codes:**
```
0x00: '+'    # ADD
0x0A: '-'    # SUBTRACT
0x05: '*'    # MULTIPLY
0x0B: '/'    # TRUE_DIVIDE
0x06: '%'    # REMAINDER
0x50: '=='   # EQUAL
0x51: '!='   # NOT_EQUAL
0x52: '<'    # LESS_THAN
0x53: '<='   # LESS_EQUAL
0x54: '>'    # GREATER_THAN
0x55: '>='   # GREATER_EQUAL
0x56: 'is'   # IDENTITY
0x60: 'and'  # LOGICAL_AND
0x61: 'or'   # LOGICAL_OR

# Inplace operations (not currently used)
0x0D: '+='   # INPLACE_ADD
0x17: '-='   # INPLACE_SUBTRACT
0x12: '*='   # INPLACE_MULTIPLY
0x18: '/='   # INPLACE_TRUE_DIVIDE
0x13: '%='   # INPLACE_REMAINDER

# Bitwise operations (not currently used)
0x01: '&'    # AND
0x07: '|'    # OR
0x0C: '^'    # XOR
0x03: '<<'   # LSHIFT
0x09: '>>'   # RSHIFT

# Special operations
0x02: '//'   # FLOOR_DIVIDE
0x04: '@'    # MATRIX_MULTIPLY
0x08: '**'   # POWER
```

### 5. Unary Operations

#### **UNARY_OP** (0x15)
Perform a unary operation.
- **Operation**: `value = pop(); push(op value)`
- **Argument**: Operator code

**Unary Operator Codes:**
```
0x00: '+'    # POSITIVE
0x01: '-'    # NEGATIVE
0x03: 'not'  # LOGICAL_NOT
```

### 6. Type Conversion

#### **TO_BOOL** (0x0A)
Convert value to boolean.
- **Operation**: `push(bool(pop()))`
- **Argument**: Unused (0)

#### **TO_INT** (0x0B)
Convert value to integer.
- **Operation**: `push(int(pop()))`
- **Argument**: Unused (0)

#### **TO_LONG** (0x0C)
Convert value to long integer.
- **Operation**: `push(long(pop()))`
- **Argument**: Unused (0)

### 7. Control Flow Operations

#### **JUMP_FORWARD** (0x19)
Jump forward by offset.
- **Operation**: `instruction_pointer += arg`
- **Argument**: Offset in instructions

#### **JUMP_BACKWARD** (0x1A)
Jump backward by offset (checks for interrupts).
- **Operation**: `instruction_pointer -= arg`
- **Argument**: Offset in instructions

#### **JUMP_BACKWARD_NO_INTERRUPT** (0x1B)
Jump backward without interrupt check.
- **Operation**: `instruction_pointer -= arg`
- **Argument**: Offset in instructions

#### **POP_JUMP_IF_TRUE** (0x1C)
Jump if top of stack is true.
- **Operation**: `if pop(): instruction_pointer += arg`
- **Argument**: Jump offset

#### **POP_JUMP_IF_FALSE** (0x1D)
Jump if top of stack is false.
- **Operation**: `if not pop(): instruction_pointer += arg`
- **Argument**: Jump offset

#### **POP_JUMP_IF_NOT_NONE** (0x1E)
Jump if top of stack is not None.
- **Operation**: `if pop() is not None: instruction_pointer += arg`
- **Argument**: Jump offset

#### **POP_JUMP_IF_NONE** (0x1F)
Jump if top of stack is None.
- **Operation**: `if pop() is None: instruction_pointer += arg`
- **Argument**: Jump offset

### 8. Loop Operations

#### **LOOP_START** (0x24)
Mark the start of a loop block.
- **Operation**: Marker instruction for loop start
- **Argument**: Unused (0)

#### **LOOP_END** (0x25)
Mark the end of a loop block.
- **Operation**: Marker instruction for loop end
- **Argument**: Unused (0)

#### **BREAK_LOOP** (0x33)
Break out of the current loop.
- **Operation**: Exit current loop
- **Argument**: Unused (0)

#### **CONTINUE_LOOP** (0x44)
Continue to next loop iteration.
- **Operation**: Jump to loop start
- **Argument**: Unused (0)

### 9. Stack Manipulation

#### **POP_TOP** (0x11)
Remove the top element from the stack.
- **Operation**: `pop()`
- **Argument**: Unused (0)

#### **COPY** (0x13)
Copy an element from the stack.
- **Operation**: `assert arg > 0; push(STACK[-arg])`
- **Argument**: Position from stack top (1-based)

#### **SWAP** (0x14)
Swap stack elements.
- **Operation**: `STACK[-arg], STACK[-1] = STACK[-1], STACK[-arg]`
- **Argument**: Position to swap with top (1-based)

#### **NOP** (0x10)
No operation.
- **Operation**: Do nothing
- **Argument**: Unused (0)

#### **FREE_TO_SET** (0x16)
Reserved for future use.
- **Operation**: No operation
- **Argument**: Unused (0)

#### **COMPARE_AND_SWAP** (0xF0)
Atomic compare-and-swap operation.
- **Operation**: Reserved for concurrency
- **Argument**: Unused (0)

## Value Types in Constants Pool

The compiler maintains a constants pool containing:
- `VAL_INT`: 64-bit integers
- `VAL_FLOAT`: Floating-point numbers (stored as strings for precision)
- `VAL_BOOL`: Boolean values
- `VAL_NONE`: None/null value
- `VAL_CODE`: Code objects for functions

## Code Object Structure

```c
typedef struct CodeObj {
    bytecode_array code;     // Compiled bytecode
    char* name;              // Function name
    uint8_t arg_count;       // Number of parameters
    uint8_t local_count;     // Number of local variables
    Value* constants;        // Constant pool
    size_t constants_count;  // Number of constants
} CodeObj;
```

## Memory Layout

### Local Variables
- Indexed by integer in `LOAD_FAST`/`STORE_FAST`
- Stored in function's local frame
- Scope-limited to function execution

### Global Variables
- Indexed by integer in `LOAD_GLOBAL`/`STORE_GLOBAL`
- Shared across the entire program
- Includes built-in functions

### Constants
- Indexed by integer in `LOAD_CONST`
- Immutable values stored in code object
- Includes literals and compiled code objects

## Bytecode Examples

### Example 1: Simple Expression
```
// Source: x = 10 + 20
LOAD_CONST     0      # Push 10
LOAD_CONST     1      # Push 20  
BINARY_OP      0x00   # Addition (+)
STORE_FAST     0      # Store in local variable 0 (x)
```

### Example 2: Function Call
```
// Source: result = add(5, 3)
LOAD_GLOBAL    0      # Load 'add' function (index 0)
PUSH_NULL            # Push null for call
LOAD_CONST     0      # Push argument 5
LOAD_CONST     1      # Push argument 3
CALL_FUNCTION  2      # Call with 2 arguments
STORE_FAST     0      # Store result in local 0
```

### Example 3: If Statement
```
// Source: if (a > b) { x = 1 } else { x = 2 }
LOAD_FAST      0      # Load a
LOAD_FAST      1      # Load b
BINARY_OP      0x52   # Compare (>)
POP_JUMP_IF_FALSE 5   # Jump to else if false
LOAD_CONST     0      # Push 1 (then branch)
STORE_FAST     2      # Store in x
JUMP_FORWARD    3     # Skip else branch
LOAD_CONST     1      # Push 2 (else branch)
STORE_FAST     2      # Store in x
```

### Example 4: While Loop
```
// Source: while (i < 10) { i = i + 1 }
LOOP_START            # Loop start marker
LOAD_FAST      0      # Load i
LOAD_CONST     0      # Push 10
BINARY_OP      0x52   # Compare (<)
POP_JUMP_IF_FALSE 6   # Exit loop if false
LOAD_FAST      0      # Load i
LOAD_CONST     1      # Push 1
BINARY_OP      0x00   # Add
STORE_FAST     0      # Store i = i + 1
JUMP_BACKWARD  8      # Jump back to loop start
LOOP_END              # Loop end marker
```

### Example 5: Array Operations
```
// Source: arr[2] = 42
LOAD_CONST     0      # Push 42
LOAD_FAST      0      # Load arr (local 0)
LOAD_CONST     1      # Push index 2
STORE_SUBSCR          # Store arr[2] = 42
```

## Implementation Notes

### Jump Offsets
- All jump offsets are measured in **instructions**, not bytes
- Each instruction is 4 bytes, but VM counts by instruction index
- Forward jumps use positive offsets, backward jumps use negative

### Stack State for Function Calls
When calling a function:
1. Callee (function object)
2. Null sentinel (PUSH_NULL)
3. Arguments (right to left)
4. CALL_FUNCTION with argument count

### Global Loading with Null Flag
The LSB of LOAD_GLOBAL argument indicates whether to push null:
- `arg & 1 == 0`: Load global only
- `arg & 1 == 1`: Load global and push null

This allows preparing the stack for function calls where null separates callee from arguments.

### Array Building
BUILD_ARRAY with argument N:
1. Pops N elements from stack
2. Creates array with those elements
3. Pushes array onto stack
4. If N=0, creates empty array

### Type Safety
- No implicit type conversions in bytecode
- Type checking happens at runtime
- Binary operations expect matching types
