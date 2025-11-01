#include "../../src/compiler/compiler.h"
#include "../../src/AST/ast.h"
#include "../../src/lexer/token.h"
#include <stdio.h>
#include <assert.h>

void test_compiler_creation_and_destruction() {
    printf("=== Test: Compiler Creation and Destruction ===\n");
    
    // Arrange
    SourceLocation loc = {0, 0};
    ASTNode* literal = ast_new_literal_expression(loc, TYPE_INT, 5);
    ASTNode* expression_stmt = ast_new_expression_statement(loc, literal);
    ASTNode* statements[] = {expression_stmt};
    ASTNode* block_stmt = ast_new_block_statement(loc, statements, 1);
    
    // Act
    compiler* comp = compiler_create(block_stmt);
    
    // Assert
    assert(comp != NULL);
    assert(comp->ast_tree == block_stmt);
    assert(comp->result != NULL);
    assert(comp->result->code_array.bytecodes == NULL);
    assert(comp->result->code_array.count == 0);
    assert(comp->result->code_array.capacity == 0);
    printf("✓ Compiler created successfully\n");
    
    // Cleanup
    compiler_destroy(comp);
    printf("✓ Compiler destroyed successfully\n\n");
}

void test_compile_literal_expression() {
    printf("=== Test: Compile Literal Expression ===\n");
    
    // Arrange
    SourceLocation loc = {0, 0};
    ASTNode* literal = ast_new_literal_expression(loc, TYPE_INT, 42);
    ASTNode* expression_stmt = ast_new_expression_statement(loc, literal);
    ASTNode* statements[] = {expression_stmt};
    ASTNode* block_stmt = ast_new_block_statement(loc, statements, 1);
    compiler* comp = compiler_create(block_stmt);
    
    // Act
    compilation_result* result = compiler_compile(comp);
    
    // Assert
    assert(result != NULL);
    assert(result->code_array.count > 0);
    assert(result->code_array.bytecodes != NULL);
    assert(result->constants_count > 0);
    printf("✓ Compilation completed successfully\n");
    printf("✓ Generated %u bytecode instructions\n", result->code_array.count);
    printf("✓ Constants pool size: %zu\n", result->constants_count);
    
    bool has_load_const = false;
    
    for (uint32_t i = 0; i < result->code_array.count; i++) {
        bytecode bc = result->code_array.bytecodes[i];
        printf("  Bytecode[%u]: op_code=%u\n", i, bc.op_code);
        if (bc.op_code == LOAD_CONST) {
            has_load_const = true;
            uint32_t const_index = bytecode_get_arg(bc);
            printf("  LOAD_CONST found with const_index=%u\n", const_index);
            assert(const_index < result->constants_count);
        }
    }
    
    assert(has_load_const);
    printf("✓ Contains LOAD_CONST instruction\n");
    
    assert(result->constants_count >= 1);
    assert(result->constants[0].type == VAL_INT);
    assert(result->constants[0].int_val == 42);
    printf("✓ Constant value is correct: %lld\n", result->constants[0].int_val);
    
    // Cleanup
    compiler_destroy(comp);
    printf("✓ Test completed successfully\n\n");
}

void test_compile_bool_literal() {
    printf("=== Test: Compile Bool Literal ===\n");
    
    // Arrange
    SourceLocation loc = {0, 0};
    ASTNode* literal = ast_new_literal_expression(loc, TYPE_BOOL, true);
    ASTNode* expression_stmt = ast_new_expression_statement(loc, literal);
    ASTNode* statements[] = {expression_stmt};
    ASTNode* block_stmt = ast_new_block_statement(loc, statements, 1);
    compiler* comp = compiler_create(block_stmt);
    
    // Act
    compilation_result* result = compiler_compile(comp);
    
    // Assert
    assert(result != NULL);
    assert(result->code_array.bytecodes != NULL);
    assert(result->constants_count >= 1);
    assert(result->constants[0].type == VAL_BOOL);
    assert(result->constants[0].bool_val == true);
    printf("✓ Boolean constant value is correct: %s\n", 
           result->constants[0].bool_val ? "true" : "false");
    
    bool has_load_const = false;
    for (uint32_t i = 0; i < result->code_array.count; i++) {
        bytecode bc = result->code_array.bytecodes[i];
        if (bc.op_code == LOAD_CONST) {
            has_load_const = true;
            uint32_t const_index = bytecode_get_arg(bc);
            assert(const_index < result->constants_count);
            assert(result->constants[const_index].type == VAL_BOOL);
            assert(result->constants[const_index].bool_val == true);
        }
    }
    assert(has_load_const);
    printf("✓ Contains correct LOAD_CONST instruction for boolean\n");
    
    // Cleanup
    compiler_destroy(comp);
    printf("✓ Test completed successfully\n\n");
}

