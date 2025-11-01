#include "compiler.h"
#include "scope.h"
#include "string_table.h"
#include "../lexer/token.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Forward declarations
static bytecode_array compiler_compile_expression(compiler* comp, ASTNode* node);

static bytecode_array concat_bytecode_arrays(bytecode_array a, bytecode_array b) {
    bytecode* new_bytecodes = malloc((a.count + b.count) * sizeof(bytecode));
    if (!new_bytecodes) return create_bytecode_array(NULL, 0);
    
    memcpy(new_bytecodes, a.bytecodes, a.count * sizeof(bytecode));
    memcpy(new_bytecodes + a.count, b.bytecodes, b.count * sizeof(bytecode));
    
    return create_bytecode_array(new_bytecodes, a.count + b.count);
}

static void free_bytecode_array(bytecode_array array) {
    if (array.bytecodes) {
        free(array.bytecodes);
    }
}

static void emit_bytecode(compilation_result* result, bytecode_array array) {
    if (!array.bytecodes || array.count == 0) return;
    
    bytecode_array new_array = concat_bytecode_arrays(result->code_array, array);
    free_bytecode_array(result->code_array);
    result->code_array = new_array;
}

static void emit_single_bytecode(compilation_result* result, bytecode bc) {
    bytecode* bc_array = malloc(sizeof(bytecode));
    bc_array[0] = bc;
    bytecode_array array = create_bytecode_array(bc_array, 1);
    emit_bytecode(result, array);
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
    if (comp->current_scope) {
        int32_t local_index = scope_find_local(comp->current_scope, name);
        if (local_index >= 0) {
            return (size_t)local_index;
        }
    }
    
    return compiler_add_global_name(comp, name);
}

// Statements
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

// Expressions
static bytecode_array compiler_compile_binary_expression(compiler* comp, ASTNode* node) {
    BinaryExpression* bin_expr = (BinaryExpression*)node;
    if (node->node_type != NODE_BINARY_EXPRESSION) {
        return create_bytecode_array(NULL, 0);
    }

    bytecode_array bc_left = compiler_compile_expression(comp, bin_expr->left);
    bytecode_array bc_right = compiler_compile_expression(comp, bin_expr->right);
    bytecode_array concated = concat_bytecode_arrays(bc_left, bc_right);
    
    free_bytecode_array(bc_left);
    free_bytecode_array(bc_right);
    
    bytecode operator_bc;
    uint8_t op_code_value;
    
    switch (bin_expr->operator_.type) {
        case OP_PLUS:
            op_code_value = 0x00;  // BINARY_OP_ADD
            break;
        case OP_MINUS:
            op_code_value = 0x0A;  // BINARY_OP_SUBTRACT
            break;
        case OP_MULT:
            op_code_value = 0x05;  // BINARY_OP_MULTIPLY
            break;
        case OP_DIV:
            op_code_value = 0x0B;  // BINARY_OP_TRUE_DIVIDE
            break;
        case OP_MOD:
            op_code_value = 0x06;  // BINARY_OP_REMAINDER
            break;
        default:
            fprintf(stderr, "Invalid token for binary operation: %d\n", bin_expr->operator_);
            free_bytecode_array(concated);
            return create_bytecode_array(NULL, 0);
    }
    
    operator_bc = bytecode_create_with_number(BINARY_OP, op_code_value);
    
    bytecode* operator_array = malloc(sizeof(bytecode));
    if (!operator_array) {
        free_bytecode_array(concated);
        return create_bytecode_array(NULL, 0);
    }
    operator_array[0] = operator_bc;
    
    bytecode_array operator_bc_array = create_bytecode_array(operator_array, 1);
    bytecode_array result = concat_bytecode_arrays(concated, operator_bc_array);
    
    free_bytecode_array(concated);
    free_bytecode_array(operator_bc_array);
    
    return result;
}

static bytecode_array compiler_compile_unary_expression(compiler* comp, ASTNode* node) {
    UnaryExpression* unary_expr = (UnaryExpression*)node;
    if (node->node_type != NODE_UNARY_EXPRESSION) {
        return create_bytecode_array(NULL, 0);
    }

    bytecode_array bc_operand = compiler_compile_expression(comp, unary_expr->operand);
    
    bytecode operator_bc;
    uint8_t op_code_value;
    
    switch (unary_expr->operator_.type) {
        case OP_PLUS:
            op_code_value = 0x00;  // +x
            break;
        case OP_MINUS:
            op_code_value = 0x0A;  // -x
            break;
        case OP_NOT:
            op_code_value = 0x05;  // not x
            break;
        default:
            fprintf(stderr, "Invalid token for unary operation: %d\n", unary_expr->operator_);
            free_bytecode_array(bc_operand);
            return create_bytecode_array(NULL, 0);
    }
    
    operator_bc = bytecode_create_with_number(UNARY_OP, op_code_value);
    
    bytecode* operator_array = malloc(sizeof(bytecode));
    if (!operator_array) {
        free_bytecode_array(bc_operand);
        return create_bytecode_array(NULL, 0);
    }
    operator_array[0] = operator_bc;
    
    bytecode_array operator_bc_array = create_bytecode_array(operator_array, 1);
    bytecode_array result = concat_bytecode_arrays(bc_operand, operator_bc_array);
    
    free_bytecode_array(bc_operand);
    free_bytecode_array(operator_bc_array);
    
    return result;
}

