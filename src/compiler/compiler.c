#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../system.h"
#include "../lexer/token.h"
#include "compiler.h"
#include "scope.h"
#include "string_table.h"

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
    // If current scope is NULL or the current scope has no parent, we're at top-level
    // and the variable should be treated as a global.
    if (comp->current_scope == NULL || comp->current_scope->parent == NULL) {
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
    DPRINT("[COMPILER] Compiling if statement\n");
    
    if (node->node_type != NODE_IF_STATEMENT) {
        DPRINT("[COMPILER] ERROR: Not an if statement\n");
        return create_bytecode_array(NULL, 0);
    }
    
    IfStatement* if_stmt = (IfStatement*)node;
    
    // Массивы для хранения скомпилированных частей
    bytecode_array result = create_bytecode_array(NULL, 0);
    
    // 1. Компилируем if условие
    DPRINT("[COMPILER] Compiling if condition\n");
    bytecode_array cond_bc = compiler_compile_expression(comp, if_stmt->condition);
    if (cond_bc.count == 0) {
        DPRINT("[COMPILER] ERROR: Failed to compile condition\n");
        return create_bytecode_array(NULL, 0);
    }
    
    // 2. Компилируем then ветку
    DPRINT("[COMPILER] Compiling then branch\n");
    bytecode_array then_bc = compiler_compile_block_statement(comp, if_stmt->then_branch);
    
    // 3. Компилируем elif ветки
    bytecode_array* elif_cond_bc = NULL;
    bytecode_array* elif_branch_bc = NULL;
    if (if_stmt->elif_count > 0) {
        elif_cond_bc = malloc(if_stmt->elif_count * sizeof(bytecode_array));
        elif_branch_bc = malloc(if_stmt->elif_count * sizeof(bytecode_array));
        
        for (size_t i = 0; i < if_stmt->elif_count; i++) {
            elif_cond_bc[i] = compiler_compile_expression(comp, if_stmt->elif_conditions[i]);
            elif_branch_bc[i] = compiler_compile_block_statement(comp, if_stmt->elif_branches[i]);
        }
    }
    
    // 4. Компилируем else ветку (если есть)
    bytecode_array else_bc;
    bool has_else = (if_stmt->else_branch != NULL);
    if (has_else) {
        DPRINT("[COMPILER] Compiling else branch\n");
        else_bc = compiler_compile_block_statement(comp, if_stmt->else_branch);
    }
    
    // 5. Сначала собираем весь код, чтобы вычислить размеры
    bytecode_array temp_code = create_bytecode_array(NULL, 0);
    
    // if условие
    temp_code = concat_bytecode_arrays(temp_code, cond_bc);
    free_bytecode_array(cond_bc);
    
    // POP_JUMP_IF_FALSE с временным смещением
    bytecode pop_jump = bytecode_create_with_number(POP_JUMP_IF_FALSE, 0);
    bytecode_array pop_jump_arr = create_single_bytecode_array(pop_jump);
    temp_code = concat_bytecode_arrays(temp_code, pop_jump_arr);
    size_t pop_jump_pos = temp_code.count - 1;
    free_bytecode_array(pop_jump_arr);
    
    // then ветка
    temp_code = concat_bytecode_arrays(temp_code, then_bc);
    free_bytecode_array(then_bc);
    
    // JUMP_FORWARD чтобы перепрыгнуть остальное
    bytecode jump_forward = bytecode_create_with_number(JUMP_FORWARD, 0);
    bytecode_array jump_arr = create_single_bytecode_array(jump_forward);
    temp_code = concat_bytecode_arrays(temp_code, jump_arr);
    size_t jump_forward_pos = temp_code.count - 1;
    free_bytecode_array(jump_arr);
    
    // elif ветки
    for (size_t i = 0; i < if_stmt->elif_count; i++) {
        // elif условие
        temp_code = concat_bytecode_arrays(temp_code, elif_cond_bc[i]);
        free_bytecode_array(elif_cond_bc[i]);
        
        // POP_JUMP_IF_FALSE для elif
        bytecode elif_pop_jump = bytecode_create_with_number(POP_JUMP_IF_FALSE, 0);
        bytecode_array elif_pop_arr = create_single_bytecode_array(elif_pop_jump);
        temp_code = concat_bytecode_arrays(temp_code, elif_pop_arr);
        free_bytecode_array(elif_pop_arr);
        
        // elif ветка
        temp_code = concat_bytecode_arrays(temp_code, elif_branch_bc[i]);
        free_bytecode_array(elif_branch_bc[i]);
        
        // JUMP_FORWARD если не последний elif или есть else
        if (i < if_stmt->elif_count - 1 || has_else) {
            bytecode elif_jump = bytecode_create_with_number(JUMP_FORWARD, 0);
            bytecode_array elif_jump_arr = create_single_bytecode_array(elif_jump);
            temp_code = concat_bytecode_arrays(temp_code, elif_jump_arr);
            free_bytecode_array(elif_jump_arr);
        }
    }
    
    // else ветка
    if (has_else) {
        temp_code = concat_bytecode_arrays(temp_code, else_bc);
        free_bytecode_array(else_bc);
    }
    
    // 6. Теперь вычисляем правильные смещения
    // Вычисляем размеры частей
    size_t then_size = then_bc.count;
    size_t else_size = has_else ? else_bc.count : 0;
    
    // Смещение для POP_JUMP_IF_FALSE
    // Если условие ложно, перепрыгиваем then ветку
    uint32_t pop_jump_offset = (uint32_t)(then_size + 1); // +1 для JUMP_FORWARD после then
    
    // Смещение для JUMP_FORWARD после then
    uint32_t jump_forward_offset = 0;
    size_t bytes_after_then = 0;
    
    // Считаем байты после then (elif + else)
    for (size_t i = 0; i < if_stmt->elif_count; i++) {
        bytes_after_then += elif_cond_bc[i].count + 1 + elif_branch_bc[i].count;
        if (i < if_stmt->elif_count - 1 || has_else) {
            bytes_after_then += 1; // JUMP_FORWARD
        }
    }
    
    if (has_else) {
        bytes_after_then += else_bc.count;
    }
    
    jump_forward_offset = (uint32_t)bytes_after_then;
    
    // Обновляем инструкции с правильными смещениями
    temp_code.bytecodes[pop_jump_pos] = bytecode_create_with_number(POP_JUMP_IF_FALSE, pop_jump_offset);
    temp_code.bytecodes[jump_forward_pos] = bytecode_create_with_number(JUMP_FORWARD, jump_forward_offset);
    
    // Освобождаем временные массивы
    if (if_stmt->elif_count > 0) {
        free(elif_cond_bc);
        free(elif_branch_bc);
    }
    
    return temp_code;
}

