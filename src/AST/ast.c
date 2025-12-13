#include "ast.h"
#include <stdlib.h>
#include <stdio.h>
#include "../debug.h"
#include <string.h>
#include <assert.h>
#include <stdbool.h>
/*
=====-------------------------------------+*%@@@@@@@@@@@@@@%%#+-------------------------------
===------------------------------------=*%%@@@@@@@@@@@@@@@@@@%@%%+----------------------------
-------------------------------------+#@%@%@@@%%%##%%@@@@@@@%%@%%%%#=-------------------------
-----------------------------------+%@%@%@@@@@@@@%#%%@@@@@@@%@%%%%%%%#------------------------
---------------------------------+##%@%@@%%@@@@@@@@@@@@@@@@@@@%@%%%%%%%+----------------------
--------------------------------#*#%%@@@@@@%*+*###%%%%%#*+*#@@@@%@%%%%%%=---------------------
-------------------------------**#%%%@@@%*=-----==++========++#@@@%%%%@%%=--------------------
------------------------------=*##%%@@@#=-------------========+*#@@@%%@%@#--------------------
------------------------------+*##%@@@*-----------------========+#@@@@%%%%=-------------------
-----------------------------++*##@@@*------------------------===+#%@@@@%%+-------------------
-----------------------------*+*#@@@#=-------------------=====--==+%@@@%@%*-------------------
-----------------------------+##@@@#----=++*###*=------=+##%%#*+--=*@@@@%@#-------------------
-----------------------------*%@@@%=-===*****##*+=====+**##%####*+==#@@@@%#-------------------
-----------------------------#%@@@+--==++++=+****+=--+###*++++*#***=*@@@%@%-------------------
-----------------------------#%@@%=----=**+##+**==---=#*+*+*@@#***+==%@@@%%-------------------
----------------------------=%@@@#------=++++*++=-----+*++++***++==-=+@@@@%-------------------
----------------------------=%@@%*---------==--------==+++++***+=---=+%@@%#-------------------
----------------------------+%@@%*-------------------==========-----=+#@@%*-------------------
----------------------------+%@%#+-------------=------==+=----========#@@@+-------------------
----------------------------=%@*#*-------------=++===*+=+=========+==*#%@%=-------------------
-----------------------------*@+**=------------+#*+*#%@%#+======+===+*#@@%--------------------
------------------------------*#**+=--------==+####+*###+======+===+*##%@+--------------------
-------------------------------+*#+=-----=***++*+**==*###*+=======++**+#+---------------------
--------------------------------*#+=----=####*++==++**######*==+=++*##*-----------------------
-------------------------------=##*=--==+#*===++*********####+++++*##+------------------------
-------------------------------=#%#*==-===--===*%%%%##**+++***++++##*-------------------------
--------------------------------*#%#++=-==------=+****+++++*++++*#%#+-------------------------
---------------------------------=###*++++=---==+*#**===+******#%%%*--------------------------
----------------------------------=#%#***#**++***#####***######%%%+---------------------------
----------------------------------==*##***#%%%##%%%%%%%%%%%%#%%%#+----------------------------
-------------------------::::...*#====+%%#%%%%%@%@@@@@@%%%%##%#*=-:-=++=-:------------------==
--------------::..........::...:+*+===--=*#%%%%%%%%%%@@@%%%%#**++-.:---::..:--------------====
===--::....:::::.................+++====---+*###%%%%%%%####**+++=::--:........:::..........:::
::................:...............:++++========+**#######******:::.:..........................
:....................................:=+===++=====++*##%##**-:................................
.........................................-=========*%%==-:..................................::
............................................:::::.::..:::::::.............................::::
..............................................:::::::::::::::::...........................::::*/

static ASTNode* ast_node_copy(ASTNode* node);

