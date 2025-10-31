#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static bytecode_array concat_bytecode_arrays(bytecode_array a, bytecode_array b) {
    if (a.count == 0) return b;
    if (b.count == 0) return a;
    
    bytecode* new_bytecodes = malloc((a.count + b.count) * sizeof(bytecode));
    if (!new_bytecodes) return create_bytecode_array(NULL, 0);
    
    memcpy(new_bytecodes, a.bytecode, a.count * sizeof(bytecode));
    memcpy(new_bytecodes + a.count, b.bytecode, b.count * sizeof(bytecode));
    
    return create_bytecode_array(new_bytecodes, a.count + b.count);
}

static void free_bytecode_array(bytecode_array array) {
    if (array.bytecode) {
        free(array.bytecode);
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

static size_t compiler_add_constant(compiler* comp, Value value) {
    compilation_result* result = comp->result;
    
    for (size_t i = 0; i < result->constants_count; i++) {
        if (values_equal(result->constants[i], value)) {
            return i;
        }
    }
    
    if (result->constants_count >= result->constants_capacity) {
        size_t new_capacity = result->constants_capacity == 0 ? 8 : result->constants_capacity * 2;
        Value* new_constants = realloc(result->constants, new_capacity * sizeof(Value));
        if (!new_constants) return SIZE_MAX;
        
        result->constants = new_constants;
        result->constants_capacity = new_capacity;
    }
    
    result->constants[result->constants_count] = value;
    return result->constants_count++;
}

static size_t compiler_add_global_name(compiler* comp, const char* name) {
    if (!comp->global_names) {
        comp->global_names = string_table_create();
    }
    return string_table_add(comp->global_names, name);
}

static size_t compiler_add_local_name(compiler* comp, const char* name) {
    if (!comp->current_scope) {
        comp->current_scope = scope_create(NULL);
    }
    return scope_add_local(comp->current_scope, name);
}

static size_t compiler_resolve_variable(compiler* comp, const char* name) {
    // Сначала ищем в локальных переменных
    if (comp->current_scope) {
        int32_t local_index = scope_find_local(comp->current_scope, name);
        if (local_index >= 0) {
            return (size_t)local_index;
        }
    }
    
    // Не нашли - добавляем в глобальные
    return compiler_add_global_name(comp, name);
}

// Statements - оставляем пустыми как требуется
static bytecode_array compiler_compile_function_declaration(compiler* comp, ASTNode* node) {
    return create_bytecode_array(NULL, 0);
}

static bytecode_array compiler_compile_variable_declaration(compiler* comp, ASTNode* node) {
    return create_bytecode_array(NULL, 0);
}

static bytecode_array compiler_compile_return_statement(compiler* comp, ASTNode* node) {
    return create_bytecode_array(NULL, 0);
}

static bytecode_array compiler_compile_if_statement(compiler* comp, ASTNode* node) {
    return create_bytecode_array(NULL, 0);
}

static bytecode_array compiler_compile_while_statement(compiler* comp, ASTNode* node) {
    return create_bytecode_array(NULL, 0);
}

static bytecode_array compiler_compile_for_statement(compiler* comp, ASTNode* node) {
    return create_bytecode_array(NULL, 0);
}

static bytecode_array compiler_compile_assignment_statement(compiler* comp, ASTNode* node) {
    return create_bytecode_array(NULL, 0);
}

// Expressions - оставляем пустыми как требуется
static bytecode_array compiler_compile_binary_expression(compiler* comp, ASTNode* node) {
    return create_bytecode_array(NULL, 0);
}

static bytecode_array compiler_compile_unary_expression(compiler* comp, ASTNode* node) {
    return create_bytecode_array(NULL, 0);
}

static bytecode_array compiler_compile_variable_expression(compiler* comp, ASTNode* node) {
    return create_bytecode_array(NULL, 0);
}

static bytecode_array compiler_compile_function_call_expression(compiler* comp, ASTNode* node) {
    return create_bytecode_array(NULL, 0);
}

static bytecode_array compiler_compile_literal_expression(compiler* comp, ASTNode* node) {
    LiteralExpression* literal = (LiteralExpression*)node;

    if (literal->type == TYPE_INT) {
        int64_t value = literal->value;
        Value constant_value = value_create_int(value);
        uint32_t const_index = compiler_add_constant(comp, constant_value);
        bytecode bc = bytecode_create_with_number(LOAD_CONST, const_index);
        bytecode* bc_array = malloc(sizeof(bytecode));
        bc_array[0] = bc;
        return create_bytecode_array(bc_array, 1);
    }
    else if (literal->type == TYPE_LONG) {
        fprintf(stderr, "Error: long literals are not supported in this version\n");
        Value constant_value = value_create_int(0);
        uint32_t const_index = compiler_add_constant(comp, constant_value);
        bytecode bc = bytecode_create_with_number(LOAD_CONST, const_index);
        bytecode* bc_array = malloc(sizeof(bytecode));
        bc_array[0] = bc;
        return create_bytecode_array(bc_array, 1);
    }
    else if (literal->type == TYPE_BOOL) {
        bool value = (bool)literal->value;
        Value constant_value = value_create_bool(value);
        uint32_t const_index = compiler_add_constant(comp, constant_value);
        bytecode bc = bytecode_create_with_number(LOAD_CONST, const_index);
        bytecode* bc_array = malloc(sizeof(bytecode));
        bc_array[0] = bc;
        return create_bytecode_array(bc_array, 1);
    }
    else {
        fprintf(stderr, "Error: type %d is not supported in this version\n", literal->type);
        Value constant_value = value_create_null();
        uint32_t const_index = compiler_add_constant(comp, constant_value);
        bytecode bc = bytecode_create_with_number(LOAD_CONST, const_index);
        bytecode* bc_array = malloc(sizeof(bytecode));
        bc_array[0] = bc;
        return create_bytecode_array(bc_array, 1);
    }
}

static bytecode_array compiler_compile_expression(compiler* comp, ASTNode* node) {
    switch (node->node_type) {
        case NODE_BINARY_EXPRESSION:
            return compiler_compile_binary_expression(comp, node);
        case NODE_UNARY_EXPRESSION:
            return compiler_compile_unary_expression(comp, node);
        case NODE_LITERAL_EXPRESSION:
            return compiler_compile_literal_expression(comp, node);
        case NODE_VARIABLE_EXPRESSION:
            return compiler_compile_variable_expression(comp, node);
        case NODE_FUNCTION_CALL_EXPRESSION:
            return compiler_compile_function_call_expression(comp, node);
        default:
            fprintf(stderr, "Unknown expression node type: %d\n", node->node_type);
            return create_bytecode_array(NULL, 0);
    }
}

static bytecode_array compiler_compile_expression_statement(compiler* comp, ASTNode* statement) {
    if (statement->node_type != NODE_EXPRESSION_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }

    ASTNode* expression = ((ExpressionStatement*) statement)->expression;
    switch (expression->node_type) {
        case NODE_BINARY_EXPRESSION:
            return compiler_compile_binary_expression(comp, expression);
        case NODE_UNARY_EXPRESSION:
            return compiler_compile_unary_expression(comp, expression);
        case NODE_LITERAL_EXPRESSION:
            return compiler_compile_literal_expression(comp, expression);
        case NODE_VARIABLE_EXPRESSION:
            return compiler_compile_variable_expression(comp, expression);
        case NODE_FUNCTION_CALL_EXPRESSION:
            return compiler_compile_function_call_expression(comp, expression);
        default:
            fprintf(stderr, "Unknown ast_node type: %d\n", statement->node_type);
            return create_bytecode_array(NULL, 0);
    }
}

static bytecode_array compiler_compile_block_statement(compiler* comp, ASTNode* node) {
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
            fprintf(stderr, "Unknown statement node type: %d\n", statement->node_type);
            return create_bytecode_array(NULL, 0);
    }
}

// Основные функции компилятора
compiler* compiler_create(ASTNode* ast_tree) {
    compiler* comp = malloc(sizeof(compiler));
    if (!comp) return NULL;
    
    comp->ast_tree = ast_tree;
    
    comp->result = malloc(sizeof(compilation_result));
    if (!comp->result) {
        free(comp);
        return NULL;
    }
    
    // Инициализируем результат компиляции
    comp->result->code = NULL;
    comp->result->count = 0;
    comp->result->capacity = 0;
    comp->result->constants = NULL;
    comp->result->constants_count = 0;
    comp->result->constants_capacity = 0;
    
    // Инициализируем таблицы компилятора
    comp->global_names = NULL;
    comp->current_scope = scope_create(NULL);
    if (!comp->current_scope) {
        free(comp->result);
        free(comp);
        return NULL;
    }
    
    return comp;
}

void compiler_destroy(compiler* comp) {
    if (!comp) return;

    if (comp->result) {
        free(comp->result->code);
        free(comp->result->constants);
        free(comp->result);
    }
    
    if (comp->global_names) {
        string_table_destroy(comp->global_names);
    }
    
    if (comp->current_scope) {
        scope_destroy(comp->current_scope);
    }
    if (comp->ast_tree) {
        ast_free(comp->ast_tree);
    }
    
    free(comp);
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