static bytecode_array compiler_compile_while_statement(compiler* comp, ASTNode* node) {
    DPRINT("[COMPILER] Compiling while statement\n");
    
    if (node->node_type != NODE_WHILE_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }
    
    WhileStatement* while_stmt = (WhileStatement*)node;
    
    bytecode_array result = create_bytecode_array(NULL, 0);
    
    // 1. LOOP_START - метка начала цикла
    bytecode loop_start_bc = bytecode_create(LOOP_START, 0, 0, 0);
    bytecode_array loop_start_arr = create_single_bytecode_array(loop_start_bc);
    result = concat_bytecode_arrays(result, loop_start_arr);
    free_bytecode_array(loop_start_arr);
    
    // Запоминаем позицию начала условия
    size_t cond_start_index = result.count;
    
    // 2. Компилируем условие
    DPRINT("[COMPILER] Compiling while condition\n");
    bytecode_array cond_bc = compiler_compile_expression(comp, while_stmt->condition);
    result = concat_bytecode_arrays(result, cond_bc);
    free_bytecode_array(cond_bc);
    
    // 3. POP_JUMP_IF_FALSE - выход из цикла (временно offset=0)
    bytecode pop_jump = bytecode_create_with_number(POP_JUMP_IF_FALSE, 0);
    bytecode_array pop_jump_arr = create_single_bytecode_array(pop_jump);
    result = concat_bytecode_arrays(result, pop_jump_arr);
    size_t pop_jump_pos = result.count - 1;
    free_bytecode_array(pop_jump_arr);
    
    // 4. Компилируем тело цикла
    DPRINT("[COMPILER] Compiling while body\n");
    bytecode_array body_bc = compiler_compile_block_statement(comp, while_stmt->body);
    
    // 5. JUMP_BACKWARD к началу условия
    // Нужно перепрыгнуть от текущей позиции назад к LOOP_START
    // Вычисляем: от конца тела до LOOP_START
    // body_bc.count - это размер тела, но нам нужно учесть условие и POP_JUMP_IF_FALSE
    // Правильная формула: (размер условия) + 1 (POP_JUMP_IF_FALSE) + (размер тела) + 1 (сам JUMP_BACKWARD)
    size_t jump_back_size = cond_bc.count + 1 + body_bc.count + 1;
    bytecode jump_back = bytecode_create_with_number(JUMP_BACKWARD, jump_back_size);
    bytecode_array jump_back_arr = create_single_bytecode_array(jump_back);
    
    // 6. LOOP_END - метка конца цикла
    bytecode loop_end_bc = bytecode_create(LOOP_END, 0, 0, 0);
    bytecode_array loop_end_arr = create_single_bytecode_array(loop_end_bc);
    
    // 7. Собираем всё: тело + JUMP_BACKWARD + LOOP_END
    result = concat_bytecode_arrays(result, body_bc);
    free_bytecode_array(body_bc);
    result = concat_bytecode_arrays(result, jump_back_arr);
    free_bytecode_array(jump_back_arr);
    result = concat_bytecode_arrays(result, loop_end_arr);
    free_bytecode_array(loop_end_arr);
    
    // 8. Теперь обновляем POP_JUMP_IF_FALSE с правильным смещением
    // Если условие ложно, перепрыгиваем: тело + JUMP_BACKWARD + LOOP_END
    uint32_t pop_offset = (uint32_t)(body_bc.count + 2); // +2 для JUMP_BACKWARD и LOOP_END
    result.bytecodes[pop_jump_pos] = bytecode_create_with_number(POP_JUMP_IF_FALSE, pop_offset);
    
    DPRINT("[COMPILER] While loop compiled:\n");
    DPRINT("  - cond_start_index: %zu\n", cond_start_index);
    DPRINT("  - cond_bc.count: %u\n", cond_bc.count);
    DPRINT("  - body_bc.count: %u\n", body_bc.count);
    DPRINT("  - pop_offset: %u (should jump to LOOP_END)\n", pop_offset);
    DPRINT("  - jump_back_size: %zu (should jump to LOOP_START)\n", jump_back_size);
    
    return result;
}

