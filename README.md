# Rename Language — Compiler & Virtual Machine

A small programming language implemented in C, featuring a lexer, parser, compiler, bytecode format, and a stack-based virtual machine. The goal of the project is to experiment with language design, compiler construction, and VM implementation.

---

## 🚀 Overview

Rename is a minimal C-like language with:

- Variables and expressions  
- Integer arithmetic  
- Blocks and scopes  
- Functions (including `main`)  
- Bytecode compilation  
- A custom virtual machine  
- Optional debugging output (`-d` / `--debug`)

The pipeline:

```

Source Code → Lexer → Parser → AST → Compiler → Bytecode → VM → Program Output

```

---

## 📦 Project Structure

```

.
├── src/
│   ├── lexer/         # Tokenizer
│   ├── parser/        # AST builder
│   ├── compiler/      # Bytecode generator
│   ├── runtime/
│   │   ├── vm/        # Virtual machine
│   │   └── objects/   # Heap-allocated objects
│   └── debug.h        # Debug macro and global flag
├── tests/             # Unit and integration tests
├── benchmarks/        # Example programs
├── docs/              # Additional documentation
├── Makefile
├── Dockerfile
└── README.md

````

---

## 🛠 Building

Build natively:

```sh
make
````

Or build using Docker:

```sh
docker build -t rename-lang .
```

The resulting executable is located in:

```
./bin/rename
```

---

## ▶️ Running Programs

Run a `.lang` source file:

```sh
./bin/rename program.lang
```

Enable debug mode:

```sh
./bin/rename -d program.lang
```

Debug mode prints detailed logs from:

* Lexer
* Parser
* AST builder
* Compiler
* VM execution
* Memory management
* Freed AST nodes, bytecode traces, stack state, etc.

---

## 📄 Example Program

`benchmarks/first_program.lang`

```c
int main() {
    int x = 5;
    int y = x + 3;
    int z = x * y / 3 + (2 + 4);
    z = x;
    return y;
}
```

Run it:

```sh
./bin/rename -d benchmarks/first_program.lang
```

---

## 🧪 Running Tests

```sh
make test
```

Clean test results:

```sh
make clean
```

Docker test run:

```sh
docker run --rm -it rename-lang
```

## 🤝 Contributing

Issues and pull requests are welcome.
This project is experimental and constantly evolving.

---

## 📜 License

```
MIT License

Copyright (c) [2025]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