static ASTNode* ast_node_allocate(NodeType node_type, SourceLocation loc) {
    // generall allocator for ast_node with node_type and loc
    // use this one to allocate smth
    ASTNode* node = NULL;
    
    switch (node_type) {
        // Expressions
        case NODE_BINARY_EXPRESSION:
            node = (ASTNode*)malloc(sizeof(BinaryExpression));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                BinaryExpression* binary_expr = (BinaryExpression*) node;
                binary_expr->left = NULL;
                binary_expr->right = NULL;
                binary_expr->operator_ = (Token){0};
            }
            break;
            
        case NODE_FUNCTION_CALL_EXPRESSION:
            node = (ASTNode*)malloc(sizeof(FunctionCallExpression));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                FunctionCallExpression* func_call = (FunctionCallExpression*) node;
                func_call->callee = NULL;
                func_call->arguments = NULL;
                func_call->argument_count = 0;
            }
            break;
            
        case NODE_UNARY_EXPRESSION:
            node = (ASTNode*)malloc(sizeof(UnaryExpression));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                UnaryExpression* unary_expr = (UnaryExpression*) node;
                unary_expr->operator_ = (Token){0};
                unary_expr->operand = NULL;
            }
            break;
            
        case NODE_LITERAL_EXPRESSION:
            node = (ASTNode*)malloc(sizeof(LiteralExpression));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                LiteralExpression* literal_expr = (LiteralExpression*) node;
                literal_expr->value = 0;
            }
            break;
            
        case NODE_VARIABLE_EXPRESSION:
            node = (ASTNode*)malloc(sizeof(VariableExpression));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                VariableExpression* var_expr = (VariableExpression*) node;
                var_expr->name = NULL;
            }
            break;
            
        // Statements
        /*
        case NODE_PROGRAM:
            // Для ProgramStatement нужно определить структуру
            node = (ASTNode*)malloc(sizeof(ASTNode)); // временно базовый узел
            if (node) {
                node->node_type = node_type;
                node->location = loc;
            }
            break;
        */    
        case NODE_ASSIGNMENT_STATEMENT:
            node = (ASTNode*)malloc(sizeof(AssignmentStatement));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                AssignmentStatement* assign_stmt = (AssignmentStatement*) node;
                assign_stmt->left = NULL;
                assign_stmt->right = NULL;
            }
            break;
            
        case NODE_VARIABLE_DECLARATION_STATEMENT:
            node = (ASTNode*)malloc(sizeof(VariableDeclarationStatement));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                VariableDeclarationStatement* var_decl = (VariableDeclarationStatement*) node;
                var_decl->var_type = (TypeVar){0};
                var_decl->name = NULL;
                var_decl->initializer = NULL;
            }
            break;
            
        case NODE_FUNCTION_DECLARATION_STATEMENT:
            node = (ASTNode*)malloc(sizeof(FunctionDeclarationStatement));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                FunctionDeclarationStatement* func_decl = (FunctionDeclarationStatement*) node;
                func_decl->return_type = (TypeVar){0};
                func_decl->name = NULL;
                func_decl->body = NULL;
            }
            break;
            
        case NODE_RETURN_STATEMENT:
            node = (ASTNode*)malloc(sizeof(ReturnStatement));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                ReturnStatement* return_stmt = (ReturnStatement*) node;
                return_stmt->expression = NULL;
            }
            break;
            
        case NODE_EXPRESSION_STATEMENT:
            node = (ASTNode*)malloc(sizeof(ExpressionStatement));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                ExpressionStatement* expr_stmt = (ExpressionStatement*) node;
                expr_stmt->expression = NULL;
            }
            break;
            
        case NODE_BLOCK_STATEMENT:
            node = (ASTNode*)malloc(sizeof(BlockStatement));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                BlockStatement* block_stmt = (BlockStatement*) node;
                block_stmt->statements = NULL;
                block_stmt->statement_count = 0;
            }
            break;
            
        case NODE_IF_STATEMENT:
            node = (ASTNode*)malloc(sizeof(IfStatement));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                IfStatement* if_stmt = (IfStatement*) node;
                if_stmt->condition = NULL;
                if_stmt->then_branch = NULL;
                if_stmt->else_branch = NULL;
            }
            break;
            
        case NODE_WHILE_STATEMENT:
            node = (ASTNode*)malloc(sizeof(WhileStatement));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                WhileStatement* while_stmt = (WhileStatement*) node;
                while_stmt->condition = NULL;
                while_stmt->body = NULL;
            }
            break;
            
        case NODE_FOR_STATEMENT:
            node = (ASTNode*)malloc(sizeof(ForStatement));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
                ForStatement* for_stmt = (ForStatement*) node;
                for_stmt->initializer = NULL;
                for_stmt->condition = NULL;
                for_stmt->increment = NULL;
                for_stmt->body = NULL;
            }
            break;
            
        default:
            node = malloc(sizeof(ASTNode));
            if (node) {
                node->node_type = node_type;
                node->location = loc;
            }
            break;
    }
    
    return node;
}

// copy specific ast_node functions
static BinaryExpression* copy_binary_expression(ASTNode* original) {
    if (!original) return NULL;
    BinaryExpression* orig = (BinaryExpression*)original;
    
    BinaryExpression* copy = malloc(sizeof(BinaryExpression));
    copy->base = orig->base;
    copy->left = ast_node_copy(orig->left);
    copy->right = ast_node_copy(orig->right);
    copy->operator_ = orig->operator_;
    
    if (orig->operator_.value) {
        copy->operator_.value = strdup(orig->operator_.value);
    }
    
    return copy;
}

static UnaryExpression* copy_unary_expression(ASTNode* original) {
    if (!original) return NULL;
    UnaryExpression* orig = (UnaryExpression*)original;
    
    UnaryExpression* copy = malloc(sizeof(UnaryExpression));
    copy->base = orig->base;
    copy->operand = ast_node_copy(orig->operand);
    copy->operator_ = orig->operator_;
    
    if (orig->operator_.value) {
        copy->operator_.value = strdup(orig->operator_.value);
    }
    
    return copy;
}

static VariableExpression* copy_variable_expression(ASTNode* original) {
    if (!original) return NULL;
    VariableExpression* orig = (VariableExpression*)original;
    
    VariableExpression* copy = malloc(sizeof(VariableExpression));
    copy->base = orig->base;
    
    if (orig->name) {
        copy->name = strdup(orig->name);
    } else {
        copy->name = NULL;
    }
    
    return copy;
}

static FunctionCallExpression* copy_call_expression(ASTNode* original) {
    if (!original) return NULL;
    FunctionCallExpression* orig = (FunctionCallExpression*)original;
    
    FunctionCallExpression* copy = malloc(sizeof(FunctionCallExpression));
    copy->base = orig->base;
    copy->callee = ast_node_copy(orig->callee);
    copy->argument_count = orig->argument_count;
    
    if (orig->argument_count > 0 && orig->arguments) {
        copy->arguments = malloc(orig->argument_count * sizeof(ASTNode*));
        for (int i = 0; i < orig->argument_count; i++) {
            copy->arguments[i] = ast_node_copy(orig->arguments[i]);
        }
    } else {
        copy->arguments = NULL;
    }
    
    return copy;
}

