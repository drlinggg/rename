
#include "compiler.h"
#include "scope.h"
#include "string_table.h"
#include "../lexer/token.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../debug.h"

// Forward declarations
static bytecode_array compiler_compile_expression(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_statement(compiler* comp, ASTNode* node);
static bytecode_array compiler_compile_block_statement(compiler* comp, ASTNode* node);

static bytecode_array concat_bytecode_arrays(bytecode_array a, bytecode_array b) {
    size_t total_count = a.count + b.count;
    if (total_count == 0) {
        return create_bytecode_array(NULL, 0);
    }

    bytecode* new_bytecodes = malloc((a.count + b.count) * sizeof(bytecode));
    if (!new_bytecodes) return create_bytecode_array(NULL, 0);
    
    memcpy(new_bytecodes, a.bytecodes, a.count * sizeof(bytecode));
    memcpy(new_bytecodes + a.count, b.bytecodes, b.count * sizeof(bytecode));
    
    return create_bytecode_array(new_bytecodes, a.count + b.count);
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

static size_t compiler_add_constant(compilation_result* result, Value value) {
    if (!result) return SIZE_MAX;
    
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

static size_t compiler_add_constant_to_compiler(compiler* comp, Value value) {
    return compiler_add_constant(comp->result, value);
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
    FunctionDeclarationStatement* func_decl = (FunctionDeclarationStatement*)node;
    if (node->node_type != NODE_FUNCTION_DECLARATION_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }

    // 1. new comp res
    compilation_result* body_result = malloc(sizeof(compilation_result));
    body_result->code_array = create_bytecode_array(NULL, 0);
    body_result->constants = NULL;
    body_result->constants_count = 0;
    body_result->constants_capacity = 0;
    
    // save current context
    CompilerScope* previous_scope = comp->current_scope;
    compilation_result* previous_result = comp->result;
    
    // change context
    comp->current_scope = scope_create(previous_scope);
    comp->result = body_result;  
    
    // 2. adding params into local scope
    for (size_t i = 0; i < func_decl->parameter_count; i++) {
        Parameter* param = &func_decl->parameters[i];
        scope_add_local(comp->current_scope, param->name);
    }
    
    // 3. compiling body
    if (func_decl->body != NULL) {
        bytecode_array body_bc = compiler_compile_block_statement(comp, func_decl->body);
        emit_bytecode(body_result, body_bc);
        free_bytecode_array(body_bc);
    }    

    // 4. add return if there is no
    if (func_decl->return_type == TYPE_NONE && body_result->code_array.count > 0) {
        bytecode* last_bc = &body_result->code_array.bytecodes[body_result->code_array.count - 1];
        if (last_bc->op_code != RETURN_VALUE) {
            Value none_value = value_create_none();
            // use body_result but not comp
            uint32_t none_index = compiler_add_constant(body_result, none_value);
            bytecode load_none = bytecode_create_with_number(LOAD_CONST, none_index);
            bytecode return_bc = bytecode_create(RETURN_VALUE, 0, 0, 0);
            
            bytecode* return_arr = malloc(2 * sizeof(bytecode));
            return_arr[0] = load_none;
            return_arr[1] = return_bc;
            bytecode_array return_array = create_bytecode_array(return_arr, 2);
            emit_bytecode(body_result, return_array);
            free_bytecode_array(return_array);
        }
    }

    // 5. create CodeObj
    CodeObj* code_obj = malloc(sizeof(CodeObj));
    code_obj->code = body_result->code_array;
    code_obj->name = strdup(func_decl->name);
    code_obj->arg_count = func_decl->parameter_count;
    code_obj->local_count = comp->current_scope->locals->count;
    code_obj->constants = body_result->constants;
    code_obj->constants_count = body_result->constants_count;
    
    // 6. create Value with CodeObj, add into constants
    Value code_value = value_create_code(code_obj);
    
    // resolve state
    comp->result = previous_result;
    uint32_t code_index = compiler_add_constant(comp->result, code_value);
    comp->current_scope = previous_scope;
    
    // 8. generate bytecode for func creation
    bytecode_array result = create_bytecode_array(NULL, 0);
    
    // LOAD_CONST for code object
    bytecode load_code = bytecode_create_with_number(LOAD_CONST, code_index);
    bytecode* load_code_arr = malloc(sizeof(bytecode));
    load_code_arr[0] = load_code;
    bytecode_array load_code_array = create_bytecode_array(load_code_arr, 1);
    result = concat_bytecode_arrays(result, load_code_array);
    free_bytecode_array(load_code_array);
    
    // MAKE_FUNCTION
    bytecode make_func = bytecode_create(MAKE_FUNCTION, 0, 0, 0);
    bytecode* make_func_arr = malloc(sizeof(bytecode));
    make_func_arr[0] = make_func;
    bytecode_array make_func_array = create_bytecode_array(make_func_arr, 1);
    result = concat_bytecode_arrays(result, make_func_array);
    free_bytecode_array(make_func_array);
    
    // STORE into
    size_t func_index;
    if (comp->current_scope != NULL && comp->current_scope->parent != NULL) {
        func_index = scope_add_local(comp->current_scope, func_decl->name);
        bytecode store_func = bytecode_create_with_number(STORE_FAST, func_index);
        bytecode* store_func_arr = malloc(sizeof(bytecode));
        store_func_arr[0] = store_func;
        bytecode_array store_func_array = create_bytecode_array(store_func_arr, 1);
        result = concat_bytecode_arrays(result, store_func_array);
        free_bytecode_array(store_func_array);
    } else {
        func_index = compiler_add_global_name(comp, func_decl->name);
        bytecode store_func = bytecode_create_with_number(STORE_GLOBAL, func_index);
        bytecode* store_func_arr = malloc(sizeof(bytecode));
        store_func_arr[0] = store_func;
        bytecode_array store_func_array = create_bytecode_array(store_func_arr, 1);
        result = concat_bytecode_arrays(result, store_func_array);
        free_bytecode_array(store_func_array);
    }
    
    free(body_result);
    
    return result;
}

static bytecode_array create_single_bytecode_array(bytecode bc) {
    bytecode* bc_array = malloc(sizeof(bytecode));
    bc_array[0] = bc;
    return create_bytecode_array(bc_array, 1);
}

static bytecode_array compiler_compile_variable_declaration(compiler* comp, ASTNode* statement) {
    VariableDeclarationStatement* decl = (VariableDeclarationStatement*)statement;
    DPRINT("[COMPILER] Compiling variable declaration: %s\n", decl->name);
    
    bytecode_array result = create_bytecode_array(NULL, 0);
    
    // 1. Компилируем инициализатор (если есть)
    if (decl->initializer) {
        DPRINT("[COMPILER] Has initializer, type: %s\n", 
               ast_node_type_to_string(decl->initializer->node_type));
        
        bytecode_array init_bc = compiler_compile_expression(comp, decl->initializer);
        DPRINT("[COMPILER] Initializer generated %u bytecodes\n", init_bc.count);
        
        if (init_bc.count == 0) {
            DPRINT("[COMPILER] WARNING: Initializer generated 0 bytecodes!\n");
        }
        
        result = concat_bytecode_arrays(result, init_bc);
        free_bytecode_array(init_bc);
    } else {
        // Если нет инициализатора, пушим None на стек
        Value none_value = value_create_none();
        uint32_t const_index = compiler_add_constant_to_compiler(comp, none_value);
        bytecode load_none = bytecode_create_with_number(LOAD_CONST, const_index);
        bytecode_array none_bc = create_single_bytecode_array(load_none);
        result = concat_bytecode_arrays(result, none_bc);
        free_bytecode_array(none_bc);
    }
    
    // 2. Находим индекс переменной и сохраняем
    if (comp->current_scope == NULL) {
        // Глобальная переменная
        int32_t global_idx = string_table_find(comp->global_names, decl->name);
        if (global_idx < 0) {
            global_idx = string_table_add(comp->global_names, decl->name);
        }
        DPRINT("[COMPILER] Variable %s is global at index %d\n", decl->name, global_idx);
        
        // STORE_GLOBAL
        bytecode store_global = bytecode_create_with_number(STORE_GLOBAL, global_idx);
        bytecode_array store_bc = create_single_bytecode_array(store_global);
        result = concat_bytecode_arrays(result, store_bc);
        free_bytecode_array(store_bc);
    } else {
        // Локальная переменная
        int32_t local_idx = scope_find_local(comp->current_scope, decl->name);
        if (local_idx < 0) {
            local_idx = scope_add_local(comp->current_scope, decl->name);
        }
        DPRINT("[COMPILER] Variable %s is local at index %d\n", decl->name, local_idx);
        
        // STORE_FAST
        bytecode store_fast = bytecode_create_with_number(STORE_FAST, local_idx);
        bytecode_array store_bc = create_single_bytecode_array(store_fast);
        result = concat_bytecode_arrays(result, store_bc);
        free_bytecode_array(store_bc);
    }
    
    DPRINT("[COMPILER] Variable declaration generated %u bytecodes\n", result.count);
    return result;
}

static bytecode_array compiler_compile_return_statement(compiler* comp, ASTNode* node) {
    if (node->node_type != NODE_RETURN_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }
    ReturnStatement* return_stmt = (ReturnStatement*) node;
    bytecode_array expr_bc = compiler_compile_expression(comp, return_stmt->expression);

    bytecode return_bc = bytecode_create(RETURN_VALUE, 0, 0, 0);
    bytecode* return_arr = malloc(sizeof(bytecode));
    return_arr[0] = return_bc;
    bytecode_array return_array = create_bytecode_array(return_arr, 1);
    
    // Объединяем: сначала вычисляем выражение, затем возвращаем
    bytecode_array result = concat_bytecode_arrays(expr_bc, return_array);
    
    // Очищаем временные массивы
    free_bytecode_array(expr_bc);
    free_bytecode_array(return_array);
    
    return result;
}

static bytecode_array compiler_compile_if_statement(compiler* comp, ASTNode* node) {
    if (node->node_type != NODE_IF_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }
}

static bytecode_array compiler_compile_while_statement(compiler* comp, ASTNode* node) {
    if (node->node_type != NODE_WHILE_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }
}

static bytecode_array compiler_compile_for_statement(compiler* comp, ASTNode* node) {
    if (node->node_type != NODE_FOR_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }
}

static bytecode_array compiler_compile_assignment_statement(compiler* comp, ASTNode* node) {
    DPRINT("DEBUG: compile_assignment_statement started\n");
    if (node->node_type != NODE_ASSIGNMENT_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }

    AssignmentStatement* stmt = (AssignmentStatement*) node;
    if (!stmt->left || stmt->left->node_type != NODE_VARIABLE_EXPRESSION) {
        return create_bytecode_array(NULL, 0);
    }
    
    // 1. right side   
    bytecode_array bc_right = compiler_compile_expression(comp, stmt->right);
    
    // 2. left side var name
    VariableExpression* var_expr = (VariableExpression*)stmt->left;
    const char* var_name = var_expr->name;
    
    bytecode_array result = bc_right;
    
    // 3. Проверяем, является ли переменная локальной
    int32_t local_index = -1;
    if (comp->current_scope) {
        local_index = scope_find_local(comp->current_scope, var_name);
    }
    
    // Если найдена локальная переменная => store_fast
    if (local_index >= 0) {
        bytecode store_bc = bytecode_create_with_number(STORE_FAST, local_index);
        bytecode* store_arr = malloc(sizeof(bytecode));
        store_arr[0] = store_bc;
        bytecode_array store_array = create_bytecode_array(store_arr, 1);
        result = concat_bytecode_arrays(result, store_array);
        free_bytecode_array(store_array);
        DPRINT("[COMPILER] Assignment to local variable %s at index %d\n", var_name, local_index);
    } else {
        // Иначе => store_global
        // Ищем в таблице глобальных имен
        int32_t global_index = -1;
        if (comp->global_names) {
            global_index = string_table_find(comp->global_names, var_name);
        }
        
        // Если не найдена, добавляем
        if (global_index < 0) {
            global_index = compiler_add_global_name(comp, var_name);
        }
        
        DPRINT("[COMPILER] Assignment to global variable %s at index %d\n", var_name, global_index);
        
        // ВАЖНО: НЕ делаем сдвиг влево! Используем индекс как есть
        // В VM в case STORE_GLOBAL: используется size_t gidx = arg;
        bytecode store_bc = bytecode_create_with_number(STORE_GLOBAL, global_index);
        bytecode* store_arr = malloc(sizeof(bytecode));
        store_arr[0] = store_bc;
        bytecode_array store_array = create_bytecode_array(store_arr, 1);
        
        result = concat_bytecode_arrays(result, store_array);
        free_bytecode_array(store_array);
    }
    
    return result;
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
        case OP_EQ:
            op_code_value = 0x50;  // COMPARE_OP_EQ
            break;
        case OP_NE:
            op_code_value = 0x51;  // COMPARE_OP_NE
            break;
        case OP_LT:
            op_code_value = 0x52;  // COMPARE_OP_LT
            break;
        case OP_LE:
            op_code_value = 0x53;  // COMPARE_OP_LE
            break;
        case OP_GT:
            op_code_value = 0x54;  // COMPARE_OP_GT
            break;
        case OP_GE:
            op_code_value = 0x55;  // COMPARE_OP_GE
            break;
        case KW_IS:
            op_code_value = 0x56;  // KW_IS
            break;

        case OP_AND:
            op_code_value = 0x60;
            break;

        case OP_OR:
            op_code_value = 0x61;
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
            op_code_value = 0x01;  // -x
            break;
        case OP_NOT:
            op_code_value = 0x03;  // not x
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
        bytecode bc = bytecode_create_with_number(LOAD_GLOBAL, global_index << 1);
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

    bytecode_array result = create_bytecode_array(NULL, 0);
    
    // 1. Callee (функция которую вызываем)
    bytecode_array callee_bc = compiler_compile_expression(comp, func_call->callee);
    result = concat_bytecode_arrays(result, callee_bc);
    free_bytecode_array(callee_bc);
    
    // 2. Null (для вызова функции)
    bytecode push_null_bc = bytecode_create(PUSH_NULL, 0, 0, 0);
    bytecode_array push_null_bc_array = create_single_bytecode_array(push_null_bc);
    result = concat_bytecode_arrays(result, push_null_bc_array);
    free_bytecode_array(push_null_bc_array);
    
    // 3. Аргументы (в прямом порядке)
    for (int i = 0; i < func_call->argument_count; i++) {
        bytecode_array arg_bc = compiler_compile_expression(comp, func_call->arguments[i]);
        result = concat_bytecode_arrays(result, arg_bc);
        free_bytecode_array(arg_bc);
    }
    
    // 4. Вызов функции
    bytecode call_func_bc = bytecode_create_with_number(CALL_FUNCTION, func_call->argument_count);
    bytecode_array call_func_bc_array = create_single_bytecode_array(call_func_bc);
    result = concat_bytecode_arrays(result, call_func_bc_array);
    free_bytecode_array(call_func_bc_array);
    
    return result;
}

static bytecode_array compiler_compile_literal_expression(compiler* comp, ASTNode* node) {
    LiteralExpression* literal = (LiteralExpression*)node;

    if (literal->type == TYPE_INT) {
        int64_t value = literal->value;
        Value constant_value = value_create_int(value);
        uint32_t const_index = compiler_add_constant_to_compiler(comp, constant_value);
        bytecode bc = bytecode_create_with_number(LOAD_CONST, const_index);
        bytecode* bc_array = malloc(sizeof(bytecode));
        bc_array[0] = bc;
        return create_bytecode_array(bc_array, 1);
    }
    else if (literal->type == TYPE_LONG) {
        fprintf(stderr, "Error: long literals are not supported in this version\n");
        Value constant_value = value_create_int(0);
        uint32_t const_index = compiler_add_constant_to_compiler(comp, constant_value);
        bytecode bc = bytecode_create_with_number(LOAD_CONST, const_index);
        bytecode* bc_array = malloc(sizeof(bytecode));
        bc_array[0] = bc;
        return create_bytecode_array(bc_array, 1);
    }    
    else if (literal->type == TYPE_BOOL) {
        bool value = (bool)literal->value;
        Value constant_value = value_create_bool(value);
        uint32_t const_index = compiler_add_constant_to_compiler(comp, constant_value);
        bytecode bc = bytecode_create_with_number(LOAD_CONST, const_index);
        bytecode* bc_array = malloc(sizeof(bytecode));
        bc_array[0] = bc;
        return create_bytecode_array(bc_array, 1);
    }
    else if (literal->type == TYPE_NONE) {
        DPRINT("[COMPILER] Compiling NONE literal\n");
        Value constant_value = value_create_none();
        uint32_t const_index = compiler_add_constant_to_compiler(comp, constant_value);
        bytecode bc = bytecode_create_with_number(LOAD_CONST, const_index);
        bytecode* bc_array = malloc(sizeof(bytecode));
        bc_array[0] = bc;
        return create_bytecode_array(bc_array, 1);
    }
    else {
        fprintf(stderr, "Error: type %d is not supported in this version\n", literal->type);
        Value constant_value = value_create_none();
        uint32_t const_index = compiler_add_constant_to_compiler(comp, constant_value);
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
            fprintf(stderr, "Unknown expression node type: %s\n", ast_node_type_to_string(node->node_type));
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
    if (node->node_type != NODE_BLOCK_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }

    BlockStatement* block = (BlockStatement*)node;
    bytecode_array result = create_bytecode_array(NULL, 0);

    for (uint32_t i = 0; i < block->statement_count; i++) {
        DPRINT("[COMPILER] Compiling statement %u in block\n", i);
        bytecode_array stmt_bc = compiler_compile_statement(comp, block->statements[i]);
        
        // Отладочный вывод байт-кода
        DPRINT("[COMPILER] Statement %u generated %u bytecodes\n", i, stmt_bc.count);
        if (stmt_bc.count > 0) {
            // Используй bytecode_get_arg для получения аргумента
            uint32_t arg = bytecode_get_arg(stmt_bc.bytecodes[0]);
            DPRINT("[COMPILER] First bytecode: op=%02X, arg=%u (bytes: %02X %02X %02X)\n", 
                   stmt_bc.bytecodes[0].op_code, 
                   arg,
                   stmt_bc.bytecodes[0].argument[0],
                   stmt_bc.bytecodes[0].argument[1],
                   stmt_bc.bytecodes[0].argument[2]);
        }
        
        bytecode_array new_result = concat_bytecode_arrays(result, stmt_bc);
        
        free_bytecode_array(result);
        free_bytecode_array(stmt_bc);
        
        result = new_result;
    }

    return result;
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
            fprintf(stderr, "Unknown statement node type: %s\n", ast_node_type_to_string(statement->node_type));
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

    comp->global_names = string_table_create();
    size_t print_idx = string_table_add(comp->global_names, "print");
    size_t input_idx = string_table_add(comp->global_names, "input");
    if (print_idx != 0) {
        fprintf(stderr, "Warning: print index is %zu, expected 0\n", print_idx);
    }
    if (input_idx != 1) {
        fprintf(stderr, "Warning: input index is %zu, expected 1\n", input_idx);
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

