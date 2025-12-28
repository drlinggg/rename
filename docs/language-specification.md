# Specification

## Concepts

## 0. Namespaces and storage

An object has a storage duration that determines its lifetime.

### Scopes

#### Variable Scoping

Variables are visible within the scope where they are declared.

#### Global Scope

```c++
int global_var = 100;  // Global variable

void main() {
    print(global_var);  // Accessible everywhere
}
```

#### Function Scope

```c++
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

```c++
// file: math_utils.lang
int factorial(int n) {
    // implementation
}

int fibonacci(int n) {
    // implementation
}

// file: main.lang
// Functions can be used if files are linked together
None main() {
    int result = factorial(5);  // Assuming proper inclusion/linking
}
```

### Shadowing

Inner scopes can shadow variables from outer scopes.

```c++
int x = 10;  // Global x

None shadow_example() {
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

```c++
string x = "global"; //there is no actual string rn but for this example.

void outer() {
    string x = "outer";
    
    None inner() {
        string x = "inner";
        print(x);        // "inner" (innermost scope)
    }
    
    None inner2() {
        print(x);        // "outer" (parent scope)
    }
}
```

## 1. Character Set

* **Letters**: A-Z, a-z
* **Digits**: 0-9
* **Operators**: + - * / = < > and or not
* **Punctuation**: ( ) , : . _ { } ;
* **Whitespace**: space, tab, newline
* **Special**: # @ $ ? \ " ' `

## 2. Type System

### Primitive Types (Stack-allocated)

| Type    | Size        | Range           | Literal Examples |
| ------- | ----------- | --------------- | ---------------- |
| `bool`  | 1 byte      | `true`, `false` | `true`, `false`  |
| `int`   | 8 bytes     | -2⁶³ to 2⁶³-1   | `42`, `-10`, `0` |
| `float` | non-limited | ~               | `3.14`, `7e-10`  |
| `None`  | 1 byte      | 1               | None             |

### Reference Types (Heap-allocated)

| Type                         | Description  | Example     |
| ---------------------------- | ------------ | ----------- |
| `int[]`, `bool[]`, `float[]` | Static array | `[1, 2, 3]` |

### Variable Declaration

```c++
int x = 5;        // Correct
x = 'a';          // Error: type mismatch

int a = 5;        // Correct
b = 5;            // Error: b not declared
```

### Type Rules

* Strict static typing
* Type must be explicitly declared
* No type inference
* No implicit conversions

## 3. Keywords and Basic Syntax

### Keywords

Reserved words that cannot be used as variable names:

* **Types**: `bool`, `int`, `int[], bool[], ...,`, `NoneType`
* **Control flow**: `if`, `elif`, `else`, `for`, `while`, `break`, `continue`, `return`
* **Other**: `true`, `false`, `None`, `void`

### Variables

* Variable names must start with English character
* Cannot use keywords as variable names
* Case-sensitive
* Must be declared with type before use

## 4. Operators

### Arithmetic

* Addition: `+`
* Multiplication: `*`
* Division: `/`
* Remainder: `%`
* Substraction: '-'

### Logical operations

* AND: `and`
* OR: `or`
* NOT: `not`

### Assignment Operations

* Assignment: `=`

### Comparison Operations

* Equality: `==`
* Inequality: `!=`
* Less than: `<`
* Greater than: `>`
* Less or equal: `<=`
* Greater or equal: `>=`

## 5. Punctuators

* `( )` - grouping
* `{ }` - code blocks
* `;` - statement terminator
* `,` - parameter separation

## 6. Constants

* Boolean: `true`, `false`
* Integer: `42`, `-10`, `0`
* None: special thing

Each constant has a type determined by its form and value.

## 7. Expressions

An expression is a sequence of operators and operands that specifies a computation. Expressions always have return value

### 7.1 Operator Precedence and Associativity

| Precedence | Operator          | Description                         | Associativity |
| ---------- | ----------------- | ----------------------------------- | ------------- |
| 1          | `()` `[]`         | Function call, subscript            | Left-to-right |
| 2          | `*` `/` `%`       | Multiplication, division, remainder | Left-to-right |
| 3          | `+` `-`           | Addition, subtraction               | Left-to-right |
| 4          | `<` `<=` `>` `>=` | Relational operators                | Left-to-right |
| 5          | `==` `!=`         | Equality operators                  | Left-to-right |
| 6          | `and`             | Logical AND                         | Left-to-right |
| 7          | `or`              | Logical OR                          | Left-to-right |
| 8          | `=`               | Assignment operators                | Right-to-left |

### 7.2 Expression Types

#### Arithmetic Expressions

```c++
int x = 5 + 3 * 2;     // 11 (multiplication has higher precedence)
int y = (5 + 3) * 2;   // 16 (parentheses change precedence)
```

#### Logical Expressions

```c++
bool a = true and false;        // false
bool b = (x > 0) or (y < 10);  // logical combination
bool c = not (a == b);          // logical NOT
```

#### Assignment Expressions

```c++
x = 10;              // simple assignment
y += 5;              // compound assignment (y = y + 5)
z *= 2 + 3;          // compound with expression
```

#### Function Call Expressions

```c++
int result = factorial(5);          // function call
int len = calculate_length(x, y);   // multiple arguments
```

### 7.3 Primary Expressions

Primary expressions are the basic building blocks of more complex expressions:

#### Literals

```c++
42           // integer literal
true         // boolean literal
[1, 2, 3]    // array literal
```

#### Identifiers

```c++
x            // variable name
counter      // identifier
```

#### Parenthesized Expressions

```c++
(x + y) * z          // parentheses override precedence
(a and b) or c       // explicit grouping
```

#### Function Calls

```c++
sin(angle)           // function call as expression
max(a, b, c)         // multiple arguments
```

### 7.4 Evaluation Rules

#### Order of Evaluation

* Operands are evaluated left to right
* Operator precedence determines grouping
* Parentheses can override default precedence
* Function arguments are evaluated before function call

### 7.5 Side Effects

Expressions may have side effects that change program state:

#### Assignment Side Effects

```c++
x = 5;              // changes value of x
```

#### Function Call Side Effects

```c++
print("hello");     // output side effect
```

### 7.6 Expression Examples

#### Complex Expressions

```c++
// Combined arithmetic and assignment
x = (a + b) * c - d / e;

// Logical expression with comparisons
bool is_valid = (age >= 18) and (score > 60) and not is_banned;

// Nested function calls
int result = max(min(a, b), min(c, d));

// Array access in expression
int value = scores[index] * multiplier + bonus;
```

### Statements

Statements are changing control of the program commands which do not have return value
for e.x. if statement, return statement, declaration statement

## 8. Functions

### 8.1 Function Declaration

```c++
int add(int a, int b) {
    return a + b;
}

void print_me(int a) {
    print(a);
}
```

### 8.2 Function Parameters

```c++
int calculate(int balance, int rate, int years) {
    return balance * (1 + rate) * years;
}
```

### 8.3 Return Statement

```c++
int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

bool is_valid_email(string email) {
    if (email.length == 0) {
        return false;
    }
    return true;
}
```

### 8.4 Function Calls

```c++
int result = add(5, 3);
int max_value = max(add(2, 3), multiply(4, 5));
```

## 9. Control Structures

### 9.1 Conditional Statements

```c++
if (x > 0) {
    print(true);
} elif (x < 0) {
    print(false);
} else {
    print(None);
}
```

### 9.2 Loops While & For

```c++
int i = 0;
while (i < 10) {
    print(i);
    i = i + 1;
}
```

```c++
for (int i = 0; i < 10; i = i + 1) {
    print(i);
}

int[] numbers = [1, 2, 3, 4, 5];
for (int i = 0; i < len(numbers); i = i + 1) {
    print(numbers[i]);
}
```

```c++
while (true) {
    if (condition) {
        break;
    }
}

for (int i = 0; i < 10; i = i + 1) {
    if (i % 2 == 0) {
        continue;
    }
    print(i);
}
```

## 10. Arrays

### 10.1 Array Declaration and Initialization

```c++
int[] numbers;
int[] scores = [90, 85, 95];
int[5] buffer;

int[] primes = [2, 3, 5, 7, 11];
int[10] data;
int[5] values = [0, 0, 0, 0, 0];
```

### 10.2 Array Operations

```c++
int[] arr = [10, 20, 30];
int first = arr[0];
int second = arr[1];
arr[2] = 40;
```

## 12. Memory Model

### 12.1 Lifetime

* Local variables: Exist while their containing block is executing
* Global variables: Exist for the entire program execution

### 12.2 Memory Management

Heap allocation for all types
Automatic memory management

## 13. Standard Library

### 13.1 Input/Output

```c++
print(5);
int a = input();
```

### 13.2 Random

```c++
int a = randint(1, 500);
```

### 14.3 Mathematical Functions

```c++
sqrt(x);
```

*This specification describes the current state of the programming language as implemented in the provided code. The language is under active development and may change in future versions.*
