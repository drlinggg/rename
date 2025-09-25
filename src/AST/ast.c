#include "ast.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static ASTNode* ast_node_allocate(NodeType node_type, SourceLocation loc) {
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


// Statements constructors

//ASTNode* ast_new_program(SourceLocation loc) {
//    ASTNode* NodeProgram = ast_node_allocate(NODE_PROGRAM, loc);
//    if (node) {
//        node->program.functions = NULL;
//        node->program.function_count = 0;
//    }
//    return (ASTNode*)node;
//}

ASTNode* ast_new_function_declaration_statement(SourceLocation loc, const char* name, TypeVar return_type) {
    ASTNode* node = ast_node_allocate(NODE_FUNCTION_DECLARATION_STATEMENT, loc);
    if (!node) return NULL;
    FunctionDeclarationStatement* casted_node = (FunctionDeclarationStatement*) node;
    casted_node->name = strdup(name);
    casted_node->return_type = return_type;
    casted_node->parameters = NULL;
    casted_node->parameter_count = 0;
    casted_node->body = NULL;
    return node;
}

ASTNode* ast_new_variable_declaration_statement(SourceLocation loc, TypeVar var_type, const char* name, ASTNode* initializer) {
    ASTNode* node = ast_node_allocate(NODE_VARIABLE_DECLARATION_STATEMENT, loc);
    if (!node) return NULL;
    VariableDeclarationStatement* casted_node = (VariableDeclarationStatement*) node;
    casted_node->var_type = var_type;
    casted_node->name = strdup(name);
    casted_node->initializer = initializer;
    return node;
}

ASTNode* ast_new_expression_statement(SourceLocation loc, ASTNode* expression) {
    ASTNode* node = ast_node_allocate(NODE_EXPRESSION_STATEMENT, loc);
    if (!node) return NULL;
    ExpressionStatement* casted_node = (ExpressionStatement*) node;
    casted_node->expression = expression;
    return node;
}

ASTNode* ast_new_return_statement(SourceLocation loc, ASTNode* expression) {
    ASTNode* node = ast_node_allocate(NODE_RETURN_STATEMENT, loc);
    if (!node) return NULL;
    ReturnStatement* casted_node = (ReturnStatement*) node;
    casted_node->expression = expression;
    return node;
}

ASTNode* ast_new_block_statement(SourceLocation loc, ASTNode** statements, size_t statement_count) {
    ASTNode* node = ast_node_allocate(NODE_BLOCK_STATEMENT, loc);
    if (!node) return NULL;
    BlockStatement* casted_node = (BlockStatement*) node;
    
    if (statement_count > 0 && statements) {
        casted_node->statements = malloc(statement_count * sizeof(ASTNode*));
        if (!casted_node->statements) {
            free(node);
            return NULL;
        }
        
        for (size_t i = 0; i < statement_count; i++) {
            casted_node->statements[i] = statements[i];
        }
        casted_node->statement_count = statement_count;
    } else {
        casted_node->statements = NULL;
        casted_node->statement_count = 0;
    }
    
    return node;
}

ASTNode* ast_new_if_statement(SourceLocation loc, ASTNode* condition, ASTNode* then_branch) {
    ASTNode* node = ast_node_allocate(NODE_IF_STATEMENT, loc);
    if (!node) return NULL;
    IfStatement* casted_node = (IfStatement*) node;
    casted_node->condition = condition;
    casted_node->then_branch = then_branch;
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
    casted_node->condition = condition;
    casted_node->body = body;
    return node;
}

ASTNode* ast_new_assignment_statement(SourceLocation loc, ASTNode* left, ASTNode* right) {
    ASTNode* node = ast_node_allocate(NODE_ASSIGNMENT_STATEMENT, loc);
    if (!node) return NULL;
    AssignmentStatement* casted_node = (AssignmentStatement*) node;
    casted_node->left = left;
    casted_node->right = right;
    return node;
}

