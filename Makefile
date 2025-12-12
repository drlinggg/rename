CC = gcc
CFLAGS = -I src -I src/AST -I src/lexer -I src/parser -I src/compiler
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

# Test files
AST_TEST = $(TEST_DIR)/ast/test_ast.c
LEXER_TEST = $(TEST_DIR)/lexer/test_lexer.c
PARSER_TEST = $(TEST_DIR)/parser/test_parser.c
BYTECODE_TEST = $(TEST_DIR)/bytecode/test_bytecode.c
COMPILER_TEST = $(TEST_DIR)/compiler/test_compiler.c

# Targets
all: test_ast test_lexer test_parser test_bytecode test_compiler

test_ast: $(AST_TEST) $(AST_SRC)
	$(CC) $(CFLAGS) $(AST_TEST) $(TOKEN_SRC) $(AST_SRC) -o $@

test_lexer: $(LEXER_TEST) $(LEXER_SRC)
	$(CC) $(CFLAGS) $(LEXER_TEST) $(TOKEN_SRC) $(LEXER_SRC) -o $@

test_parser: $(PARSER_TEST) $(PARSER_SRC) $(AST_SRC) $(LEXER_SRC)
	$(CC) $(CFLAGS) $(PARSER_TEST) $(PARSER_SRC) $(TOKEN_SRC) $(AST_SRC) $(LEXER_SRC) -o $@

test_bytecode: $(BYTECODE_TEST) $(BYTECODE_SRC)
	$(CC) $(CFLAGS) $(BYTECODE_TEST) $(BYTECODE_SRC) -o $@

test_compiler: $(COMPILER_TEST) $(COMPILER_SRC) $(VALUE_SRC) $(SCOPE_SRC) $(STRING_TABLE_SRC) $(BYTECODE_SRC) $(AST_SRC) $(TOKEN_SRC)
	$(CC) $(CFLAGS) $(COMPILER_TEST) $(COMPILER_SRC) $(VALUE_SRC) $(SCOPE_SRC) $(STRING_TABLE_SRC) $(BYTECODE_SRC) $(AST_SRC) $(TOKEN_SRC) -o $@

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
	@echo "[Make] All tests passed!"

test_vm: $(TEST_DIR)/runtime/test_vm.c $(BYTECODE_SRC) $(VALUE_SRC) $(BYTECODE_SRC) $(AST_SRC) $(TOKEN_SRC) src/runtime/vm/object.c src/runtime/vm/heap.c src/runtime/vm/vm.c src/runtime/gc/gc.c src/runtime/jit/jit.c
	$(CC) $(CFLAGS) $(TEST_DIR)/runtime/test_vm.c $(BYTECODE_SRC) $(VALUE_SRC) $(AST_SRC) $(TOKEN_SRC) src/runtime/vm/object.c src/runtime/vm/heap.c src/runtime/vm/vm.c src/runtime/gc/gc.c src/runtime/jit/jit.c -o $@

.PHONY: test_vm

runner: tools/runner.c $(BYTECODE_SRC) $(VALUE_SRC) $(BYTECODE_SRC) $(AST_SRC) $(TOKEN_SRC) $(COMPILER_SRC) $(SCOPE_SRC) $(STRING_TABLE_SRC) src/runtime/vm/object.c src/runtime/vm/heap.c src/runtime/vm/vm.c src/runtime/gc/gc.c src/runtime/jit/jit.c src/lexer/lexer.c src/parser/parser.c
	@mkdir -p bin
	$(CC) $(CFLAGS) tools/runner.c $(BYTECODE_SRC) $(VALUE_SRC) src/debug.c $(AST_SRC) $(TOKEN_SRC) $(COMPILER_SRC) $(SCOPE_SRC) $(STRING_TABLE_SRC) src/runtime/vm/object.c src/runtime/vm/heap.c src/runtime/vm/vm.c src/runtime/gc/gc.c src/runtime/jit/jit.c src/lexer/lexer.c src/parser/parser.c -o bin/rename

.PHONY: runner

run_benchmark: runner
	@echo "[Make] Running benchmark: benchmarks/first_program.lang"
	./bin/rename benchmarks/first_program.lang

.PHONY: run_benchmark

clean:
	@echo "[Make] Cleaning ..."
	rm -f test_ast test_lexer test_parser test_bytecode test_compiler
	rm -f bin/rename

.PHONY: all test clean