static bytecode_array compiler_compile_for_statement(compiler* comp, ASTNode* node) {
    DPRINT("[COMPILER] Compiling for statement\n");
    
    if (node->node_type != NODE_FOR_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }
    
    ForStatement* for_stmt = (ForStatement*)node;
    
    bytecode_array result = create_bytecode_array(NULL, 0);
    
    // 1. Инициализатор
    if (for_stmt->initializer != NULL) {
        bytecode_array init_bc = compiler_compile_statement(comp, for_stmt->initializer);
        result = concat_bytecode_arrays(result, init_bc);
        free_bytecode_array(init_bc);
    }
    
    // 2. JUMP_FORWARD к УСЛОВИЮ (пропускаем инкремент в первой итерации)
    bytecode jump_to_condition = bytecode_create_with_number(JUMP_FORWARD, 0);
    bytecode_array jump_to_condition_arr = create_single_bytecode_array(jump_to_condition);
    result = concat_bytecode_arrays(result, jump_to_condition_arr);
    size_t jump_to_condition_pos = result.count - 1;
    free_bytecode_array(jump_to_condition_arr);
    
    // 3. LOOP_START должен быть перед условием
    bytecode loop_start = bytecode_create(LOOP_START, 0, 0, 0);
    bytecode_array loop_start_arr = create_single_bytecode_array(loop_start);
    result = concat_bytecode_arrays(result, loop_start_arr);
    free_bytecode_array(loop_start_arr);
    
    // 4. Инкремент
    if (for_stmt->increment != NULL) {
        bytecode_array inc_bc = compiler_compile_statement(comp, for_stmt->increment);
        result = concat_bytecode_arrays(result, inc_bc);
        free_bytecode_array(inc_bc);
    }
    
    // 5. Условие
    if (for_stmt->condition != NULL) {
        // 5.1 Обновляем JUMP_FORWARD чтобы он прыгал к условию
        uint32_t jump_to_condition_offset = (uint32_t)(result.count - jump_to_condition_pos - 1);
        result.bytecodes[jump_to_condition_pos] = bytecode_create_with_number(JUMP_FORWARD, jump_to_condition_offset);
        
        // 5.2 Компилируем условие
        ASTNode* condition_expr;
        if (for_stmt->condition->node_type == NODE_EXPRESSION_STATEMENT) {
            ExpressionStatement* expr_stmt = (ExpressionStatement*)for_stmt->condition;
            condition_expr = expr_stmt->expression;
        } else {
            condition_expr = for_stmt->condition;
        }
        
        bytecode_array cond_bc = compiler_compile_expression(comp, condition_expr);
        result = concat_bytecode_arrays(result, cond_bc);
        
        // 5.3 POP_JUMP_IF_FALSE для выхода из цикла
        bytecode pop_jump = bytecode_create_with_number(POP_JUMP_IF_FALSE, 0);
        bytecode_array pop_jump_arr = create_single_bytecode_array(pop_jump);
        result = concat_bytecode_arrays(result, pop_jump_arr);
        size_t pop_jump_pos = result.count - 1;
        free_bytecode_array(pop_jump_arr);
        
        // 5.4 Тело цикла
        bytecode_array body_bc = compiler_compile_block_statement(comp, for_stmt->body);
        
        // 5.5 JUMP_BACKWARD к LOOP_START
        // Находим позицию LOOP_START (она была добавлена перед инкрементом)
        size_t loop_start_pos = jump_to_condition_pos + 1; // +1 потому что LOOP_START идет сразу после JUMP_FORWARD
        size_t current_pos = result.count + body_bc.count; // Позиция JUMP_BACKWARD
        size_t jump_back_size = current_pos - loop_start_pos + 1; // +1 для самого JUMP_BACKWARD
        bytecode jump_back = bytecode_create_with_number(JUMP_BACKWARD, jump_back_size);
        bytecode_array jump_back_arr = create_single_bytecode_array(jump_back);
        
        // 5.6 LOOP_END
        bytecode loop_end = bytecode_create(LOOP_END, 0, 0, 0);
        bytecode_array loop_end_arr = create_single_bytecode_array(loop_end);
        
        // 5.7 Обновляем POP_JUMP_IF_FALSE
        uint32_t pop_offset = (uint32_t)(body_bc.count + 2); // +2 для JUMP_BACKWARD и LOOP_END
        result.bytecodes[pop_jump_pos] = bytecode_create_with_number(POP_JUMP_IF_FALSE, pop_offset);
        
        DPRINT("[COMPILER] For loop structure:\n");
        DPRINT("  - jump_to_condition_pos: %zu\n", jump_to_condition_pos);
        DPRINT("  - loop_start_pos: %zu\n", loop_start_pos);
        DPRINT("  - jump_to_condition_offset: %u\n", jump_to_condition_offset);
        DPRINT("  - pop_offset: %u\n", pop_offset);
        DPRINT("  - jump_back_size: %zu\n", jump_back_size);
        
        // 5.8 Собираем все: тело + JUMP_BACKWARD + LOOP_END
        result = concat_bytecode_arrays(result, body_bc);
        free_bytecode_array(body_bc);
        
        result = concat_bytecode_arrays(result, jump_back_arr);
        free_bytecode_array(jump_back_arr);
        
        result = concat_bytecode_arrays(result, loop_end_arr);
        free_bytecode_array(loop_end_arr);
        
        // Освобождаем cond_bc
        free_bytecode_array(cond_bc);
    } else {
        // Бесконечный цикл без условия
        DPRINT("[COMPILER] Compiling infinite for loop (no condition)\n");
        
        // 5.1 Обновляем JUMP_FORWARD чтобы он прыгал к LOOP_START
        uint32_t jump_to_condition_offset = 1; // Всего одна инструкция - LOOP_START
        result.bytecodes[jump_to_condition_pos] = bytecode_create_with_number(JUMP_FORWARD, jump_to_condition_offset);
        
        bytecode_array body_bc = compiler_compile_block_statement(comp, for_stmt->body);
        
        // JUMP_BACKWARD к LOOP_START
        size_t loop_start_pos = jump_to_condition_pos + 1;
        size_t current_pos = result.count + body_bc.count;
        size_t jump_back_size = current_pos - loop_start_pos + 1;
        bytecode jump_back = bytecode_create_with_number(JUMP_BACKWARD, jump_back_size);
        bytecode_array jump_back_arr = create_single_bytecode_array(jump_back);
        
        bytecode loop_end = bytecode_create(LOOP_END, 0, 0, 0);
        bytecode_array loop_end_arr = create_single_bytecode_array(loop_end);
        
        result = concat_bytecode_arrays(result, body_bc);
        free_bytecode_array(body_bc);
        
        result = concat_bytecode_arrays(result, jump_back_arr);
        free_bytecode_array(jump_back_arr);
        
        result = concat_bytecode_arrays(result, loop_end_arr);
        free_bytecode_array(loop_end_arr);
    }
    
    DPRINT("[COMPILER] For statement compiled, generated %u bytecodes\n", result.count);
    return result;
}

