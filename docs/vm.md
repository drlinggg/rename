## Architecture of VM

### VM structure:
- **Stack Call** - stack of frames
- **globals** - Global storage for global objects, global namespace, list
- **global_varnames**
- **builtins** - Global storage for builtins, list
- **builtin_varnames**
- **Constants pool** - Global storage for constants which nobody can change, list
- **Heap** - for objects with dynamical time of life, list

methods:
- create
- frame_push
- frame_pop
- execute
- destroy

TODO GARBAGE COLLECTOR

### Frame structure:
- **Code** - bytecode operations list
- **Stack** (LIFO) - stack of results of bytecode operations
- **instruction pointer** used for jumping between operations
- **locals** - list of local values, local namespace
- **varnames** - list of local varnames, used for debug
- **Return address / value** - todo think about return mechanism

methods:
- create
- push
- execute bytecode op
- pop
- destroy

### instructions format:
```
opcode (1 байт) | arg (2 байта)
```

### constants pool
```python
[
    10,     # index 0
    20,     # index 1
    "hello" # index 2
]
```

### global context
```python
[
    100,
    <function object>,
    <function object>
]
```

### local context
```python
[
  100,
  "123adsa",
  <function object>
]
```