ASTNode* ast_new_for_statement(SourceLocation loc, ASTNode* initializer, ASTNode* condition, ASTNode* increment, ASTNode* body) {
    ASTNode* node = ast_node_allocate(NODE_FOR_STATEMENT, loc);
    if (!node) return NULL;
    ForStatement* casted_node = (ForStatement*) node;
    casted_node->initializer = initializer;
    casted_node->condition = condition;
    casted_node->increment = increment;
    casted_node->body = body;
    return node;
}

// Expressions constructors

ASTNode* ast_new_binary_expression(SourceLocation loc, ASTNode* left, Token op, ASTNode* right) {
    ASTNode* node = ast_node_allocate(NODE_BINARY_EXPRESSION, loc);
    if (!node) return NULL;
    BinaryExpression* casted_node = (BinaryExpression*) node;
    casted_node->left = left;
    casted_node->right = right;
    casted_node->operator_ = op;
    return node;
}

ASTNode* ast_new_unary_expression(SourceLocation loc, Token op, ASTNode* operand) {
    ASTNode* node = ast_node_allocate(NODE_UNARY_EXPRESSION, loc);
    if (!node) return NULL;
    UnaryExpression* casted_node = (UnaryExpression*) node;
    casted_node->operator_ = op;
    casted_node->operand = operand;
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
    casted_node->callee = callee;
    casted_node->arguments = args;
    casted_node->argument_count = arg_count;
    return node;
}