static bytecode_array compiler_compile_assignment_statement(compiler* comp, ASTNode* node) {
    if (node->node_type != NODE_ASSIGNMENT_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }
    
    AssignmentStatement* assign = (AssignmentStatement*)node;
    DPRINT("[COMPILER] Compiling assignment statement\n");
    
    bytecode_array result = create_bytecode_array(NULL, 0);
    
    // 1. Сначала компилируем правую часть (значение)
    DPRINT("[COMPILER] Compiling RHS of assignment\n");
    bytecode_array rhs_bc = compiler_compile_expression(comp, assign->right);
    result = concat_bytecode_arrays(result, rhs_bc);
    free_bytecode_array(rhs_bc);
    
    // 2. Проверяем, является ли левая часть индексацией массива
    if (assign->left->node_type == NODE_SUBSCRIPT_EXPRESSION) {
        DPRINT("[COMPILER] Assignment to array element\n");
        SubscriptExpression* subscript = (SubscriptExpression*)assign->left;
        
        // Компилируем массив
        DPRINT("[COMPILER] Compiling array expression\n");
        bytecode_array array_bc = compiler_compile_expression(comp, subscript->array);
        result = concat_bytecode_arrays(result, array_bc);
        free_bytecode_array(array_bc);
        
        // Компилируем индекс
        DPRINT("[COMPILER] Compiling index expression\n");
        bytecode_array index_bc = compiler_compile_expression(comp, subscript->index);
        result = concat_bytecode_arrays(result, index_bc);
        free_bytecode_array(index_bc);
        
        // Теперь в стеке: [значение, массив, индекс]
        // Нужно переставить для STORE_SUBSCR: [массив, индекс, значение]
        
        // Делаем ROT_THREE или серию SWAP
        // Или лучше: просто вызываем STORE_SUBSCR который ожидает правильный порядок
        
        // Добавляем STORE_SUBSCR
        DPRINT("[COMPILER] Adding STORE_SUBSCR instruction\n");
        bytecode store_subscr_bc = bytecode_create(STORE_SUBSCR, 0, 0, 0);
        bytecode_array store_subscr_array = create_single_bytecode_array(store_subscr_bc);
        result = concat_bytecode_arrays(result, store_subscr_array);
        free_bytecode_array(store_subscr_array);
        
    } else if (assign->left->node_type == NODE_VARIABLE_EXPRESSION) {
        DPRINT("[COMPILER] Assignment to variable\n");
        // Это обычное присваивание переменной
        VariableExpression* var_expr = (VariableExpression*)assign->left;
        
        // Проверяем, существует ли переменная
        int32_t var_index = scope_find_local(comp->current_scope, var_expr->name);
        if (var_index >= 0) {
            // Локальная переменная
            DPRINT("[COMPILER] Storing in local variable %s at index %d\n", var_expr->name, var_index);
            bytecode store_bc = bytecode_create_with_number(STORE_FAST, (uint32_t)var_index);
            bytecode_array store_array = create_single_bytecode_array(store_bc);
            result = concat_bytecode_arrays(result, store_array);
            free_bytecode_array(store_array);
        } else {
            // Глобальная переменная
            DPRINT("[COMPILER] Storing in global variable %s\n", var_expr->name);
            int32_t global_idx = string_table_find(comp->global_names, var_expr->name);
            if (global_idx < 0) {
                global_idx = string_table_add(comp->global_names, var_expr->name);
            }
            bytecode store_bc = bytecode_create_with_number(STORE_GLOBAL, (uint32_t)global_idx);
            bytecode_array store_array = create_single_bytecode_array(store_bc);
            result = concat_bytecode_arrays(result, store_array);
            free_bytecode_array(store_array);
        }
    } else {
        DPRINT("[COMPILER] ERROR: Invalid LHS for assignment (node_type=%d)\n", assign->left->node_type);
    }
    
    DPRINT("[COMPILER] Assignment compiled to %u bytecodes\n", result.count);
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

        case OP_AND: // and
            op_code_value = 0x60;
            break;

        case OP_OR: // or
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

static bytecode_array compiler_compile_literal_expression_long_arithmetics(compiler* comp, ASTNode* node) {
    LiteralExpressionLongArithmetics* literal = (LiteralExpressionLongArithmetics*)node;

    if (literal->type == TYPE_FLOAT) {
        DPRINT("[COMPILER] Compiling float literal\n");
        Value constant_value = value_create_float(literal->value);
        uint32_t const_index = compiler_add_constant_to_compiler(comp, constant_value);
        bytecode bc = bytecode_create_with_number(LOAD_CONST, const_index);
        bytecode* bc_array = malloc(sizeof(bytecode));
        bc_array[0] = bc;
        return create_bytecode_array(bc_array, 1);

    }
    else {
        fprintf(stderr, "Error: type %d for long arithmetic is not supported in this version\n", literal->type);
        Value constant_value = value_create_none();
        uint32_t const_index = compiler_add_constant_to_compiler(comp, constant_value);
        bytecode bc = bytecode_create_with_number(LOAD_CONST, const_index);
        bytecode* bc_array = malloc(sizeof(bytecode));
        bc_array[0] = bc;
        return create_bytecode_array(bc_array, 1);
    }
}


static bytecode_array compiler_compile_array_declaration(compiler* comp, ASTNode* node) {
    ArrayDeclarationStatement* array_decl = (ArrayDeclarationStatement*)node;
    if (node->node_type != NODE_ARRAY_DECLARATION_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }
    
    bytecode_array result = create_bytecode_array(NULL, 0);
    
    // 1. Компилируем размер массива (если есть)
    if (array_decl->size) {
        bytecode_array size_bc = compiler_compile_expression(comp, array_decl->size);
        result = concat_bytecode_arrays(result, size_bc);
        free_bytecode_array(size_bc);
    } else {
        // Если размер не указан, помещаем 0
        bytecode zero_bc = bytecode_create_with_number(LOAD_CONST, 0);
        bytecode_array zero_array = create_single_bytecode_array(zero_bc);
        result = concat_bytecode_arrays(result, zero_array);
        free_bytecode_array(zero_array);
    }
    
    // 2. Компилируем инициализатор (если есть)
    if (array_decl->initializer) {
        // Если есть инициализатор ([1, 2, 3]), компилируем его
        bytecode_array init_bc = compiler_compile_expression(comp, array_decl->initializer);
        result = concat_bytecode_arrays(result, init_bc);
        free_bytecode_array(init_bc);
    } else {
        // Нет инициализатора, создаем пустой массив
        // Размер массива уже в стеке
        // Для создания пустого массива используем BUILD_ARRAY с флагом "пустой"
        bytecode create_bc = bytecode_create_with_number(BUILD_ARRAY, 0); // arg=0 означает пустой массив
        bytecode_array create_array = create_single_bytecode_array(create_bc);
        result = concat_bytecode_arrays(result, create_array);
        free_bytecode_array(create_array);
    }
    
    // 3. Сохраняем в переменную
    size_t var_index = scope_add_local(comp->current_scope, array_decl->name);
    if (var_index == SIZE_MAX) {
        DPRINT("[COMPILER] ERROR: Failed to add variable '%s' to scope\n", array_decl->name);
        free_bytecode_array(result);
        return create_bytecode_array(NULL, 0);
    }
    
    bytecode store_bc = bytecode_create_with_number(STORE_FAST, (uint32_t)var_index);
    bytecode_array store_array = create_single_bytecode_array(store_bc);
    result = concat_bytecode_arrays(result, store_array);
    free_bytecode_array(store_array);
    
    return result;
}


static bytecode_array compiler_compile_array_expression(compiler* comp, ASTNode* node) {
    ArrayExpression* array_expr = (ArrayExpression*)node;
    if (node->node_type != NODE_ARRAY_EXPRESSION) {
        return create_bytecode_array(NULL, 0);
    }

    DPRINT("[COMPILER] Compiling array expression with %u elements\n", array_expr->element_count);
    
    bytecode_array result = create_bytecode_array(NULL, 0);
    
    // Компилируем каждый элемент массива
    for (uint32_t i = 0; i < array_expr->element_count; i++) {
        DPRINT("[COMPILER] Compiling array element %u\n", i);
        bytecode_array elem_bc = compiler_compile_expression(comp, array_expr->elements[i]);
        result = concat_bytecode_arrays(result, elem_bc);
        free_bytecode_array(elem_bc);
    }
    
    // Добавляем инструкцию BUILD_ARRAY с количеством элементов
    // Здесь arg = количество элементов на стеке (не размер массива)
    DPRINT("[COMPILER] Creating array with BUILD_ARRAY %u\n", array_expr->element_count);
    bytecode build_array_bc = bytecode_create_with_number(BUILD_ARRAY, array_expr->element_count);
    bytecode_array build_array_array = create_single_bytecode_array(build_array_bc);
    result = concat_bytecode_arrays(result, build_array_array);
    free_bytecode_array(build_array_array);
    
    DPRINT("[COMPILER] Array expression compiled to %u bytecodes\n", result.count);
    return result;
}

static bytecode_array compiler_compile_subscript_expression(compiler* comp, ASTNode* node) {
    SubscriptExpression* subscript_expr = (SubscriptExpression*)node;
    if (node->node_type != NODE_SUBSCRIPT_EXPRESSION) {
        return create_bytecode_array(NULL, 0);
    }
    
    bytecode_array result = create_bytecode_array(NULL, 0);
    
    // 1. Компилируем массив
    bytecode_array array_bc = compiler_compile_expression(comp, subscript_expr->array);
    result = concat_bytecode_arrays(result, array_bc);
    free_bytecode_array(array_bc);
    
    // 2. Компилируем индекс
    bytecode_array index_bc = compiler_compile_expression(comp, subscript_expr->index);
    result = concat_bytecode_arrays(result, index_bc);
    free_bytecode_array(index_bc);
    
    // 3. Добавляем операцию загрузки элемента по индексу
    bytecode load_subscr_bc = bytecode_create(LOAD_SUBSCR, 0, 0, 0);
    bytecode_array load_array = create_single_bytecode_array(load_subscr_bc);
    result = concat_bytecode_arrays(result, load_array);
    free_bytecode_array(load_array);
    
    return result;
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
        case NODE_LITERAL_EXPRESSION_LONG_ARITHMETICS:
            return compiler_compile_literal_expression_long_arithmetics(comp, node);
        case NODE_VARIABLE_EXPRESSION:
            return compiler_compile_variable_expression(comp, node);
        case NODE_FUNCTION_CALL_EXPRESSION:
            return compiler_compile_function_call_expression(comp, node);
        case NODE_ARRAY_EXPRESSION:
            return compiler_compile_array_expression(comp, node);
        case NODE_SUBSCRIPT_EXPRESSION:
            return compiler_compile_subscript_expression(comp, node);
        default:
            fprintf(stderr, "[COMPILER] Unknown expression node type: %s\n", ast_node_type_to_string(node->node_type));
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
        case NODE_LITERAL_EXPRESSION_LONG_ARITHMETICS:
            return compiler_compile_literal_expression_long_arithmetics(comp, expression);
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
        bytecode_array stmt_bc = compiler_compile_statement(comp, block->statements[i]);
        bytecode_array new_result = concat_bytecode_arrays(result, stmt_bc);
        free_bytecode_array(result);
        free_bytecode_array(stmt_bc);
        result = new_result;
    }

    return result;
}