void test_compile_empty_block() {
    printf("=== Test: Compile Empty Block ===\n");
    
    // Arrange
    SourceLocation loc = {0, 0};
    ASTNode* statements[] = {};
    ASTNode* block_stmt = ast_new_block_statement(loc, statements, 0);
    compiler* comp = compiler_create(block_stmt);
    
    // Act
    compilation_result* result = compiler_compile(comp);
    
    // Assert
    assert(result != NULL);
    printf("✓ Empty block compilation completed\n");
    printf("✓ Generated %u bytecode instructions\n", result->code_array.count);
    printf("✓ Constants pool size: %zu\n", result->constants_count);
    
    if (result->code_array.count == 0) {
        printf("✓ No bytecode generated (as expected for empty block)\n");
    }
    
    // Cleanup
    compiler_destroy(comp);
    printf("✓ Test completed successfully\n\n");
}

void test_null_safety() {
    printf("=== Test: Null Safety ===\n");
    
    // Act & Assert
    compiler* null_comp = compiler_create(NULL);
    assert(null_comp == NULL);
    printf("✓ compiler_create(NULL) returns NULL\n");
    
    compiler_destroy(NULL);
    printf("✓ compiler_destroy(NULL) is safe\n");
    
    compilation_result* null_result = compiler_compile(NULL);
    assert(null_result == NULL);
    printf("✓ compiler_compile(NULL) returns NULL\n");
    
    printf("✓ Null safety test completed successfully\n\n");
}

void test_bytecode_integrity() {
    printf("=== Test: Bytecode Integrity ===\n");
    
    // Arrange
    SourceLocation loc = {0, 0};
    ASTNode* literal = ast_new_literal_expression(loc, TYPE_INT, 123);
    ASTNode* expression_stmt = ast_new_expression_statement(loc, literal);
    ASTNode* statements[] = {expression_stmt};
    ASTNode* block_stmt = ast_new_block_statement(loc, statements, 1);
    compiler* comp = compiler_create(block_stmt);
    
    // Act
    compilation_result* result = compiler_compile(comp);
    
    // Assert
    assert(result != NULL);
    assert(result->code_array.bytecodes != NULL);
    
    for (uint32_t i = 0; i < result->code_array.count; i++) {
        bytecode bc = result->code_array.bytecodes[i];
        printf("  Bytecode[%u]: op_code=%u\n", i, bc.op_code);
        
        assert(bc.op_code <= 255);
        assert(bc.op_code == LOAD_CONST);
    }
    
    printf("✓ All bytecode instructions have valid op_codes\n");
    
    // Cleanup
    compiler_destroy(comp);
    printf("✓ Bytecode integrity test completed successfully\n\n");
}

void test_compile_variable_expression_no_such_var(){
    printf("=== Test: compile variable expr no such var ===\n");
    // Arrange
    SourceLocation loc = {0, 0};
    ASTNode* variable = ast_new_variable_expression(loc, "x");
    ASTNode* expression_stmt = ast_new_expression_statement(loc, variable);
    ASTNode* statements[] = {expression_stmt};
    ASTNode* block_stmt = ast_new_block_statement(loc, statements, 1);
    compiler* comp = compiler_create(block_stmt);

    // Act
    compilation_result* result = compiler_compile(comp);
    
    // Assert
    assert(result != NULL);
    assert(result->code_array.bytecodes != NULL);
    assert(result->code_array.bytecodes[0].op_code == NOP);

    // Cleanup
    compiler_destroy(comp);
    printf("✓ compile variable expr test completed successfully\n\n");
}

void test_compile_variable_expression(){
    printf("=== Test: compile variable expr ===\n");
    // Arrange
    SourceLocation loc = {0, 0};
    ASTNode* assign = ast_new_variable_declaration_statement(loc, TYPE_INT, "x", ast_new_literal_expression(loc, TYPE_INT, 5));

    ASTNode* variable = ast_new_variable_expression(loc, "x");
    ASTNode* expression_stmt = ast_new_expression_statement(loc, variable);

    ASTNode* statements[] = {assign, expression_stmt};
    ASTNode* block_stmt = ast_new_block_statement(loc, statements, 2);
    compiler* comp = compiler_create(block_stmt);

    // Act
    compilation_result* result = compiler_compile(comp);
    
    // Assert
    assert(result != NULL);
    assert(result->code_array.bytecodes != NULL);
    assert(result->code_array.count > 1);
    assert(result->code_array.bytecodes[0].op_code == NOP);

    // Cleanup
    compiler_destroy(comp);
    printf("✓ compile variable expr test completed successfully\n\n");
}

void test_compile_binary_expression_plus() {
    printf("=== Test: Compile Binary Expression Plus ===\n");
    
    // Arrange
    SourceLocation loc = {0, 0};
    ASTNode* left = ast_new_literal_expression(loc, TYPE_INT, 5);
    ASTNode* right = ast_new_literal_expression(loc, TYPE_INT, 5);
    Token* plus_token = token_create(OP_PLUS, "+", 1, 1);
    ASTNode* binary = ast_new_binary_expression(loc, left, *plus_token, right);
    ASTNode* expression_stmt = ast_new_expression_statement(loc, binary);
    ASTNode* statements[] = {expression_stmt};
    ASTNode* block_stmt = ast_new_block_statement(loc, statements, 1);
    compiler* comp = compiler_create(block_stmt);
    
    // Act
    compilation_result* result = compiler_compile(comp);
    
    // Assert
    assert(result != NULL);
    assert(result->code_array.bytecodes != NULL);
    assert(result->code_array.count > 0);
    printf("✓ Binary expression compilation completed\n");
    printf("✓ Generated %u bytecode instructions\n", result->code_array.count);
    
    // Cleanup
    compiler_destroy(comp);
    token_free(plus_token);
    printf("✓ Test completed successfully\n\n");
}

