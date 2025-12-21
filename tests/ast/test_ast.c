#include "../../src/parser/parser.h"
#include "../../src/lexer/token.h"
#include "../../src/AST/ast.h"
#include "../../src/debug.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

Token create_token(TokenType type, const char* value, int line, int column) {
    Token token;
    token.type = type;
    token.value = value;
    token.line = line;
    token.column = column;
    return token;
}

void test_print_ast() {
    ASTNode** literals = malloc(2 * sizeof(LiteralExpression*));
    literals[0] = ast_new_literal_expression((SourceLocation){.line=5, .column=6}, TYPE_INT, 5);
    literals[1] = ast_new_literal_expression((SourceLocation){.line=8, .column=6}, TYPE_INT, 7);

    ASTNode* bin_expr = ast_new_binary_expression(
        (SourceLocation){.line=1, .column=1}, 
        literals[0], 
        (Token){.type=OP_PLUS}, 
        literals[1]
    );

    ASTNode* expr_stmt = ast_new_expression_statement(
        (SourceLocation){.line=1, .column=1}, 
        bin_expr
    );
    
    ast_print_tree(expr_stmt, 0);
    ast_free(expr_stmt);
    free(literals);
}

void test_memory_leak() {
    for (int i = 0; i < 1000; i++) {
        ASTNode** literals = malloc(2 * sizeof(LiteralExpression*));
        literals[0] = ast_new_literal_expression((SourceLocation){.line=5, .column=6}, TYPE_INT, 5);
        literals[1] = ast_new_literal_expression((SourceLocation){.line=8, .column=6}, TYPE_INT, 7);

        ASTNode* bin_expr = ast_new_binary_expression(
            (SourceLocation){.line=1, .column=1}, 
            literals[0], 
            (Token){.type=OP_PLUS}, 
            literals[1]
        );

        ASTNode* expr_stmt = ast_new_expression_statement(
            (SourceLocation){.line=1, .column=1}, 
            bin_expr
        );
        ast_free(expr_stmt);

        free(literals);
    }
}

ASTNode* test_ast_new_binary_expression() {
    SourceLocation loc = {1, 1};
    ASTNode* left = ast_new_literal_expression(loc, TYPE_INT, 10);
    ASTNode* right = ast_new_literal_expression(loc, TYPE_INT, 20);
    Token op = {OP_PLUS, "+", 1};
    
    ASTNode* node = ast_new_binary_expression(loc, left, op, right);
    assert(node != NULL);
    assert(node->node_type == NODE_BINARY_EXPRESSION);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    BinaryExpression* binary_expr = (BinaryExpression*)node;
    
    assert(binary_expr->left != left);
    assert(binary_expr->left->node_type == left->node_type);
    
    assert(binary_expr->right != right);
    assert(binary_expr->right->node_type == right->node_type);
    
    assert(binary_expr->operator_.type == op.type);
    assert(strcmp(binary_expr->operator_.value, op.value) == 0);
    assert(binary_expr->operator_.line == op.line);
    
    // Освобождаем временные узлы
    ast_free(left);
    ast_free(right);
    return node;
}

ASTNode* test_ast_new_unary_expression() {
    SourceLocation loc = {1, 1};
    Token op = {OP_MINUS, "-", 1};
    ASTNode* operand = ast_new_literal_expression(loc, TYPE_INT, 5);
    
    ASTNode* node = ast_new_unary_expression(loc, op, operand);
    assert(node != NULL);
    assert(node->node_type == NODE_UNARY_EXPRESSION);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    UnaryExpression* unary_expr = (UnaryExpression*)node;
    
    assert(unary_expr->operator_.type == op.type);
    assert(strcmp(unary_expr->operator_.value, op.value) == 0);
    assert(unary_expr->operator_.line == op.line);
    
    assert(unary_expr->operand != operand);
    assert(unary_expr->operand->node_type == operand->node_type);
    
    // Освобождаем временный узел
    ast_free(operand);
    return node;
}

ASTNode* test_ast_new_literal_expression() {
    SourceLocation loc = {1, 1};
    ASTNode* node = ast_new_literal_expression(loc, TYPE_INT, 42);
    assert(node != NULL);
    assert(node->node_type == NODE_LITERAL_EXPRESSION);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    LiteralExpression* literal = (LiteralExpression*)node;
    assert(literal->type == TYPE_INT);
    assert(literal->value == 42);
    return node;
}

