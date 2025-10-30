CC = gcc
CFLAGS = -I src -I src/AST -I src/lexer -I src/parser -I src/compiler
SRC_DIR = src
TEST_DIR = tests

# Source files
AST_SRC = $(SRC_DIR)/AST/ast.c
LEXER_SRC = $(SRC_DIR)/lexer/lexer.c
PARSER_SRC = $(SRC_DIR)/parser/parser.c
BYTECODE_SRC = $(SRC_DIR)/compiler/bytecode.c

# Test files
AST_TEST = $(TEST_DIR)/ast/test_ast.c
LEXER_TEST = $(TEST_DIR)/lexer/test_lexer.c
PARSER_TEST = $(TEST_DIR)/parser/test_parser.c
BYTECODE_TEST = $(TEST_DIR)/bytecode/test_bytecode.c

# Targets
all: test_ast test_lexer test_parser test_bytecode

test_ast: $(AST_TEST) $(AST_SRC)
	$(CC) $(CFLAGS) $(AST_TEST) $(AST_SRC) -o $@

test_lexer: $(LEXER_TEST) $(LEXER_SRC)
	$(CC) $(CFLAGS) $(LEXER_TEST) $(LEXER_SRC) -o $@

test_parser: $(PARSER_TEST) $(PARSER_SRC) $(AST_SRC) $(LEXER_SRC)
	$(CC) $(CFLAGS) $(PARSER_TEST) $(PARSER_SRC) $(AST_SRC) $(LEXER_SRC) -o $@

test_bytecode: $(BYTECODE_TEST) $(BYTECODE_SRC)
	$(CC) $(CFLAGS) $(BYTECODE_TEST) $(BYTECODE_SRC) -o $@

test: all
	@echo "[Make] Running AST tests..."
	./test_ast || exit 1
	@echo "[Make] Running Lexer tests..."
	./test_lexer || exit 1
	@echo "[Make] Running Parser tests..."
	./test_parser || exit 1
	@echo "[Make] Running Bytecode tests..."
	./test_bytecode || exit 1
	@echo "[Make] tests passed!"

clean:
	rm -f test_ast test_lexer test_parser test_bytecode

.PHONY: all test clean