static AssignmentStatement* copy_assignment_statement(ASTNode* original) {
    if (!original) return NULL;
    AssignmentStatement* orig = (AssignmentStatement*)original;
    
    AssignmentStatement* copy = malloc(sizeof(AssignmentStatement));
    copy->base = orig->base;
    copy->left = ast_node_copy(orig->left);
    copy->right = ast_node_copy(orig->right);
    
    return copy;
}

static VariableDeclarationStatement* copy_variable_declaration_statement(ASTNode* original) {
    if (!original) return NULL;
    VariableDeclarationStatement* orig = (VariableDeclarationStatement*)original;
    
    VariableDeclarationStatement* copy = malloc(sizeof(VariableDeclarationStatement));
    copy->base = orig->base;
    copy->var_type = orig->var_type;
    
    if (orig->name) {
        copy->name = strdup(orig->name);
    } else {
        copy->name = NULL;
    }
    
    copy->initializer = ast_node_copy(orig->initializer);
    
    return copy;
}

static ReturnStatement* copy_return_statement(ASTNode* original) {
    if (!original) return NULL;
    ReturnStatement* orig = (ReturnStatement*)original;
    
    ReturnStatement* copy = malloc(sizeof(ReturnStatement));
    copy->base = orig->base;
    copy->expression = ast_node_copy(orig->expression);
    
    return copy;
}

static ExpressionStatement* copy_expression_statement(ASTNode* original) {
    if (!original) return NULL;
    ExpressionStatement* orig = (ExpressionStatement*)original;
    
    ExpressionStatement* copy = malloc(sizeof(ExpressionStatement));
    copy->base = orig->base;
    copy->expression = ast_node_copy(orig->expression);
    
    return copy;
}

static IfStatement* copy_if_statement(ASTNode* original) {
    if (!original) return NULL;
    IfStatement* orig = (IfStatement*)original;
    
    IfStatement* copy = malloc(sizeof(IfStatement));
    copy->base = orig->base;
    copy->condition = ast_node_copy(orig->condition);
    copy->then_branch = ast_node_copy(orig->then_branch);
    copy->elif_count = orig->elif_count;
    
    if (orig->elif_count > 0 && orig->elif_conditions && orig->elif_branches) {
        copy->elif_conditions = malloc(orig->elif_count * sizeof(ASTNode*));
        copy->elif_branches = malloc(orig->elif_count * sizeof(ASTNode*));
        for (size_t i = 0; i < orig->elif_count; i++) {
            copy->elif_conditions[i] = ast_node_copy(orig->elif_conditions[i]);
            copy->elif_branches[i] = ast_node_copy(orig->elif_branches[i]);
        }
    } else {
        copy->elif_conditions = NULL;
        copy->elif_branches = NULL;
    }
    
    copy->else_branch = ast_node_copy(orig->else_branch);
    
    return copy;
}

static WhileStatement* copy_while_statement(ASTNode* original) {
    if (!original) return NULL;
    WhileStatement* orig = (WhileStatement*)original;
    
    WhileStatement* copy = malloc(sizeof(WhileStatement));
    copy->base = orig->base;
    copy->condition = ast_node_copy(orig->condition);
    copy->body = ast_node_copy(orig->body);
    
    return copy;
}

static ForStatement* copy_for_statement(ASTNode* original) {
    if (!original) return NULL;
    ForStatement* orig = (ForStatement*)original;
    
    ForStatement* copy = malloc(sizeof(ForStatement));
    copy->base = orig->base;
    copy->initializer = ast_node_copy(orig->initializer);
    copy->condition = ast_node_copy(orig->condition);
    copy->increment = ast_node_copy(orig->increment);
    copy->body = ast_node_copy(orig->body);
    
    return copy;
}

static BlockStatement* copy_block_statement(ASTNode* original) {
    if (!original) return NULL;
    BlockStatement* orig = (BlockStatement*)original;
    
    // Создаем новую базовую структуру
    BlockStatement* copy = malloc(sizeof(BlockStatement));
    if (!copy) return NULL;
    
    // Правильно инициализируем базовую структуру
    copy->base.node_type = orig->base.node_type;
    copy->base.location = orig->base.location;
    
    copy->statement_count = orig->statement_count;
    
    if (orig->statement_count > 0 && orig->statements) {
        copy->statements = malloc(orig->statement_count * sizeof(ASTNode*));
        if (!copy->statements) {
            free(copy);
            return NULL;
        }
        for (size_t i = 0; i < orig->statement_count; i++) {
            copy->statements[i] = ast_node_copy(orig->statements[i]);
            if (!copy->statements[i]) {
                // В случае ошибки освобождаем уже скопированные statements
                for (size_t j = 0; j < i; j++) {
                    ast_free(copy->statements[j]);
                }
                free(copy->statements);
                free(copy);
                return NULL;
            }
        }
    } else {
        copy->statements = NULL;
    }
    
    return copy;
}

