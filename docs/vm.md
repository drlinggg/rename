## Architecture of VM

### VM structure:
- **Stack Call** - stack of frames
- **Global context** - Global storage for global objects, global namespace, hashmap
- **Constants pool** - Global storage for constants which nobody can change
- **Heap** - for objects with dynamical time of life, hashmap

methods:
- create
- frame_push
- frame_pop
- execute
- destroy

### Frame structure:
- **Code** - bytecode operations list
- **Operation stack** (LIFO) - stack of results of bytecode operations
- **instruction pointer** used for jumping between operations
- **local context** - dict of local variables, local namespace
- **Return address / value** - todo think about return mechanism

methods:
- create
- push
- execute bytecode op
- pop
- destroy

### instructions format:
```
opcode (1 байт) | arg (2 байта) | arg_value (depends of context idk rn)
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
{
    "x": 100,
    "main": <function object>,
    "factorial": <function object>]
}
```

### local context
```python
{
    "local_x": 100,
    "local_variable_string": "123adsa",
    "local_factorial": <function object>
}
```
