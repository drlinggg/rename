# Specification

## Concepts

## 0. Namespaces and storage
An object has a storage duration that determines its lifetime. #todo add about GC

### Scopes
#### Variable Scoping
Variables are visible within the scope where they are declared.

#### Global Scope
```javascript
int global_var = 100;  // Global variable

void main() {
    print(global_var);  // Accessible everywhere
}
```

#### Function Scope
```javascript
void example() {
    int local_var = 50;  // Local to this function
    if (true) {
        int block_var = 10;  // Local to this block
        print(local_var);    // Accessible: 50
    }
    // print(block_var);     // Error: block_var not accessible here
}
```

### File-based Namespaces
Each source file implicitly defines a namespace.

```javascript
// file: math_utils.lang
int factorial(int n) {
    // implementation
}

int fibonacci(int n) {
    // implementation
}

// file: main.lang
// Functions can be used if files are linked together
void main() {
    int result = factorial(5);  // Assuming proper inclusion/linking
}
```

### Import System (if implemented)
pass

### Shadowing
Inner scopes can shadow variables from outer scopes.

```javascript
int x = 10;  // Global x

void shadow_example() {
    int x = 20;      // Shadows global x
    if (true) {
        int x = 30;  // Shadows function x
        print(x);    // 30
    }
    print(x);        // 20
}

print(x);            // 10
```

### Namespace Resolution
The language uses lexical scoping (static scoping) for variable resolution.

```javascript
string x = "global";

void outer() {
    string x = "outer";
    
    void inner() {
        string x = "inner";
        print(x);        // "inner" (innermost scope)
    }
    
    void inner2() {
        print(x);        // "outer" (parent scope)
    }
}
```

## 1. Character Set
- **Letters**: A-Z, a-z
- **Digits**: 0-9  
- **Operators**: + - * / = < > and or not
- **Punctuation**: ( ) , : . _ { } ;
- **Whitespace**: space, tab, newline
- **Special**: # @ $ ? \ " ' `

## 2. Type System

### Primitive Types (Stack-allocated)
| Type | Size | Range | Literal Examples |
|------|------|-------|------------------|
| `bool` | 1 byte | `true`, `false` | `true`, `false` |
| `int` | 8 bytes | -2⁶³ to 2⁶³-1 | `42`, `-10`, `0` |
| `long` | 16 bytes | -2¹²⁷ to 2¹²⁷-1 | `123L`, `-999L` |

### Reference Types (Heap-allocated)
| Type | Description | Example |
|------|-------------|---------|
| `array` | Dynamic array | `[1, 2, 3]` | 

### Variable Declaration
```javascript
int x = 5;        // Correct
x = 'a';          // Error: type mismatch

int a = 5;        // Correct
b = 5;            // Error: b not declared
```

### Type Rules
- Strict static typing
- Type must be explicitly declared
- No type inference
- No implicit conversions

## 3. Keywords and Basic Syntax

### Keywords
Reserved words that cannot be used as variable names:
- **Types**: `bool`, `int`, `long`, `array`, `void`
- **Control flow**: `if`, `elif`, `else`, `for`, `while`, `break`, `continue`, `return`
- **Other**: `true`, `false`

### Variables
- Variable names must start with English character
- Cannot use keywords as variable names
- Case-sensitive
- Must be declared with type before use

## 4. Operators

### Arithmetic
- Addition: `+`
- Multiplication: `*`
- Division: `/`
- Remainder: `%`

### Logical operations
- AND: `and`
- OR: `or` 
- NOT: `not`

### Assignment Operations
- Assignment: `=`
- Compound assignments: `+=`, `-=`, `*=`, `/=`

### Comparison Operations
- Equality: `==`
- Inequality: `!=`
- Less than: `<`
- Greater than: `>`
- Less or equal: `<=`
- Greater or equal: `>=`

## 5. Punctuators
- `( )` - grouping
- `{ }` - code blocks  
- `;` - statement terminator
- `,` - parameter separation
- `.` - member access

## 6. Constants
- Boolean: `true`, `false`
- Integer: `42`, `-10`, `0`
- No floating-point or character constants in initial version

Each constant has a type determined by its form and value.