static bytecode_array compiler_compile_break_statement(compiler* comp, ASTNode* node) {
    DPRINT("[COMPILER] Compiling break statement\n");
    
    if (node->node_type != NODE_BREAK_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }
    
    bytecode break_bc = bytecode_create(BREAK_LOOP, 0, 0, 0);
    return create_single_bytecode_array(break_bc);
}

static bytecode_array compiler_compile_continue_statement(compiler* comp, ASTNode* node) {
    DPRINT("[COMPILER] Compiling continue statement\n");
    
    if (node->node_type != NODE_CONTINUE_STATEMENT) {
        return create_bytecode_array(NULL, 0);
    }
    
    bytecode continue_bc = bytecode_create(CONTINUE_LOOP, 0, 0, 0);
    return create_single_bytecode_array(continue_bc);
}

static bytecode_array compiler_compile_statement(compiler* comp, ASTNode* statement) {
    switch (statement->node_type) {
        case NODE_FUNCTION_DECLARATION_STATEMENT:
            return compiler_compile_function_declaration(comp, statement);
        case NODE_ARRAY_DECLARATION_STATEMENT:
            return compiler_compile_array_declaration(comp, statement);
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
        case NODE_BREAK_STATEMENT:
            return compiler_compile_break_statement(comp, statement);
        case NODE_CONTINUE_STATEMENT:
            return compiler_compile_continue_statement(comp, statement);
        default:
            fprintf(stderr, "[COMPILER] Unknown statement node type: %s\n", ast_node_type_to_string(statement->node_type));
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
    size_t randint_idx = string_table_add(comp->global_names, "randint");
    size_t sqrt_idx = string_table_add(comp->global_names, "sqrt");

    if (print_idx != 0) {
        DPRINT("Warning: print index is %zu, expected 0\n", print_idx);
    }
    if (input_idx != 1) {
        DPRINT("Warning: input index is %zu, expected 1\n", input_idx);
    }
    if (randint_idx != 2) {
        DPRINT("Warning: randint index is %zu, expected 2\n", randint_idx);
    }
    if (sqrt_idx != 3) {
        DPRINT("Warning: sqrt index is %zu, expected 3\n", sqrt_idx);
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

static void compiler_collect_declarations(compiler* comp, ASTNode* node) {
    if (!node) return;
    
    if (node->node_type == NODE_FUNCTION_DECLARATION_STATEMENT) {
        FunctionDeclarationStatement* func_decl = (FunctionDeclarationStatement*)node;
        // Добавляем имя функции в таблицу символов
        compiler_add_global_name(comp, func_decl->name);
    }
    else if (node->node_type == NODE_BLOCK_STATEMENT) {
        BlockStatement* block = (BlockStatement*)node;
        for (size_t i = 0; i < block->statement_count; i++) {
            compiler_collect_declarations(comp, block->statements[i]);
        }
    }
}

compilation_result* compiler_compile(compiler* compiler) {
    if (!compiler) return NULL;
    
    // Первый проход: сбор деклараций
    compiler_collect_declarations(compiler, compiler->ast_tree);
    
    // Второй проход: компиляция тел
    if (compiler->ast_tree->node_type != NODE_BLOCK_STATEMENT) {
        return NULL;
    }

    BlockStatement* casted = (BlockStatement*) compiler->ast_tree;
    
    for (uint32_t i = 0; i < casted->statement_count; i++) {
        bytecode_array result = compiler_compile_statement(compiler, casted->statements[i]);
        emit_bytecode(compiler->result, result);
        free_bytecode_array(result);
    }
    DPRINT("[COMPILER] compiling completed, dumping bytecode:\n");
    bytecode_array_print(&(compiler->result->code_array));
    return compiler->result;
}