static FunctionDeclarationStatement* copy_function_declaration_statement(ASTNode* original) {
    if (!original) return NULL;
    FunctionDeclarationStatement* orig = (FunctionDeclarationStatement*)original;
    
    FunctionDeclarationStatement* copy = malloc(sizeof(FunctionDeclarationStatement));
    if (!copy) return NULL;
    
    // Правильно инициализируем базовую структуру
    copy->base.node_type = orig->base.node_type;
    copy->base.location = orig->base.location;
    
    copy->return_type = orig->return_type;
    
    if (orig->name) {
        copy->name = strdup(orig->name);
        if (!copy->name) {
            free(copy);
            return NULL;
        }
    } else {
        copy->name = NULL;
    }
    
    copy->parameter_count = orig->parameter_count;
    
    if (orig->parameter_count > 0 && orig->parameters) {
        copy->parameters = malloc(orig->parameter_count * sizeof(Parameter));
        if (!copy->parameters) {
            free(copy->name);
            free(copy);
            return NULL;
        }
        for (size_t i = 0; i < orig->parameter_count; i++) {
            copy->parameters[i] = orig->parameters[i];
        
            if (orig->parameters[i].name) {
                copy->parameters[i].name = strdup(orig->parameters[i].name);
                if (!copy->parameters[i].name) {
                    // Освобождаем уже скопированные имена параметров
                    for (size_t j = 0; j < i; j++) {
                        free(copy->parameters[j].name);
                    }
                    free(copy->parameters);
                    free(copy->name);
                    free(copy);
                    return NULL;
                }
            } else {
                copy->parameters[i].name = NULL;
            }
        }
    } else {
        copy->parameters = NULL;
    }    

    copy->body = ast_node_copy(orig->body);
    if (!copy->body) {
        // Освобождаем ресурсы если копирование тела не удалось
        if (copy->parameters) {
            for (size_t i = 0; i < copy->parameter_count; i++) {
                free(copy->parameters[i].name);
            }
            free(copy->parameters);
        }
        free(copy->name);
        free(copy);
        return NULL;
    }
    
    return copy;
}

static ASTNode* ast_node_copy(ASTNode* original) {
    // General ast_node copy function, use this one to copy smth
    if (!original) return NULL;
    
    switch (original->node_type) {
        case NODE_BINARY_EXPRESSION:
            return (ASTNode*)copy_binary_expression(original);
            
        case NODE_UNARY_EXPRESSION:
            return (ASTNode*)copy_unary_expression(original);
            
        case NODE_LITERAL_EXPRESSION: {
            LiteralExpression* orig = (LiteralExpression*)original;
            LiteralExpression* copy = malloc(sizeof(LiteralExpression));
            *copy = *orig;
            return (ASTNode*)copy;
        }
            
        case NODE_VARIABLE_EXPRESSION:
            return (ASTNode*)copy_variable_expression(original);
            
        case NODE_FUNCTION_CALL_EXPRESSION:
            return (ASTNode*)copy_call_expression(original);
            
        case NODE_ASSIGNMENT_STATEMENT:
            return (ASTNode*)copy_assignment_statement(original);
            
        case NODE_VARIABLE_DECLARATION_STATEMENT:
            return (ASTNode*)copy_variable_declaration_statement(original);
            
        case NODE_RETURN_STATEMENT:
            return (ASTNode*)copy_return_statement(original);
            
        case NODE_EXPRESSION_STATEMENT:
            return (ASTNode*)copy_expression_statement(original);
            
        case NODE_BLOCK_STATEMENT:
            return (ASTNode*)copy_block_statement(original);
            
        case NODE_IF_STATEMENT:
            return (ASTNode*)copy_if_statement(original);
            
        case NODE_WHILE_STATEMENT:
            return (ASTNode*)copy_while_statement(original);
            
        case NODE_FOR_STATEMENT:
            return (ASTNode*)copy_for_statement(original);
            
        case NODE_FUNCTION_DECLARATION_STATEMENT:
            return (ASTNode*)copy_function_declaration_statement(original);
            
        case NODE_PROGRAM:
            fprintf(stderr, "Program node copying not implemented yet\n");
            return NULL;
            
        default:
            fprintf(stderr, "Unknown node type in copy function: %d\n", original->node_type);
            return NULL;
    }
}

// Statements constructors

//ASTNode* ast_new_program(SourceLocation loc) {
//    ASTNode* NodeProgram = ast_node_allocate(NODE_PROGRAM, loc);
//    if (node) {
//        node->program.functions = NULL;
//        node->program.function_count = 0;
//    }
//    return (ASTNode*)node;
//}

Parameter* ast_new_parameter(const char* name, TypeVar type){
    Parameter* parameter = malloc(sizeof(Parameter));
    parameter->name = strdup(name);
    parameter->type = type;
    return parameter;
}


ASTNode* ast_new_function_declaration_statement(SourceLocation loc, const char* name, TypeVar return_type, Parameter* parameters, size_t parameter_count, ASTNode* body) {
    ASTNode* node = ast_node_allocate(NODE_FUNCTION_DECLARATION_STATEMENT, loc);
    if (!node) return NULL;
    
    FunctionDeclarationStatement* casted_node = (FunctionDeclarationStatement*) node;
    casted_node->name = strdup(name);
    casted_node->return_type = return_type;
    casted_node->parameters = parameters;
    casted_node->parameter_count = parameter_count;
    casted_node->body = ast_node_copy(body);
    
    return node;
}

ASTNode* ast_new_variable_declaration_statement(SourceLocation loc, TypeVar var_type, const char* name, ASTNode* initializer) {
    ASTNode* node = ast_node_allocate(NODE_VARIABLE_DECLARATION_STATEMENT, loc);
    if (!node) return NULL;
    VariableDeclarationStatement* casted_node = (VariableDeclarationStatement*) node;
    casted_node->var_type = var_type;
    casted_node->name = strdup(name);
    casted_node->initializer = ast_node_copy(initializer);
    return node;
}

ASTNode* ast_new_expression_statement(SourceLocation loc, ASTNode* expression) {
    ASTNode* node = ast_node_allocate(NODE_EXPRESSION_STATEMENT, loc);
    if (!node) return NULL;
    ExpressionStatement* casted_node = (ExpressionStatement*) node;
    casted_node->expression = ast_node_copy(expression);
    return node;
}