static bytecode_array compiler_compile_variable_expression(compiler* comp, ASTNode* node) {
    VariableExpression* var_expr = (VariableExpression*)node;
    if (node->node_type != NODE_VARIABLE_EXPRESSION) {
        return create_bytecode_array(NULL, 0);
    }

    int32_t local_index = scope_find_local(comp->current_scope, var_expr->name);
    if (local_index >= 0) {
        bytecode bc = bytecode_create_with_number(LOAD_FAST, local_index);
        bytecode* bc_array = malloc(sizeof(bytecode));
        bc_array[0] = bc;
        return create_bytecode_array(bc_array, 1);
    }

    int32_t global_index = string_table_find(comp->global_names, var_expr->name);
    if (global_index >= 0) {
        bytecode bc = bytecode_create_with_number(LOAD_GLOBAL, global_index);
        bytecode* bc_array = malloc(sizeof(bytecode));
        bc_array[0] = bc;
        return create_bytecode_array(bc_array, 1);
    }
    
    fprintf(stderr, "Error: variable '%s' is not defined\n", var_expr->name);
    bytecode bc = bytecode_create(NOP, 0, 0, 0);
    bytecode* bc_array = malloc(sizeof(bytecode));
    bc_array[0] = bc;
    return create_bytecode_array(bc_array, 1);
}

static bytecode_array compiler_compile_function_call_expression(compiler* comp, ASTNode* node) {
    FunctionCallExpression* func_call = (FunctionCallExpression*)node;
    if (node->node_type != NODE_FUNCTION_CALL_EXPRESSION) {
        return create_bytecode_array(NULL, 0);
    }

    bytecode_array result_array = create_bytecode_array(NULL, 0);
    
    // 1. Callee
    bytecode_array callee_bc = compiler_compile_expression(comp, func_call->callee);
    emit_bytecode(comp->result, callee_bc);
    free_bytecode_array(callee_bc);
    
    // 2. Null
    bytecode push_null_bc = bytecode_create(PUSH_NULL, 0, 0, 0);
    bytecode* push_null_array = malloc(sizeof(bytecode));
    push_null_array[0] = push_null_bc;
    bytecode_array push_null_bc_array = create_bytecode_array(push_null_array, 1);
    emit_bytecode(comp->result, push_null_bc_array);
    free_bytecode_array(push_null_bc_array);
    
    // 3. Args
    int arg_count = 0;
    for (int i = 0; i < func_call->argument_count; i++) {
        bytecode_array arg_bc = compiler_compile_expression(comp, func_call->arguments[i]);
        emit_bytecode(comp->result, arg_bc);
        free_bytecode_array(arg_bc);
        arg_count++;
    }
    
    // 4. call_func
    bytecode call_func_bc = bytecode_create_with_number(CALL_FUNCTION, arg_count);
    bytecode* call_func_array = malloc(sizeof(bytecode));
    call_func_array[0] = call_func_bc;
    bytecode_array call_func_bc_array = create_bytecode_array(call_func_array, 1);
    emit_bytecode(comp->result, call_func_bc_array);
    free_bytecode_array(call_func_bc_array);
    
    return result_array;
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
    if (!node) return create_bytecode_array(NULL, 0);
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

compiler* compiler_create(ASTNode* ast_tree) {
    compiler* comp = malloc(sizeof(compiler));
    if (!comp) return NULL;
    if (!ast_tree) return NULL;
    
    comp->ast_tree = ast_tree;
    
    comp->result = malloc(sizeof(compilation_result));
    if (!comp->result) {
        free(comp);
        return NULL;
    }
    
    comp->result->code_array = create_bytecode_array(NULL, 0);
    comp->result->constants = NULL;
    comp->result->constants_count = 0;
    comp->result->constants_capacity = 0;
    
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
        free_bytecode_array(comp->result->code_array);
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
    if (!compiler) return NULL;
    if (compiler->ast_tree->node_type != NODE_BLOCK_STATEMENT) {
        return NULL;
    }

    BlockStatement* casted = (BlockStatement*) compiler->ast_tree;
    
    for (uint32_t i = 0; i < casted->statement_count; i++) {
        bytecode_array result = compiler_compile_statement(compiler, casted->statements[i]);
        emit_bytecode(compiler->result, result);
        free_bytecode_array(result);
    }
    return compiler->result;
}
