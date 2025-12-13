CC = gcc
CFLAGS = -I src -I src/AST -I src/lexer -I src/parser -I src/compiler -I src/runtime -I src/runtime/vm -I src/runtime/gc -I src/runtime/jit
SRC_DIR = src
TEST_DIR = tests

# Source files
AST_SRC = $(SRC_DIR)/AST/ast.c
LEXER_SRC = $(SRC_DIR)/lexer/lexer.c
TOKEN_SRC = $(SRC_DIR)/lexer/token.c
PARSER_SRC = $(SRC_DIR)/parser/parser.c
BYTECODE_SRC = $(SRC_DIR)/compiler/bytecode.c
COMPILER_SRC = $(SRC_DIR)/compiler/compiler.c
VALUE_SRC = $(SRC_DIR)/compiler/value.c
SCOPE_SRC = $(SRC_DIR)/compiler/scope.c
STRING_TABLE_SRC = $(SRC_DIR)/compiler/string_table.c
DEBUG_SRC = $(SRC_DIR)/debug.c

# Runtime files
OBJECT_SRC = src/runtime/vm/object.c
HEAP_SRC = src/runtime/vm/heap.c
VM_SRC = src/runtime/vm/vm.c
GC_SRC = src/runtime/gc/gc.c
JIT_SRC = src/runtime/jit/jit.c

# Test files
AST_TEST = $(TEST_DIR)/ast/test_ast.c
LEXER_TEST = $(TEST_DIR)/lexer/test_lexer.c
PARSER_TEST = $(TEST_DIR)/parser/test_parser.c
BYTECODE_TEST = $(TEST_DIR)/bytecode/test_bytecode.c
COMPILER_TEST = $(TEST_DIR)/compiler/test_compiler.c
VM_TEST = $(TEST_DIR)/runtime/test_vm.c

# Targets
all: test_ast test_lexer test_parser test_bytecode test_compiler test_vm

test_ast: $(AST_TEST) $(AST_SRC) $(DEBUG_SRC) $(TOKEN_SRC)
	$(CC) $(CFLAGS) $(AST_TEST) $(AST_SRC) $(DEBUG_SRC) $(TOKEN_SRC) -o $@

test_lexer: $(LEXER_TEST) $(LEXER_SRC) $(DEBUG_SRC) $(TOKEN_SRC)
	$(CC) $(CFLAGS) $(LEXER_TEST) $(LEXER_SRC) $(DEBUG_SRC) $(TOKEN_SRC) -o $@

test_parser: $(PARSER_TEST) $(PARSER_SRC) $(AST_SRC) $(LEXER_SRC) $(DEBUG_SRC) $(TOKEN_SRC)
	$(CC) $(CFLAGS) $(PARSER_TEST) $(PARSER_SRC) $(AST_SRC) $(LEXER_SRC) $(DEBUG_SRC) $(TOKEN_SRC) -o $@

test_bytecode: $(BYTECODE_TEST) $(BYTECODE_SRC) $(DEBUG_SRC)
	$(CC) $(CFLAGS) $(BYTECODE_TEST) $(BYTECODE_SRC) $(DEBUG_SRC) -o $@

test_compiler: $(COMPILER_TEST) $(COMPILER_SRC) $(VALUE_SRC) $(SCOPE_SRC) $(STRING_TABLE_SRC) $(BYTECODE_SRC) $(AST_SRC) $(DEBUG_SRC) $(TOKEN_SRC)
	$(CC) $(CFLAGS) $(COMPILER_TEST) $(COMPILER_SRC) $(VALUE_SRC) $(SCOPE_SRC) $(STRING_TABLE_SRC) $(BYTECODE_SRC) $(AST_SRC) $(DEBUG_SRC) $(TOKEN_SRC) -o $@

test_vm: $(VM_TEST) $(BYTECODE_SRC) $(VALUE_SRC) $(AST_SRC) $(TOKEN_SRC) $(OBJECT_SRC) $(HEAP_SRC) $(VM_SRC) $(GC_SRC) $(JIT_SRC) $(DEBUG_SRC)
	$(CC) $(CFLAGS) $(VM_TEST) $(BYTECODE_SRC) $(VALUE_SRC) $(AST_SRC) $(TOKEN_SRC) $(OBJECT_SRC) $(HEAP_SRC) $(VM_SRC) $(GC_SRC) $(JIT_SRC) $(DEBUG_SRC) -o $@

test: all
	@echo "[Make] Running AST tests..."
	./test_ast || exit 1
	@echo "[Make] Running Lexer tests..."
	./test_lexer || exit 1
	@echo "[Make] Running Parser tests..."
	./test_parser || exit 1
	@echo "[Make] Running Bytecode tests..."
	./test_bytecode || exit 1
	@echo "[Make] Running Compiler tests..."
	./test_compiler || exit 1
	@echo "[Make] Running VM tests..."
	./test_vm || exit 1
	@echo "[Make] All tests passed!"

runner: tools/runner.c $(BYTECODE_SRC) $(VALUE_SRC) $(AST_SRC) $(TOKEN_SRC) $(COMPILER_SRC) $(SCOPE_SRC) $(STRING_TABLE_SRC) $(LEXER_SRC) $(PARSER_SRC) $(OBJECT_SRC) $(HEAP_SRC) $(VM_SRC) $(GC_SRC) $(JIT_SRC) $(DEBUG_SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) tools/runner.c $(BYTECODE_SRC) $(VALUE_SRC) $(AST_SRC) $(TOKEN_SRC) $(COMPILER_SRC) $(SCOPE_SRC) $(STRING_TABLE_SRC) $(LEXER_SRC) $(PARSER_SRC) $(OBJECT_SRC) $(HEAP_SRC) $(VM_SRC) $(GC_SRC) $(JIT_SRC) $(DEBUG_SRC) -o bin/rename

run_benchmark: runner
	@echo "[Make] Running benchmark: benchmarks/first_program.lang"
	./bin/rename benchmarks/first_program.lang

clean:
	@echo "[Make] Cleaning ..."
	rm -f test_ast test_lexer test_parser test_bytecode test_compiler test_vm
	rm -f bin/rename

.PHONY: all test clean runner run_benchmark test_vm