ASTNode* ast_new_return_statement(SourceLocation loc, ASTNode* expression) {
    ASTNode* node = ast_node_allocate(NODE_RETURN_STATEMENT, loc);
    if (!node) return NULL;
    ReturnStatement* casted_node = (ReturnStatement*) node;
    casted_node->expression = ast_node_copy(expression);
    return node;
}

ASTNode* ast_new_block_statement(SourceLocation loc, ASTNode** statements, size_t statement_count) {
    ASTNode* node = ast_node_allocate(NODE_BLOCK_STATEMENT, loc);
    if (!node) return NULL;
    BlockStatement* casted_node = (BlockStatement*) node;
    
    casted_node->statements = malloc(statement_count * sizeof(ASTNode*));
    if (!casted_node->statements) {
        free(node);
        return NULL;
    }
    
    for (size_t i = 0; i < statement_count; i++) {
        casted_node->statements[i] = statements[i];
    }
    
    casted_node->statement_count = statement_count;
    return node;
}

ASTNode* ast_new_if_statement(SourceLocation loc, ASTNode* condition, ASTNode* then_branch) {
    ASTNode* node = ast_node_allocate(NODE_IF_STATEMENT, loc);
    if (!node) return NULL;
    IfStatement* casted_node = (IfStatement*) node;
    casted_node->condition = ast_node_copy(condition);
    casted_node->then_branch = ast_node_copy(then_branch);
    casted_node->elif_conditions = NULL;
    casted_node->elif_branches = NULL;
    casted_node->elif_count = 0;
    casted_node->else_branch = NULL;
    return node;
}

ASTNode* ast_new_while_statement(SourceLocation loc, ASTNode* condition, ASTNode* body) {
    ASTNode* node = ast_node_allocate(NODE_WHILE_STATEMENT, loc);
    if (!node) return NULL;
    WhileStatement* casted_node = (WhileStatement*) node;
    casted_node->condition = ast_node_copy(condition);
    casted_node->body = ast_node_copy(body);
    return node;
}

ASTNode* ast_new_assignment_statement(SourceLocation loc, ASTNode* left, ASTNode* right) {
    ASTNode* node = ast_node_allocate(NODE_ASSIGNMENT_STATEMENT, loc);
    if (!node) return NULL;
    AssignmentStatement* casted_node = (AssignmentStatement*) node;
    casted_node->left = ast_node_copy(left);
    casted_node->right = ast_node_copy(right);
    return node;
}

ASTNode* ast_new_for_statement(SourceLocation loc, ASTNode* initializer, ASTNode* condition, ASTNode* increment, ASTNode* body) {
    ASTNode* node = ast_node_allocate(NODE_FOR_STATEMENT, loc);
    if (!node) return NULL;
    ForStatement* casted_node = (ForStatement*) node;
    casted_node->initializer = ast_node_copy(initializer);
    casted_node->condition = ast_node_copy(condition);
    casted_node->increment = ast_node_copy(increment);
    casted_node->body = ast_node_copy(body);
    return node;
}

// Expressions constructors

ASTNode* ast_new_binary_expression(SourceLocation loc, ASTNode* left, Token op, ASTNode* right) {
    ASTNode* node = ast_node_allocate(NODE_BINARY_EXPRESSION, loc);
    if (!node) return NULL;
    BinaryExpression* casted_node = (BinaryExpression*) node;

    casted_node->left = ast_node_copy(left);
    casted_node->right = ast_node_copy(right);
    casted_node->operator_ = op;
    if (op.value) {
        casted_node->operator_.value = strdup(op.value);
    }    
    return node;
}

ASTNode* ast_new_unary_expression(SourceLocation loc, Token op, ASTNode* operand) {
    ASTNode* node = ast_node_allocate(NODE_UNARY_EXPRESSION, loc);
    if (!node) return NULL;
    UnaryExpression* casted_node = (UnaryExpression*) node;
    casted_node->operator_ = op;
    if (op.value) {
        casted_node->operator_.value = strdup(op.value);
    }
    casted_node->operand = ast_node_copy(operand);
    return node;
}

ASTNode* ast_new_literal_expression(SourceLocation loc, TypeVar type, int64_t value) { 
    ASTNode* node = ast_node_allocate(NODE_LITERAL_EXPRESSION, loc);
    if (!node) return NULL;
    LiteralExpression* casted_node = (LiteralExpression*) node;
    casted_node->type = type;
    casted_node->value = value;
    return node;
}

ASTNode* ast_new_variable_expression(SourceLocation loc, const char* name) {
    ASTNode* node = ast_node_allocate(NODE_VARIABLE_EXPRESSION, loc);
    if (!node) return NULL;
    VariableExpression* casted_node = (VariableExpression*) node;
    casted_node->name = strdup(name);
    return node;
}

ASTNode* ast_new_call_expression(SourceLocation loc, ASTNode* callee, ASTNode** args, int arg_count) {
    ASTNode* node = ast_node_allocate(NODE_FUNCTION_CALL_EXPRESSION, loc);
    if (!node) return NULL;
    FunctionCallExpression* casted_node = (FunctionCallExpression*) node;
    casted_node->callee = ast_node_copy(callee);
    
    if (arg_count > 0 && args) {
        casted_node->arguments = malloc(arg_count * sizeof(ASTNode*));
        if (!casted_node->arguments) {
            free(node);
            return NULL;
        }
        for (int i = 0; i < arg_count; i++) {
            casted_node->arguments[i] = ast_node_copy(args[i]);
        }
        casted_node->argument_count = arg_count;
    } else {
        casted_node->arguments = NULL;
        casted_node->argument_count = 0;
    }
    return node;
}

