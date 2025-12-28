# Rename Language — Compiler & Virtual Machine

**Rename** is a small, experimental C-like programming language implemented in C, featuring a lexer, parser, compiler, bytecode format, and a stack-based virtual machine. The project is designed for learning language design, compiler construction, and VM implementation.

---

## 🚀 Overview

Rename is a minimal C-like language that supports:

* Variables and expressions
* Integer arithmetic
* Blocks and scopes
* Functions (including `main`)
* Compilation to bytecode
* Execution on a custom virtual machine
* Optional debugging output (`-d` / `--debug`)
* Mark & Sweep garbage collection
* Just-In-Time (JIT) compilation ([see docs/jit.md](docs/jit.md))

**Compilation and execution pipeline:**

```
Source Code → Lexer → Parser → AST → Compiler → Bytecode → VM → Program Output
```

For more details on the VM and bytecode behavior, see:

* [Virtual Machine](docs/vm.md) — Stack-based VM, memory management, frames, objects
* [Bytecode](docs/bytecode.md) — Instruction set, format, and execution examples

---

## 📦 Project Structure

```
.
├── src/
│   ├── lexer/         # Tokenizer
│   ├── parser/        # AST builder
│   ├── compiler/      # Bytecode generator
│   ├── runtime/
│   │   ├── vm/        # Virtual machine implementation
│   │   ├── gc/        # Garbage collector
│   │   └── jit/       # JIT compiler module
│   └── system.h       # Debug, GC, JIT macros and global flags for it
├── tests/             # Unit and integration tests
├── benchmarks/        # Example programs
├── docs/              # Detailed documentation
├── Makefile
├── Dockerfile
└── README.md
```

---

## 🛠 Building

**Build natively:**

```bash
make
```

**Build with Docker:**

```bash
docker build -t rename-lang .
```

**Executable location:**

```
./bin/rename
```

---

## ▶️ Running Programs

Run a `.lang` file:

```bash
./bin/rename program.lang
```

Enable debug mode:

```bash
./bin/rename -d program.lang
```

Debug mode prints detailed logs from:

* Lexer
* Parser
* Compiler
* VM execution ([see docs/vm.md](docs/vm.md))
* Memory management
* Freed AST nodes and bytecode traces ([see docs/bytecode.md](docs/bytecode.md))
* Stack state

Enable JIT mode:

```bash
./bin/rename -j program.lang
```

---

## 📄 Example Program

`benchmarks/first_program.lang`:

```c
int main() {
    int x = 5;
    int y = x + 3;
    int z = x * y / 3 + (2 + 4);
    z = x;
    return y;
}
```

Run it with debugging:

```bash
./bin/rename -d benchmarks/first_program.lang
```

---

## 🧪 Testing

Run all tests:

```bash
make test
```

Clean test artifacts:

```bash
make clean
```

Docker test run:

```bash
docker run --rm -it rename-lang
```

---

## 📝 Documentation

* [JIT Compiler](docs/jit.md) — Just-In-Time compilation, optimization passes, caching, and integration with VM
* [Virtual Machine](docs/vm.md) — Stack frames, objects, memory management, and instruction execution
* [Bytecode](docs/bytecode.md) — Instruction set, argument types, stack effects, and execution examples
* [Language Specification](docs/language-specification.md) — Advanced guide for language semantics and syntax

---

## 🤝 Contributing

Contributions are welcome!
Since Rename is experimental, we encourage:

* Adding tests for new instructions
* Preserving semantic correctness during optimizations
* Improving VM and JIT performance
* Reporting issues or submitting pull requests

---

## 📜 License

```text
MIT License

Copyright (c) 2025

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