ASTNode* test_ast_new_variable_expression() {
    SourceLocation loc = {1, 1};
    const char* original_name = "test_var";
    
    ASTNode* node = ast_new_variable_expression(loc, original_name);
    assert(node != NULL);
    assert(node->node_type == NODE_VARIABLE_EXPRESSION);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    VariableExpression* var_expr = (VariableExpression*)node;
    
    assert(var_expr->name != original_name);
    assert(strcmp(var_expr->name, original_name) == 0);
    return node;
}

ASTNode* test_ast_new_assignment_statement() {
    SourceLocation loc = {1, 1};
    ASTNode* left = ast_new_variable_expression(loc, "x");
    ASTNode* right = ast_new_literal_expression(loc, TYPE_INT, 100);
    
    ASTNode* node = ast_new_assignment_statement(loc, left, right);
    assert(node != NULL);
    assert(node->node_type == NODE_ASSIGNMENT_STATEMENT);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    AssignmentStatement* assign_stmt = (AssignmentStatement*)node;
    
    assert(assign_stmt->left != left);
    assert(assign_stmt->left->node_type == left->node_type);
    
    assert(assign_stmt->right != right);
    assert(assign_stmt->right->node_type == right->node_type);
    
    // Освобождаем временные узлы
    ast_free(left);
    ast_free(right);
    return node;
}

ASTNode* test_ast_new_variable_declaration_statement() {
    SourceLocation loc = {1, 1};
    const char* original_name = "count";
    ASTNode* initializer = ast_new_literal_expression(loc, TYPE_INT, 0);
    
    ASTNode* node = ast_new_variable_declaration_statement(loc, TYPE_INT, original_name, initializer);
    assert(node != NULL);
    assert(node->node_type == NODE_VARIABLE_DECLARATION_STATEMENT);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    VariableDeclarationStatement* var_decl = (VariableDeclarationStatement*)node;
    assert(var_decl->var_type == TYPE_INT);
    
    assert(var_decl->name != original_name);
    assert(strcmp(var_decl->name, original_name) == 0);
    
    assert(var_decl->initializer != initializer);
    assert(var_decl->initializer->node_type == initializer->node_type);
    
    // Освобождаем временный узел
    ast_free(initializer);
    return node;
}

ASTNode* test_ast_new_return_statement() {
    SourceLocation loc = {1, 1};
    ASTNode* expression = ast_new_literal_expression(loc, TYPE_INT, 0);
    
    ASTNode* node = ast_new_return_statement(loc, expression);
    assert(node != NULL);
    assert(node->node_type == NODE_RETURN_STATEMENT);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    ReturnStatement* return_stmt = (ReturnStatement*)node;
    
    assert(return_stmt->expression != expression);
    assert(return_stmt->expression->node_type == expression->node_type);
    
    // Освобождаем временный узел
    ast_free(expression);
    return node;
}

ASTNode* test_ast_new_if_statement() {
    SourceLocation loc = {1, 1};
    ASTNode* condition = ast_new_literal_expression(loc, TYPE_BOOL, 1);
    
    // Create empty block for then branch
    ASTNode** then_statements = NULL;
    ASTNode* then_branch = ast_new_block_statement(loc, then_statements, 0);
    
    ASTNode* node = ast_new_if_statement(loc, condition, then_branch);
    assert(node != NULL);
    assert(node->node_type == NODE_IF_STATEMENT);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    IfStatement* if_stmt = (IfStatement*)node;
    
    assert(if_stmt->condition != condition);
    assert(if_stmt->condition->node_type == condition->node_type);
    
    assert(if_stmt->then_branch != then_branch);
    assert(if_stmt->then_branch->node_type == then_branch->node_type);
    
    assert(if_stmt->elif_conditions == NULL);
    assert(if_stmt->elif_branches == NULL);
    assert(if_stmt->elif_count == 0);
    assert(if_stmt->else_branch == NULL);
    
    // Освобождаем временные узлы
    ast_free(condition);
    ast_free(then_branch);
    return node;
}

ASTNode* test_ast_new_while_statement() {
    SourceLocation loc = {1, 1};
    ASTNode* condition = ast_new_literal_expression(loc, TYPE_BOOL, 1);
    
    // Create empty block for body
    ASTNode** body_statements = NULL;
    ASTNode* body = ast_new_block_statement(loc, body_statements, 0);
    
    ASTNode* node = ast_new_while_statement(loc, condition, body);
    assert(node != NULL);
    assert(node->node_type == NODE_WHILE_STATEMENT);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    WhileStatement* while_stmt = (WhileStatement*)node;
    
    assert(while_stmt->condition != condition);
    assert(while_stmt->condition->node_type == condition->node_type);
    
    assert(while_stmt->body != body);
    assert(while_stmt->body->node_type == body->node_type);
    
    // Освобождаем временные узлы
    ast_free(condition);
    ast_free(body);
    return node;
}