const TypeVar token_type_to_type_var(TokenType token_type) {
    switch (token_type) {
        case KW_INT:
            return TYPE_INT;
        case KW_BOOL:
            return TYPE_BOOL;
        //case KW_LONG:
        //    return TYPE_LONG;
        case KW_ARRAY:
            return TYPE_ARRAY;
        //case KW_STRUCT:
        //    return TYPE_STRUCT;
        default:
            return TYPE_INT; //TODO FIX HERE
    }
}

void ast_free(ASTNode* node) {
    if (!node) {
        DPRINT("ast_free: NULL node\n");
        return;
    }
    DPRINT("ast_free: node_type=%s\n", ast_node_type_to_string(node->node_type));
    switch (node->node_type) {

            /*
        case NODE_PROGRAM: {
            for (size_t i = 0; i < node->function_count; i++) {
                ast_free(node->functions[i]);
            }
            free(node->functions);
            break;
        }
            
            */
        case NODE_FUNCTION_DECLARATION_STATEMENT: {
            FunctionDeclarationStatement* casted_node = (FunctionDeclarationStatement*) node;
            free(casted_node->name);

            for (size_t i = 0; i < casted_node->parameter_count; i++) {
                free(casted_node->parameters[i].name);
            }
            free(casted_node->parameters);

            ast_free(casted_node->body);
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }
            
        case NODE_VARIABLE_DECLARATION_STATEMENT: {
            VariableDeclarationStatement* casted_node = (VariableDeclarationStatement*) node;
            free(casted_node->name);
            ast_free(casted_node->initializer);
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }
            
        case NODE_EXPRESSION_STATEMENT: {
            ExpressionStatement* casted_node = (ExpressionStatement*) node;
            ast_free(casted_node->expression);
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }
            
        case NODE_RETURN_STATEMENT: {
            ReturnStatement* casted_node = (ReturnStatement*) node;
            ast_free(casted_node->expression);
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }
            
        case NODE_BLOCK_STATEMENT: {
            BlockStatement* casted_node = (BlockStatement*) node;
            if (casted_node->statements) {
                for (size_t i = 0; i < casted_node->statement_count; i++) {
                    ast_free(casted_node->statements[i]);
                }
                free(casted_node->statements);
            }            
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }
            
        case NODE_IF_STATEMENT: {
            IfStatement* casted_node = (IfStatement*) node;
            ast_free(casted_node->condition);
            ast_free(casted_node->then_branch);

            if (casted_node->elif_conditions && casted_node->elif_branches) {
                for (size_t i = 0; i < casted_node->elif_count; i++) {
                    ast_free(casted_node->elif_conditions[i]);
                    ast_free(casted_node->elif_branches[i]);
                }
                free(casted_node->elif_conditions);
                free(casted_node->elif_branches);
            }

            ast_free(casted_node->else_branch);
            free(casted_node);
            break;
        }            

        case NODE_WHILE_STATEMENT: {
            WhileStatement* casted_node = (WhileStatement*) node;
            ast_free(casted_node->condition);
            ast_free(casted_node->body);
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }
            
        case NODE_FOR_STATEMENT: {
            ForStatement* casted_node = (ForStatement*) node;
            ast_free(casted_node->initializer);
            ast_free(casted_node->condition);
            ast_free(casted_node->increment);
            ast_free(casted_node->body);
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }

        case NODE_ASSIGNMENT_STATEMENT: {
            AssignmentStatement* casted_node = (AssignmentStatement*) node;
            ast_free(casted_node->left);
            ast_free(casted_node->right);
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }
            
        case NODE_BINARY_EXPRESSION: {
            BinaryExpression* casted_node = (BinaryExpression*) node;
            ast_free(casted_node->left);
            ast_free(casted_node->right);
            if (casted_node->operator_.value) {
                free(casted_node->operator_.value);
            }
            free(casted_node);            
            node = NULL;
            casted_node = NULL;
            break;
        }
            
        case NODE_UNARY_EXPRESSION: {
            UnaryExpression* casted_node = (UnaryExpression*) node;
            ast_free(casted_node->operand);
            if (casted_node->operator_.value) {
                free(casted_node->operator_.value);
            }
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }
            
        case NODE_LITERAL_EXPRESSION: {
            LiteralExpression* casted_node = (LiteralExpression*) node;
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }
            
        case NODE_VARIABLE_EXPRESSION: {
            VariableExpression* casted_node = (VariableExpression*) node;
            free(casted_node->name);
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }
            
        case NODE_FUNCTION_CALL_EXPRESSION: {
            FunctionCallExpression* casted_node = (FunctionCallExpression*) node;
            if (casted_node->arguments) {
                for (int i = 0; i < casted_node->argument_count; i++) {
                    ast_free(casted_node->arguments[i]);
                }
                free(casted_node->arguments);
            }
            ast_free(casted_node->callee);
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }

        default: {
            DPRINT("Freeing node type: %d at %p\n", ast_node_type_to_string(node->node_type), (void*)node);
            free(node);
            node = NULL;
            break;
        }
    }
}


