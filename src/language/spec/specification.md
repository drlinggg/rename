# Specification

## Concepts

## 0. Namespaces and storage
An object has a storage duration that determines its lifetime. #todo add about GC
### Scopes
#### Variable Scoping
Variables are visible within the scope where they are declared.

#### Global Scope
```javascript
let global_var = 100;  // Global variable

func main(): void {
    print(global_var);  // Accessible everywhere
}
```

#### Function Scope
```javascript
func example(): void {
    let local_var = 50;  // Local to this function
    if true {
        let block_var = 10;  // Local to this block
        print(local_var);    // Accessible: 50
    }
    // print(block_var);     // Error: block_var not accessible here
}
```

#### Struct Scope
```javascript
struct Person {
    name: string;
    age: int;
    
    // Method (if supported)
    func get_info(): string {
        return this.name + ", " + int_to_string(this.age);
    }
}

func use_person(): void {
    let p = Person { name: "Alice", age: 30 };
    print(p.name);        // Access through struct instance
    // print(name);       // Error: name not in current scope
}
```

### File-based Namespaces
Each source file implicitly defines a namespace.

```javascript
// file: math_utils.lang
func factorial(n: int): int {
    // implementation
}

func fibonacci(n: int): int {
    // implementation
}

// file: main.lang
// Functions can be used if files are linked together
func main(): void {
    let result = factorial(5);  // Assuming proper inclusion/linking
}
```

### 11.2 Import System (if implemented)

### Shadowing
Inner scopes can shadow variables from outer scopes.

```javascript
let x = 10;  // Global x

func shadow_example(): void {
    let x = 20;      // Shadows global x
    if true {
        let x = 30;  // Shadows function x
        print(x);    // 30
    }
    print(x);        // 20
}

print(x);            // 10
```

### Namespace Resolution
The language uses lexical scoping (static scoping) for variable resolution.

```javascript
let x = "global";

func outer(): void {
    let x = "outer";
    
    func inner(): void {
        let x = "inner";
        print(x);        // "inner" (innermost scope)
    }
    
    func inner2(): void {
        print(x);        // "outer" (parent scope)
    }
}
```

## 1. Character Set
- **Letters**: A-Z, a-z
- **Digits**: 0-9  
- **Operators**: + - * / = < > and or not
- **Punctuation**: ( ) , : . _
- **Whitespace**: space, tab, newline
- **Special**: # @ $ ? \ " ' `

## 2. Type System

### Primitive Types (Stack-allocated)
| Type | Size | Range | Literal Examples |
|------|------|-------|------------------|
| `bool` | 1 byte | `true`, `false` | `true`, `false` |
| `int` | 8 bytes | -2⁶³ to 2⁶³-1 | `42`, `-10`, `0` |
| `long` | 16 bytes | -2¹²⁷ to 2¹²⁷-1 | `123L`, `-999L` |
| `float` | 8 bytes | IEEE 754 | `3.14f`, `-0.5f` | # необязательно
| `double` | 16 bytes | IEEE 754 | `2.71828`, `-1.5` | # необязательно
| `char` | 1 byte | 0-127 | `'a'`, `'\n'` | # необязательно

### Reference Types (Heap-allocated)
| Type | Description | Example |
|------|-------------|---------|
| `string` | Immutable UTF-8 string | `"hello"` | # необязательно
| `array` | Dynamic array | `[1, 2, 3]` | 
| `dict` | Hash map | `{"key": "value"}` | # необязательно
| `struct` | User-defined structure | `Point { x: 10, y: 20 }` | # необязательно

### Conversions
#tobedone with ###cast operations

## 3. Keywords and Basic Syntax

### Keywords
Reserved words that cannot be used as variable names:
- ALL TYPES
- Control flow: if, elif, else, for, while, break, continue
- Other: return, ?var?, ?func?

### Variables
- Variable names must start with English character
- Cannot use keywords as variable names
- Case-sensitive

### Constants

- None (null value) #еслимыхотимдобавить
- Integer constants: 0, 42, -10 (digits 0-9)
- Floating constants: 0.2, 3.14, -5.67
- Character constants: 'a', 'abdsa3123d'
- Enumeration constants: (to be defined)
- bool: (True, False)

