#include "../../src/compiler/compiler.h"
#include "../../src/AST/ast.h"
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
    assert(result->count > 0);
    assert(result->constants_count > 0);
    printf("✓ Compilation completed successfully\n");
    printf("✓ Generated %zu bytecode instructions\n", result->count);
    printf("✓ Constants pool size: %zu\n", result->constants_count);
    
    bool has_load_const = false;
    bool has_pop_top = false;
    
    for (size_t i = 0; i < result->count; i++) {
        bytecode bc = result->code[i];
        if (bc.op_code == LOAD_CONST) has_load_const = true;
        if (bc.op_code == POP_TOP) has_pop_top = true;
    }
    
    assert(has_load_const);
    //assert(has_pop_top);
    //printf("✓ Contains LOAD_CONST and POP_TOP instructions\n");
    
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
    assert(result->constants_count >= 1);
    assert(result->constants[0].type == VAL_BOOL);
    assert(result->constants[0].bool_val == true);
    printf("✓ Boolean constant value is correct: %s\n", 
           result->constants[0].bool_val ? "true" : "false");
    
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
    printf("✓ Generated %zu bytecode instructions\n", result->count);
    printf("✓ Constants pool size: %zu\n", result->constants_count);
    
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

// gcc tests/compiler/test_compiler.c src/compiler/compiler.c src/compiler/value.c src/compiler/scope.c src/compiler/string_table.c src/compiler/bytecode.c

int main() {
    printf("Starting Compiler Tests...\n\n");
    
    test_compiler_creation_and_destruction();
    test_compile_literal_expression();
    test_compile_bool_literal();
    test_compile_empty_block();
    //test_null_safety();
    
    printf("All tests passed! ✅\n");
    return 0;
}