ASTNode* test_ast_new_function_declaration_statement() {
    SourceLocation loc = {1, 1};
    const char* original_name = "main";

    ASTNode* node = ast_new_function_declaration_statement(loc, original_name, TYPE_INT, NULL, 0, NULL);
    assert(node != NULL);
    assert(node->node_type == NODE_FUNCTION_DECLARATION_STATEMENT);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    FunctionDeclarationStatement* func_decl = (FunctionDeclarationStatement*)node;
    
    assert(func_decl->name != original_name);
    assert(strcmp(func_decl->name, original_name) == 0);
    
    assert(func_decl->return_type == TYPE_INT);
    assert(func_decl->parameters == NULL);
    assert(func_decl->parameter_count == 0);
    assert(func_decl->body == NULL);
    
    return node;
}

ASTNode* test_ast_new_call_expression() {
    SourceLocation loc = {1, 1};
    ASTNode* callee = ast_new_variable_expression(loc, "printf");
    
    ASTNode* arg1 = ast_new_literal_expression(loc, TYPE_INT, 1);
    ASTNode* arg2 = ast_new_literal_expression(loc, TYPE_INT, 2);
    ASTNode** args = (ASTNode**)malloc(2 * sizeof(ASTNode*));
    args[0] = arg1;
    args[1] = arg2;
    
    ASTNode* node = ast_new_call_expression(loc, callee, args, 2);
    assert(node != NULL);
    assert(node->node_type == NODE_FUNCTION_CALL_EXPRESSION);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    FunctionCallExpression* call_expr = (FunctionCallExpression*)node;
    
    assert(call_expr->callee != callee);
    assert(call_expr->callee->node_type == callee->node_type);
    
    assert(call_expr->arguments != args);
    assert(call_expr->argument_count == 2);
    
    assert(call_expr->arguments[0] != args[0]);
    assert(call_expr->arguments[0]->node_type == args[0]->node_type);
    assert(call_expr->arguments[1] != args[1]);
    assert(call_expr->arguments[1]->node_type == args[1]->node_type);
    
    // Освобождаем временные узлы и массив
    ast_free(callee);
    ast_free(arg1);
    ast_free(arg2);
    free(args);
    return node;
}

ASTNode* test_ast_new_for_statement() {
    SourceLocation loc = {1, 1};
    
    // Создаем все временные узлы
    ASTNode* temp_literal1 = ast_new_literal_expression(loc, TYPE_INT, 0);
    ASTNode* initializer = ast_new_variable_declaration_statement(loc, TYPE_INT, "i", temp_literal1);
    
    ASTNode* temp_var1 = ast_new_variable_expression(loc, "i");
    ASTNode* temp_literal2 = ast_new_literal_expression(loc, TYPE_INT, 10);
    ASTNode* condition = ast_new_binary_expression(loc, temp_var1, (Token){OP_LT, "<", 1}, temp_literal2);
    
    ASTNode* temp_var2 = ast_new_variable_expression(loc, "i");
    ASTNode* temp_var3 = ast_new_variable_expression(loc, "i");
    ASTNode* temp_literal3 = ast_new_literal_expression(loc, TYPE_INT, 1);
    ASTNode* temp_binary = ast_new_binary_expression(loc, temp_var3, (Token){OP_PLUS, "+", 1}, temp_literal3);
    ASTNode* temp_assign = ast_new_assignment_statement(loc, temp_var2, temp_binary);
    ASTNode* increment = ast_new_expression_statement(loc, temp_assign);
    
    ASTNode** body_statements = NULL;
    ASTNode* body = ast_new_block_statement(loc, body_statements, 0);
    
    ASTNode* node = ast_new_for_statement(loc, initializer, condition, increment, body);
    assert(node != NULL);
    assert(node->node_type == NODE_FOR_STATEMENT);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    ForStatement* for_stmt = (ForStatement*)node;
    
    assert(for_stmt->initializer != initializer);
    assert(for_stmt->initializer->node_type == initializer->node_type);
    
    assert(for_stmt->condition != condition);
    assert(for_stmt->condition->node_type == condition->node_type);
    
    assert(for_stmt->increment != increment);
    assert(for_stmt->increment->node_type == increment->node_type);
    
    assert(for_stmt->body != body);
    assert(for_stmt->body->node_type == body->node_type);
    
    // Освобождаем ВСЕ временные узлы
    ast_free(temp_literal1);
    ast_free(temp_var1);
    ast_free(temp_literal2);
    ast_free(temp_var2);
    ast_free(temp_var3);
    ast_free(temp_literal3);
    ast_free(temp_binary);
    ast_free(temp_assign);
    ast_free(initializer);
    ast_free(condition);
    ast_free(increment);
    ast_free(body);
    
    return node;
}