## 4. Operators
### Arithmetic
- add (+)
- mult (*)
- division (/)
- division without remainder (//) #???
- remainder (%) #???
### Logical operations
- and (and)
- or (or)
- not (not)
### Assignment Operations
- Assignment: =
- Compound assignments: +=, -=, *=, /=
### Comparison Operations
- Equality: ==
- Inequality: !=
- Less than: <
- Greater than: >
- Less or equal: <=
- Greater or equal: >=

### 5. Punctuators
 - ( ) . - grouping and access
 - ++, --, +, -, /, % - operators
 - =, ==, *=, +=, /=, -=, ',' - assignment and separation


### 6. Constants
- None
- integer-constant (0-9)
- floating-constant (0.2)
- character-constant ('a', 'abdsa3123d')
- enumeration-constant #idk

Each constant has a type determined by its form and value.

### 7. Expressions

Expression is a sequence of operators and operands.
```
i=(6 + 1) % 2
i=i+1
```

An expression is a sequence of operators and operands that specifies a computation. Expressions are evaluated to produce a value, and may have side effects.

### 7.1 Operator Precedence and Associativity

| Precedence | Operator | Description | Associativity |
|------------|----------|-------------|---------------|
| 1 | `()` `[]` `.` | Function call, subscript, member access | Left-to-right |
| 2 | `++` `--` | Postfix increment/decrement | Left-to-right |
| 3 | `++` `--` `+` `-` `!` `not` | Prefix increment/decrement, unary plus/minus, logical NOT | Right-to-left |
| 4 | `*` `/` `%` | Multiplication, division, remainder | Left-to-right |
| 5 | `+` `-` | Addition, subtraction | Left-to-right |
| 6 | `<` `<=` `>` `>=` | Relational operators | Left-to-right |
| 7 | `==` `!=` | Equality operators | Left-to-right |
| 8 | `and` | Logical AND | Left-to-right |
| 9 | `or` | Logical OR | Left-to-right |
| 10 | `=` `+=` `-=` `*=` `/=` `%=` | Assignment operators | Right-to-left |

### 7.2 Expression Types

#### Arithmetic Expressions
```
x = 5 + 3 * 2;     // 11 (multiplication has higher precedence)
y = (5 + 3) * 2;   // 16 (parentheses change precedence)
```

#### Logical Expressions
```
a = true and false;        // false
b = (x > 0) or (y < 10);  // logical combination
c = not (a == b);          // logical NOT
```

#### Assignment Expressions
```
x = 10;              // simple assignment
y += 5;              // compound assignment (y = y + 5)
z *= 2 + 3;          // compound with expression
```

#### Function Call Expressions
```
result = factorial(5);          // function call
len = calculate_length(x, y);   // multiple arguments
```

### 7.3 Primary Expressions

Primary expressions are the basic building blocks of more complex expressions:

#### Literals
```
42           // integer literal
3.14         // floating-point literal
true         // boolean literal
'x'          // character literal
"hello"      // string literal
[1, 2, 3]    // array literal
```

#### Identifiers
```
x            // variable name
counter      // identifier
MAX_SIZE     // constant
```

#### Parenthesized Expressions
```
(x + y) * z          // parentheses override precedence
(a and b) or c       // explicit grouping
```

#### Function Calls
```
sin(angle)           // function call as expression
max(a, b, c)         // multiple arguments
```

### 7.4 Evaluation Rules

#### Order of Evaluation
- Operands are evaluated left to right
- Operator precedence determines grouping
- Parentheses can override default precedence
- Function arguments are evaluated before function call

#### Short-Circuit Evaluation
```
// If 'x' is false, 'y' is not evaluated
let result = x and y;

// If 'a' is true, 'b' is not evaluated  
let value = a or b;
```

### 7.5 Side Effects

Expressions may have side effects that change program state:

#### Assignment Side Effects
```
x = 5;              // changes value of x
counter += 1;       // modifies counter
```

#### Increment/Decrement Side Effects
```
i++;                // postfix increment
```

#### Function Call Side Effects
```
print("hello");     // output side effect
append(array, item); // modifies array
```

### 7.6 Expression Examples

#### Complex Expressions
```
// Combined arithmetic and assignment
x = (a + b) * c - d / e;

// Logical expression with comparisons
is_valid = (age >= 18) and (score > 60) and not is_banned;

// Nested function calls
result = max(min(a, b), min(c, d));

// Array access in expression
value = scores[index] * multiplier + bonus;
```

## 8. Functions

### 8.1 Function Declaration
Functions are declared using the `func` keyword followed by the function name, parameters, return type, and body. #можно поменять, затем все примеры заменить надо

```javascript
// Basic function
func greet(name: string): void {
    print("Hello, " + name);
}

// Function with return value
func add(a: int, b: int): int {
    return a + b;
}

// Function with multiple parameters
func create_point(x: double, y: double, z: double): Point {
    return Point { x: x, y: y, z: z };
}
```

### 8.2 Function Parameters
Parameters are passed by value. Each parameter must specify its type.

```javascript
// Parameters with different types
func calculate(balance: double, rate: float, years: int): double {
    return balance * (1.0 + rate) ** years;
}

// Default parameters (if supported)
func connect(host: string, port: int = 8080, timeout: int = 30): bool {
    // implementation
}
```

### 8.3 Return Statement
The `return` statement exits a function and optionally returns a value.

```javascript
func max(a: int, b: int): int {
    if a > b {
        return a;
    } else {
        return b;
    }
}

// Early return
func is_valid_email(email: string): bool {
    if email.length == 0 {
        return false;
    }
    // further validation
    return true;
}
```

### 8.4 Function Calls
Functions are called using the function name followed by arguments in parentheses.

```javascript
// Simple call
let result = add(5, 3);

// Nested calls
let max_value = max(add(2, 3), multiply(4, 5));

// Method chaining
let processed = data.process().filter().sort();
```


## 9. Structs and Methods

### Struct Declaration
Structs create new namespaces for their fields and methods.

```javascript
struct Vector2 {
    x: double;
    y: double;
}

struct Rectangle {
    position: Vector2;
    width: double;
    height: double;
}
```

### Struct Instantiation
```javascript
func create_geometry(): void {
    let point = Vector2 { x: 10.5, y: 20.3 };
    let rect = Rectangle { 
        position: point, 
        width: 100.0, 
        height: 50.0 
    };
}
```