void ast_free(ASTNode* node) {
    if (!node) return;
    
    switch (node->node_type) {
        case NODE_PROGRAM: {
            /*
            for (size_t i = 0; i < node->function_count; i++) {
                ast_free(node->functions[i]);
            }
            free(node->functions);
            */
            break;
        }
            
        case NODE_FUNCTION_DECLARATION_STATEMENT: {
            FunctionDeclarationStatement* casted_node = (FunctionDeclarationStatement*) node;
            free(casted_node->name);
            for (size_t i = 0; i < casted_node->parameter_count; i++) {
                free(casted_node->parameters[i].variable);
            }
            free(casted_node->parameters);
            ast_free(casted_node->body);
            free(casted_node->body);
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
            for (size_t i = 0; i < casted_node->statement_count; i++) {
                ast_free(casted_node->statements[i]);
            }
            free(casted_node->statements);
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }
            
        case NODE_IF_STATEMENT: {
            IfStatement* casted_node = (IfStatement*) node;
            ast_free(casted_node->condition);
            ast_free(casted_node->then_branch);

            for (int i = 0; i < casted_node->elif_count; i++) {
                ast_free(casted_node->elif_branches[i]);
            }

            if (casted_node->else_branch != NULL) {
                ast_free(casted_node->else_branch);
            }

            free(casted_node);
            node = NULL;
            casted_node = NULL;
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
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }
            
        case NODE_UNARY_EXPRESSION: {
            UnaryExpression* casted_node = (UnaryExpression*) node;
            ast_free(casted_node->operand);
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
            for (size_t i = 0; i < casted_node->argument_count; i++) {
                ast_free(casted_node->arguments[i]);
            }
            free(casted_node->arguments);
            free(casted_node);
            node = NULL;
            casted_node = NULL;
            break;
        }

        default: {
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
        case KW_FUN: return "KW_FUN";

        // 1.4 Structs, object.attr
        case KW_STRUCT: return "KW_STRUCT";
        case KW_DOT: return "KW_DOT";
        
        // 2. Identifiers
        case IDENTIFIER: return "IDENTIFIER";
        
        // 3. Literals
        case INT_LITERAL: return "INT_LITERAL";
        case BOOL_LITERAL: return "BOOL_LITERAL";
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
        case OP_ASSIGN_PLUS: return "OP_ASSIGN_PLUS";
        case OP_ASSIGN_MINUS: return "OP_ASSIGN_MINUS";
        case OP_ASSIGN_MULT: return "OP_ASSIGN_MULT";
        case OP_ASSIGN_DIV: return "OP_ASSIGN_DIV";
        case OP_ASSIGN_MOD: return "OP_ASSIGN_MOD";

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


void ast_print(ASTNode* node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    printf("%s\n", ast_node_type_to_string(node->node_type));
    
    switch (node->node_type) {
        case NODE_PROGRAM: {
            // TODO
            break;
        }
            
        case NODE_FUNCTION_DECLARATION_STATEMENT: {
            FunctionDeclarationStatement* casted_node = (FunctionDeclarationStatement*) node;

            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Function: %s (returns type %d)\n", casted_node->name, casted_node->return_type);
            
            for (size_t i = 0; i < casted_node->parameter_count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                printf("Parameter: %s (type %d)\n", 
                       casted_node->parameters[i].variable->name, 
                       casted_node->parameters[i].type);
            }
            ast_print(casted_node->body, indent + 1);
            break;
        }
        
        case NODE_VARIABLE_DECLARATION_STATEMENT: {
            VariableDeclarationStatement* casted_node = (VariableDeclarationStatement*) node;
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Variable: %s (type %d)\n", casted_node->name, casted_node->var_type);
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
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Return\n");
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
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("If Condition:\n");
            ast_print(casted_node->condition, indent + 2);
            
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Then Branch:\n");
            ast_print(casted_node->then_branch, indent + 2);
            
            for (size_t i = 0; i < casted_node->elif_count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                printf("Elif Condition %zu:\n", i + 1);
                ast_print(casted_node->elif_conditions[i], indent + 2);
                ast_print(casted_node->elif_branches[i], indent + 2);
            }
            
            if (casted_node->else_branch) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Else Branch:\n");
                ast_print(casted_node->else_branch, indent + 2);
            }
            break;
        }
            
        case NODE_WHILE_STATEMENT: {
            WhileStatement* casted_node = (WhileStatement*) node;
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("While Condition:\n");
            ast_print(casted_node->condition, indent + 2);
            
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("While Body:\n");
            ast_print(casted_node->body, indent + 2);
            break;
        }
            
        case NODE_FOR_STATEMENT: {
            ForStatement* casted_node = (ForStatement*) node;
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("For Initializer:\n");
            ast_print(casted_node->initializer, indent + 2);
            
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("For Condition:\n");
            ast_print(casted_node->condition, indent + 2);
            
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("For Increment:\n");
            ast_print(casted_node->increment, indent + 2);
            
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("For Body:\n");
            ast_print(casted_node->body, indent + 2);
            break;
        }

        case NODE_ASSIGNMENT_STATEMENT: {
            AssignmentStatement* casted_node = (AssignmentStatement*) node;
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Assignment:\n");
            ast_print(casted_node->left, indent + 2);
            ast_print(casted_node->right, indent + 2);
            break;
        }
            
        case NODE_BINARY_EXPRESSION: {
            BinaryExpression* casted_node = (BinaryExpression*) node;
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Operator: %s\n", token_type_to_string(casted_node->operator_.type));
            ast_print(casted_node->left, indent + 1);
            ast_print(casted_node->right, indent + 1);
            break;
        }
        
        case NODE_UNARY_EXPRESSION: {
            UnaryExpression* casted_node = (UnaryExpression*) node;
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Unary Operator: %s\n", token_type_to_string(casted_node->operator_.type));
            ast_print(casted_node->operand, indent + 1);
            break;
        }
            
        case NODE_LITERAL_EXPRESSION: {
            LiteralExpression* casted_node = (LiteralExpression*) node;
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Literal: type=%s, value=%lld\n", type_var_to_string(casted_node->type), (long long)casted_node->value);
            break;
        }
            
        case NODE_VARIABLE_EXPRESSION: {
            VariableExpression* casted_node = (VariableExpression*) node;
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Variable: %s\n", casted_node->name);
            break;
        }
            
        case NODE_FUNCTION_CALL_EXPRESSION: {
            FunctionCallExpression* casted_node = (FunctionCallExpression*) node;
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Function Call:\n");
            ast_print(casted_node->callee, indent + 2);
            
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Arguments (%d):\n", casted_node->argument_count);
            for (int i = 0; i < casted_node->argument_count; i++) {
                ast_print(casted_node->arguments[i], indent + 2);
            }
            break;
        }

        default: {
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Unknown node type: %d\n", node->node_type);
            break;
        }
    }
}

