# JIT Compiler Documentation

## Overview
The Just-In-Time (JIT) compiler is an optimization system that dynamically analyzes and optimizes bytecode at runtime. It provides multiple optimization passes to improve execution speed without modifying the original source code.

## Architecture

### 1. Core Components

#### 1.1 JIT Compiler Structure
```c
typedef struct JIT {
    CodeObj** compiled_cache;    // Cache of optimized functions
    size_t cache_size;           // Current cache size
    size_t cache_capacity;       // Cache capacity
    size_t compiled_count;       // Total compiled functions
} JIT;
```

#### 1.2 Optimization Passes
The JIT compiler applies multiple optimization passes:
- **Constant Folding**: Pre-computes constant expressions at compile time
- **Compare-and-Swap**: Optimizes bubble sort patterns to atomic operations
- **Peephole Optimization**: Local pattern-based optimizations

### 2. Constant Folding Optimization

#### 2.1 Optimization Statistics
```c
typedef struct {
    size_t folded_constants;      // Number of constants folded
    size_t removed_instructions;  // Instructions removed
    size_t peephole_optimizations; // Peephole optimizations applied
} FoldStats;
```

#### 2.2 Detectable Patterns

##### 2.2.1 Binary Operations on Constants
```
LOAD_CONST a      # Load constant a
LOAD_CONST b      # Load constant b
BINARY_OP op      # Binary operation
→ Replaced with:
LOAD_CONST result # Pre-computed result
```

##### 2.2.2 Constant Conditionals
```
LOAD_CONST true/false
POP_JUMP_IF_FALSE target
→ Optimized based on constant value:
- If false: JUMP_FORWARD target (unconditional jump)
- If true: Both instructions removed (no-op)
```

##### 2.2.3 Operation Chains
```
LOAD_CONST 1
LOAD_CONST 2
BINARY_OP ADD     # 1 + 2 = 3
LOAD_CONST 3
BINARY_OP MUL     # 3 * 3 = 9
→ Replaced with:
LOAD_CONST 9
```

#### 2.3 Implementation Details

##### 2.3.1 Constant Folding Logic
```c
static Value fold_binary_constant(Value a, Value b, uint8_t op) {
    if (a.type == VAL_INT && b.type == VAL_INT) {
        int64_t x = a.int_val;
        int64_t y = b.int_val;
        
        switch (op) {
            case 0x00: return value_create_int(x + y);      // ADD
            case 0x0A: return value_create_int(x - y);      // SUB
            case 0x05: return value_create_int(x * y);      // MUL
            case 0x0B: return value_create_int(y != 0 ? x / y : 0); // DIV
            case 0x06: return value_create_int(y != 0 ? x % y : 0); // MOD
            // ... comparison operations
        }
    }
    return value_create_none();
}
```

##### 2.3.2 Jump Recalculation
After removing instructions, all jump offsets must be recalculated:
```c
static void recalculate_jumps(bytecode_array* bc) {
    // 1. Build map: old index → new index
    // 2. Update all jump instructions with new offsets
    // 3. Remove NOP instructions and compact array
}
```

### 3. Compare-and-Swap Optimization

#### 3.1 Bubble Sort Pattern Detection
The JIT detects and optimizes bubble sort swap operations:

**Original Bubble Sort Swap:**
```c
if (arr[j] > arr[j + 1]) {
    int temp = arr[j + 1];
    arr[j + 1] = arr[j];
    arr[j] = temp;
}
```

**Bytecode Pattern (28 instructions):**
```
LOAD_FAST array      # Load array
LOAD_FAST j         # Load index j
LOAD_SUBSCR         # Load arr[j]

LOAD_FAST array      # Load array
LOAD_FAST j         # Load index j
LOAD_CONST 1        # Load constant 1
BINARY_OP ADD       # Compute j+1
LOAD_SUBSCR         # Load arr[j+1]

BINARY_OP GT        # Compare arr[j] > arr[j+1]
POP_JUMP_IF_FALSE   # Skip swap if false

# ... swap operations (15 more instructions)
```

**Optimized to (6 instructions):**
```
LOAD_FAST array      # Load array
LOAD_FAST j         # Load index j
LOAD_FAST j         # Load index j (again)
LOAD_CONST 1        # Load constant 1
BINARY_OP ADD       # Compute j+1
COMPARE_AND_SWAP    # Atomic compare and swap
```

#### 3.2 Pattern Matching Structure
```c
typedef struct {
    size_t array_local_idx;   // Array variable index
    size_t index_local_idx;   // Loop index variable (j/i)
    size_t temp_local_idx;    // Temporary variable index
    size_t const_one_idx;     // Constant 1 index
} PatternMatch;
```

#### 3.3 Optimization Statistics
```c
typedef struct {
    size_t optimized_patterns; // Number of patterns optimized
} CmpswapStats;
```

### 4. JIT Cache Management

#### 4.1 Cache Operations

##### 4.1.1 Cache Lookup
```c
static CodeObj* jit_find_in_cache(JIT* jit, CodeObj* code) {
    // Search cache by function name
    for (size_t i = 0; i < jit->cache_size; i++) {
        if (strcmp(cached->name, code->name) == 0) {
            return cached;  // Cache hit
        }
    }
    return NULL;  // Cache miss
}
```

##### 4.1.2 Cache Insertion
```c
static void jit_add_to_cache(JIT* jit, CodeObj* optimized_code) {
    // Expand cache if needed
    if (jit->cache_size >= jit->cache_capacity) {
        size_t new_capacity = jit->cache_capacity * 2;
        CodeObj** new_cache = realloc(jit->compiled_cache, 
                                     new_capacity * sizeof(CodeObj*));
        jit->compiled_cache = new_cache;
        jit->cache_capacity = new_capacity;
    }
    
    // Add to cache
    jit->compiled_cache[jit->cache_size++] = optimized_code;
    jit->compiled_count++;
}
```