const char* ast_node_type_to_string(int node_type) {
    switch (node_type) {
        case NODE_PROGRAM: return "Program";
        case NODE_FUNCTION_DECLARATION_STATEMENT: return "FunctionDeclarationStatement";
        case NODE_VARIABLE_DECLARATION_STATEMENT: return "VariableDeclarationStatement";
        case NODE_EXPRESSION_STATEMENT: return "ExpressionStatement";
        case NODE_RETURN_STATEMENT: return "ReturnStatement";
        case NODE_BLOCK_STATEMENT: return "BlockStatement";
        case NODE_IF_STATEMENT: return "IfStatement";
        case NODE_WHILE_STATEMENT: return "WhileStatement";
        case NODE_ASSIGNMENT_STATEMENT: return "AssignmentStatement";
        case NODE_FOR_STATEMENT: return "ForStatement";

        case NODE_BINARY_EXPRESSION: return "BinaryExpression";
        case NODE_UNARY_EXPRESSION: return "UnaryExpression";
        case NODE_LITERAL_EXPRESSION: return "LiteralExpression";
        case NODE_VARIABLE_EXPRESSION: return "VariableExpression";
        case NODE_FUNCTION_CALL_EXPRESSION: return "FunctionCallExpression";
        default: return "Unknown";
    }
}

const char* type_var_to_string(int type_var) {
    switch (type_var) {
        case TYPE_INT: return "int";
        case TYPE_LONG: return "long";
        case TYPE_BOOL: return "bool";
        case TYPE_ARRAY: return "array";
        case TYPE_STRUCT: return "struct";
        default: return "unknown_type";
    }
}

const char* token_type_to_string(int token_type) {
    switch (token_type) {
        // 1.1 TYPES
        case KW_INT: return "KW_INT";
        case KW_BOOL: return "KW_BOOL";
        case KW_LONG: return "KW_LONG";
        case KW_ARRAY: return "KW_ARRAY";

        // 1.2 Special constants
        case KW_NONE: return "KW_NONE";
        case KW_TRUE: return "KW_TRUE";
        case KW_FALSE: return "KW_FALSE";

        // 1.3 Control flow
        case KW_IF: return "KW_IF";
        case KW_ELSE: return "KW_ELSE";
        case KW_ELIF: return "KW_ELIF";
        case KW_WHILE: return "KW_WHILE";
        case KW_FOR: return "KW_FOR";
        case KW_BREAK: return "KW_BREAK";
        case KW_CONTINUE: return "KW_CONTINUE";
        case KW_RETURN: return "KW_RETURN";

        // 1.4 Structs, object.attr
        case KW_STRUCT: return "KW_STRUCT";
        case KW_DOT: return "KW_DOT";
        
        // 2. Identifiers
        case IDENTIFIER: return "IDENTIFIER";
        
        // 3. Literals
        case INT_LITERAL: return "INT_LITERAL";
        //case BOOL_LITERAL: return "BOOL_LITERAL";
        case LONG_LITERAL: return "LONG_LITERAL";

        // 4. Operators

        // 4.1. + - / * %
        case OP_PLUS: return "OP_PLUS";
        case OP_MINUS: return "OP_MINUS";
        case OP_MULT: return "OP_MULT";
        case OP_DIV: return "OP_DIV";
        case OP_MOD: return "OP_MOD";

        // 4.2. = += -= /= *= %=
        case OP_ASSIGN: return "OP_ASSIGN";
        /*case OP_ASSIGN_PLUS: return "OP_ASSIGN_PLUS";
        case OP_ASSIGN_MINUS: return "OP_ASSIGN_MINUS";
        case OP_ASSIGN_MULT: return "OP_ASSIGN_MULT";
        case OP_ASSIGN_DIV: return "OP_ASSIGN_DIV";
        case OP_ASSIGN_MOD: return "OP_ASSIGN_MOD";*/

        // 4.3. == != <=>
        case OP_EQ: return "OP_EQ";
        case OP_NE: return "OP_NE";
        case OP_LT: return "OP_LT";
        case OP_GT: return "OP_GT";
        case OP_LE: return "OP_LE";
        case OP_GE: return "OP_GE";

        // 4.4. and or not
        case OP_AND: return "OP_AND";
        case OP_OR: return "OP_OR";
        case OP_NOT: return "OP_NOT";
        
        // 5. Delimiters

        // 5.1. { } ( ) [ ]
        case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN";
        case LBRACE: return "LBRACE";
        case RBRACE: return "RBRACE";
        case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET";

        // 5.2 ; ,
        case SEMICOLON: return "SEMICOLON";
        case COMMA: return "COMMA";
        
        // 6. Special
        case END_OF_FILE: return "END_OF_FILE";
        case ERROR: return "ERROR";
        
        default: return "UNKNOWN_TOKEN";
    }
}


void ast_print_tree(ASTNode* node, int indent) {
    if (!node) return;
    if (!debug_enabled) return;
    
    for (int i = 0; i < indent; i++) DPRINT("  ");
    DPRINT("\nAST TREE:\n");
    ast_print(node, indent);
    DPRINT("\n");
}