void test_compile_binary_expression_minus() {
    printf("=== Test: Compile Binary Expression Minus ===\n");
    
    // Arrange
    SourceLocation loc = {0, 0};
    ASTNode* left = ast_new_literal_expression(loc, TYPE_INT, 10);
    ASTNode* right = ast_new_literal_expression(loc, TYPE_INT, 5);
    Token* minus_token = token_create(OP_MINUS, "-", 1, 1);
    ASTNode* binary = ast_new_binary_expression(loc, left, *minus_token, right);
    ASTNode* expression_stmt = ast_new_expression_statement(loc, binary);
    ASTNode* statements[] = {expression_stmt};
    ASTNode* block_stmt = ast_new_block_statement(loc, statements, 1);
    compiler* comp = compiler_create(block_stmt);
    
    // Act
    compilation_result* result = compiler_compile(comp);
    
    // Assert
    assert(result != NULL);
    assert(result->code_array.bytecodes != NULL);
    assert(result->code_array.count > 0);
    printf("✓ Binary expression compilation completed\n");
    printf("✓ Generated %u bytecode instructions\n", result->code_array.count);
    
    // Cleanup
    compiler_destroy(comp);
    token_free(minus_token);
    printf("✓ Test completed successfully\n\n");
}

void test_compile_binary_expression_mult() {
    printf("=== Test: Compile Binary Expression Multiply ===\n");
    
    // Arrange
    SourceLocation loc = {0, 0};
    ASTNode* left = ast_new_literal_expression(loc, TYPE_INT, 3);
    ASTNode* right = ast_new_literal_expression(loc, TYPE_INT, 4);
    Token* star_token = token_create(OP_MULT, "*", 1, 1);
    ASTNode* binary = ast_new_binary_expression(loc, left, *star_token, right);
    ASTNode* expression_stmt = ast_new_expression_statement(loc, binary);
    ASTNode* statements[] = {expression_stmt};
    ASTNode* block_stmt = ast_new_block_statement(loc, statements, 1);
    compiler* comp = compiler_create(block_stmt);
    
    // Act
    compilation_result* result = compiler_compile(comp);
    
    // Assert
    assert(result != NULL);
    assert(result->code_array.bytecodes != NULL);
    assert(result->code_array.count > 0);
    printf("✓ Binary expression compilation completed\n");
    printf("✓ Generated %u bytecode instructions\n", result->code_array.count);
    
    // Cleanup
    compiler_destroy(comp);
    token_free(star_token);
    printf("✓ Test completed successfully\n\n");
}

void test_compile_binary_expression_div() {
    printf("=== Test: Compile Binary Expression Divide ===\n");
    
    // Arrange
    SourceLocation loc = {0, 0};
    ASTNode* left = ast_new_literal_expression(loc, TYPE_INT, 15);
    ASTNode* right = ast_new_literal_expression(loc, TYPE_INT, 3);
    Token* slash_token = token_create(OP_DIV, "/", 1, 1);
    ASTNode* binary = ast_new_binary_expression(loc, left, *slash_token, right);
    ASTNode* expression_stmt = ast_new_expression_statement(loc, binary);
    ASTNode* statements[] = {expression_stmt};
    ASTNode* block_stmt = ast_new_block_statement(loc, statements, 1);
    compiler* comp = compiler_create(block_stmt);
    
    // Act
    compilation_result* result = compiler_compile(comp);
    
    // Assert
    assert(result != NULL);
    assert(result->code_array.bytecodes != NULL);
    assert(result->code_array.count > 0);
    printf("✓ Binary expression compilation completed\n");
    printf("✓ Generated %u bytecode instructions\n", result->code_array.count);
    
    // Cleanup
    compiler_destroy(comp);
    token_free(slash_token);
    printf("✓ Test completed successfully\n\n");
}

// gcc tests/compiler/test_compiler.c src/compiler/compiler.c src/compiler/value.c src/compiler/scope.c src/compiler/string_table.c src/compiler/bytecode.c src/AST/ast.c src/lexer/token.c
int main() {
    printf("Starting Compiler Tests...\n\n");
    
    test_compiler_creation_and_destruction();
    test_compile_literal_expression();
    test_compile_bool_literal();
    test_compile_empty_block();
    test_null_safety();
    test_bytecode_integrity();
    test_compile_variable_expression_no_such_var();
    //test_compile_variable_expression(); todo
    test_compile_binary_expression_plus();
    test_compile_binary_expression_minus();
    test_compile_binary_expression_mult();
    test_compile_binary_expression_div();
    
    printf("All tests passed! ✅\n");
    return 0;
}