#### 4.2 Cache Statistics
```c
typedef struct {
    size_t cache_size;        // Current cache size
    size_t cache_capacity;    // Cache capacity
    size_t compiled_count;    // Total compiled functions
} JITStats;
```

### 5. Integration with VM

#### 5.1 Compilation Hook
The JIT is integrated into the VM through a compilation hook:
```c
// In VM execution:
void* jit_compile_function(JIT* jit, void* code_ptr) {
    CodeObj* original = (CodeObj*)code_ptr;
    
    // 1. Check cache
    CodeObj* cached = jit_find_in_cache(jit, original);
    if (cached) return cached;
    
    // 2. Apply optimizations
    CodeObj* optimized = apply_optimizations(original);
    
    // 3. Cache result
    if (optimized != original) {
        jit_add_to_cache(jit, optimized);
    }
    
    return optimized ? optimized : original;
}
```

#### 5.2 VM Integration Points
```c
// In VM bytecode dispatch:
case MAKE_FUNCTION:
    Object* maybe = frame_stack_pop(frame);
    if (maybe && maybe->type == OBJ_CODE) {
        CodeObj* codeptr = maybe->as.codeptr;
        
        // JIT compilation hook
        JIT_COMPILE_IF_ENABLED(frame->vm, codeptr);
        
        // ... continue execution
    }
```

### 6. Optimization Pipeline

#### 6.1 Complete Optimization Flow
```
1. Input CodeObj
   ↓
2. Check Cache
   ├── Cache Hit → Return cached version
   └── Cache Miss → Continue
   ↓
3. Constant Folding Pass
   ├── Fold binary operations
   ├── Optimize conditionals
   ├── Fold operation chains
   ↓
4. Compare-and-Swap Pass
   ├── Detect bubble sort patterns
   ├── Replace with COMPARE_AND_SWAP
   ↓
5. Jump Recalculation
   ├── Update all jump offsets
   ├── Remove NOP instructions
   ↓
6. Cache Result
   ↓
7. Return Optimized CodeObj
```

#### 6.2 Optimization Iterations
The JIT applies optimizations iteratively until no more changes occur:
```c
do {
    changed = 0;
    
    // Apply constant folding
    if (aggressive_constant_folding(optimized, bc, stats)) {
        changed = 1;
        recalculate_jumps(bc);
    }
    
    // Apply peephole optimizations
    if (optimize_constant_ifs(optimized, bc, stats)) {
        changed = 1;
        recalculate_jumps(bc);
    }
    
} while (changed && iteration++ < 100);  // Safety limit
```

### 7. Safety and Correctness

#### 7.1 Safety Guarantees
- **Preserves semantics**: Optimizations are semantics-preserving
- **Bounds checking**: All array accesses validated
- **Type safety**: Operations only applied to compatible types
- **Jump validity**: All jumps recalculated and validated

#### 7.2 Error Handling
```c
CodeObj* jit_optimize_constant_folding(CodeObj* original, FoldStats* stats) {
    if (!original || !original->code.bytecodes || !original->constants) {
        DPRINT("[JIT-CF] Invalid input\n");
        return NULL;
    }
    
    // Create deep copy for safety
    CodeObj* optimized = deep_copy_codeobj(original);
    if (!optimized) {
        DPRINT("[JIT-CF] Failed to copy CodeObj\n");
        return NULL;
    }
    
    // Apply optimizations...
    return optimized;
}
```

#### 7.3 Memory Management
- **Deep copying**: Original CodeObj never modified
- **Reference counting**: Proper management of constants
- **Cache cleanup**: Proper destruction of cached objects

### 8. Performance Characteristics

#### 8.1 Optimization Benefits
- **Constant folding**: Eliminates runtime computation
- **Pattern optimization**: Reduces instruction count by 78% for swaps
- **Cache hits**: Zero-cost optimization for repeated calls

#### 8.2 Overhead Considerations
- **Compilation time**: One-time cost per function
- **Memory overhead**: Cache storage for optimized functions
- **Cache management**: Lookup overhead on each function call

#### 8.3 Typical Performance Gains
```
Original bubble sort swap: 28 instructions
Optimized swap: 6 instructions
Reduction: 78.6% fewer instructions

Constant expressions: 3 instructions → 1 instruction
Reduction: 66.7% fewer instructions
```

### 9. Debugging and Monitoring

#### 9.1 Debug Output
```c
// Enable debug output
#define DEBUG_ENABLED 1

// Debug prints
DPRINT("[JIT] Compiling function: %s\n", original->name);
DPRINT("[JIT-CF] Folded constants: %zu\n", stats->folded_constants);
DPRINT("[JIT-CMPSWAP] Found pattern at position %zu\n", i);
```

#### 9.2 Statistics Collection
```c
// Get JIT statistics
JITStats stats = jit_get_stats(jit);
printf("Cache size: %zu/%zu\n", stats.cache_size, stats.cache_capacity);
printf("Compiled functions: %zu\n", stats.compiled_count);

// Print cache contents
jit_print_cache(jit);
```

#### 9.3 Optimization Verification
```c
// Verify optimization didn't break anything
assert(optimized->code.count <= original->code.count);
assert(optimized->constants_count >= original->constants_count);

// Bytecode verification (simplified)
for (size_t i = 0; i < optimized->code.count; i++) {
    bytecode bc = optimized->code.bytecodes[i];
    assert(bc.op_code != 0xFF);  // Invalid opcode
    assert(bc.op_code != NOP || i == 0 || i == optimized->code.count - 1);
}
```

*This JIT compiler provides significant performance improvements through pattern-based optimizations while maintaining semantic correctness and safety.*