void ast_print(ASTNode* node, int indent) {
    if (!node) return;
    if (!debug_enabled) return;
    
    for (int i = 0; i < indent; i++) DPRINT("  ");
    DPRINT("%s\n", ast_node_type_to_string(node->node_type));
    
    switch (node->node_type) {
        case NODE_PROGRAM: {
            // TODO
            break;
        }
            
        case NODE_FUNCTION_DECLARATION_STATEMENT: {
            FunctionDeclarationStatement* casted_node = (FunctionDeclarationStatement*) node;

            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("Function: %s (returns type %s)\n", casted_node->name, type_var_to_string(casted_node->return_type));
            
            for (size_t i = 0; i < casted_node->parameter_count; i++) {
                for (int j = 0; j < indent + 1; j++) DPRINT("  ");
                DPRINT("Parameter: %s (type %s)\n", 
                       casted_node->parameters[i].name, 
                       type_var_to_string(casted_node->parameters[i].type));
            }
            ast_print(casted_node->body, indent + 1);
            break;
        }
        
        case NODE_VARIABLE_DECLARATION_STATEMENT: {
            VariableDeclarationStatement* casted_node = (VariableDeclarationStatement*) node;
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("Variable: %s (type %s)\n", casted_node->name, type_var_to_string(casted_node->var_type));
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("Initializer: ");
            ast_print(casted_node->initializer, indent + 1);
            break;
        }
            
        case NODE_EXPRESSION_STATEMENT: {
            ExpressionStatement* casted_node = (ExpressionStatement*) node;
            ast_print(casted_node->expression, indent + 1);
            break;
        }
            
        case NODE_RETURN_STATEMENT: {
            ReturnStatement* casted_node = (ReturnStatement*) node;
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("Return\n");
            ast_print(casted_node->expression, indent + 1);
            break;
        }
            
        case NODE_BLOCK_STATEMENT: {
            BlockStatement* casted_node = (BlockStatement*) node;
            for (size_t i = 0; i < casted_node->statement_count; i++) {
                ast_print(casted_node->statements[i], indent + 1);
            }
            break;
        }
            
        case NODE_IF_STATEMENT: {
            IfStatement* casted_node = (IfStatement*) node;
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("If Condition:\n");
            ast_print(casted_node->condition, indent + 2);
            
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("Then Branch:\n");
            ast_print(casted_node->then_branch, indent + 2);
            
            for (size_t i = 0; i < casted_node->elif_count; i++) {
                for (int j = 0; j < indent + 1; j++) DPRINT("  ");
                DPRINT("Elif Condition %zu:\n", i + 1);
                ast_print(casted_node->elif_conditions[i], indent + 2);
                ast_print(casted_node->elif_branches[i], indent + 2);
            }
            
            if (casted_node->else_branch) {
                for (int i = 0; i < indent + 1; i++) DPRINT("  ");
                DPRINT("Else Branch:\n");
                ast_print(casted_node->else_branch, indent + 2);
            }
            break;
        }
            
        case NODE_WHILE_STATEMENT: {
            WhileStatement* casted_node = (WhileStatement*) node;
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("While Condition:\n");
            ast_print(casted_node->condition, indent + 2);
            
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("While Body:\n");
            ast_print(casted_node->body, indent + 2);
            break;
        }
            
        case NODE_FOR_STATEMENT: {
            ForStatement* casted_node = (ForStatement*) node;
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("For Initializer:\n");
            ast_print(casted_node->initializer, indent + 2);
            
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("For Condition:\n");
            ast_print(casted_node->condition, indent + 2);
            
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("For Increment:\n");
            ast_print(casted_node->increment, indent + 2);
            
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("For Body:\n");
            ast_print(casted_node->body, indent + 2);
            break;
        }

        case NODE_ASSIGNMENT_STATEMENT: {
            AssignmentStatement* casted_node = (AssignmentStatement*) node;
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("Assignment:\n");
            ast_print(casted_node->left, indent + 2);
            ast_print(casted_node->right, indent + 2);
            break;
        }
            
        case NODE_BINARY_EXPRESSION: {
            BinaryExpression* casted_node = (BinaryExpression*) node;
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("Operator: %s\n", token_type_to_string(casted_node->operator_.type));
            ast_print(casted_node->left, indent + 1);
            ast_print(casted_node->right, indent + 1);
            break;
        }
        
        case NODE_UNARY_EXPRESSION: {
            UnaryExpression* casted_node = (UnaryExpression*) node;
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("Unary Operator: %s\n", token_type_to_string(casted_node->operator_.type));
            ast_print(casted_node->operand, indent + 1);
            break;
        }
            
        case NODE_LITERAL_EXPRESSION: {
            LiteralExpression* casted_node = (LiteralExpression*) node;
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("Literal: type=%s, value=%lld\n", type_var_to_string(casted_node->type), (long long)casted_node->value);
            break;
        }
            
        case NODE_VARIABLE_EXPRESSION: {
            VariableExpression* casted_node = (VariableExpression*) node;
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("Variable: %s\n", casted_node->name);
            break;
        }
            
        case NODE_FUNCTION_CALL_EXPRESSION: {
            FunctionCallExpression* casted_node = (FunctionCallExpression*) node;
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("Function Call:\n");
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            VariableExpression* casted_callee = (VariableExpression* ) casted_node->callee;
            DPRINT("%s\n",casted_callee->name);
            
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("Arguments (%d):\n", casted_node->argument_count);
            for (int i = 0; i < casted_node->argument_count; i++) {
                ast_print(casted_node->arguments[i], indent + 2);
            }
            break;
        }

        default: {
            for (int i = 0; i < indent + 1; i++) DPRINT("  ");
            DPRINT("Unknown node type: %d\n", node->node_type);
            break;
        }
    }
}

bool add_statement_to_block(ASTNode* block_stmt, ASTNode* stmt) {
    // returns 1 if realloc is done and 0 otherwise
    BlockStatement* casted_node = (BlockStatement*) block_stmt;
    casted_node->statement_count += 1;
        
    ASTNode** new_statements = realloc(
        casted_node->statements,
        casted_node->statement_count * sizeof(ASTNode*)
    );
    
    if (!new_statements) {
        fprintf(stderr, "Memory allocation failed\n");
        casted_node->statement_count -= 1;
        free(casted_node);
        return 0;
    }
    casted_node->statements = new_statements;
    casted_node->statements[casted_node->statement_count - 1] = ast_node_copy(stmt);
    return 1;
}