## 7. Expressions

An expression is a sequence of operators and operands that specifies a computation.

### 7.1 Operator Precedence and Associativity

| Precedence | Operator | Description | Associativity |
|------------|----------|-------------|---------------|
| 1 | `()` `[]` `.` | Function call, subscript, member access | Left-to-right |
| 2 | `*` `/` `%` | Multiplication, division, remainder | Left-to-right |
| 3 | `+` `-` | Addition, subtraction | Left-to-right |
| 4 | `<` `<=` `>` `>=` | Relational operators | Left-to-right |
| 5 | `==` `!=` | Equality operators | Left-to-right |
| 6 | `and` | Logical AND | Left-to-right |
| 7 | `or` | Logical OR | Left-to-right |
| 8 | `=` `+=` `-=` `*=` `/=` | Assignment operators | Right-to-left |

### 7.2 Expression Types

#### Arithmetic Expressions
```javascript
int x = 5 + 3 * 2;     // 11 (multiplication has higher precedence)
int y = (5 + 3) * 2;   // 16 (parentheses change precedence)
```

#### Logical Expressions
```javascript
bool a = true and false;        // false
bool b = (x > 0) or (y < 10);  // logical combination
bool c = not (a == b);          // logical NOT
```

#### Assignment Expressions
```javascript
x = 10;              // simple assignment
y += 5;              // compound assignment (y = y + 5)
z *= 2 + 3;          // compound with expression
```

#### Function Call Expressions
```javascript
int result = factorial(5);          // function call
int len = calculate_length(x, y);   // multiple arguments
```

### 7.3 Primary Expressions

Primary expressions are the basic building blocks of more complex expressions:

#### Literals
```javascript
42           // integer literal
true         // boolean literal
[1, 2, 3]    // array literal
```

#### Identifiers
```javascript
x            // variable name
counter      // identifier
```

#### Parenthesized Expressions
```javascript
(x + y) * z          // parentheses override precedence
(a and b) or c       // explicit grouping
```

#### Function Calls
```javascript
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
```javascript
// If 'x' is false, 'y' is not evaluated
bool result = x and y;

// If 'a' is true, 'b' is not evaluated  
bool value = a or b;
```

### 7.5 Side Effects

Expressions may have side effects that change program state:

#### Assignment Side Effects
```javascript
x = 5;              // changes value of x
counter += 1;       // modifies counter
```

#### Function Call Side Effects
```javascript
print("hello");     // output side effect
```

### 7.6 Expression Examples

#### Complex Expressions
```javascript
// Combined arithmetic and assignment
x = (a + b) * c - d / e;

// Logical expression with comparisons
bool is_valid = (age >= 18) and (score > 60) and not is_banned;

// Nested function calls
int result = max(min(a, b), min(c, d));

// Array access in expression
int value = scores[index] * multiplier + bonus;
```

## 8. Functions

### 8.1 Function Declaration
Functions are declared with return type, name, parameters, and body.

```javascript
// Basic function
void greet(string name) {
    print("Hello, " + name);
}

// Function with return value
int add(int a, int b) {
    return a + b;
}
```

### 8.2 Function Parameters
Parameters are passed by value. Each parameter must specify its type.

```javascript
// Parameters with different types
int calculate(int balance, int rate, int years) {
    return balance * (1 + rate) * years;
}
```

### 8.3 Return Statement
The `return` statement exits a function and optionally returns a value.

```javascript
int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

// Early return
bool is_valid_email(string email) {
    if (email.length == 0) {
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
int result = add(5, 3);

// Nested calls
int max_value = max(add(2, 3), multiply(4, 5));
```

## 9. Structs and Methods

### Struct Declaration
Structs create new namespaces for their fields and methods.

```
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
```
void create_geometry(): {
    let point = Vector2 { x: 10.5, y: 20.3 };
    let rect = Rectangle { 
        position: point, 
        width: 100.0, 
        height: 50.0 
    };
}
```

## 10. Control Structures

### Conditional Statements

### Loops

## 11. Arrays

### Array Declaration and Usage


#todo
1)Именованные аргументы при вызове функций по желанию
2)Встроенный менеджер пакетов и система сборки