ASTNode* test_ast_new_expression_statement() {
    SourceLocation loc = {1, 1};
    ASTNode* expression = ast_new_literal_expression(loc, TYPE_INT, 42);
    
    ASTNode* node = ast_new_expression_statement(loc, expression);
    assert(node != NULL);
    assert(node->node_type == NODE_EXPRESSION_STATEMENT);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    ExpressionStatement* expr_stmt = (ExpressionStatement*)node;
    
    assert(expr_stmt->expression != expression);
    assert(expr_stmt->expression->node_type == expression->node_type);
    
    ast_free(expression);
    return node;
}

ASTNode* test_ast_new_block_statement() {
    SourceLocation loc = {1, 1};
    
    // Create some statements for the block
    ASTNode* literal1 = ast_new_literal_expression(loc, TYPE_INT, 1);
    ASTNode* literal2 = ast_new_literal_expression(loc, TYPE_INT, 2);
    ASTNode* stmt1 = ast_new_expression_statement(loc, literal1);
    ast_free(literal1);
    ASTNode* stmt2 = ast_new_expression_statement(loc, literal2);
    ast_free(literal2);
    
    ASTNode** statements = (ASTNode**)malloc(2 * sizeof(ASTNode*));
    statements[0] = stmt1;
    statements[1] = stmt2;
    
    ASTNode* node = ast_new_block_statement(loc, statements, 2);
    assert(node != NULL);
    assert(node->node_type == NODE_BLOCK_STATEMENT);
    assert(node->location.line == 1);
    assert(node->location.column == 1);
    
    BlockStatement* block_stmt = (BlockStatement*)node;
    
    assert(block_stmt->statement_count == 2);
    
    assert(block_stmt->statements[0]->node_type == statements[0]->node_type);
    assert(block_stmt->statements[1]->node_type == statements[1]->node_type);
    return node;
}

bool test_add_statement_to_block() {
    SourceLocation loc = {1, 1};
    
    // Create empty block
    ASTNode** statements = NULL;
    ASTNode* block = ast_new_block_statement(loc, statements, 0);
    ASTNode* literal = ast_new_literal_expression(loc, TYPE_INT, 42);
    // Create statement to add
    ASTNode* stmt = ast_new_expression_statement(loc, literal);
    ast_free(literal);
    
    bool result = add_statement_to_block(block, stmt);
    assert(result == true);
    
    BlockStatement* block_stmt = (BlockStatement*)block;
    assert(block_stmt->statement_count == 1);
    assert(block_stmt->statements[0] != stmt);
    assert(block_stmt->statements[0]->node_type == stmt->node_type);
    return result;
}

// gcc tests/ast/test_ast.c src/lexer/token.h src/AST/ast.c src/parser/parser.c
int main() {
    debug_enabled = 1;
    //test_memory_leak();
    //test_print_ast();
    ASTNode* test1 = test_ast_new_binary_expression();
    ASTNode* test2 = test_ast_new_unary_expression();
    ASTNode* test3 = test_ast_new_literal_expression();
    ASTNode* test4 = test_ast_new_variable_expression();
    ASTNode* test5 = test_ast_new_assignment_statement();
    ASTNode* test6 = test_ast_new_variable_declaration_statement();
    ASTNode* test7 = test_ast_new_return_statement();
    ASTNode* test8 = test_ast_new_if_statement();
    ASTNode* test9 = test_ast_new_while_statement();
    ASTNode* test10 = test_ast_new_function_declaration_statement();
    ASTNode* test11 = test_ast_new_call_expression();
    ASTNode* test12 = test_ast_new_for_statement();
    ASTNode* test13 = test_ast_new_expression_statement();
    ASTNode* test14 = test_ast_new_block_statement();
    bool test15 = test_add_statement_to_block();

    ast_free(test1);
    ast_free(test2);
    ast_free(test3);
    ast_free(test4);
    ast_free(test5);
    ast_free(test6);
    ast_free(test7);
    ast_free(test8);
    ast_free(test9);
    ast_free(test10);
    ast_free(test11);
    ast_free(test12);
    ast_free(test13);
    ast_free(test14); // segmentation error here
    printf("All tests passed successfully!\n");
    return 0;
}
