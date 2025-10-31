#include "compiler.h"
#include "../AST/ast.c"
#include <stdlib.h>

// Statements
// general
static bytecode_array compiler_compile_statement(compiler* comp, ASTNode* node);
// private
static bytecode_array compiler_compile_function_declaration(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_variable_declaration(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_expression_statement(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_return_statement(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_block_statement(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_if_statement(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_while_statement(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_for_statement(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_assignment_statement(compiler* comp, ASTNode* node);

// Expressions
// general
static bytecode_array compiler_compile_expression(compiler* comp, ASTNode* node);
// private
static bytecode_array compiler_compile_binary_expression(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_unary_expression(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_literal_expression(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_variable_expression(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_function_call_expression(compiler* comp, ASTNode* node);

// additional 
//static size_t compiler_add_constant(compiler* comp, Value value);
//static size_t compiler_add_global_name(compiler* comp, const char* name);
//static size_t compiler_add_local_name(compiler* comp, const char* name);
//static size_t compiler_resolve_variable(compiler* comp, const char* name);


// Statements
static bytecode_array compiler_compile_function_declaration(compiler* comp, ASTNode* node) {
}

static bytecode_array compiler_compile_variable_declaration(compiler* comp, ASTNode* node) {
}

static bytecode_array compiler_compile_return_statement(compiler* comp, ASTNode* node) {
}

static bytecode_array compiler_compile_block_statement(compiler* comp, ASTNode* node) {
}

static bytecode_array compiler_compile_if_statement(compiler* comp, ASTNode* node) {
}

static bytecode_array compiler_compile_while_statement(compiler* comp, ASTNode* node) {
}

static bytecode_array compiler_compile_for_statement(compiler* comp, ASTNode* node) {
}

static bytecode_array compiler_compile_assignment_statement(compiler* comp, ASTNode* node) {
}

// Expressions
static bytecode_array compiler_compile_binary_expression(compiler* comp, ASTNode* node) {
}

static bytecode_array compiler_compile_unary_expression(compiler* comp, ASTNode* node) {
}

static bytecode_array compiler_compile_literal_expression(compiler* comp, ASTNode* node) {
}

static bytecode_array compiler_compile_variable_expression(compiler* comp, ASTNode* node) {
}

static bytecode_array compiler_compile_function_call_expression(compiler* comp, ASTNode* node) {
}

static bytecode_array compiler_compile_expression_statement(compiler* comp, ASTNode* statement) {
    switch (statement->node_type) {
        case NODE_BINARY_EXPRESSION:
            return compiler_compile_binary_expression(comp, statement);
        case NODE_UNARY_EXPRESSION:
            return compiler_compile_unary_expression(comp, statement);
        case NODE_LITERAL_EXPRESSION:
            return compiler_compile_literal_expression(comp, statement);
        case NODE_VARIABLE_EXPRESSION:
            return compiler_compile_variable_expression(comp, statement);
        case NODE_FUNCTION_CALL_EXPRESSION:
            return compiler_compile_function_call_expression(comp, statement);
        default:
            fprintf(stderr, "Unknown node type: %d\n", statement->node_type);
            return create_bytecode_array(NULL, 0);
    }
}

static bytecode_array compiler_compile_statement(compiler* comp, ASTNode* statement) {
    switch (statement->node_type) {
        case NODE_FUNCTION_DECLARATION_STATEMENT:
            return compiler_compile_function_declaration(comp, statement);
        case NODE_VARIABLE_DECLARATION_STATEMENT:
            return compiler_compile_variable_declaration(comp, statement);
        case NODE_EXPRESSION_STATEMENT:
            return compiler_compile_expression_statement(comp, statement);
        case NODE_RETURN_STATEMENT:
            return compiler_compile_return_statement(comp, statement);
        case NODE_BLOCK_STATEMENT:
            return compiler_compile_block_statement(comp, statement);
        case NODE_IF_STATEMENT:
            return compiler_compile_if_statement(comp, statement);
        case NODE_WHILE_STATEMENT:
            return compiler_compile_while_statement(comp, statement);
        case NODE_FOR_STATEMENT:
            return compiler_compile_for_statement(comp, statement);
        case NODE_ASSIGNMENT_STATEMENT:
            return compiler_compile_assignment_statement(comp, statement);
        
        default:
            fprintf(stderr, "Unknown node type: %d\n", statement->node_type);
            return create_bytecode_array(NULL, 0);
    }
}

static void emit_bytecode(compilation_result* result, bytecode* bcs, size_t count) {
    if (!bcs || count == 0) return;
    
    if (result->count + count > result->capacity) {
        size_t new_capacity = result->capacity == 0 ? 10 : result->capacity * 2;
        while (new_capacity < result->count + count) {
            new_capacity *= 2;
        }
        
        result->capacity = new_capacity;
        result->code = realloc(result->code, result->capacity * sizeof(bytecode));
    }
    
    for (size_t i = 0; i < count; i++) {
        result->code[result->count++] = bcs[i];
    }
}

static void emit_single_bytecode(compilation_result* result, bytecode bc) {
    emit_bytecode(result, &bc, 1);
}

compiler* compiler_create(ASTNode* ast_tree) {
    compiler* comp = malloc(sizeof(compiler));
    comp->ast_tree = ast_tree;
    comp->ptr = 0;
    comp->result = malloc(sizeof(compilation_result));
    comp->result->code = NULL;
    comp->result->count = 0;
    comp->result->capacity = 0;
    return comp;
}

void compiler_destroy(compiler* compiler) {
    if (!compiler) {
        return;
    }

    if (compiler->result) {
        free(compiler->result->code);
        free(compiler->result);
    }

    if (compiler->ast_tree) {
        ast_free(compiler->ast_tree);
    }
    free(compiler);
}

compilation_result* compiler_compile(compiler* compiler) {
    if (compiler->ast_tree->node_type != NODE_BLOCK_STATEMENT) {
        return NULL;
    }

    BlockStatement* casted = (BlockStatement*) compiler->ast_tree;

    for (uint32_t i = 0; i < casted->statement_count; i++) {
        bytecode_array result = compiler_compile_statement(compiler, casted->statements[i]);
        emit_bytecode(compiler->result, result.bytecode, result.count);
    }

    return compiler->result;
}
